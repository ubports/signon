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

#include "signond-common.h"
#include "signonauthsession.h"
#include "signonidentityinfo.h"
#include "signonidentity.h"
#include "signonauthsessionadaptor.h"
#include "signonui_interface.h"
#include "accesscontrolmanager.h"

#include <SignOn/uisessiondata_priv.h>
#include <SignOn/authpluginif.h>
#include <SignOn/Error>

#define MAX_IDLE_TIME SIGNOND_MAX_IDLE_TIME
/*
 * the watchdog searches for idle sessions with period of half of idle timeout
 * */
#define IDLE_WATCHDOG_TIMEOUT SIGNOND_MAX_IDLE_TIME * 500

#define SSO_KEY_USERNAME QLatin1String("UserName")
#define SSO_KEY_PASSWORD QLatin1String("Secret")
#define SSO_KEY_CAPTION QLatin1String("Caption")

using namespace SignonDaemonNS;
using namespace SignOnCrypto;

/*
 * cache of session queues, as was mentined they cannot be static
 * */
QMap<QString, SignonSessionCore *> sessionsOfStoredCredentials;
/*
 * List of "zero" authsessions, needed for global signout
 * */
QList<SignonSessionCore *> sessionsOfNonStoredCredentials;

static QVariantMap filterVariantMap(const QVariantMap &other)
{
    QVariantMap result;

    foreach(QString key, other.keys()) {
        if (!other.value(key).isNull() && other.value(key).isValid())
            result.insert(key, other.value(key));
    }

    return result;
}

static QString sessionName(const quint32 id, const QString &method)
{
   return QString::number(id) + QLatin1String("+") + method;
}

static pid_t pidOfContext(const QDBusConnection &connection, const QDBusMessage &message)
{
    return connection.interface()->servicePid(message.service()).value();
}

SignonSessionCore::SignonSessionCore(quint32 id,
                                     const QString &method,
                                     int timeout,
                                     SignonDaemon *parent) :
                                     SignonDisposable(timeout, parent),
                                     m_id(id),
                                     m_method(method),
                                     m_passwordUpdate(QString())
{
    TRACE();
    m_signonui = NULL;
    m_watcher = NULL;

    if (!(m_encryptor = new Encryptor))
        qFatal("Cannot allocate memory for encryptor");

    m_signonui = new SignonUiAdaptor(
                                    SIGNON_UI_SERVICE,
                                    SIGNON_UI_DAEMON_OBJECTPATH,
                                    QDBusConnection::sessionBus());
}

SignonSessionCore::~SignonSessionCore()
{
    TRACE();

    delete m_plugin;
    delete m_watcher;
    delete m_signonui;
    delete m_encryptor;

    m_plugin = NULL;
    m_signonui = NULL;
    m_watcher = NULL;
}

SignonSessionCore *SignonSessionCore::sessionCore(const quint32 id, const QString &method, SignonDaemon *parent)
{
    TRACE();
    QString objectName;
    QString key = sessionName(id, method);

    if (id) {
        if (sessionsOfStoredCredentials.contains(key)) {
            return sessionsOfStoredCredentials.value(key);
        }
    }

    SignonSessionCore *ssc = new SignonSessionCore(id, method, parent->authSessionTimeout(), parent);

    if (ssc->setupPlugin() == false) {
        TRACE() << "The resulted object is corrupted and has to be deleted";
        delete ssc;
        return NULL;
    }

    if (id)
        sessionsOfStoredCredentials.insert(key, ssc);
    else
        sessionsOfNonStoredCredentials.append(ssc);

    TRACE() << "The new session is created :" << key;
    return ssc;
}

quint32 SignonSessionCore::id() const
{
    TRACE();
    keepInUse();
    return m_id;
}

QString SignonSessionCore::method() const
{
    TRACE();
    keepInUse();
    return m_method;
}

