/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include <QBuffer>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include "accesscontrolmanager.h"
#include "signond-common.h"
#include "credentialsaccessmanager.h"
#include "signonidentity.h"

#if HAVE_LIBCREDS
#include <sys/creds.h>
#endif

#define SSO_AEGIS_PACKAGE_ID_TOKEN_PREFIX QLatin1String("AID::")
#define SSO_DEFAULT_CREDS_STR_BUFFER_SIZE 256

#ifdef SIGNON_DISABLE_ACCESS_CONTROL
#define RETURN_IF_AC_DISABLED(val)  return (val)
#else
#define RETURN_IF_AC_DISABLED(val)
#endif

namespace SignonDaemonNS {

    static const char keychainToken[] = "signond::keychain-access";

    bool AccessControlManager::isPeerAllowedToUseIdentity(const QDBusContext &peerContext,
                                                          const quint32 identityId)
    {
        RETURN_IF_AC_DISABLED(true);

        // TODO - improve this, the error handling and more precise behaviour

        CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
        if (db == 0) {
            TRACE() << "NULL db pointer, secure storage might be unavailable,";
            return false;
        }
        QStringList acl = db->accessControlList(identityId);

        TRACE() << QString(QLatin1String("Access control list of identity: "
                                         "%1: [%2].Tokens count: %3\t"))
            .arg(identityId)
            .arg(acl.join(QLatin1String(", ")))
            .arg(acl.size());

        if (db->errorOccurred())
            return false;

        if (acl.isEmpty())
            return true;

        return peerHasOneOfTokens(peerContext, acl);
    }

    AccessControlManager::IdentityOwnership AccessControlManager::isPeerOwnerOfIdentity(
                                                                       const QDBusContext &peerContext,
                                                                       const quint32 identityId)
    {
        RETURN_IF_AC_DISABLED(ApplicationIsOwner);

        CredentialsDB *db = CredentialsAccessManager::instance()->credentialsDB();
        if (db == 0) {
            TRACE() << "NULL db pointer, secure storage might be unavailable,";
            return ApplicationIsNotOwner;
        }
        QStringList ownerTokens = db->ownerList(identityId);

        if (db->errorOccurred())
            return ApplicationIsNotOwner;

        if (ownerTokens.isEmpty())
            return IdentityDoesNotHaveOwner;

        return peerHasOneOfTokens(peerContext, ownerTokens) ? ApplicationIsOwner : ApplicationIsNotOwner;
    }

    bool AccessControlManager::isPeerKeychainWidget(const QDBusContext &peerContext)
    {
        RETURN_IF_AC_DISABLED(true);

        static QString keychainWidgetToken = QLatin1String(keychainToken);
        return peerHasToken(peerContext, keychainWidgetToken);
    }

    QString AccessControlManager::idTokenOfPeer(const QDBusContext &peerContext)
    {
        RETURN_IF_AC_DISABLED(QString());

        QStringList peerTokens = accessTokens(peerContext);
        foreach(QString token, peerTokens) {
            if (token.startsWith(SSO_AEGIS_PACKAGE_ID_TOKEN_PREFIX))
                return token;
        }
        return QString();
    }

    QString AccessControlManager::idTokenOfPid(pid_t pid)
    {
        RETURN_IF_AC_DISABLED(QString());

        QStringList peerTokens = accessTokens(pid);
        foreach(QString token, peerTokens) {
            if (token.startsWith(SSO_AEGIS_PACKAGE_ID_TOKEN_PREFIX))
                return token;
        }
        return QString();
    }

    bool AccessControlManager::peerHasOneOfTokens(const QDBusContext &peerContext,
                                                  const QStringList &tokens)
    {
        QStringList peerTokens = accessTokens(peerContext);

        TRACE() << peerTokens << " vs. " << tokens;

        foreach(QString token, tokens)
            if (peerTokens.contains(token))
                return true;

        BLAME() << "given peer does not have needed permissions";
        return false;
    }

    QStringList AccessControlManager::accessTokens(const pid_t peerPid)
    {
#if HAVE_LIBCREDS
        creds_t ccreds = creds_gettask(peerPid);

        creds_value_t value;
        creds_type_t type;
        QStringList tokens;

        char buf[SSO_DEFAULT_CREDS_STR_BUFFER_SIZE];
        for (int i = 0; (type = creds_list(ccreds, i,  &value)) != CREDS_BAD; ++i) {
            long actualSize = creds_creds2str(type, value, buf, SSO_DEFAULT_CREDS_STR_BUFFER_SIZE);

            if (actualSize >= SSO_DEFAULT_CREDS_STR_BUFFER_SIZE) {
                qWarning() << "Size limit exceeded for aegis token as string.";
                buf[SSO_DEFAULT_CREDS_STR_BUFFER_SIZE-1] = 0;
            } else {
                buf[actualSize] = 0;
            }

            tokens << QString(QString::fromLatin1(buf));
        }

        creds_free(ccreds);
        return tokens;
#else
        Q_UNUSED(peerPid);
        return QStringList();
#endif
    }

    QStringList AccessControlManager::accessTokens(const QDBusContext &peerContext)
    {
        return accessTokens(pidOfPeer(peerContext));
    }

    bool AccessControlManager::peerHasToken(const QDBusContext &context, const QString &token)
    {
        return peerHasToken(pidOfPeer(context), token);
    }

    bool AccessControlManager::peerHasToken(const pid_t processPid, const QString &token)
    {
#if HAVE_LIBCREDS
        creds_type_t require_type;
        creds_value_t require_value;
        creds_t ccreds;

        /* Translate credential string into binary */
        require_type = creds_str2creds(token.toUtf8().data(), &require_value);
        if (require_type == CREDS_BAD) {
            TRACE() << "The string " << token << " does not translate into credentials value";
            return false;
        }

        ccreds = creds_gettask(processPid);
        bool hasAccess = creds_have_p(ccreds, require_type, require_value);

        TRACE() << "Process ACCESS:" << (hasAccess ? "TRUE" : "FALSE");
        creds_free(ccreds);

        return hasAccess;
#else
        Q_UNUSED(processPid);
        Q_UNUSED(token);
        return true;
#endif
    }

    pid_t AccessControlManager::pidOfPeer(const QDBusContext &peerContext)
    {
        QString service = peerContext.message().service();
        return peerContext.connection().interface()->servicePid(service).value();
    }

} //namespace SignonDaemonNS
