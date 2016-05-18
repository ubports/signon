/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 * Copyright (C) 2012-2016 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#include <QProcess>
#include <QUrl>
#include <QTimer>
#include <QBuffer>
#include <QDataStream>
#include <unistd.h>

#include "debug.h"
#ifdef HAVE_LIBPROXY
#include "my-network-proxy-factory.h"
#endif
#include "remotepluginprocess.h"

// signon-plugins-common
#include "SignOn/blobiohandler.h"
#include "SignOn/ipc.h"

using namespace SignOn;

namespace RemotePluginProcessNS {

/* ---------------------- RemotePluginProcess ---------------------- */

RemotePluginProcess::RemotePluginProcess(QObject *parent):
    QObject(parent),
    m_currentOperation(PLUGIN_OP_STOP)
{
    m_plugin = NULL;
    m_readnotifier = NULL;
    m_errnotifier = NULL;

    qRegisterMetaType<SignOn::SessionData>("SignOn::SessionData");
    qRegisterMetaType<QString>("QString");
}

RemotePluginProcess::~RemotePluginProcess()
{
    delete m_plugin;
    delete m_readnotifier;
    delete m_errnotifier;
}

RemotePluginProcess *
RemotePluginProcess::createRemotePluginProcess(QString &type, QObject *parent)
{
    RemotePluginProcess *rpp = new RemotePluginProcess(parent);

    //this is needed before plugin is initialized
    rpp->setupProxySettings();

    if (!rpp->loadPlugin(type) ||
       !rpp->setupDataStreams() ||
       rpp->m_plugin->type() != type) {
        delete rpp;
        return NULL;
    }
    return rpp;
}

bool RemotePluginProcess::loadPlugin(QString &type)
{
    TRACE() << " loading auth library for " << type;

    QLibrary lib(getPluginName(type));

    if (!lib.load()) {
        qCritical() << QString("Failed to load %1 (reason: %2)")
            .arg(getPluginName(type)).arg(lib.errorString());
        return false;
    }

    TRACE() << "library loaded";

    typedef AuthPluginInterface* (*SsoAuthPluginInstanceF)();
    SsoAuthPluginInstanceF instance =
        (SsoAuthPluginInstanceF)lib.resolve("auth_plugin_instance");
    if (!instance) {
        qCritical() << QString("Failed to resolve init function in %1 "
                               "(reason: %2)")
            .arg(getPluginName(type)).arg(lib.errorString());
        return false;
    }

    TRACE() << "constructor resolved";

    m_plugin = qobject_cast<AuthPluginInterface *>(instance());

    if (!m_plugin) {
        qCritical() << QString("Failed to cast object for %1 type")
            .arg(type);
        return false;
    }

    connect(m_plugin, SIGNAL(result(const SignOn::SessionData&)),
            this, SLOT(result(const SignOn::SessionData&)));

    connect(m_plugin, SIGNAL(store(const SignOn::SessionData&)),
            this, SLOT(store(const SignOn::SessionData&)));

    connect(m_plugin, SIGNAL(error(const SignOn::Error &)),
            this, SLOT(error(const SignOn::Error &)));

    connect(m_plugin, SIGNAL(userActionRequired(const SignOn::UiSessionData&)),
            this, SLOT(userActionRequired(const SignOn::UiSessionData&)));

    connect(m_plugin, SIGNAL(refreshed(const SignOn::UiSessionData&)),
            this, SLOT(refreshed(const SignOn::UiSessionData&)));

    connect(m_plugin,
            SIGNAL(statusChanged(const AuthPluginState, const QString&)),
            this, SLOT(statusChanged(const AuthPluginState, const QString&)));

    m_plugin->setParent(this);

    TRACE() << "plugin is fully initialized";
    return true;
}

bool RemotePluginProcess::setupDataStreams()
{
    TRACE();

    m_inFile.open(STDIN_FILENO, QIODevice::ReadOnly);
    m_outFile.open(STDOUT_FILENO, QIODevice::WriteOnly);

    m_readnotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read);
    m_errnotifier = new QSocketNotifier(STDIN_FILENO,
                                        QSocketNotifier::Exception);