bool SignonSessionCore::setupPlugin()
{
    m_plugin = PluginProxy::createNewPluginProxy(m_method);

    if (!m_plugin) {
        TRACE() << "Plugin of type " << m_method << " cannot be found";
        return false;
    }

    connect(m_plugin,
            SIGNAL(processResultReply(const QString&, const QVariantMap&)),
            this,
            SLOT(processResultReply(const QString&, const QVariantMap&)),
            Qt::DirectConnection);

    connect(m_plugin,
            SIGNAL(processStore(const QString&, const QVariantMap&)),
            this,
            SLOT(processStore(const QString&, const QVariantMap&)),
            Qt::DirectConnection);

    connect(m_plugin,
            SIGNAL(processUiRequest(const QString&, const QVariantMap&)),
            this,
            SLOT(processUiRequest(const QString&, const QVariantMap&)),
            Qt::DirectConnection);

    connect(m_plugin,
            SIGNAL(processRefreshRequest(const QString&, const QVariantMap&)),
            this,
            SLOT(processRefreshRequest(const QString&, const QVariantMap&)),
            Qt::DirectConnection);

    connect(m_plugin,
            SIGNAL(processError(const QString&, int, const QString&)),
            this,
            SLOT(processError(const QString&, int, const QString&)),
            Qt::DirectConnection);

    connect(m_plugin,
            SIGNAL(stateChanged(const QString&, int, const QString&)),
            this,
            SLOT(stateChangedSlot(const QString&, int, const QString&)),
            Qt::DirectConnection);

    return true;
}

void SignonSessionCore::stopAllAuthSessions()
{
    qDeleteAll(sessionsOfStoredCredentials);
    sessionsOfStoredCredentials.clear();

    qDeleteAll(sessionsOfNonStoredCredentials);
    sessionsOfNonStoredCredentials.clear();
}

QStringList SignonSessionCore::loadedPluginMethods(const QString &method)
{
    foreach (SignonSessionCore *corePtr, sessionsOfStoredCredentials) {
        if (corePtr->method() == method)
            return corePtr->queryAvailableMechanisms(QStringList());
    }

    foreach (SignonSessionCore *corePtr, sessionsOfNonStoredCredentials) {
        if (corePtr->method() == method)
            return corePtr->queryAvailableMechanisms(QStringList());
    }

    return QStringList();
}

QStringList SignonSessionCore::queryAvailableMechanisms(const QStringList &wantedMechanisms)
{
    keepInUse();

    if (!wantedMechanisms.size())
        return m_plugin->mechanisms();

    return m_plugin->mechanisms().toSet().intersect(wantedMechanisms.toSet()).toList();
}

void SignonSessionCore::process(const QDBusConnection &connection,
                                 const QDBusMessage &message,
                                 const QVariantMap &sessionDataVa,
                                 const QString &mechanism,
                                 const QString &cancelKey)
{
    keepInUse();
    if (m_encryptor->isVariantMapEncrypted(sessionDataVa)) {
        pid_t pid = pidOfContext(connection, message);
        QVariantMap decodedData(m_encryptor->
                                decodeVariantMap(sessionDataVa, pid));
        if (m_encryptor->status() != Encryptor::Ok) {
            replyError(connection,
                       message,
                       Error::EncryptionFailure,
                       QString::fromLatin1("Failed to decrypt incoming message"));
            return;
        }
        m_listOfRequests.enqueue(RequestData(connection,
                                             message,
                                             decodedData,
                                             mechanism,
                                             cancelKey));
    } else {
        m_listOfRequests.enqueue(RequestData(connection,
                                             message,
                                             sessionDataVa,
                                             mechanism,
                                             cancelKey));
    }
    QMetaObject::invokeMethod(this, "startNewRequest", Qt::QueuedConnection);
}

void SignonSessionCore::cancel(const QString &cancelKey)
{
    TRACE();

    int requestIndex;
    for (requestIndex = 0; requestIndex < m_listOfRequests.size(); requestIndex++) {
        if (m_listOfRequests.at(requestIndex).m_cancelKey == cancelKey)
            break;
    }

    TRACE() << "The request is found with index " << requestIndex;

    if (requestIndex < m_listOfRequests.size()) {
        if (requestIndex == 0) {
            m_canceled = cancelKey;
            m_plugin->cancel();

            if (m_watcher && !m_watcher->isFinished()) {
                m_signonui->cancelUiRequest(cancelKey);
                delete m_watcher;
                m_watcher = 0;
            }
        }

        /*
         * We must let to the m_listOfRequests to have the canceled request data
         * in order to delay the next request execution until the actual cancelation
         * will happen. We will know about that precisely: plugin must reply via
         * resultSlot or via errorSlot.
         * */
        RequestData rd((requestIndex == 0 ?
                        m_listOfRequests.head() :
                        m_listOfRequests.takeAt(requestIndex)));

        QDBusMessage errReply = rd.m_msg.createErrorReply(SIGNOND_SESSION_CANCELED_ERR_NAME,
                                                          SIGNOND_SESSION_CANCELED_ERR_STR);
        rd.m_conn.send(errReply);
        TRACE() << "Size of the queue is " << m_listOfRequests.size();
    }
}

