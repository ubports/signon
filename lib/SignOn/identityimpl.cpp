/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
 * Contact: Aurel Popirtac <ext-aurel.popirtac@nokia.com>
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
#include <stdarg.h>

#include <QByteArray>
#include <QDBusArgument>
#include <QTimer>

#include <signond/signoncommon.h>

#include "libsignoncommon.h"
#include "identityimpl.h"
#include "identityinfo.h"
#include "identityinfoimpl.h"
#include "authsessionimpl.h"

#define SIGNOND_AUTH_SESSION_CANCEL_TIMEOUT 5000 //ms

#define SIGNOND_IDENTITY_QUERY_AVAILABLE_METHODS_METHOD \
    SIGNOND_NORMALIZE_METHOD_SIGNATURE("queryAvailableMethods()")
#define SIGNOND_IDENTITY_REQUEST_CREDENTIALS_UPDATE_METHOD \
    SIGNOND_NORMALIZE_METHOD_SIGNATURE("requestCredentialsUpdate(const QString &)")
#define SIGNOND_IDENTITY_STORE_CREDENTIALS_METHOD \
    SIGNOND_NORMALIZE_METHOD_SIGNATURE("storeCredentials(const IdentityInfo &)")
#define SIGNOND_IDENTITY_REMOVE_METHOD \
    SIGNOND_NORMALIZE_METHOD_SIGNATURE("remove()")
#define SIGNOND_IDENTITY_QUERY_INFO_METHOD \
    SIGNOND_NORMALIZE_METHOD_SIGNATURE("queryInfo()")
#define SIGNOND_IDENTITY_VERIFY_USER_METHOD \
    SIGNOND_NORMALIZE_METHOD_SIGNATURE("verifyUser(const QString &)")
#define SIGNOND_IDENTITY_VERIFY_SECRET_METHOD \
    SIGNOND_NORMALIZE_METHOD_SIGNATURE("verifySecret(const QString &)")
#define SIGNOND_IDENTITY_SIGN_OUT_METHOD \
    SIGNOND_NORMALIZE_METHOD_SIGNATURE("signOut()")


namespace SignOn {

    /*
         !!! TODO remove deprecated error signals emition when the time is right. !!!
         *** One month after release of new error management
    */
    IdentityImpl::IdentityImpl(Identity *parent, const quint32 id)
        : QObject(parent),
          m_parent(parent),
          m_identityInfo(new IdentityInfo),
          m_operationQueueHandler(this),
          m_tmpIdentityInfo(NULL),
          m_DBusInterface(NULL),
          m_state(NeedsRegistration),
          m_infoQueried(true),
          m_signOutRequestedByThisIdentity(false)
    {
        m_identityInfo->setId(id);
        sendRegisterRequest();
    }

    IdentityImpl::~IdentityImpl()
    {
        if (m_identityInfo)
            delete m_identityInfo;

        if (m_tmpIdentityInfo)
            delete m_tmpIdentityInfo;

        if (!m_authSessions.empty())
            foreach (AuthSession *session, m_authSessions)
                destroySession(session);
    }

    void IdentityImpl::updateState(State state)
    {
        const char *stateStr;
        switch (state)
        {
            case PendingRegistration: stateStr = "PendingRegistration"; break;
            case NeedsRegistration: stateStr = "NeedsRegistration"; break;
            case NeedsUpdate: stateStr = "NeedsUpdate"; break;
            case Ready: stateStr = "Ready"; break;
            case Removed: stateStr = "Removed"; break;
            default: stateStr = "Unknown"; break;
        }
        TRACE() << "Updating state: " << stateStr;
        m_state = state;
    }

    void IdentityImpl::copyInfo(const IdentityInfo &info)
    {
        m_identityInfo->impl->copy(*(info.impl));
    }

    quint32 IdentityImpl::id() const
    {
       return m_identityInfo->id();
    }

    AuthSession *IdentityImpl::createSession(const QString &methodName, QObject *parent)
    {
        foreach (AuthSession *authSession, m_authSessions) {
            if (authSession->name() == methodName) {
                qWarning() << QString::fromLatin1(
                        "Authentication session for method "
                        "`%1` already requested.").arg(methodName);
                return 0;
            }
        }

        AuthSession *session = new AuthSession(id(), methodName, parent);
        m_authSessions.append(session);
        return session;
    }