    connect(m_readnotifier, SIGNAL(activated(int)), this, SLOT(startTask()));
    connect(m_errnotifier, SIGNAL(activated(int)),
            this, SIGNAL(processStopped()));

    m_blobIOHandler = new BlobIOHandler(&m_inFile, &m_outFile, this);

    connect(m_blobIOHandler,
            SIGNAL(dataReceived(const QVariantMap &)),
            this,
            SLOT(sessionDataReceived(const QVariantMap &)));

    connect(m_blobIOHandler,
            SIGNAL(error()),
            this,
            SLOT(blobIOError()));

    m_blobIOHandler->setReadChannelSocketNotifier(m_readnotifier);

    return true;
}

bool RemotePluginProcess::setupProxySettings()
{
    TRACE();

#ifdef HAVE_LIBPROXY
    /* Use a libproxy-based proxy factory; this code will no longer be
     * needed when https://bugreports.qt-project.org/browse/QTBUG-26295
     * is fixed. */
    MyNetworkProxyFactory *proxyFactory = new MyNetworkProxyFactory();
    QNetworkProxyFactory::setApplicationProxyFactory(proxyFactory);
#endif

    return true;
}

void RemotePluginProcess::blobIOError()
{
    error(
        Error(Error::InternalServer,
        QLatin1String("Failed to I/O session data to/from the signon daemon.")));
}

void RemotePluginProcess::result(const SignOn::SessionData &data)
{
    QDataStream out(&m_outFile);
    QVariantMap resultDataMap;

    foreach(QString key, data.propertyNames())
        resultDataMap[key] = data.getProperty(key);

    out << (quint32)PLUGIN_RESPONSE_RESULT;

    m_blobIOHandler->sendData(resultDataMap);

    m_outFile.flush();
}

void RemotePluginProcess::store(const SignOn::SessionData &data)
{
    QDataStream out(&m_outFile);
    QVariantMap storeDataMap;

    foreach(QString key, data.propertyNames())
        storeDataMap[key] = data.getProperty(key);

    out << (quint32)PLUGIN_RESPONSE_STORE;

    m_blobIOHandler->sendData(storeDataMap);

    m_outFile.flush();
}

void RemotePluginProcess::error(const SignOn::Error &err)
{
    QDataStream out(&m_outFile);

    out << (quint32)PLUGIN_RESPONSE_ERROR;
    out << (quint32)err.type();
    out << err.message();
    m_outFile.flush();

    TRACE() << "error is sent" << err.type() << " " << err.message();
}

void RemotePluginProcess::userActionRequired(const SignOn::UiSessionData &data)
{
    TRACE();

    QDataStream out(&m_outFile);
    QVariantMap resultDataMap;

    foreach(QString key, data.propertyNames())
        resultDataMap[key] = data.getProperty(key);

    out << (quint32)PLUGIN_RESPONSE_UI;
    m_blobIOHandler->sendData(resultDataMap);
    m_outFile.flush();
}

void RemotePluginProcess::refreshed(const SignOn::UiSessionData &data)
{
    TRACE();

    QDataStream out(&m_outFile);
    QVariantMap resultDataMap;

    foreach(QString key, data.propertyNames())
        resultDataMap[key] = data.getProperty(key);

    out << (quint32)PLUGIN_RESPONSE_REFRESHED;

    m_blobIOHandler->sendData(resultDataMap);

    m_outFile.flush();
}

void RemotePluginProcess::statusChanged(const AuthPluginState state,
                                        const QString &message)
{
    TRACE();
    QDataStream out(&m_outFile);

    out << (quint32)PLUGIN_RESPONSE_SIGNAL;
    out << (quint32)state;
    out << message;

    m_outFile.flush();
}