void SignonSessionCore::setId(quint32 id)
{
    keepInUse();

    if (m_id == id)
        return;

    QString key;

    if (id == 0) {
        key = sessionName(m_id, m_method);
        sessionsOfNonStoredCredentials.append(sessionsOfStoredCredentials.take(key));
    } else {
        key = sessionName(id, m_method);
        if (sessionsOfStoredCredentials.contains(key)) {
            qCritical() << "attempt to assign existing id";
            return;
        }

        sessionsOfNonStoredCredentials.removeOne(this);
        sessionsOfStoredCredentials[key] = this;
    }
    m_id = id;
}

void SignonSessionCore::startProcess()
{

    TRACE() << "the number of requests is : " << m_listOfRequests.length();

    keepInUse();

    RequestData data = m_listOfRequests.head();
    QVariantMap parameters = data.m_params;

    if (m_id) {
        CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
        if (db != NULL) {
            SignonIdentityInfo info = db->credentials(m_id);
            if (info.id() != SIGNOND_NEW_IDENTITY) {
                if (!parameters.contains(SSO_KEY_PASSWORD))
                    parameters[SSO_KEY_PASSWORD] = info.password();
                //database overrules over sessiondata for validated username,
                //so that identity cannot be misused
                if (info.validated() || !parameters.contains(SSO_KEY_USERNAME))
                    parameters[SSO_KEY_USERNAME] = info.userName();

                pid_t pid = pidOfContext(data.m_conn, data.m_msg);
                QSet<QString> clientTokenSet = AccessControlManager::accessTokens(pid).toSet();
                QSet<QString> identityAclTokenSet = info.accessControlList().toSet();
                QSet<QString> paramsTokenSet = clientTokenSet.intersect(identityAclTokenSet);

                if (!paramsTokenSet.isEmpty()) {
                    QStringList tokenList = paramsTokenSet.toList();
                    parameters[SSO_ACCESS_CONTROL_TOKENS] = tokenList;
                }
            } else {
                BLAME() << "Error occurred while getting data from credentials database.";
                //credentials not available, so authentication probably fails
            }
            QVariantMap storedParams = db->loadData(m_id, m_method);
            //TODO unite might generate multiple entries
            parameters.unite(storedParams);
        } else {
            BLAME() << "Null database handler object.";
        }
    }

    if (parameters.contains(SSOUI_KEY_UIPOLICY)
        && parameters[SSOUI_KEY_UIPOLICY] == RequestPasswordPolicy) {
        parameters.remove(SSO_KEY_PASSWORD);
    }

    if (!m_plugin->process(data.m_cancelKey, parameters, data.m_mechanism)) {
        QDBusMessage errReply = data.m_msg.createErrorReply(SIGNOND_RUNTIME_ERR_NAME,
                                                            SIGNOND_RUNTIME_ERR_STR);
        data.m_conn.send(errReply);
        m_listOfRequests.removeFirst();
        QMetaObject::invokeMethod(this, "startNewRequest", Qt::QueuedConnection);
    } else
        stateChangedSlot(data.m_cancelKey, SignOn::SessionStarted, QLatin1String("The request is started successfully"));
}

