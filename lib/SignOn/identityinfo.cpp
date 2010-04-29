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

#include <QVariant>

#include "libsignoncommon.h"
#include "identityinfo.h"
#include "identityinfoimpl.h"
#include "identity.h"

namespace SignOn {

    IdentityInfo::IdentityInfo()
        : impl(new IdentityInfoImpl(this))
    {
        qRegisterMetaType<IdentityInfo>("SignOn::IdentityInfo");

        if (qMetaTypeId<IdentityInfo>() < QMetaType::User)
            BLAME() << "IdentityInfo::IdentityInfo() - IdentityInfo meta type not registered.";

        impl->m_empty = true;
        impl->m_id = 0;
        impl->m_storeSecret = false;
    }

    IdentityInfo::IdentityInfo(const IdentityInfo &other)
        : impl(new IdentityInfoImpl(this))
    {
        impl->copy(*(other.impl));
    }

    IdentityInfo &IdentityInfo::operator=(const IdentityInfo &other)
    {
        impl->copy(*(other.impl));
        return *this;
    }

    IdentityInfo::IdentityInfo(
            const QString &caption,
            const QString &userName,
            const QMap<MethodName, MechanismsList> &methods)
        : impl(new IdentityInfoImpl(this))
    {
        impl->m_empty = false;
        impl->m_caption = caption;
        impl->m_userName = userName;

        QMapIterator<QString, QStringList> it(methods);
        while (it.hasNext()) {
            it.next();
            impl->m_authMethods.insert(it.key(), QVariant(it.value()));
        }
    }

    IdentityInfo::~IdentityInfo()
    {
        if (impl) delete impl;
        impl = 0;
    }

    void IdentityInfo::setId(const quint32 id)
    {
        impl->m_id = id;
    }

    quint32 IdentityInfo::id() const
    {
        return impl->m_id;
    }

    void IdentityInfo::setUserName(const QString &userName)
    {
        impl->m_userName = userName;
    }

    const QString IdentityInfo::userName() const
    {
        return impl->m_userName;
    }

    void IdentityInfo::setCaption(const QString &caption)
    {
        impl->m_caption = caption;
    }

    const QString IdentityInfo::caption() const
    {
        return impl->m_caption;
    }

    void IdentityInfo::setRealms(const QStringList &realms)
    {
        impl->m_realms = realms;
    }

    QStringList IdentityInfo::realms() const
    {
        return impl->m_realms;
    }

    void IdentityInfo::setAccessControlList(const QStringList &accessControlList)
    {
        impl->m_accessControlList = accessControlList;

        //remove 'owner aegis tokens' if manually specified by the user
        int index = 0;
        QRegExp aegisIdTokenPrefixRegExp(QLatin1String("^AID::*"));
        while ((index = impl->m_accessControlList.indexOf(aegisIdTokenPrefixRegExp, index)) != -1)
            impl->m_accessControlList.removeAt(index);

        TRACE() << "Access control list set:" << impl->m_accessControlList;
    }

    QStringList IdentityInfo::accessControlList() const
    {
        return impl->m_accessControlList;
    }

    const QString IdentityInfo::secret() const
    {
        return impl->m_secret;
    }

    void IdentityInfo::setSecret(const QString &secret, const bool storeSecret)
    {
        impl->m_secret = secret;
        impl->m_storeSecret = storeSecret;
    }

    bool IdentityInfo::isStoringSecret() const
    {
        return impl->m_storeSecret;
    }

    void IdentityInfo::setStoreSecret(const bool storeSecret)
    {
        impl->m_storeSecret = storeSecret;
    }

    void IdentityInfo::setMethod(const MethodName &method, const MechanismsList &mechanismsList)
    {
        if (impl->hasMethod(method))
            impl->updateMethod(method, mechanismsList);
        else
            impl->addMethod(method, mechanismsList);
    }

    void IdentityInfo::removeMethod(const MethodName &method)
    {
        impl->removeMethod(method);
    }

    void IdentityInfo::setType(IdentityInfo::CredentialsType type)
    {
        impl->setType(type);
    }

    IdentityInfo::CredentialsType IdentityInfo::type() const
    {
        return impl->type();
    }

    QList<MethodName> IdentityInfo::methods() const
    {
        return impl->m_authMethods.keys();
    }

    MechanismsList IdentityInfo::mechanisms(const MethodName &method) const
    {
        return impl->m_authMethods.value(method, QVariant(QStringList())).toStringList();
    }

} //namespace SignOn
