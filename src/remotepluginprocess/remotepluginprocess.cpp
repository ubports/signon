/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@nokia.com>
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
#include <QNetworkProxy>
#include <QProcess>
#include <QUrl>
#include <QTimer>

#include "remotepluginprocess.h"
using namespace SignOn;

namespace RemotePluginProcessNS {

    static int signalFd[2];
    static CancelEventThread *cancelThread = NULL;

    RemotePluginProcess::RemotePluginProcess(QObject *parent) : QObject(parent)
    {
        m_plugin = NULL;
        m_readnotifier = NULL;
        m_errnotifier = NULL;

        qRegisterMetaType<SignOn::SessionData>("SignOn::SessionData");
        qRegisterMetaType<AuthPluginError>("AuthPluginError");
        qRegisterMetaType<QString>("QString");
    }

    RemotePluginProcess::~RemotePluginProcess()
    {
        delete m_plugin;
        delete m_readnotifier;
        delete m_errnotifier;

        if (cancelThread) {
            cancelThread->quit();
            cancelThread->wait();
            delete cancelThread;
        }
    }

    RemotePluginProcess* RemotePluginProcess::createRemotePluginProcess(QString &type, QObject *parent)
    {
        RemotePluginProcess *rpp = new RemotePluginProcess(parent);

        //this is needed before plugin is initialized
        rpp->setupProxySettings();

        if (!rpp->loadPlugin(type) ||
           !rpp->setupDataStreams() ||
           !rpp->setupSignalHandlers() ||
           rpp->m_plugin->type() != type) {
            delete rpp;
            return NULL;
        }
        return rpp;
    }

    bool RemotePluginProcess::loadPlugin(QString &type)
    {
        TRACE() << type;

        QLibrary lib(getPluginName(type));

        if (!lib.load()) {
            qCritical() << QString("Failed to load %1 (reason: %2)")
                .arg(getPluginName(type)).arg(lib.errorString());
            return false;
        }

        typedef AuthPluginInterface* (*SsoAuthPluginInstanceF)();
        SsoAuthPluginInstanceF instance = (SsoAuthPluginInstanceF)lib.resolve("auth_plugin_instance");
        if (!instance) {
            qCritical() << QString("Failed to resolve init function in %1 (reason: %2)")
                .arg(getPluginName(type)).arg(lib.errorString());
            return false;
        }

        m_plugin = qobject_cast<AuthPluginInterface *>(instance());

        if (!m_plugin) {
            qCritical() << QString("Failed to cast object for %1 type")
                .arg(type);
            return false;
        }

        if (!cancelThread)
            cancelThread = new CancelEventThread(m_plugin);

        connect(m_plugin, SIGNAL(result(const SignOn::SessionData&)),
                  this, SLOT(result(const SignOn::SessionData&)));

        connect(m_plugin, SIGNAL(error(const AuthPluginError, const QString&)),
                  this, SLOT(error(const AuthPluginError, const QString&)));

        connect(m_plugin, SIGNAL(userActionRequired(const SignOn::UiSessionData&)),
                  this, SLOT(userActionRequired(const SignOn::UiSessionData&)));

        connect(m_plugin, SIGNAL(refreshed(const SignOn::UiSessionData&)),
                  this, SLOT(refreshed(const SignOn::UiSessionData&)));

        connect(m_plugin, SIGNAL(statusChanged(const AuthPluginState, const QString&)),
                  this, SLOT(statusChanged(const AuthPluginState, const QString&)));

        m_plugin->setParent(this);
        return true;
    }