QString RemotePluginProcess::getPluginName(const QString &type)
{
    QString dirName = qgetenv("SSO_PLUGINS_DIR");
    if (dirName.isEmpty())
        dirName = QDir::cleanPath(SIGNOND_PLUGINS_DIR);
    QString fileName = dirName +
                       QDir::separator() +
                       QString(SIGNON_PLUGIN_PREFIX) +
                       type +
                       QString(SIGNON_PLUGIN_SUFFIX);

    return fileName;
}

void RemotePluginProcess::type()
{
    QDataStream out(&m_outFile);
    out << m_plugin->type();
}

void RemotePluginProcess::mechanisms()
{
    QDataStream out(&m_outFile);
    QStringList mechanisms = m_plugin->mechanisms();
    QVariant mechsVar = mechanisms;
    out << mechsVar;
}

void RemotePluginProcess::process()
{
    QDataStream in(&m_inFile);

    in >> m_currentMechanism;

    int processBlobSize = -1;
    in >> processBlobSize;

    m_currentOperation = PLUGIN_OP_PROCESS;
    m_blobIOHandler->receiveData(processBlobSize);
}

void RemotePluginProcess::userActionFinished()
{
    QDataStream in(&m_inFile);
    int processBlobSize = -1;
    in >> processBlobSize;

    m_currentOperation = PLUGIN_OP_PROCESS_UI;
    m_blobIOHandler->receiveData(processBlobSize);
}

void RemotePluginProcess::refresh()
{
    QDataStream in(&m_inFile);
    int processBlobSize = -1;
    in >> processBlobSize;

    m_currentOperation = PLUGIN_OP_REFRESH;
    m_blobIOHandler->receiveData(processBlobSize);
}

void RemotePluginProcess::sessionDataReceived(const QVariantMap &sessionDataMap)
{
    if (m_currentOperation == PLUGIN_OP_PROCESS) {
        SessionData inData(sessionDataMap);
        m_plugin->process(inData, m_currentMechanism);
        m_currentMechanism.clear();

    } else if(m_currentOperation == PLUGIN_OP_PROCESS_UI) {
        UiSessionData inData(sessionDataMap);
        m_plugin->userActionFinished(inData);

    } else if(m_currentOperation == PLUGIN_OP_REFRESH) {
        UiSessionData inData(sessionDataMap);
        m_plugin->refresh(inData);

    } else {
        TRACE() << "Wrong operation code.";
        error(Error(Error::InternalServer,
                    QLatin1String("Plugin process - invalid operation code.")));
    }

    m_currentOperation = PLUGIN_OP_STOP;
}

void RemotePluginProcess::startTask()
{
    if (m_blobIOHandler->isReading()) {
        /* A data blob is being read; there's nothing for us here */
        return;
    }

    quint32 opcode = PLUGIN_OP_STOP;
    bool is_stopped = false;

    QDataStream in(&m_inFile);
    in >> opcode;

    /* If the plugin is busy, the only allowed action here is canceling */
    if (m_currentOperation != PLUGIN_OP_STOP && opcode != PLUGIN_OP_CANCEL) {
        qCritical() << "Operation requested while plugin busy! - code" <<
            opcode;
        m_plugin->abort();
        Q_EMIT processStopped();
        return;
    }

    switch (opcode) {
    case PLUGIN_OP_CANCEL:
        m_plugin->cancel();
        break;
    case PLUGIN_OP_TYPE:
        type();
        break;
    case PLUGIN_OP_MECHANISMS:
        mechanisms();
        break;
    case PLUGIN_OP_PROCESS:
        process();
        break;
    case PLUGIN_OP_PROCESS_UI:
        userActionFinished();
        break;
    case PLUGIN_OP_REFRESH:
        refresh();
        break;
    case PLUGIN_OP_STOP:
        is_stopped = true;
        break;
    default:
        {
            qCritical() << " unknown operation code: " << opcode;
            is_stopped = true;
        }
        break;
    };

    TRACE() << "operation is completed";

    if (!is_stopped) {
        if (!m_outFile.flush())
            is_stopped = true;
    }

    if (is_stopped)
    {
        m_plugin->abort();
        emit processStopped();
    }
}

} //namespace RemotePluginProcessNS

