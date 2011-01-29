/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
 * Contact: Aurel Popirtac <ext-Aurel.Popirtac@nokia.com>
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

/*!
  @file accesscontrol.h
  Definition of the Aegis Access Control object.
  @ingroup Accounts_and_SSO_Framework
 */

#ifndef ACCESSCONTROLMANAGER_H
#define ACCESSCONTROLMANAGER_H

#include <sys/creds.h>

#include <QIODevice>
#include <QMap>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>

#include "signonauthsession.h"

namespace SignonDaemonNS {

    /*!
        @class AccessControlManager
        Helps filtering incoming Singnon Daemon requests,
        based on security priviledges of the client processes.
        @ingroup Accounts_and_SSO_Framework
     */
    class AccessControlManager
    {
    public:
        /*!
          @enum IdentityOwnership
          Specifies the owner relationship of an application over a specific identity,
          or the lack of ownership over that specific identity.
          @see isPeerOwnerOfIdentity(const QDBusContext &peerContext, const quint32 identityId))
        */
        enum IdentityOwnership {
            ApplicationIsOwner = 0,
            ApplicationIsNotOwner,
            IdentityDoesNotHaveOwner
        };

        /*!
            Checks if a client process is allowed to use a specific SignonIdentity.
            @param peerContext, the DBUS context created by the process to be checked for allowance.
            @param identityId, the SignonIdentity to be used.
            @returns true, if the peer is allowed, false otherwise.
        */
        static bool isPeerAllowedToUseIdentity(const QDBusContext &peerContext,
                                               const quint32 identityId);

        /*!
            Checks if a specific process is the owner of a SignonIdentity, thus having full control over it.
            @param peerContext, the DBUS context created by the process to be checked for ownership
            @param identityId, the SignonIdentity in context.
            @retval ApplicationIsOwner/ApplicationIsNotOwner if the identity is/isn't the owner
                    or IdentityDoesNotHaveOwner if the identity does not have an owner at all.
        */
        static IdentityOwnership isPeerOwnerOfIdentity(const QDBusContext &peerContext,
                                                       const quint32 identityId);

        /*!
            Checks if a specific process is allowed to use the SignonAuthSession functionality.
            @param peerContext, to DBUS context created by the process to be checked for allowance
            @param authSession, the authentication session to be used by the peer request.
            @returns true, if the peer is allowed, false otherwise.
        */
        static bool isPeerAllowedToUseAuthSession(const QDBusContext &peerContext,
                                                  const SignonAuthSession &authSession)
        {
            return isPeerAllowedToUseIdentity(peerContext, authSession.id());
        }

        /*!
            Checks if a specific process is allowed to use the SignonAuthSession functionality.
            @param peerContext, to DBUS context created by the process to be checked for allowance
            @param ownerIdentityId, id of the Identity owning the authentication session.
            @returns true, if the peer is allowed, false otherwise.
        */
        static bool isPeerAllowedToUseAuthSession(const QDBusContext &peerContext,
                                                  const quint32 ownerIdentityId)
        {
            return isPeerAllowedToUseIdentity(peerContext, ownerIdentityId);
        }

        /*!
            @param peerContext, the DBUS context created by the process to be checked.
            @returns true, if the peer is the Keychain Widget, false otherwise.
        */
        static bool isPeerKeychainWidget(const QDBusContext &peerContext);

        /*!
            Looks up for the Aegis identifier token of a specific client process.
            @param peerContext, the DBUS context created by the process.
            @returns the Aegis identifier token of the process, or an empty string if none found.
        */
        static QString idTokenOfPeer(const QDBusContext &peerContext);

        /*!
            Looks up for the Aegis identifier token of a specific client process.
            @param pid, the process id of service owaner.
            @returns the Aegis identifier token of the process, or an empty string if none found.
        */
        static QString idTokenOfPid(pid_t pid);

        /*!
            @param peerContext, the context, which process id we want to know
            @returns process id of service client.
        */
        static pid_t pidOfPeer(const QDBusContext &peerContext);

        /*!
            @param peerId, the id of the process for which to retrieve the tokens list
            @returns A list with the Aegis Access Control tokens of the process.
        */
        static QStringList accessTokens(const pid_t peerPid);
        /*!
            @overload accessTokens(const pid_t peerPid)
            @param peerContext, the peer for which to retrieve the tokens list
            @returns A list with the Aegis Access Control tokens of the process.
        */
        static QStringList accessTokens(const QDBusContext &peerContext);

    private:
        /*!
            Checks if a specific peer has a set of Aegis Access Control tokens.
            @param peerContext, to DBUS context created by the process to be checked.
            @param tokens, the set of tokens that the peer will be checked for.
            @returns true, if the peer has all the tokens, false otherwise.
        */
        static bool peerHasOneOfTokens(const QDBusContext &peerContext, const QStringList &tokens);

        /*!
            Checks if a specific peer has a set the Aegis Access Control token.
            @param peerContext, to DBUS context created by the process to be checked.
            @param token, the token that the peer will be checked for.
            @returns true, if the peer has the tokens, false otherwise.
        */
        static bool peerHasToken(const pid_t peerPid, const QString &token);

        /*!
            @overload peerHasToken(const pid_t processPid, const QString &token)
            @param peerContext, to DBUS context created by the process to be checked.
            @param token, the token that the peer will be checked for.
            @returns true, if the peer has the tokens, false otherwise.
        */
        static bool peerHasToken(const QDBusContext &context, const QString &token);

        static void listCredentials(QIODevice *device, creds_t creds, const QString &ownerInfo = 0);

    };

} // namespace SignonDaemonNS

#endif // ACCESSCONTROLMANAGER_H