    bool RemotePluginProcess::setupDataStreams()
    {
        TRACE();

        m_infile.open(STDIN_FILENO, QIODevice::ReadOnly);
        m_outfile.open(STDOUT_FILENO, QIODevice::WriteOnly);

        m_readnotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read);
        m_errnotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Exception);

        connect(m_readnotifier, SIGNAL(activated(int)), this, SLOT(startTask()));
        connect(m_errnotifier, SIGNAL(activated(int)), this, SIGNAL(processStopped()));

        return true;
    }

    bool RemotePluginProcess::setupSignalHandlers()
    {
        TRACE();
         if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalFd))
            return false;

         sn = new QSocketNotifier(signalFd[1], QSocketNotifier::Read, this);
         connect(sn, SIGNAL(activated(int)), this, SLOT(handleSignal()));

         return true;
    }

    bool RemotePluginProcess::setupProxySettings()
    {
        TRACE();
        //set application default proxy
        QNetworkProxy networkProxy = QNetworkProxy::applicationProxy();
        //get proxy settings from GConf
        //TODO

        //get system env for proxy
        QString proxy = qgetenv("http_proxy");
        if (!proxy.isEmpty()) {
            QUrl proxyUrl(proxy);
            if (!proxyUrl.host().isEmpty()) {
                networkProxy = QNetworkProxy(QNetworkProxy::HttpProxy,
                                        proxyUrl.host(),
                                        proxyUrl.port(),
                                        proxyUrl.userName(),
                                        proxyUrl.password());
            }
        }
        //TODO add other proxy types

        TRACE() << networkProxy.hostName() << ":" << networkProxy.port();
        QNetworkProxy::setApplicationProxy(networkProxy);
        return true;
    }

    void RemotePluginProcess::handleSignal()
    {
         TRACE();
         sn->setEnabled(false);
         int signal;
         ::read(signalFd[1], &signal, sizeof(int));

         switch (signal) {
             case SIGINT: { emit processStopped(); break; }
             case SIGHUP: break;
             case SIGTERM: { emit processStopped(); break; }
             default: break;
         };
    }

    void RemotePluginProcess::signalHandler(int signal)
    {
        TRACE();
        ::write(signalFd[0], &signal, sizeof(int));
    }

    void RemotePluginProcess::result(const SignOn::SessionData &data)
    {
        TRACE();

        disableCancelThread();
        QDataStream out(&m_outfile);
        QVariantMap resultDataMap;

        foreach(QString key, data.propertyNames())
            resultDataMap[key] = data.getProperty(key);

        out << (quint32)PLUGIN_RESPONSE_RESULT;
        out << QVariant(resultDataMap);
        m_outfile.flush();

        TRACE() << resultDataMap;
    }

    void RemotePluginProcess::error(const AuthPluginError error, const QString &errorMessage)
    {
        disableCancelThread();

        QDataStream out(&m_outfile);

        out << (quint32)PLUGIN_RESPONSE_ERROR;
        out << (quint32)error;
        out << errorMessage;
        m_outfile.flush();

        TRACE() << "error is sent" << error << " " << errorMessage;
    }

    void RemotePluginProcess::userActionRequired(const SignOn::UiSessionData &data)
    {
        TRACE();
        disableCancelThread();

        QDataStream out(&m_outfile);
        QVariantMap resultDataMap;

        foreach(QString key, data.propertyNames())
            resultDataMap[key] = data.getProperty(key);

        out << (quint32)PLUGIN_RESPONSE_UI;
        out << QVariant(resultDataMap);
        m_outfile.flush();
    }

    void RemotePluginProcess::refreshed(const SignOn::UiSessionData &data)
    {
        TRACE();
        disableCancelThread();

        QDataStream out(&m_outfile);
        QVariantMap resultDataMap;

        foreach(QString key, data.propertyNames())
            resultDataMap[key] = data.getProperty(key);

        m_readnotifier->setEnabled(true);

        out << (quint32)PLUGIN_RESPONSE_REFRESHED;
        out << QVariant(resultDataMap);
        m_outfile.flush();
    }

    void RemotePluginProcess::statusChanged(const AuthPluginState state, const QString &message)
    {
        TRACE();
        QDataStream out(&m_outfile);

        out << (quint32)PLUGIN_RESPONSE_SIGNAL;
        out << (quint32)state;
        out << message;

        m_outfile.flush();
    }

    QString RemotePluginProcess::getPluginName(const QString &type)
    {
        QString fileName = QDir::cleanPath(SIGNON_PLUGINS_DIR) +
                           QDir::separator() +
                           QString(SIGNON_PLUGIN_PREFIX) +
                           type +
                           QString(SIGNON_PLUGIN_SUFFIX);

        return fileName;
    }

    void RemotePluginProcess::type()
    {
        QDataStream out(&m_outfile);
        QByteArray typeBa;
        typeBa.append(m_plugin->type());
        out << typeBa;
    }

    void RemotePluginProcess::mechanisms()
    {
        QDataStream out(&m_outfile);
        QStringList mechanisms = m_plugin->mechanisms();
        QVariant mechsVar = mechanisms;
        out << mechsVar;
    }

    void RemotePluginProcess::process()
    {
//        TRACE();
        QDataStream in(&m_infile);

        QVariant infoVa;
        QVariant mechanismVa;

        in >> infoVa;
        in >> mechanismVa;

        SessionData inData(infoVa.toMap());
        QString mechanism(mechanismVa.toString());

        enableCancelThread();
        TRACE() << "The cancel thread is started";

        TRACE() << infoVa.toMap();
        m_plugin->process(inData, mechanism);
    }

    void RemotePluginProcess::userActionFinished()
    {
        QDataStream in(&m_infile);
        QVariant infoVa;
        in >> infoVa;
        UiSessionData inData((QVariantMap)(infoVa.toMap()));

        enableCancelThread();
        m_plugin->userActionFinished(inData);
    }

    void RemotePluginProcess::refresh()
    {
        QDataStream in(&m_infile);
        QVariant infoVa;
        in >> infoVa;
        UiSessionData inData(infoVa.toMap());

        enableCancelThread();
        m_plugin->refresh(inData);
    }

    void RemotePluginProcess::enableCancelThread()
    {
        QEventLoop loop;
        connect(cancelThread,
                SIGNAL(started()),
                &loop,
                SLOT(quit()));

        m_readnotifier->setEnabled(false);
        QTimer::singleShot(0.5*1000, &loop, SLOT(quit()));
        cancelThread->start();
        loop.exec();
        QThread::yieldCurrentThread();
    }

    void RemotePluginProcess::disableCancelThread()
    {
        /**
         * Sort of terrible workaround which
         * I do not know how to fix: the thread
         * could hang up during wait up without
         * these loops and sleeps
         */
        cancelThread->quit();

        TRACE() << "Before the isFinished loop ";

        int i = 0;
        while (!cancelThread->isFinished()) {
            cancelThread->quit();
            TRACE() << "Internal iteration " << i++;
            usleep(0.005 * 1000000);
        }

        if (!cancelThread->wait(500)) {
            BLAME() << "Cannot disable cancel thread";
            int i;
            for (i = 0; i < 5; i++) {
                usleep(0.01 * 1000000);
                if (cancelThread->wait(500))
                    break;
            }

            if (i == 5) {
                BLAME() << "Cannot do anything with cancel thread";
                cancelThread->terminate();
                cancelThread->wait();
            }
        }

        m_readnotifier->setEnabled(true);
    }

    void RemotePluginProcess::startTask()
    {
        quint32 opcode = PLUGIN_OP_STOP;
        bool is_stopped = false;

        QDataStream in(&m_infile);
        in >> opcode;

        switch (opcode) {
            case PLUGIN_OP_CANCEL:
            {
                m_plugin->cancel(); break;
                //still do not have clear understanding
                //of the cancellation-stop mechanism
                //is_stopped = true;
            }
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
            default:
            {
                qCritical() << " unknown operation code: " << opcode;
                is_stopped = true;
            }
            break;
        };

        TRACE() << "operation is completed";

        if (!is_stopped) {
            if (!m_outfile.flush())
                is_stopped = true;
        }

        if (is_stopped)
            emit processStopped();
    }

    CancelEventThread::CancelEventThread(AuthPluginInterface *plugin)
    {
        m_plugin = plugin;
        m_cancelNotifier = 0;
    }

    CancelEventThread::~CancelEventThread()
    {
        delete m_cancelNotifier;
    }

    void CancelEventThread::run()
    {
        if (!m_cancelNotifier) {
            m_cancelNotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read);
            connect(m_cancelNotifier, SIGNAL(activated(int)), this, SLOT(cancel()), Qt::DirectConnection);
        }

        m_cancelNotifier->setEnabled(true);
        exec();
        m_cancelNotifier->setEnabled(false);
    }

    void CancelEventThread::cancel()
    {
        char buf[4];
        memset(buf, 0, 4);
        int n = 0;

        if (!(n = read(STDIN_FILENO, buf, 4))) {
            qCritical() << "Cannot read from cancel socket";
            return;
        }

        /*
         * Read the actual value of
         * */
        quint32 opcode;
        QDataStream ds(QByteArray(buf, 4));
        ds >> opcode;
        if (opcode != PLUGIN_OP_CANCEL)
            qCritical() << "wrong operation code: breakage of remotepluginprocess threads synchronization: " << opcode;

        m_plugin->cancel();
    }
} //namespace RemotePluginProcessNS

