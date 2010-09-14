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
#ifndef SIGNONIDENTITYINFO_H
#define SIGNONIDENTITYINFO_H

#include <QMap>
#include <QStringList>
#include <QVariant>

namespace SignonDaemonNS {

    typedef QString MethodName;
    typedef QStringList MechanismsList;

    /*!
     * @struct SignonIdentityInfo
     * Daemon side representation of identity information.
     * @todo description.
     */
    struct SignonIdentityInfo
    {
        SignonIdentityInfo();
        SignonIdentityInfo(const quint32 id,
                           const QString &userName, const QString &password,
                           const QMap<QString, QVariant> &methods,
                           const QString &caption, const QStringList &realms = QStringList(),
                           const QStringList &accessControlList = QStringList(),
                           int type = 0, int refCount = 0, bool validated = false);

        const QList<QVariant> toVariantList();
        static const QList<QVariant> listToVariantList(const QList<SignonIdentityInfo> &list);
        static const QMap<QString, QVariant> mapListToMapVariant(const QMap<QString, QStringList> &mapList);
        static const QMap<QString, QStringList> mapVariantToMapList(const QMap<QString, QVariant> &mapList);

        const QString serialize();
        bool operator== (const SignonIdentityInfo &other) const;

        quint32 m_id;
        QString m_userName;
        QString m_password;
        QString m_caption;
        QStringList m_realms;
        QMap<MethodName, MechanismsList> m_methods;
        QStringList m_accessControlList;
        int m_type;
        int m_refCount;
        bool m_validated;
    }; //struct SignonIdentityInfo

} //namespace SignonDaemonNS

#endif // SIGNONIDENTITYINFO_H
