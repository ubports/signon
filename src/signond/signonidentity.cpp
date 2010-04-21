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

#include <iostream>

#include "signond-common.h"
#include "signonidentity.h"
#include "signonui_interface.h"

#include "accesscontrolmanager.h"
#include "signonidentityadaptor.h"

namespace SignonDaemonNS {

    static QTimer idleAssasinTimer;

    SignonIdentity::SignonIdentity(quint32 id, int timeout,
                                   SignonDaemon *parent)
            : SignonDisposable(timeout, parent),
              m_pInfo(NULL),
              m_pSignonDaemon(parent)
    {
        m_id = id;

        /*
         * creation of unique name for the given identity
         * */
        static quint32 incr = 0;
        QString objectName = QLatin1String("/com/nokia/singlesignon/SignonIdentity_")
                             + QString::number(incr++, 16);
        setObjectName(objectName);

        m_signonui = new SignonUiAdaptor(
                                        SIGNON_UI_SERVICE,
                                        SIGNON_UI_DAEMON_OBJECTPATH,
                                        SIGNON_BUS,
                                        this);
    }

    SignonIdentity::~SignonIdentity()
    {
        QDBusConnection connection = SIGNON_BUS;
        connection.unregisterObject(objectName());

        if (credentialsStored())
            m_pSignonDaemon->m_storedIdentities.remove(m_id);
        else
            m_pSignonDaemon->m_unstoredIdentities.remove(objectName());

        delete m_signonui;
    }

    bool SignonIdentity::init()
    {
        QDBusConnection connection = SIGNON_BUS;

        if (!connection.isConnected()) {
            QDBusError err = connection.lastError();
            TRACE() << "Connection cannot be established:" << err.errorString(err.type()) ;
            return false;
        }

        QDBusConnection::RegisterOptions registerOptions = QDBusConnection::ExportAllContents;

#ifndef SIGNON_DISABLE_ACCESS_CONTROL
        (void)new SignonIdentityAdaptor(this);
        registerOptions = QDBusConnection::ExportAdaptors;
#endif

        if (!connection.registerObject(objectName(), this, registerOptions)) {
            TRACE() << "Object cannot be registered: " << objectName();
            return false;
        }

        return true;
    }

    SignonIdentity *SignonIdentity::createIdentity(quint32 id, SignonDaemon *parent)
    {
        SignonIdentity *identity =
            new SignonIdentity(id, parent->identityTimeout(), parent);

        if (!identity->init()) {
            TRACE() << "The created identity is invalid and will be deleted.\n";
            delete identity;
            return NULL;
        }

        return identity;
    }

    SignonIdentityInfo SignonIdentity::queryInfo(bool &ok, bool queryPassword)
    {
        ok = true;

        if (m_pInfo) {
            return *m_pInfo;
        } else {
            CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
            m_pInfo = new SignonIdentityInfo(db->credentials(m_id, queryPassword));

            if (db->error().type() != QSqlError::NoError) {
                ok = false;
                delete m_pInfo;
                m_pInfo = NULL;
                return SignonIdentityInfo();
            }
        }
        return *m_pInfo;
    }

    quint32 SignonIdentity::requestCredentialsUpdate(const QString &displayMessage)
    {
        RequestCounter::instance()->addIdentityResquest();
        Q_UNUSED(displayMessage);

        QDBusMessage errReply = message().createErrorReply(
                                                SSO_IDENTITY_UNKNOWN_ERR_NAME,
                                                QLatin1String("Not implemented."));
        SIGNON_BUS.send(errReply);
        keepInUse();
        return 0;
    }

    QList<QVariant> SignonIdentity::queryInfo()
    {
        RequestCounter::instance()->addIdentityResquest();
        TRACE() << "QUERYING INFO";
        bool ok;
        SignonIdentityInfo info = queryInfo(ok, false);

        TRACE() << info.serialize();
        if (!ok) {
            TRACE();
            QDBusMessage errReply = message().createErrorReply(
                                        SSO_IDENTITY_CREDENTIALS_NOT_AVAILABLE_ERR_NAME,
                                        QString(SSO_IDENTITY_CREDENTIALS_NOT_AVAILABLE_ERR_STR
                                                + QLatin1String("Database querying error occurred.")));
            SIGNON_BUS.send(errReply);
            return QList<QVariant>();
        }

        if (info.m_id == 0) {
            TRACE();
            QDBusMessage errReply = message().createErrorReply(SSO_IDENTITY_NOT_FOUND_ERR_NAME,
                                                               SSO_IDENTITY_NOT_FOUND_ERR_STR);
            SIGNON_BUS.send(errReply);
            return QList<QVariant>();
        }

        TRACE() << "INFO as variant list:" << info.toVariantList();
        keepInUse();
        return info.toVariantList();
    }