void SignonSessionCore::replyError(const QDBusConnection &conn, const QDBusMessage &msg, int err, const QString &message)
{
    keepInUse();

    QString errName;
    QString errMessage;

    //TODO this is needed for old error codes
    if(err < Error::AuthSessionErr) {
        BLAME() << "Deprecated error code: " << err;
            if (message.isEmpty())
                errMessage = SIGNOND_UNKNOWN_ERR_STR;
            else
                errMessage = message;
            errName = SIGNOND_UNKNOWN_ERR_NAME;
    }

    if (Error::AuthSessionErr < err && err < Error::UserErr) {
        switch(err) {
            case Error::MechanismNotAvailable:
                errName = SIGNOND_MECHANISM_NOT_AVAILABLE_ERR_NAME;
                errMessage = SIGNOND_MECHANISM_NOT_AVAILABLE_ERR_STR;
                break;
            case Error::MissingData:
                errName = SIGNOND_MISSING_DATA_ERR_NAME;
                errMessage = SIGNOND_MISSING_DATA_ERR_STR;
                break;
            case Error::InvalidCredentials:
                errName = SIGNOND_INVALID_CREDENTIALS_ERR_NAME;
                errMessage = SIGNOND_INVALID_CREDENTIALS_ERR_STR;
                break;
            case Error::NotAuthorized:
                errName = SIGNOND_NOT_AUTHORIZED_ERR_NAME;
                errMessage = SIGNOND_NOT_AUTHORIZED_ERR_STR;
                break;
            case Error::WrongState:
                errName = SIGNOND_WRONG_STATE_ERR_NAME;
                errMessage = SIGNOND_WRONG_STATE_ERR_STR;
                break;
            case Error::OperationNotSupported:
                errName = SIGNOND_OPERATION_NOT_SUPPORTED_ERR_NAME;
                errMessage = SIGNOND_OPERATION_NOT_SUPPORTED_ERR_STR;
                break;
            case Error::NoConnection:
                errName = SIGNOND_NO_CONNECTION_ERR_NAME;
                errMessage = SIGNOND_NO_CONNECTION_ERR_STR;
                break;
            case Error::Network:
                errName = SIGNOND_NETWORK_ERR_NAME;
                errMessage = SIGNOND_NETWORK_ERR_STR;
                break;
            case Error::Ssl:
                errName = SIGNOND_SSL_ERR_NAME;
                errMessage = SIGNOND_SSL_ERR_STR;
                break;
            case Error::Runtime:
                errName = SIGNOND_RUNTIME_ERR_NAME;
                errMessage = SIGNOND_RUNTIME_ERR_STR;
                break;
            case Error::SessionCanceled:
                errName = SIGNOND_SESSION_CANCELED_ERR_NAME;
                errMessage = SIGNOND_SESSION_CANCELED_ERR_STR;
                break;
            case Error::TimedOut:
                errName = SIGNOND_TIMED_OUT_ERR_NAME;
                errMessage = SIGNOND_TIMED_OUT_ERR_STR;
                break;
            case Error::UserInteraction:
                errName = SIGNOND_USER_INTERACTION_ERR_NAME;
                errMessage = SIGNOND_USER_INTERACTION_ERR_STR;
                break;
            case Error::OperationFailed:
                errName = SIGNOND_OPERATION_FAILED_ERR_NAME;
                errMessage = SIGNOND_OPERATION_FAILED_ERR_STR;
                break;
            case Error::EncryptionFailure:
                errName = SIGNOND_ENCRYPTION_FAILED_ERR_NAME;
                errMessage = SIGNOND_ENCRYPTION_FAILED_ERR_STR;
                break;
            case Error::TOSNotAccepted:
                errName = SIGNOND_TOS_NOT_ACCEPTED_ERR_NAME;
                errMessage = SIGNOND_TOS_NOT_ACCEPTED_ERR_STR;
                break;
            case Error::ForgotPassword:
                errName = SIGNOND_FORGOT_PASSWORD_ERR_NAME;
                errMessage = SIGNOND_FORGOT_PASSWORD_ERR_STR;
                break;
            default:
                if (message.isEmpty())
                    errMessage = SIGNOND_UNKNOWN_ERR_STR;
                else
                    errMessage = message;
                errName = SIGNOND_UNKNOWN_ERR_NAME;
                break;
        };
    }

    if(err > Error::UserErr) {
        errName = SIGNOND_USER_ERROR_ERR_NAME;
        errMessage = (QString::fromLatin1("%1:%2")).arg(err).arg(message);
    }

    QDBusMessage errReply;
    errReply = msg.createErrorReply(errName, ( message.isEmpty() ? errMessage : message ));
    conn.send(errReply);
}

