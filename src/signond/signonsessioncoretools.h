/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2011 Nokia Corporation.
 *
 * Contact: Aurel Popirtac <ext-aurel.popirtac@nokia.com>
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

#ifndef SIGNONSESSIONCORETOOLS_H
#define SIGNONSESSIONCORETOOLS_H

#include <QObject>
#include <QVariantMap>
#include <QDBusMessage>

#include "signonidentityinfo.h"

namespace SignonDaemonNS {

/*!
 * @brief Helper method which unites two variant maps.
 * @param map1 base map to be united with
 * @param map2 map to be united with base. If map2 and map1 contain common
 *        keys, the values in map1 will be overwritten by map2 values
 * @returns a union of the map1 and map2 with unique keys,
 */
QVariantMap mergeVariantMaps(const QVariantMap &map1, const QVariantMap &map2);

/*!
 * @class StoreOperation
 * Describes a credentials store operatation.
 */
struct StoreOperation {
    enum StoreType {
        Credentials = 0,
        Blob
    };

    StoreOperation(const StoreType type);
    StoreOperation(const StoreOperation &src);
    ~StoreOperation();

public:
    StoreType m_storeType;
    SignonIdentityInfo m_info;
    //Blob store related
    QString m_authMethod;
    QVariantMap m_blobData;
};

/*!
 * @class RequestData
 * Request data.
 * @todo description.
 */
struct RequestData
{
    RequestData(const QDBusConnection &conn,
                const QDBusMessage &msg,
                const QVariantMap &params,
                const QString &mechanism,
                const QString &cancelKey);

    RequestData(const RequestData &other);
    ~RequestData();

public:
    QDBusConnection m_conn;
    QDBusMessage m_msg;
    QVariantMap m_params;
    QString m_mechanism;
    QString m_cancelKey;
};

/*!
 * @class AuthCoreCache
 * Caches credentials or BLOB authentication data.
 * The cache is credentials' record oriented (credentials ID as key).
 * Once all references of authentication sessions for a specifc
 * credentials ID are destroyed, the cache for that specific ID
 * will be deleted.
 */
class AuthCoreCache: public QObject
{
    Q_OBJECT

public:
    class AuthCache
    {
        friend class AuthCoreCache;

    private:
        QString m_username;
        QString m_password;
        QHash<QString,QVariantMap> m_blobData;
    };

private:
    static AuthCoreCache *m_instance;
    AuthCoreCache(QObject *parent = 0);

public:
    static AuthCoreCache *instance(QObject *parent = 0);
    ~AuthCoreCache();

    bool lookupCredentials(quint32 id,
                           QString &username,
                           QString &password) const;
    QVariantMap lookupData(quint32 id, const QString &method) const;

    void updateCredentials(quint32 id,
                           const QString &username,
                           const QString &password);
    void updateData(quint32 id, const QString &method,
                    const QVariantMap &data);

    void clear();

private:
    QHash<quint32, AuthCache> m_cache;
};

} //SignonDaemonNS

#endif //SIGNONSESSIONCORETOOLS_H