    void IdentityImpl::destroySession(AuthSession *session)
    {
        session->blockSignals(true);
        m_authSessions.removeOne(session);
        session->deleteLater();
    }

    void IdentityImpl::queryAvailableMethods()
    {
        TRACE() << "Querying available identity authentication methods.";
        checkConnection();

        switch (m_state) {
            case NeedsRegistration:
                m_operationQueueHandler.enqueueOperation(
                                            SIGNOND_IDENTITY_QUERY_AVAILABLE_METHODS_METHOD);
                sendRegisterRequest();
                break;
            case PendingRegistration:
                m_operationQueueHandler.enqueueOperation(
                                            SIGNOND_IDENTITY_QUERY_AVAILABLE_METHODS_METHOD);
                break;
            case NeedsUpdate:
                m_operationQueueHandler.enqueueOperation(
                                            SIGNOND_IDENTITY_QUERY_AVAILABLE_METHODS_METHOD);

                /* This flag tells the queryInfo() reply slot that the current query
                   should not reply with the 'info()' signal */
                m_infoQueried = false;
                updateContents();
                break;
            case Removed:
                emit m_parent->error(Identity::NotFoundError,
                                     QLatin1String("Removed from database."));
                emit m_parent->error(Error(Error::IdentityNotFound,
                                     QLatin1String("Removed from database.")));
                return;
            case Ready:
                /* fall trough */
            default:
                emit m_parent->methodsAvailable(m_identityInfo->methods());
        }
    }

