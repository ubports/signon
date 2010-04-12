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
#ifndef IDENTITY_H
#define IDENTITY_H

#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QPointer>

#include "signoncommon.h"
#include "authsession.h"
#include "identityinfo.h"

#define SSO_NEW_IDENTITY 0

namespace SignOn {

    typedef QPointer<AuthSession> AuthSessionP;

    /*!
     * @class Identity
     * @headerfile identity.h SignOn/Identity
     *
     * Represents an database entry for a single identity.
     * Identity is client side presentation of a credential.
     */
    class SIGNON_EXPORT Identity : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY(Identity)

        friend class IdentityImpl;

    public:
        /*!
         * @enum IdentityError
         * Codes for errors that may be reported by Identity objects
         * @see Identity::error()
         */
        enum IdentityError {
            UnknownError = 1,               /**< Catch-all for errors not distinguished by another code. */
            InternalServerError = 2,        /**< Signon Daemon internal error. */
            InternalCommunicationError = 3, /**< Communication with Signon Daemon error . */
            PermissionDeniedError = 4,      /**< The operation cannot be performed due to insufficient client permissions. */
            IdentityErr = 200,              /* placeholder to rearrange enumeration */
            MethodNotAvailableError,        /**< The requested mechanism is not available. */
            NotFoundError,                  /**< The identity matching this Identity object was not found on the service. */
            StoreFailedError,               /**< Storing credentials failed. */
            RemoveFailedError,              /**< Removing credentials failed. */
            SignOutFailedError,             /**< SignOut failed. */
            CanceledError,                  /**< Operation was canceled by user. */
            CredentialsNotAvailableError    /** Query fails*/
        };

    protected:
        /*!
         * @internal
         */
        Identity(const quint32 id = SSO_NEW_IDENTITY,
                 QObject *parent = 0);

    public:
        /*!
         * Construct a new identity object.
         *
         * Can return NULL if client is untrusted.
         *
         * @param info the identity information.
         * @param parent the parent object of the identity
         * @return pointer to new identity object or NULL if it fails to create.
         */
        static Identity *newIdentity(const IdentityInfo &info = IdentityInfo(), QObject *parent = 0);

        /*!
         * Construct an identity object associated with an existing identity record.
         *
         * Can return NULL if client is untrusted.
         *
         * @param id identity ID on the service.
         * @param parent the parent object of the identity
         * @return pointer to identity object or NULL if it fails to create.
         */
        static Identity *existingIdentity(const quint32 id, QObject *parent = 0);

        /*!
         * Destructor.
         */
        virtual ~Identity();

        /*!
         * Unique id of given identity
         *
         * @return  identity ID of the identity. For new identity which is not stored, NEW_IDENTITY is returned.
         */
        quint32 id() const;

        /*!
         * Query list of available authentication methods for given identity.
         * List is returned by emitting signal methodsAvailable().
         * If the operation fails, the error() signal is emitted.
         * @see methodsAvailable().
         * If the operation fails, a signal error() is emitted.
         */
        void queryAvailableMethods();

        /*!
         * Create new session for authentication. This will create connection
         * to authentication plugin.
         * The Identity object is parent and owner of all created authentication sessions.
         *
         * @param methodName name of authentication method to use
         * @return new Authentication session or NULL if not able to create
         */
        AuthSessionP createSession(const QString &methodName);

        /*!
         * Destroys an authentication session.
         *
         * @param session the session to be destroyed
         */
        void destroySession(const AuthSessionP &session);

        /*!
         * Request user to give new secret into database.
         * Client can use requestCredentialsUpdate() to launch external
         * dialog for asking new secret, that will be stored into database.
         * On success, a signal credentialsStored() is emitted.
         * If the operation fails, a signal error() is emitted.
         *
         * @param message message to be shown for the user
         */
        void requestCredentialsUpdate(const QString &message = 0);

        /*!
         * Store credential parameters for this authentication identity.
         * IdentityInfo contains restrictions on methods and mechanisms
         * for given Identity. @see IdentityInfo
         * On success, a signal credentialsStored() is emitted.
         * If the operation fails, a signal error() is emitted.
         *
         * Untrusted clients may be blocked from performing this operation,
         * subject to the security framework restrictions.
         *
         * If default value is used for the parameter the Identity object
         * stores the internally stored information, e.g. the IdentityInfo object
         * used to create a new identity using Identity::newIdentity()
         *
         * @param info the credentials to store
         */
        void storeCredentials(const IdentityInfo &info = IdentityInfo());

        /*!
         * Remove this identity from database.
         * On success, a signal removed() is emitted
         * If the operation fails, a signal error() is emitted.
         *
         * Untrusted clients may be blocked from performing this operation,
         * subject to the security framework restrictions.
         */
        void remove();

        /*!
         * Query stored credential parameters for this authentication identity.
         * On success, a signal info() is emitted with parameters
         * in the service.
         * If the operation fails, a signal error() is emitted.
         * Untrusted clients may be blocked from performing this operation,
         * subject to the security framework restrictions.
         */
        void queryInfo();

        /*!
         * Get secret verification from user and compare it to stored secret.
         * This will launch external dialog for asking secret.
         * When verification is completed, signal userVerified() is emitted.
         * If the operation fails, a signal error() is emitted.
         *
         * @param message message to be shown for the user
         */
        void verifyUser(const QString &message = 0);

        /*!
         * Verify if given secret match stored secret.
         * When verification is completed, signal secretVerified() is emitted.
         * If the operation fails, a signal error() is emitted.
         *
         * @param secret string to be verified
         */
        void verifySecret(const QString &secret);

         /*!
         * Sign out Identity from all services. All authentication sessions using this Identity
         * will be invalidated and all tokens cleared from cache.
         * When sign out is completed, signal signedOut() is emitted.
         * If the operation fails, a signal error() is emitted.
         *
         * All clients using same identity will receive signedOut signal.
         */
        void signOut();

    Q_SIGNALS:
        /*!
         * Emitted when an error occurs while performing an operation.
         * @param code the error code
         * @param message a description string for troubleshooting purposes
         */
        void error(Identity::IdentityError code, const QString &message);

        /*!
         * Emitted when the list of available methods have been obtained
         * for identity.
         *
         * @param methods a list of available methods
         */
        void methodsAvailable(const QStringList &methods);

        /*!
         * Emitted when credentials passed by storeCredentials() method
         * have been successfully stored on the service.
         * @param id identifier of the credentials that has been stored
         */
        void credentialsStored(const quint32 id);

        /*!
         * Emitted when credentials passed by queryInfo() method
         * @param info the credentials as have been stored on the service
         */
        void info(const IdentityInfo &info);

        /*!
         * Emitted when user verification is completed.
         * @param valid is given secret same as stored
         */
        void userVerified(const bool valid);

        /*!
         * Emitted when secret verification is completed.
         * @param valid is given secret same as stored
         */
        void secretVerified(const bool valid);

        /*!
         * Emitted when identity is signed out.
         */
        void signedOut();

        /*!
         * Emitted when identity is removed.
         */
        void removed();

    private:
        class IdentityImpl *impl;
    };

}  // namespace SignOn

#endif /* IDENTITY_H */
