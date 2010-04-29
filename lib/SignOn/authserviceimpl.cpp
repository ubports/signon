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
#include <QDBusArgument>
#include <QDBusConnectionInterface>
#include <QTimer>

#include <signond/signoncommon.h>

#include "libsignoncommon.h"
#include "identityinfo.h"
#include "authserviceimpl.h"
#include "authservice.h"
#include "signonerror.h"


namespace SignOn {

    /* ----------------------- IdentityRegExp ----------------------- */

    AuthService::IdentityRegExp::IdentityRegExp(const QString &pattern)
            : m_pattern(pattern)
    {}

    AuthService::IdentityRegExp::IdentityRegExp(const IdentityRegExp &src)
            : m_pattern(src.pattern())
    {}

    bool AuthService::IdentityRegExp::isValid() const
    {
        return false;
    }

    QString AuthService::IdentityRegExp::pattern() const
    {
        return m_pattern;
    }

    /* ----------------------- AuthServiceImpl ----------------------- */

    /*
         !!! TODO remove deprecated error signals emition when the time is right. !!!
         *** One month after release of new error management
    */
    AuthServiceImpl::AuthServiceImpl(AuthService *parent)
        : m_parent(parent)
    {
        TRACE();
        m_DBusInterface = new QDBusInterface(SIGNOND_SERVICE,
                                             SIGNOND_DAEMON_OBJECTPATH,
                                             SIGNOND_DAEMON_INTERFACE,
                                             SIGNOND_BUS,
                                             this);
        if (!m_DBusInterface->isValid())
            BLAME() << "Signon Daemon not started. Start on demand "
                       "could delay the first call's result.";
    }

    AuthServiceImpl::~AuthServiceImpl()
    {
    }

