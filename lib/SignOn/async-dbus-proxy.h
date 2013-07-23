/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 * Copyright (C) 2013 Canonical Ltd.
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

#ifndef SIGNON_ASYNC_DBUS_PROXY_H
#define SIGNON_ASYNC_DBUS_PROXY_H

#include <QDBusError>
#include <QObject>
#include <QQueue>
#include <QVariant>

class QDBusConnection;
class QDBusObjectPath;

/*
 * @cond IMPL
 */
namespace SignOn {

class DBusInterface;
class Operation;
class Connection;

class AsyncDBusProxy: public QObject
{
    Q_OBJECT

public:
    AsyncDBusProxy(const QString &service,
                   const char *interface,
                   QObject *clientObject);
    ~AsyncDBusProxy();

    void setConnection(const QDBusConnection &connection);
    void setObjectPath(const QDBusObjectPath &objectPath);
    void setError(const QDBusError &error);

    int queueCall(const QString &method,
                  const QList<QVariant> &args,
                  const char *replySlot, const char *errorSlot);
    int queueCall(const QString &method,
                  const QList<QVariant> &args,
                  QObject *receiver,
                  const char *replySlot, const char *errorSlot);
    bool connect(const char *name, QObject *receiver, const char *slot);

    bool cancelCall(int id);

Q_SIGNALS:
    void objectPathNeeded();

private:
    enum Status {
        Incomplete,
        Ready,
        Invalid
    };
    void setStatus(Status status);
    void update();
    void doCall(const QString &method, const QList<QVariant> &args,
                QObject *receiver,
                const char *replySlot, const char *errorSlot);
    void sendErrorReply(QObject *receiver, const char *slot);

private:
    QString m_serviceName;
    const char *m_interfaceName;
    QString m_path;
    QDBusConnection *m_connection;
    QObject *m_clientObject;
    QQueue<Operation *> m_operationsQueue;
    QQueue<Connection *> m_connectionsQueue;
    DBusInterface *m_interface;
    int m_nextCallId;
    Status m_status;
    QDBusError m_lastError;
};

} //SignOn

/*
 * @endcond IMPL
 */

#endif // SIGNON_ASYNC_DBUS_PROXY_H
