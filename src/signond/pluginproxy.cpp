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

#include "pluginproxy.h"

#include <sys/types.h>
#include <pwd.h>

#include <QStringList>
#include <QThreadStorage>
#include <QThread>

#include "SignOn/uisessiondata_priv.h"
#include "SignOn/signonplugincommon.h"

/*
 *   TODO: remove the "SignOn/authpluginif.h" include below after the removal
 *         of the deprecated error handling (needed here only for the deprecated
 *         AuthPluginError::PLUGIN_ERROR_GENERAL).
 */
#include "SignOn/authpluginif.h"


using namespace SignOn;

//TODO get this from config
#define REMOTEPLUGIN_BIN_PATH QLatin1String("/usr/bin/signonpluginprocess")
#define PLUGINPROCESS_TIMEOUT 5000

namespace SignonDaemonNS {

    PluginProcess::PluginProcess(QObject *parent) : QProcess(parent)
    {
    }

    PluginProcess::~PluginProcess()
    {
        if (state() != QProcess::NotRunning)
            terminate();

        waitForFinished();
    }

    void PluginProcess::setupChildProcess()
    {
        // Drop all root privileges in the child process and switch to signon user
#ifndef NO_SIGNON_USER
        //get uid and gid
        struct passwd *passwdRecord = getpwnam("signon");
        if ( !passwdRecord ){
            fprintf(stderr, "failed to get user: signon\n");
            emit QProcess::finished(2, QProcess::NormalExit);
            exit(2);
        }
#ifdef SIGNOND_TRACE
        //this is run in remote plugin process, so trace should go to stderr
        fprintf(stderr, "got user: %s with uid: %d\n", passwdRecord->pw_name, passwdRecord->pw_uid);
#endif
        if (( ::setgid(passwdRecord->pw_gid))
                || (::setuid(passwdRecord->pw_uid))
                || (::getuid() != passwdRecord->pw_uid)
                ) {
            fprintf(stderr, "failed to set user: %s with uid: %d", passwdRecord->pw_name, passwdRecord->pw_uid);
            emit QProcess::finished(2, QProcess::NormalExit);
            exit(2);
        }
 #endif
    }

    PluginProxy::PluginProxy(QString type, QObject *parent)
            : QObject(parent)
    {
        TRACE();

        m_type = type;
        m_isProcessing = false;
        m_process = new PluginProcess(this);

        connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(onReadStandardError()));