    void AuthServiceImpl::queryMethods()
    {
        bool result = false;

        if ((!m_DBusInterface->isValid()) && m_DBusInterface->lastError().isValid())
            result = callWithTimeout(QString::fromLatin1(__func__),
                                     SLOT(queryMethodsReply(const QStringList&)));
        else
            result = m_DBusInterface->callWithCallback(
                                                QString::fromLatin1(__func__),
                                                QList<QVariant>(),
                                                this,
                                                SLOT(queryMethodsReply(const QStringList&)),
                                                SLOT(errorReply(const QDBusError &)));
        if (!result) {
            emit m_parent->error(
                    AuthService::InternalCommunicationError,
                    SIGNOND_INTERNAL_COMMUNICATION_ERR_STR);

            emit m_parent->error(
                    Error(Error::InternalCommunication,
                          SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
        }
    }

    void AuthServiceImpl::queryMechanisms(const QString &method)
    {
        bool result = false;

        if ((!m_DBusInterface->isValid()) && m_DBusInterface->lastError().isValid())
            result = callWithTimeout(QString::fromLatin1(__func__),
                                     SLOT(queryMechanismsReply(const QStringList&)),
                                     QList<QVariant>() << method);
        else
            result = m_DBusInterface->callWithCallback(
                                                QString::fromLatin1(__func__),
                                                QList<QVariant>() << method,
                                                this,
                                                SLOT(queryMechanismsReply(const QStringList &)),
                                                SLOT(errorReply(const QDBusError &)));
        if (!result) {
            emit m_parent->error(
                    AuthService::InternalCommunicationError,
                    SIGNOND_INTERNAL_COMMUNICATION_ERR_STR);

            emit m_parent->error(
                    Error(Error::InternalCommunication,
                          SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
        } else {
            m_methodsForWhichMechsWereQueried.enqueue(method);
        }
    }

    void AuthServiceImpl::queryIdentities(const AuthService::IdentityFilter &filter)
    {
        QList<QVariant> args;
        QMap<QString, QVariant> filterMap;
        if (!filter.empty()) {
            QMapIterator<AuthService::IdentityFilterCriteria,
                         AuthService::IdentityRegExp> it(filter);

            while (it.hasNext()) {
                it.next();

                if (!it.value().isValid())
                    continue;

                const char *criteriaStr = 0;
                switch ((AuthService::IdentityFilterCriteria)it.key()) {
                    case AuthService::AuthMethod: criteriaStr = "AuthMethod"; break;
                    case AuthService::Username: criteriaStr = "Username"; break;
                    case AuthService::Realm: criteriaStr = "Realm"; break;
                    case AuthService::Caption: criteriaStr = "Caption"; break;
                    default: break;
                }
                filterMap.insert(QLatin1String(criteriaStr),
                                 QVariant(it.value().pattern()));
            }

        }
        // todo - check if DBUS supports default args, if yes move this line in the block above
        args << filterMap;

        bool result = false;
        if ((!m_DBusInterface->isValid()) && m_DBusInterface->lastError().isValid())
            result = callWithTimeout(QString::fromLatin1(__func__),
                                     SLOT(queryIdentitiesReply(const QList<QVariant> &)),
                                     args);
        else
            result = m_DBusInterface->callWithCallback(
                                                QString::fromLatin1(__func__),
                                                args,
                                                this,
                                                SLOT(queryIdentitiesReply(const QList<QVariant> &)),
                                                SLOT(errorReply(const QDBusError &)));
        if (!result) {
            emit m_parent->error(
                    AuthService::InternalCommunicationError,
                    SIGNOND_INTERNAL_COMMUNICATION_ERR_STR);

            emit m_parent->error(
                    Error(Error::InternalCommunication,
                          SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
        }
    }

    void AuthServiceImpl::clear()
    {
        bool result = false;
        if ((!m_DBusInterface->isValid()) && m_DBusInterface->lastError().isValid())
            result = callWithTimeout(QLatin1String(__func__),
                                     SLOT(clearReply()));
        else
            result = m_DBusInterface->callWithCallback(QLatin1String(__func__),
                                                QList<QVariant>(),
                                                this,
                                                SLOT(clearReply()),
                                                SLOT(errorReply(const QDBusError &)));
        if (!result) {
            emit m_parent->error(
                    AuthService::InternalCommunicationError,
                    SIGNOND_INTERNAL_COMMUNICATION_ERR_STR);

            emit m_parent->error(
                    Error(Error::InternalCommunication,
                          SIGNOND_INTERNAL_COMMUNICATION_ERR_STR));
        }
    }

    bool AuthServiceImpl::callWithTimeout(const QString &operation,
                                          const char *replySlot,
                                          const QList<QVariant> &args)
    {
        QDBusMessage msg = QDBusMessage::createMethodCall(m_DBusInterface->service(),
                                                          m_DBusInterface->path(),
                                                          m_DBusInterface->interface(),
                                                          operation);
        if (!args.isEmpty())
            msg.setArguments(args);

        return m_DBusInterface->connection().callWithCallback(msg,
                                                              this,
                                                              replySlot,
                                                              SLOT(errorReply(const QDBusError&)),
                                                              SIGNOND_MAX_TIMEOUT);
    }

    void AuthServiceImpl::queryMethodsReply(const QStringList &methods)
    {
        emit m_parent->methodsAvailable(methods);
    }

    void AuthServiceImpl::queryMechanismsReply(const QStringList &mechs)
    {
        TRACE() << mechs;
        QString method;
        if (!m_methodsForWhichMechsWereQueried.empty())
            method = m_methodsForWhichMechsWereQueried.dequeue();

        emit m_parent->mechanismsAvailable(method, mechs);
    }

    void AuthServiceImpl::queryIdentitiesReply(const QList<QVariant> &identitiesData)
    {
        QList<IdentityInfo> infoList;

        QList<QVariant> nonConstData = identitiesData;
        while (!nonConstData.empty()) {
            QDBusArgument arg(nonConstData.takeFirst().value<QDBusArgument>());
            QList<QVariant> identityData = qdbus_cast<QList<QVariant> >(arg);

            quint32 id = identityData.takeFirst().toUInt();
            QString username = identityData.takeFirst().toString();
            QString password = identityData.takeFirst().toString();
            QString caption = identityData.takeFirst().toString();
            QStringList realms = identityData.takeFirst().toStringList();

            arg = QDBusArgument(identityData.takeFirst().value<QDBusArgument>());
            QMap<QString, QVariant> map = qdbus_cast<QMap<QString, QVariant> >(arg);
            QMapIterator<QString, QVariant> it(map);
            QMap<QString, QStringList> authMethods;
            while (it.hasNext()) {
                it.next();
                authMethods.insert(it.key(), it.value().toStringList());
            }

            QStringList accessControlList = identityData.takeFirst().toStringList();
            int type = identityData.takeFirst().toInt();

            IdentityInfo info(caption,
                              username,
                              authMethods);
            info.setId(id);
            info.setSecret(password);
            info.setRealms(realms);
            info.setAccessControlList(accessControlList);
            info.setType((IdentityInfo::CredentialsType)type);

            infoList << info;
        }

        emit m_parent->identities(infoList);
    }

    void AuthServiceImpl::clearReply()
    {
        emit m_parent->cleared();
    }

    void AuthServiceImpl::errorReply(const QDBusError &err)
    {
        TRACE();

        /* Signon specific errors */
        if (err.name() == SIGNOND_UNKNOWN_ERR_NAME) {
            emit m_parent->error(AuthService::UnknownError, err.message());
            emit m_parent->error(Error(Error::Unknown, err.message()));
            return;
        } else if (err.name() == SIGNOND_INTERNAL_SERVER_ERR_NAME) {
            emit m_parent->error(AuthService::InternalServerError, err.message());
            emit m_parent->error(Error(Error::InternalServer, err.message()));
            return;
        } else if (err.name() == SIGNOND_METHOD_NOT_KNOWN_ERR_NAME) {
            emit m_parent->error(AuthService::MethodNotKnownError, err.message());
            emit m_parent->error(Error(Error::MethodNotKnown, err.message()));
            return;
        } else if (err.name() == SIGNOND_INVALID_QUERY_ERR_NAME) {
            emit m_parent->error(AuthService::InvalidQueryError, err.message());
            emit m_parent->error(Error(Error::InvalidQuery, err.message()));
            return;
        } else if (err.name() == SIGNOND_PERMISSION_DENIED_ERR_NAME) {
            emit m_parent->error(AuthService::PermissionDeniedError, err.message());
            emit m_parent->error(Error(Error::PermissionDenied, err.message()));
            return;
        }

        /* Qt DBUS specific errors */
        if (err.type() != QDBusError::NoError) {
            emit m_parent->error(AuthService::UnknownError, err.message());
            emit m_parent->error(Error(Error::InternalCommunication, err.message()));
            return;
        }

        emit m_parent->error(AuthService::UnknownError, err.message());
        emit m_parent->error(Error(Error::Unknown, err.message()));
    }

} //namespace SignOn