void SignonSessionCore::processResultReply(const QString &cancelKey, const QVariantMap &data)
{
    TRACE();

    keepInUse();

    if (!m_listOfRequests.size())
        return;

    RequestData rd = m_listOfRequests.dequeue();

    if (cancelKey != m_canceled) {
        QVariantList arguments;
        QVariantMap data2 = filterVariantMap(data);

        //update database entry
        if (m_id != SIGNOND_NEW_IDENTITY) {
            CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
            if (db != NULL) {
                SignonIdentityInfo info = db->credentials(m_id);
                //allow update only for not validated username
                if (!info.validated()
                        && data2.contains(SSO_KEY_USERNAME)
                        && !data2[SSO_KEY_USERNAME].toString().isEmpty())
                    info.setUserName(data2[SSO_KEY_USERNAME].toString());
                if (!m_passwordUpdate.isEmpty())
                    info.setPassword(m_passwordUpdate);
                if (data2.contains(SSO_KEY_PASSWORD)
                        && !data2[SSO_KEY_PASSWORD].toString().isEmpty())
                    info.setPassword(data2[SSO_KEY_PASSWORD].toString());
                info.setValidated(true);
                if (!(db->updateCredentials(info)))
                    BLAME() << "Error occured while updating credentials.";
            } else {
                BLAME() << "Error occured while updating credentials. Null database handler object.";
            }
        }

        //remove secret field from output
        if (m_method != QLatin1String("password") && data2.contains(SSO_KEY_PASSWORD))
            data2.remove(SSO_KEY_PASSWORD);

        pid_t pid = pidOfContext(rd.m_conn, rd.m_msg);
        QVariantMap encodedData(m_encryptor->
                                encodeVariantMap(data2, pid));
        if (m_encryptor->status() != Encryptor::Ok) {
            replyError(rd.m_conn,
                       rd.m_msg,
                       Error::EncryptionFailure,
                       QString::fromLatin1("Failed to encrypt outgoing message"));
        } else {
            arguments << encodedData;
            rd.m_conn.send(rd.m_msg.createReply(arguments));
        }

        m_canceled = QString();

        if (m_watcher && !m_watcher->isFinished()) {
            m_signonui->cancelUiRequest(rd.m_cancelKey);
            delete m_watcher;
            m_watcher = 0;
        }
    }

    m_canceled = QString();
    QMetaObject::invokeMethod(this, "startNewRequest", Qt::QueuedConnection);
}

void SignonSessionCore::processStore(const QString &cancelKey, const QVariantMap &data)
{
    Q_UNUSED(cancelKey);
    TRACE();

    keepInUse();
    qint32 id = m_id;
    m_passwordUpdate = QString();
    if (id == SIGNOND_NEW_IDENTITY) {
        BLAME() << "Cannot store without identity";
        return;
    }
    QVariantMap data2 = data;
    //do not store username or password
    if (data2.contains(SSO_KEY_PASSWORD))
        data2.remove(SSO_KEY_PASSWORD);
    if (data2.contains(SSO_KEY_USERNAME))
        data2.remove(SSO_KEY_USERNAME);
    if (data2.contains(SSO_ACCESS_CONTROL_TOKENS))
        data2.remove(SSO_ACCESS_CONTROL_TOKENS);

    //store data into db
    CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
    if (db != NULL) {
        if(!db->storeData(id, m_method, data2)) {
            BLAME() << "Error occured while storing data.";
        }
    } else {
        BLAME() << "Error occured while storing data. Null database handler object.";
    }
    return;
}

void SignonSessionCore::processUiRequest(const QString &cancelKey, const QVariantMap &data)
{
    TRACE();

    keepInUse();

    if (cancelKey != m_canceled && m_listOfRequests.size()) {
        QString uiRequestId = m_listOfRequests.head().m_cancelKey;

        if (m_watcher) {
            if (!m_watcher->isFinished())
                m_signonui->cancelUiRequest(uiRequestId);

            delete m_watcher;
            m_watcher = 0;
        }

        m_listOfRequests.head().m_params = filterVariantMap(data);
        m_listOfRequests.head().m_params[SSOUI_KEY_REQUESTID] = uiRequestId;

        if (m_id == SIGNOND_NEW_IDENTITY)
            m_listOfRequests.head().m_params[SSOUI_KEY_STORED_IDENTITY] = false;
        else
            m_listOfRequests.head().m_params[SSOUI_KEY_STORED_IDENTITY] = true;

        //check that we have caption
        if (!data.contains(SSO_KEY_CAPTION)) {
            TRACE() << "Caption missing";
            if (m_id != SIGNOND_NEW_IDENTITY) {
                CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
                if (db != NULL) {
                    SignonIdentityInfo info = db->credentials(m_id);
                    m_listOfRequests.head().m_params.insert(SSO_KEY_CAPTION, info.caption());
                    TRACE() << "Got caption: " << info.caption();
                }
            }
        }

        //TODO: figure out how we going to synchronize
        //different login dialogs in order not to show
        //multiple login dialogs for the same credentials
        //data2["CredentialsId"] = m_id;
        m_watcher = new QDBusPendingCallWatcher(m_signonui->queryDialog(m_listOfRequests.head().m_params),
                                                this);
        connect(m_watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(queryUiSlot(QDBusPendingCallWatcher*)));
    }
}