    bool SignonIdentity::verifyUser(const QString &displayMessage)
    {
        RequestCounter::instance()->addIdentityResquest();
        Q_UNUSED(displayMessage)

        QDBusMessage errReply = message().createErrorReply(SSO_IDENTITY_UNKNOWN_ERR_NAME,
                                                           QLatin1String("Not implemented."));
        SIGNON_BUS.send(errReply);
        keepInUse();
        return false;
    }

    bool SignonIdentity::verifySecret(const QString &secret)
    {
        RequestCounter::instance()->addIdentityResquest();

        bool ok;
        queryInfo(ok);
        if (!ok) {
            TRACE();
            QDBusMessage errReply = message().createErrorReply(
                                        SSO_IDENTITY_CREDENTIALS_NOT_AVAILABLE_ERR_NAME,
                                        QString(SSO_IDENTITY_CREDENTIALS_NOT_AVAILABLE_ERR_STR
                                                + QLatin1String("Database querying error occurred.")));
            SIGNON_BUS.send(errReply);
            return false;
        }

        CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
        bool ret = db->checkPassword(m_pInfo->m_userName, secret);

        keepInUse();
        return ret;
    }

    void SignonIdentity::remove()
    {
        RequestCounter::instance()->addIdentityResquest();
        CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
        if (!db->removeCredentials(m_id)) {
            TRACE() << "Error occurred while inserting/updating credemtials.";
            QDBusMessage errReply = message().createErrorReply(
                                                        SSO_IDENTITY_REMOVE_FAILED_ERR_NAME,
                                                        QString(SSO_IDENTITY_REMOVE_FAILED_ERR_STR
                                                                + QLatin1String("Database error occurred.")));
            SIGNON_BUS.send(errReply);
        }
        emit infoUpdated((int)SignOn::IdentityRemoved);
        keepInUse();
    }

    bool SignonIdentity::signOut()
    {
        RequestCounter::instance()->addIdentityResquest();
        TRACE() << "Signout request. Identity ID: " << id();
        /*
           - If the identity is stored (thus registered here)
           signal 'sign out' to all identities subsribed to this object,
           otherwise the only identity subscribed to this is the newly
           created client side identity, which called this method.
           - This is just a safety check, as the client identity - if it is a new one -
           should not inform server side to sign out.
        */
        if (id() != SSO_NEW_IDENTITY)
            emit infoUpdated((int)SignOn::IdentitySignedOut);

        return true;
    }

    quint32 SignonIdentity::storeCredentials(const quint32 id,
                                             const QString &userName,
                                             const QString &secret,
                                             const bool storeSecret,
                                             const QMap<QString, QVariant> &methods,
                                             const QString &caption,
                                             const QStringList &realms,
                                             const QStringList &accessControlList,
                                             const int type)
    {
        QString aegisIdToken = AccessControlManager::idTokenOfPeer(static_cast<QDBusContext>(*this));

        QStringList accessControlListLocal = accessControlList;
        if (!aegisIdToken.isNull())
            accessControlListLocal.prepend(aegisIdToken);

        RequestCounter::instance()->addIdentityResquest();

        SignonIdentityInfo info(id, userName, secret, methods, caption,
                                realms, accessControlListLocal, type);

        TRACE() << info.serialize();

        CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
        bool newIdentity = (id == SSO_NEW_IDENTITY);

        if (newIdentity)
            m_id = db->insertCredentials(info, storeSecret);
        else
            db->updateCredentials(info, storeSecret);

        if (db->errorOccurred()) {
            if (newIdentity)
                m_id = SSO_NEW_IDENTITY;

            TRACE() << "Error occurred while inserting/updating credentials.";
            QDBusMessage errReply = message().createErrorReply(SSO_IDENTITY_STORE_FAILED_ERR_NAME,
                                                               SSO_IDENTITY_STORE_FAILED_ERR_STR);
            SIGNON_BUS.send(errReply);
        } else {
            if (m_pInfo) {
                delete m_pInfo;
                m_pInfo = NULL;
            }
            m_pSignonDaemon->identityStored(this);
            TRACE() << "FRESH, JUST STORED CREDENTIALS ID:" << m_id;
            emit infoUpdated((int)SignOn::IdentityDataUpdated);
        }

        keepInUse();
        return m_id;
    }

} //namespace SignonDaemonNS