    void IdentityImpl::requestCredentialsUpdate(const QString &message)
    {
        TRACE() << "Requesting credentials update.";
        checkConnection();

        switch (m_state) {
            case NeedsRegistration:
                m_operationQueueHandler.enqueueOperation(
                                SIGNOND_IDENTITY_REQUEST_CREDENTIALS_UPDATE_METHOD,
                                QList<QGenericArgument *>() << (new Q_ARG(QString, message)));
                sendRegisterRequest();
                return;
            case PendingRegistration:
                m_operationQueueHandler.enqueueOperation(
                                SIGNOND_IDENTITY_REQUEST_CREDENTIALS_UPDATE_METHOD,
                                QList<QGenericArgument *>() << (new Q_ARG(QString, message)));
                return;
            case NeedsUpdate:
                break;
            case Removed:
                emit m_parent->error(Identity::NotFoundError,
                                     QLatin1String("Removed from database."));
                emit m_parent->error(
                        Error(Error::IdentityNotFound,
                              QLatin1String("Removed from database.")));
                return;
            case Ready:
                /* fall trough */
            default:
                break;
        }

        QList<QVariant> args;
        args << message;
        bool result = sendRequest(__func__, args,
                                  SLOT(storeCredentialsReply(const quint32)));
        if (!result) {
            TRACE() << "Error occurred.";
            emit m_parent->error(Identity::UnknownError,
                                 QLatin1String("DBUS Communication error occurred."));
            emit m_parent->error(
                    Error(Error::InternalCommunication,
                          SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
        }
    }

    void IdentityImpl::storeCredentials(const IdentityInfo &info)
    {
        TRACE() << "Storing credentials";
        checkConnection();

        switch (m_state) {
            case NeedsRegistration:
                {
                IdentityInfo localInfo =
                    info.impl->isEmpty() ? *m_identityInfo : *(m_tmpIdentityInfo = new IdentityInfo(info));

                m_operationQueueHandler.enqueueOperation(
                                        SIGNOND_IDENTITY_STORE_CREDENTIALS_METHOD,
                                        QList<QGenericArgument *>() << (new Q_ARG(SignOn::IdentityInfo, localInfo)));
                sendRegisterRequest();
                return;
                }
            case PendingRegistration:
                {
                IdentityInfo localInfo =
                    info.impl->isEmpty() ? *m_identityInfo : *(m_tmpIdentityInfo = new IdentityInfo(info));
                m_operationQueueHandler.enqueueOperation(
                                        SIGNOND_IDENTITY_STORE_CREDENTIALS_METHOD,
                                        QList<QGenericArgument *>() << (new Q_ARG(SignOn::IdentityInfo, localInfo)));
                return;
                }
            case Removed:
                break;
            case NeedsUpdate:
                break;
            case Ready:
                /* fall trough */
            default:
                break;
        }

        if (info.impl->isEmpty()) {
            emit m_parent->error(
                Error(Error::StoreFailed,
                      QLatin1String("Invalid Identity data.")));
            return;
        }

        QList<QVariant> args;

        args << m_identityInfo->id()
             << info.userName()
             << info.secret()
             << info.isStoringSecret()
             << QVariant(info.impl->m_authMethods)
             << info.caption()
             << info.realms()
             << QVariant(info.accessControlList())
             << info.type()
             << info.refCount();

        TRACE() << args;

        bool result = sendRequest(__func__, args,
                                  SLOT(storeCredentialsReply(const quint32)));
        if (!result) {
            TRACE() << "Error occurred.";
            emit m_parent->error(Identity::UnknownError,
                                 QLatin1String("DBUS Communication error occurred."));
            emit m_parent->error(
                    Error(Error::InternalCommunication,
                          SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
        }
    }

    void IdentityImpl::remove()
    {
        TRACE() << "Removing credentials.";

        /* If the Identity is not stored, it makes no sense to request a removal
            -- this condition could be removed; there is the case when there is an ongoing
               store operation.
        */

        if (id() != SIGNOND_NEW_IDENTITY) {
            checkConnection();

            switch (m_state) {
                case NeedsRegistration:
                    m_operationQueueHandler.enqueueOperation(SIGNOND_IDENTITY_REMOVE_METHOD);
                    sendRegisterRequest();
                    return;
                case PendingRegistration:
                    m_operationQueueHandler.enqueueOperation(SIGNOND_IDENTITY_REMOVE_METHOD);
                    return;
                case Removed:
                    emit m_parent->error(Identity::NotFoundError,
                                         QLatin1String("Already removed from database."));
                    emit m_parent->error(
                            Error(Error::IdentityNotFound,
                                  QLatin1String("Already removed from database.")));
                    return;
                case NeedsUpdate:
                    break;
                case Ready:
                /* fall trough */
                default:
                    break;
            }

            bool result = sendRequest(__func__, QList<QVariant>(),
                                      SLOT(removeReply()));
            if (!result) {
                TRACE() << "Error occurred.";
                emit m_parent->error(Identity::UnknownError,
                                     QLatin1String("DBUS Communication error occurred."));
                emit m_parent->error(
                        Error(Error::InternalCommunication,
                              SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
            }
        } else {
            emit m_parent->error(Identity::UnknownError,
                                 QLatin1String("Remove request failed. The identity is not stored"));
            emit m_parent->error(
                    Error(Error::Unknown,
                          QLatin1String("Remove request failed. The identity is not stored")));
        }
    }

    void IdentityImpl::queryInfo()
    {
        TRACE() << "Querying info.";
        checkConnection();

        switch (m_state) {
            case NeedsRegistration:
                m_operationQueueHandler.enqueueOperation(SIGNOND_IDENTITY_QUERY_INFO_METHOD);
                sendRegisterRequest();
                return;
            case PendingRegistration:
                m_operationQueueHandler.enqueueOperation(SIGNOND_IDENTITY_QUERY_INFO_METHOD);
                return;
            case Removed:
                emit m_parent->error(Identity::NotFoundError,
                                     QLatin1String("Removed from database."));
                emit m_parent->error(
                        Error(Error::IdentityNotFound,
                              QLatin1String("Removed from database.")));
                return;
            case NeedsUpdate:
                m_infoQueried = true;
                updateContents();
                break;
            case Ready:
                emit m_parent->info(IdentityInfo(*m_identityInfo));
                return;
            default:
                break;
        }
    }

    void IdentityImpl::verifyUser(const QString &message)
    {
        TRACE() << "Verifying user.";
        checkConnection();

        switch (m_state) {
            case NeedsRegistration:
                m_operationQueueHandler.enqueueOperation(
                                        SIGNOND_IDENTITY_VERIFY_USER_METHOD,
                                        QList<QGenericArgument *>() << (new Q_ARG(QString, message)));
                sendRegisterRequest();
                return;
            case PendingRegistration:
                m_operationQueueHandler.enqueueOperation(
                                        SIGNOND_IDENTITY_VERIFY_USER_METHOD,
                                        QList<QGenericArgument *>() << (new Q_ARG(QString, message)));
                return;
            case Removed:
                emit m_parent->error(Identity::NotFoundError,
                                 QLatin1String("Removed from database."));
                emit m_parent->error(
                        Error(Error::IdentityNotFound,
                              QLatin1String("Removed from database.")));
                return;
            case NeedsUpdate:
                break;
            case Ready:
                /* fall trough */
            default:
                break;
        }

        bool result = sendRequest(__func__, QList<QVariant>() << message,
                                  SLOT(verifyUserReply(const bool)));
        if (!result) {
            TRACE() << "Error occurred.";
            emit m_parent->error(Identity::UnknownError,
                                 QLatin1String("DBUS Communication error occurred."));
            emit m_parent->error(
                    Error(Error::InternalCommunication,
                          SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
        }
    }

    void IdentityImpl::verifySecret(const QString &secret)
    {
        TRACE() << "Verifying secret." << secret;
        checkConnection();

        switch (m_state) {
            case NeedsRegistration:
                m_operationQueueHandler.enqueueOperation(
                                        SIGNOND_IDENTITY_VERIFY_SECRET_METHOD,
                                        QList<QGenericArgument *>() << (new Q_ARG(QString, secret)));
                sendRegisterRequest();
                return;
            case PendingRegistration:
                m_operationQueueHandler.enqueueOperation(
                                        SIGNOND_IDENTITY_VERIFY_SECRET_METHOD,
                                        QList<QGenericArgument *>() << (new Q_ARG(QString, secret)));
                return;
            case Removed:
                emit m_parent->error(Identity::NotFoundError,
                                 QLatin1String("Removed from database."));
                emit m_parent->error(
                        Error(Error::IdentityNotFound,
                              QLatin1String("Removed from database.")));
                return;
            case NeedsUpdate:
                break;
            case Ready:
                /* fall trough */
            default:
                break;
        }

        bool result = sendRequest(__func__, QList<QVariant>() << QVariant(secret),
                                  SLOT(verifySecretReply(const bool)));
        if (!result) {
            TRACE() << "Error occurred.";
            emit m_parent->error(Identity::UnknownError,
                                 QLatin1String("DBUS Communication error occurred."));
            emit m_parent->error(
                    Error(Error::InternalCommunication,
                          SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
        }
    }

    void IdentityImpl::signOut()
    {
        TRACE() << "Signing out.";
        checkConnection();

        /* if this is a stored identity, inform server about signing out
           so that other client identity objects having the same id will
           be able to perform the operation.
        */
        if (id() != SIGNOND_NEW_IDENTITY) {
            switch (m_state) {
                case NeedsRegistration:
                    m_operationQueueHandler.enqueueOperation(SIGNOND_IDENTITY_SIGN_OUT_METHOD);
                    sendRegisterRequest();
                    return;
                case PendingRegistration:
                    m_operationQueueHandler.enqueueOperation(SIGNOND_IDENTITY_SIGN_OUT_METHOD);
                    return;
                case Removed:
                    break;
                case NeedsUpdate:
                    break;
                case Ready:
                    break;
                default:
                    break;
            }

            bool result = sendRequest(__func__, QList<QVariant>(),
                                      SLOT(signOutReply()));
            if (!result) {
                TRACE() << "Error occurred.";
                emit m_parent->error(Identity::UnknownError,
                                     QLatin1String("DBUS Communication error occurred."));
                emit m_parent->error(
                        Error(Error::InternalCommunication,
                              SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
            } else {
                m_signOutRequestedByThisIdentity = true;
            }
        }

        clearAuthSessionsCache();
        TRACE() << "SIGN OUT REQUESTER" << QString().sprintf("%08X", (int)this);
    }

    void IdentityImpl::clearAuthSessionsCache()
    {
        while (!m_authSessions.empty()) {
            AuthSession *session = m_authSessions.takeFirst();
            connect(session,
                    SIGNAL(error(AuthSession::AuthSessionError, const QString &)),
                    this,
                    SLOT(authSessionCancelReply(AuthSession::AuthSessionError)));

            session->cancel();
            QTimer::singleShot(SIGNOND_AUTH_SESSION_CANCEL_TIMEOUT, session, SLOT(deleteLater()));
        }
    }

    void IdentityImpl::authSessionCancelReply(AuthSession::AuthSessionError error)
    {
        TRACE() << "CANCEL SESSION REPLY";

        bool deleteTheSender = false;
        switch (error) {
            /* fall trough */
            case AuthSession::CanceledError:
            case AuthSession::WrongStateError: deleteTheSender = true; break;
            default: break;
        }

        if (deleteTheSender) {
            QObject *sender = QObject::sender();
            if (sender) {
                TRACE() << "DELETING SESSION";
                sender->deleteLater();
            }
        }
    }

    void IdentityImpl::storeCredentialsReply(const quint32 id)
    {
        if (id != this->id()) {
            m_identityInfo->setId(id);
            foreach (AuthSession *session, m_authSessions)
                session->impl->setId(id);
        }

        if (m_tmpIdentityInfo) {
            *m_identityInfo = *m_tmpIdentityInfo;
            delete m_tmpIdentityInfo;
            m_tmpIdentityInfo = NULL;
        }
        emit m_parent->credentialsStored(id);
    }

    void IdentityImpl::removeReply()
    {
        m_identityInfo->impl->clear();
        updateState(Removed);
        emit m_parent->removed();
    }

    void IdentityImpl::queryInfoReply(const QList<QVariant> &infoData)
    {
        updateCachedData(infoData);
        updateState(Ready);

        if (m_infoQueried)
            emit m_parent->info(IdentityInfo(*m_identityInfo));
        else
            emit m_parent->methodsAvailable(m_identityInfo->methods());

        m_infoQueried = true;
    }

    void IdentityImpl::verifyUserReply(const bool valid)
    {
        emit m_parent->userVerified(valid);
    }

    void IdentityImpl::verifySecretReply(const bool valid)
    {
        emit m_parent->secretVerified(valid);
    }

    void IdentityImpl::signOutReply()
    {
        TRACE() << "SIGN OUT REQUESTER REPLY" << QString().sprintf("%08X", (int)this);
        emit m_parent->signedOut();
    }

    void IdentityImpl::infoUpdated(int state)
    {
        const char *stateStr;
        switch ((IdentityState)state) {
            /* Data updated on the server side. */
            case IdentityDataUpdated:
                updateState(NeedsUpdate);
                stateStr = "NeedsUpdate";
                break;
            /* Data removed on the server side. */
            case IdentityRemoved:
                updateState(Removed);
                stateStr = "Removed";
                break;
            /* A remote client identity signed out,
               thus server informed this object to do the same */
            case IdentitySignedOut:
                TRACE() << "SIGN OUT SIGNAL RECEIVED" << QString().sprintf("%08X", (int)this);

                //if this is not the identity that requested the signing out
                if (!m_signOutRequestedByThisIdentity) {
                    clearAuthSessionsCache();
                    emit m_parent->signedOut();
                }
                stateStr = "SignedOut";
                break;
            default: stateStr = "Unknown";
                break;
        }
        TRACE() << "\n\nSERVER INFO UPDATED." << stateStr << QString(QLatin1String(" %1 ")).arg(id()) << "\n\n";
    }

    void IdentityImpl::errorReply(const QDBusError& err)
    {
        TRACE() << err.name();

        /* Signon specific errors */
        if (err.name() == SIGNOND_UNKNOWN_ERR_NAME) {
            emit m_parent->error(Identity::UnknownError, err.message());
            emit m_parent->error(Error(Error::Unknown, err.message()));
            return;
        } else if (err.name() == SIGNOND_INTERNAL_SERVER_ERR_NAME) {
            emit m_parent->error(Identity::InternalServerError, err.message());
            emit m_parent->error(Error(Error::InternalServer, err.message()));
            return;
        } else if (err.name() == SIGNOND_IDENTITY_NOT_FOUND_ERR_NAME) {
            emit m_parent->error(Identity::NotFoundError, err.message());
            emit m_parent->error(Error(Error::IdentityNotFound, err.message()));
            return;
        } else if (err.name() == SIGNOND_METHOD_NOT_AVAILABLE_ERR_NAME) {
            emit m_parent->error(Identity::MethodNotAvailableError, err.message());
            emit m_parent->error(Error(Error::MethodNotAvailable, err.message()));
            return;
        } else if (err.name() == SIGNOND_PERMISSION_DENIED_ERR_NAME) {
            emit m_parent->error(Identity::PermissionDeniedError, err.message());
            emit m_parent->error(Error(Error::PermissionDenied, err.message()));
            return;
        } else if (err.name() == SIGNOND_PERMISSION_DENIED_ERR_NAME) {
            emit m_parent->error(Identity::PermissionDeniedError, err.message());
            emit m_parent->error(Error(Error::PermissionDenied, err.message()));
            return;
        } else if (err.name() == SIGNOND_STORE_FAILED_ERR_NAME) {
            emit m_parent->error(Identity::StoreFailedError, err.message());
            emit m_parent->error(Error(Error::StoreFailed, err.message()));
            if (m_tmpIdentityInfo) {
                delete m_tmpIdentityInfo;
                m_tmpIdentityInfo = NULL;
            }
            return;
        } else if (err.name() == SIGNOND_REMOVE_FAILED_ERR_NAME) {
            emit m_parent->error(Identity::RemoveFailedError, err.message());
            emit m_parent->error(Error(Error::RemoveFailed, err.message()));
            return;
        } else if (err.name() == SIGNOND_SIGNOUT_FAILED_ERR_NAME) {
            emit m_parent->error(Identity::SignOutFailedError, err.message());
            emit m_parent->error(Error(Error::SignOutFailed, err.message()));
            return;
        } else if (err.name() == SIGNOND_IDENTITY_OPERATION_CANCELED_ERR_NAME) {
            emit m_parent->error(Identity::CanceledError, err.message());
            emit m_parent->error(Error(Error::IdentityOperationCanceled, err.message()));
            return;
        } else if (err.name() == SIGNOND_CREDENTIALS_NOT_AVAILABLE_ERR_NAME) {
            emit m_parent->error(Identity::CredentialsNotAvailableError, err.message());
            emit m_parent->error(Error(Error::CredentialsNotAvailable, err.message()));
            return;
        }
        else {
            if (m_state == this->PendingRegistration)
                updateState(NeedsRegistration);

            TRACE() << "Non internal SSO error reply.";
        }

        /* Qt DBUS specific errors */
        if (err.type() != QDBusError::NoError) {
            emit m_parent->error(Identity::UnknownError, err.message());
            emit m_parent->error(Error(Error::InternalCommunication, err.message()));
            return;
        }

        emit m_parent->error(Identity::UnknownError, err.message());
        emit m_parent->error(Error(Error::Unknown, err.message()));
    }

    void IdentityImpl::updateContents()
    {
        bool result = sendRequest("queryInfo", QList<QVariant>(),
                                  SLOT(queryInfoReply(const QList<QVariant> &)));

        if (!result) {
            TRACE() << "Error occurred.";
            emit m_parent->error(Identity::UnknownError,
                                 SIGNOND_UNKNOWN_ERR_STR);
            emit m_parent->error(
                    Error(Error::InternalCommunication,
                          SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
        }
    }

    bool IdentityImpl::sendRequest(const char *remoteMethod, const QList<QVariant> &args, const char *replySlot)
    {
        return m_DBusInterface->callWithCallback(QLatin1String(remoteMethod),
                                                  args,
                                                  this,
                                                  replySlot,
                                                  SLOT(errorReply(const QDBusError&)));
    }

    bool IdentityImpl::sendRegisterRequest()
    {
        QDBusInterface iface(SIGNOND_SERVICE,
                             SIGNOND_DAEMON_OBJECTPATH,
                             SIGNOND_DAEMON_INTERFACE,
                             SIGNOND_BUS);

        if (!iface.isValid()) {
            TRACE() << "Signon Daemon not started. Start on demand "
                       "could delay the first call's result.";
            if (iface.lastError().isValid()) {
                QDBusError err = iface.lastError();
                TRACE() << "\nError name:" << err.name()
                        << "\nMessage: " << err.message()
                        << "\nType: " << QDBusError::errorString(err.type());

                m_operationQueueHandler.clearOperationsQueue();
                updateState(NeedsRegistration);
                return false;
            }
        }

        QList<QVariant> args;
        QString registerMethodName = QLatin1String("registerNewIdentity");
        QByteArray registerReplyMethodName =
            SLOT(registerReply(const QDBusObjectPath &));

        if (id() != SIGNOND_NEW_IDENTITY) {
            registerMethodName = QLatin1String("registerStoredIdentity");
            args << m_identityInfo->id();
            registerReplyMethodName =
                SLOT(registerReply(const QDBusObjectPath &, const QList<QVariant> &));
        }

        if (!iface.callWithCallback(
                                registerMethodName,
                                args,
                                this,
                                registerReplyMethodName.data(),
                                SLOT(errorReply(const QDBusError &)))) {

            QDBusError err = iface.lastError();
            TRACE() << "\nError name:" << err.name()
                    << "\nMessage: " << err.message()
                    << "\nType: " << QDBusError::errorString(err.type());
            return false;
        }
        updateState(PendingRegistration);
        return true;
    }

    void IdentityImpl::updateCachedData(const QList<QVariant> &infoDataConst)
    {
        QList<QVariant> infoData = infoDataConst;
        if (!infoData.isEmpty())
            m_identityInfo->setId(infoData.takeFirst().toUInt());

        if (!infoData.isEmpty())
            m_identityInfo->setUserName(infoData.takeFirst().toString());

        if (!infoData.isEmpty())
            m_identityInfo->setSecret(infoData.takeFirst().toString());

        if (!infoData.isEmpty())
            m_identityInfo->setCaption(infoData.takeFirst().toString());

        if (!infoData.isEmpty())
            m_identityInfo->setRealms(infoData.takeFirst().toStringList());

        if (!infoData.isEmpty()) {
            QDBusArgument arg(infoData.takeFirst().value<QDBusArgument>());
            QMap<QString, QVariant> map = qdbus_cast<QMap<QString, QVariant> >(arg);
            QMapIterator<QString, QVariant> it(map);
            while (it.hasNext()) {
                it.next();
                m_identityInfo->setMethod(it.key(), it.value().toStringList());
            }
        }

        if (!infoData.isEmpty())
            m_identityInfo->setType((IdentityInfo::CredentialsType)(infoData.takeFirst().toInt()));

        if (!infoData.isEmpty())
            m_identityInfo->setRefCount((infoData.takeFirst().toInt()));

    }

    void IdentityImpl::checkConnection()
    {
        if (m_state == PendingRegistration || m_state == NeedsRegistration)
            return;

        if (!m_DBusInterface
            || !m_DBusInterface->isValid()
            || m_DBusInterface->lastError().isValid())
        {
            updateState(NeedsRegistration);
            m_operationQueueHandler.stopOperationsProcessing();
        }
    }

    void IdentityImpl::registerReply(const QDBusObjectPath &objectPath)
    {
        registerReply(objectPath, QList<QVariant>());
    }

    void IdentityImpl::registerReply(const QDBusObjectPath &objectPath, const QList<QVariant> &infoData)
    {
        m_DBusInterface = new QDBusInterface(SIGNOND_SERVICE,
                                             objectPath.path(),
                                             QString(),
                                             SIGNOND_BUS,
                                             this);
        if (!m_DBusInterface->isValid()) {
            TRACE() << "The interface cannot be registered!!! " << m_DBusInterface->lastError();
            updateState(NeedsRegistration);

            delete m_DBusInterface;
            m_DBusInterface = NULL;

            int count = m_operationQueueHandler.queuedOperationsCount();
            for (int i = 0; i < count; ++i) {
                emit m_parent->error(Identity::UnknownError,
                                     QLatin1String("Could not establish valid "
                                                   "connection to remote object."));
                emit m_parent->error(
                        Error(Error::Unknown,
                              QLatin1String("Could not establish valid "
                                            "connection to remote object.")));
            }
            return;
        }

        connect(
                m_DBusInterface,
                SIGNAL(infoUpdated(int)),
                this,
                SLOT(infoUpdated(int)));

        connect(m_DBusInterface, SIGNAL(unregistered()), SLOT(removeObjectDestroyed()));

        if (!infoData.empty())
            updateCachedData(infoData);

        updateState(Ready);
        if (m_operationQueueHandler.queuedOperationsCount() > 0)
            m_operationQueueHandler.execQueuedOperations();
    }

    void IdentityImpl::removeObjectDestroyed()
    {
        updateState(NeedsRegistration);
    }

} //namespace SignOn