void SignonSessionCore::processRefreshRequest(const QString &cancelKey, const QVariantMap &data)
{
    TRACE();

    keepInUse();

    if (cancelKey != m_canceled && m_listOfRequests.size()) {
        QString uiRequestId = m_listOfRequests.head().m_cancelKey;

        if (m_watcher) {
            if (!m_watcher->isFinished())
                m_signonui->cancelUiRequest(uiRequestId);

            delete m_watcher;
            m_watcher = 0;
        }

        m_listOfRequests.head().m_params = filterVariantMap(data);
        m_watcher = new QDBusPendingCallWatcher(m_signonui->refreshDialog(m_listOfRequests.head().m_params),
                                                this);
        connect(m_watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(queryUiSlot(QDBusPendingCallWatcher*)));
    }
}

void SignonSessionCore::processError(const QString &cancelKey, int err, const QString &message)
{
    TRACE();
    keepInUse();

    if (!m_listOfRequests.size())
        return;

    RequestData rd = m_listOfRequests.dequeue();

    if (cancelKey != m_canceled) {
        replyError(rd.m_conn, rd.m_msg, err, message);

        if (m_watcher && !m_watcher->isFinished()) {
            m_signonui->cancelUiRequest(rd.m_cancelKey);
            delete m_watcher;
            m_watcher = 0;
        }
    }

    m_canceled = QString();
    QMetaObject::invokeMethod(this, "startNewRequest", Qt::QueuedConnection);
}

void SignonSessionCore::stateChangedSlot(const QString &cancelKey, int state, const QString &message)
{
    if (cancelKey != m_canceled && m_listOfRequests.size()) {
        RequestData rd = m_listOfRequests.head();
        emit stateChanged(rd.m_cancelKey, (int)state, message);
    }

    keepInUse();
}

void SignonSessionCore::childEvent(QChildEvent *ce)
{
    if (ce->added())
        keepInUse();
    else if (ce->removed())
        SignonDisposable::destroyUnused();
}

void SignonSessionCore::queryUiSlot(QDBusPendingCallWatcher *call)
{
    keepInUse();

    QDBusPendingReply<QVariantMap> reply = *call;
    bool isRequestToRefresh = false;
    Q_ASSERT_X( m_listOfRequests.size() != 0, __func__, "queue of requests is empty");

    if (!reply.isError() && reply.count()) {
        QVariantMap resultParameters = reply.argumentAt<0>();
        if (resultParameters.contains(SSOUI_KEY_REFRESH)) {
            isRequestToRefresh = true;
            resultParameters.remove(SSOUI_KEY_REFRESH);
        }

        m_listOfRequests.head().m_params = resultParameters;
    } else {
        m_listOfRequests.head().m_params.insert(SSOUI_KEY_ERROR, (int)SignOn::QUERY_ERROR_NO_SIGNONUI);
    }

    if (m_listOfRequests.head().m_cancelKey != m_canceled) {
        if (isRequestToRefresh) {
            TRACE() << "REFRESH IS REQUIRED";

            m_listOfRequests.head().m_params.remove(SSOUI_KEY_REFRESH);
            m_plugin->processRefresh(m_listOfRequests.head().m_cancelKey,
                                     m_listOfRequests.head().m_params);
        } else {
            if (m_listOfRequests.head().m_params.contains(SSO_KEY_PASSWORD))
                m_passwordUpdate = m_listOfRequests.head().m_params[SSO_KEY_PASSWORD].toString();
            m_plugin->processUi(m_listOfRequests.head().m_cancelKey,
                                m_listOfRequests.head().m_params);
        }
    }

    delete m_watcher;
    m_watcher = NULL;
}

void SignonSessionCore::startNewRequest()
{
    keepInUse();

    // there is no request
    if (!m_listOfRequests.length()) {
        TRACE() << "the data queue is EMPTY!!!";
        return;
    }

    //plugin is busy
    if (m_plugin && m_plugin->isProcessing()) {
        TRACE() << " the plugin is in challenge processing";
        return;
    }

    //there is some UI operation with plugin
    if (m_watcher && !m_watcher->isFinished()) {
        TRACE() << "watcher is in running mode";
        return;
    }

    TRACE() << "Start the authentication process";
    startProcess();
}

void SignonSessionCore::destroy()
{
    if (m_plugin->isProcessing() ||
        m_watcher != NULL) {
        keepInUse();
        return;
    }

    if (m_id)
        sessionsOfStoredCredentials.remove(sessionName(m_id, m_method));
    else
        sessionsOfNonStoredCredentials.removeOne(this);

    emit destroyed();
    deleteLater();
}