        /*
         * TODO: some error handling should be added here, at least remove of current
         * request data from the top of the queue and reply an error code
         * */
        connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onExit(int, QProcess::ExitStatus)));
        connect(m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onError(QProcess::ProcessError)));
    }

    PluginProxy::~PluginProxy()
    {
    }

    PluginProxy* PluginProxy::createNewPluginProxy(const QString &type)
    {
        PluginProxy *pp = new PluginProxy(type);

        QStringList args = QStringList() << pp->m_type;
        pp->m_process->start(REMOTEPLUGIN_BIN_PATH, args);

        QByteArray tmp;

        if (!pp->waitForStarted(PLUGINPROCESS_TIMEOUT)) {
            TRACE() << "The process cannot be started";
            delete pp;
            return NULL;
        }

        if (!pp->readOnReady(tmp, PLUGINPROCESS_TIMEOUT)) {
            TRACE() << "The process cannot load plugin";
            delete pp;
            return NULL;
        }

        pp->m_type = pp->queryType();
        pp->m_mechanisms = pp->queryMechanisms();

        connect(pp->m_process, SIGNAL(readyReadStandardOutput()), pp, SLOT(onReadStandardOutput()));

        TRACE() << "The process is started";
        return pp;
    }

   bool PluginProxy::process(const QString &cancelKey, const QVariantMap &inData, const QString &mechanism)
   {
        TRACE();
        if (!restartIfRequired())
            return false;

        m_cancelKey = cancelKey;
        QVariant value = inData.value(SSOUI_KEY_UIPOLICY);
        m_uiPolicy = value.toInt();
        //TODO check if this part can be done in SignonSessionCore::startProcess()
        QVariantMap inDataTmp = inData;
        if (m_uiPolicy == RequestPasswordPolicy)
            inDataTmp.remove(SSOUI_KEY_PASSWORD);
        QDataStream in(m_process);

        in << (quint32)PLUGIN_OP_PROCESS;
        in << QVariant(inDataTmp);
        in << QVariant(mechanism);

        m_isProcessing = true;

        return true;
    }

   bool PluginProxy::processUi(const QString &cancelKey, const QVariantMap &inData)
   {
        TRACE() << inData;

        if (!restartIfRequired())
            return false;

        m_cancelKey = cancelKey;

        QDataStream in(m_process);

        in << (quint32)PLUGIN_OP_PROCESS_UI;
        in << QVariant(inData);

        m_isProcessing = true;

        return true;
    }

   bool PluginProxy::processRefresh(const QString &cancelKey, const QVariantMap &inData)
   {
        TRACE() << inData;

        if (!restartIfRequired())
            return false;

        m_cancelKey = cancelKey;

        QDataStream in(m_process);

        in << (quint32)PLUGIN_OP_REFRESH;
        in << QVariant(inData);

        m_isProcessing = true;

        return true;
    }

   void PluginProxy::cancel()
   {
       TRACE();
       QDataStream in(m_process);
       in << (quint32)PLUGIN_OP_CANCEL;
    }

   void PluginProxy::stop()
   {
       TRACE();
       QDataStream in(m_process);
       in << (quint32)PLUGIN_OP_STOP;
    }

    bool PluginProxy::readOnReady(QByteArray &buffer, int timeout)
    {
        bool ready = m_process->waitForReadyRead(timeout);

        if (ready) {
            if (!m_process->bytesAvailable())
                return false;

            while (m_process->bytesAvailable())
                buffer += m_process->readAllStandardOutput();
        }

        return ready;
    }

    bool PluginProxy::isProcessing()
    {
        return m_isProcessing;
    }

    void PluginProxy::onReadStandardOutput()
    {
        disconnect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadStandardOutput()));

        QByteArray token;

        QVariant infoVa;
        QVariantMap info;

        if (!m_process->bytesAvailable()) {
            qCritical() << "No information available on process";
            m_isProcessing = false;
            emit processError(m_cancelKey, PLUGIN_ERROR_GENERAL, QString());
            return;
        }

        QByteArray buffer;

        while (m_process->bytesAvailable())
            buffer += m_process->readAllStandardOutput();


        /*
         * we need to analyze the whole buffer
         * and if it contains error then emit only error
         * otherwise process/emit the incoming information
         * one by one
         * */
        QDataStream out(buffer);
        bool isResultObtained = false;

        while (out.status() == QDataStream::Ok) {
            quint32 opres;
            out >> opres; //result of operation: error code

            TRACE() << opres;

            if (opres == PLUGIN_RESPONSE_RESULT) {
                TRACE() << "PLUGIN_RESPONSE_RESULT";
                out >> infoVa; //SessionData in QVariant
                info = infoVa.toMap();
                m_isProcessing = false;

                if (!isResultObtained)
                    emit processResultReply(m_cancelKey, info);
                else
                    BLAME() << "Unexpected plugin response: " << info;

                isResultObtained = true;
            } else if (opres == PLUGIN_RESPONSE_UI) {
                TRACE() << "PLUGIN_RESPONSE_UI";
                out >> infoVa; //UiSessionData in QVariant
                info = infoVa.toMap();

                if (!isResultObtained) {
                    bool allowed = true;

                    if (m_uiPolicy == NoUserInteractionPolicy)
                        allowed = false;

                    if (m_uiPolicy == ValidationPolicy &&
                        !info.contains(SSOUI_KEY_CAPTCHAIMG) &&
                        !info.contains(SSOUI_KEY_CAPTCHAURL))
                        allowed = false;

                    if (!allowed) {
                        //set error and return;
                        TRACE() << "ui policy prevented ui launch";
                        info.insert(SSOUI_KEY_ERROR, QUERY_ERROR_FORBIDDEN);
                        processUi(m_cancelKey, info);
                    } else {
                        TRACE() << "open ui";
                        emit processUiRequest(m_cancelKey, info);
                    }
                } else {
                    BLAME() << "Unexpected plugin ui response: " << info;
                }
            } else if (opres == PLUGIN_RESPONSE_REFRESHED) {
                TRACE() << "PLUGIN_RESPONSE_REFRESHED";
                out >> infoVa; //UiSessionData in QVariant
                info = infoVa.toMap();

                if (!isResultObtained)
                    emit processRefreshRequest(m_cancelKey, info);
                else
                    BLAME() << "Unexpected plugin ui response: " << info;
            } else if (opres == PLUGIN_RESPONSE_ERROR) {
                TRACE() << "PLUGIN_RESPONSE_ERROR";
                quint32 err;
                QString errorMessage;
                out >> err;
                out >> errorMessage;
                m_isProcessing = false;

                if (!isResultObtained)
                    emit processError(m_cancelKey, (int)err, errorMessage);
                else
                    BLAME() << "Unexpected plugin error: " << errorMessage;

                isResultObtained = true;
            } else if (opres == PLUGIN_RESPONSE_SIGNAL) {
                TRACE() << "PLUGIN_RESPONSE_SIGNAL";
                quint32 state;
                QString message;

                out >> state;
                out >> message;

                if (!isResultObtained)
                    emit stateChanged(m_cancelKey, (int)state, message);
                else
                    BLAME() << "Unexpected plugin signal: " << state << " " << message;
            }
        }

        connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadStandardOutput()));
    }

    void PluginProxy::onReadStandardError()
    {
        QString ba = QString::fromLatin1(m_process->readAllStandardError());
        TRACE() << ba;
    }

    void PluginProxy::onExit(int exitCode, QProcess::ExitStatus exitStatus)
    {
        TRACE() << "Plugin process exit with code " << exitCode << " : " << exitStatus;

        if (m_isProcessing || exitStatus == QProcess::CrashExit) {
            qCritical() << "Challenge produces CRASH!";
            emit processError(m_cancelKey, PLUGIN_ERROR_GENERAL, QLatin1String("plugin processed crashed"));
        }
        if (exitCode == 2) {
            TRACE() << "plugin process terminated because cannot change user";
        }

        m_isProcessing = false;
    }

    void PluginProxy::onError(QProcess::ProcessError err)
    {
        TRACE() << "Error: " << err;
    }

    QString PluginProxy::queryType()
    {
        TRACE();

        if (!restartIfRequired())
            return QString();

        QDataStream ds(m_process);
        ds << (quint32)PLUGIN_OP_TYPE;

        QByteArray typeBa, buffer;
        bool result;

        if ((result = readOnReady(buffer, PLUGINPROCESS_TIMEOUT))) {
            QDataStream out(buffer);
            out >> typeBa;
        } else
            qCritical("PluginProxy returned NULL result");

        return QString::fromLatin1(typeBa);
    }

    QStringList PluginProxy::queryMechanisms()
    {
        TRACE();

        if (!restartIfRequired())
            return QStringList();

        QDataStream in(m_process);
        in << (quint32)PLUGIN_OP_MECHANISMS;

        QByteArray buffer;
        QStringList strList;
        bool result;

        if ((result = readOnReady(buffer, PLUGINPROCESS_TIMEOUT))) {
            QVariant mechanismsVar;
            QDataStream out(buffer);

            out >> mechanismsVar;
            QVariantList varList = mechanismsVar.toList();

            for (int i = 0; i < varList.count(); i++)
                    strList << varList.at(i).toString();

            TRACE() << strList;
        } else
            qCritical("PluginProxy returned NULL result");

        return strList;
    }

    bool PluginProxy::waitForStarted(int timeout)
    {
        return m_process->waitForStarted(timeout);
    }

    bool PluginProxy::waitForFinished(int timeout)
    {
        return m_process->waitForFinished(timeout);
    }

    bool PluginProxy::restartIfRequired()
    {
        if (m_process->state() == QProcess::NotRunning) {
            TRACE() << "RESTART REQUIRED";
            m_process->start(REMOTEPLUGIN_BIN_PATH, QStringList(m_type));

            QByteArray tmp;
            if (!waitForStarted(PLUGINPROCESS_TIMEOUT) || !readOnReady(tmp, PLUGINPROCESS_TIMEOUT))
                return false;
        }
        return true;
    }

} //namespace SignonDaemonNS
