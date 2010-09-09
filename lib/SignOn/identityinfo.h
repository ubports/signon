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
#ifndef IDENTITY_INFO_H
#define IDENTITY_INFO_H

#include <QStringList>
#include <QMetaType>

#include "libsignoncommon.h"

namespace SignOn {

    /*!
     * @typedef QString MethodName
     * Defines a string as an authentication method.
     */
    typedef QString MethodName;

    /*!
     * @typedef QStringList MechanismsList
     * Defines a string list as a list of mechanisms.
     */
    typedef QStringList MechanismsList;

    /*!
     * @class IdentityInfo
     * @headerfile identityinfo.h SignOn/IdentityInfo
     *
     * Contains identity information. This information is stored into database.
     * @see queryIdentities()
     */
    class SIGNON_EXPORT IdentityInfo
    {
        friend class AuthServiceImpl;
        friend class IdentityImpl;

    public:
        /*!
         * @enum CredentialsType
         * Values used to describe the type of the identity
         * @attention Mixed types, i.e Application|Web are not yet supported. Just single types
         * work for the time being.
         */
        enum CredentialsType {
            Other = 0,
            Application = 1 << 0,
            Web = 1 << 1,
            Network = 1 << 2
        };

    public:
        /*!
         * Create new empty IdentityInfo object.
         */
        IdentityInfo();

        /*!
         * Copy constructor.
         */
        IdentityInfo(const IdentityInfo &other);

        /*!
         * Assignment operator.
         */
        IdentityInfo &operator=(const IdentityInfo &other);

        /*!
         * Create new IdentityInfo object with given values.
         * @param caption Description of identity
         * @param userName username
         * @param methods allowed methods for identity
         */
        IdentityInfo(const QString &caption, const QString &userName,
                     const QMap<MethodName,MechanismsList> &methods);

        /*!
         * Destructor.
         */
        ~IdentityInfo();

        /*!
          * Returns identity identifier.
          * @return identifier for the identity.
          */
        quint32 id() const;

        /*!
         * Sets the secret. When performing a challeng on the owner Identity object,
         * if the secret is set on its corresponding IdentityInfo, it will be added
         * to the parameter list that is passed to the corresponding authentication plugin
         * challenge implementation. By default a newly created IdentityInfo does not contain
         * a secret and has a policy of not storing any. If the secret is set the default policy
         * will be to store it. This behaviour can also be set with IdentityInfo::setStoreSecret()
         *
         * @see PluginInterface::secretKey
         * @see PluginInterface::challenge
         * @param secret
         * @param storeSecret sets whether the secret is stored or not
         *
         */
        void setSecret(const QString &secret, const bool storeSecret = true);

        /*!
          * Return whether secret is to be stored.
         * @return true whether the secret is being stored or not.
         */
        bool isStoringSecret() const;

        /*!
         * Sets whether the secret is stored or not.
         * @param storeSecret whether the secret must be stored in the DB.
         */
        void setStoreSecret(const bool storeSecret);

        /*!
         * Sets the username.
         *
         * @see userNameKey
         * @param userName
         */
        void setUserName(const QString &userName);

        /*!
          * Return username.
         * @return username for the identity.
         */
        const QString userName() const;

        /*!
         * Set human readable caption of the identity
         * @param caption
         */
        void setCaption(const QString &caption);

        /*!
          * Return human-readable representation of the identity.
         * @return human-readable representation of the identity.
         */
        const QString caption() const;

        /*!
         * Sets the realms, e.g. URL's with which the Identity using this IdentityInfo
         * shall work with.
         *
         * @param realms the list of the realms to be set.
         */
        void setRealms(const QStringList &realms);

        /*!
         * Gets the realms, e.g. URL's with which the Identity using this IdentityInfo
         * works with.
         *
         * @returns the list of supported realms.
         */
        QStringList realms() const;

        /*!
         * Sets the list of access control application tokens, by this defining the applications
         * that will be able to access this specific set of credentials
         *
         * @attention this list is to be set by the 1st application that stores the credentials
         * and will be editable by only the same application. Applications own token will be added into ACL automatically.
         *
         * @param accessControlList list of access control tokens.
         */
        void setAccessControlList(const QStringList &accessControlList);

        /*!
         * Gets the list of access control application tokens defining the applications
         * that are able to access this specific set of credentials
         *
         * @attention this is accessible only to the owner application.
         *
         * @returns the access control tokens which defines the applications allowed to access this set
         * of credentials.
         */
        QStringList accessControlList() const;

        /*!
         * Set method into identity info.
         * If given method is not included, new will be added. If it is already set,
         * then mechanism list assosiated to it is updated. Empty list will clear mechanisms.
         * These values are used to limit Identity to use specified methods and mechanisms.
         * @param method method name to change.
         * @param mechanismsList list of mechanisms that are allowed.
         */
        void setMethod(const MethodName &method, const MechanismsList &mechanismsList);

        /*!
         * Remove method from identity info.
         * @param method method name to remove.
         */
        void removeMethod(const MethodName &method);

        /*!
         * Set type into identity info.
         * The type is used to generically identify where this identity is being used.
         *
         * @attention if this method is not called, the IdentityInfo type will default
         * to SignOn::OtherIdentity
         *
         * @param type the type we want to assign to this IdentityInfo
         */
        void setType(CredentialsType type);

        /*!
         * Retrieve identity type from identity info.
         * @returns the identity type for this IdentityInfo.
         */
        CredentialsType type() const;

        /*!
         * List all methods in identity info
         * @return param method method name to remove.
         */
        QList<MethodName> methods() const;

        /*!
         * List all mechanisms for certain method in identity info.
         * @param method method name to list mechanisms.
         * @return list of mechanisms.
         */
        MechanismsList mechanisms(const MethodName &method) const;

        /*!
         * Set refcount into identity info.
         * The type is used to generically identify where this identity is being used.
         *
         * @note Server can restrict changes to differ +-1 from previous.
         *
         * @param refCount set refcount
         */
        void setRefCount(qint32 refCount);

        /*!
         * Retrieve refcount from identity info.
         * @returns the refcount for this IdentityInfo.
         */
        qint32 refCount() const;

    private:
        void setId(const quint32 id);
        const QString secret() const;

    private:
        class IdentityInfoImpl *impl;
    };

}  // namespace SignOn

Q_DECLARE_METATYPE(SignOn::IdentityInfo)

#endif /* IDENTITY_INFO_H */
