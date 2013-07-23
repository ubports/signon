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

#include "async-dbus-proxy.h"

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDebug>
#include <QMetaMethod>
#include <QMetaType>

#include "dbusinterface.h"
#include "libsignoncommon.h"

using namespace SignOn;

namespace SignOn {

class Operation
{
public:
    Operation(const QString &name,
              const QList<QVariant> &args,
              QObject *receiver,
              const char *replySlot,
              const char *errorSlot,
              int id):
        m_name(name),
        m_args(args),
        m_receiver(receiver),
        m_replySlot(replySlot),
        m_errorSlot(errorSlot),
        m_id(id)
    {
    }
    ~Operation() {};

    QString m_name;
    QList<QVariant> m_args;
    QObject *m_receiver;
    const char *m_replySlot;
    const char *m_errorSlot;
    int m_id;
};

class Connection
{
public:
    Connection(const char *name, QObject *receiver, const char *slot):
        m_name(name),
        m_receiver(receiver),
        m_slot(slot)
    {
    }
    ~Connection() {}

    const char *m_name;
    QObject *m_receiver;
    const char *m_slot;
};

} // namespace

AsyncDBusProxy::AsyncDBusProxy(const QString &service,
                               const char *interface,
                               QObject *clientObject):
    m_serviceName(service),
    m_interfaceName(interface),
    m_connection(NULL),
    m_clientObject(clientObject),
    m_interface(NULL),
    m_nextCallId(1),
    m_status(Incomplete)
{
}

AsyncDBusProxy::~AsyncDBusProxy()
{
    delete m_connection;
}

void AsyncDBusProxy::setStatus(Status status)
{
    m_status = status;

    if (status == Ready) {
        /* connect the signals and execute all pending methods */
        Q_FOREACH(Connection *connection, m_connectionsQueue) {
            m_interface->connect(connection->m_name,
                                 connection->m_receiver,
                                 connection->m_slot);
        }
        qDeleteAll(m_connectionsQueue);
        m_connectionsQueue.clear();

        Q_FOREACH(Operation *op, m_operationsQueue) {
            doCall(op->m_name,
                   op->m_args,
                   op->m_receiver,
                   op->m_replySlot,
                   op->m_errorSlot);
        }
        qDeleteAll(m_operationsQueue);
        m_operationsQueue.clear();
    } else if (status == Invalid) {
        /* signal error on all operations */
        Q_FOREACH(Operation *op, m_operationsQueue) {
            sendErrorReply(op->m_receiver, op->m_errorSlot);
        }
        qDeleteAll(m_operationsQueue);
        m_operationsQueue.clear();
    }
}

void AsyncDBusProxy::update()
{
    if (m_interface != NULL) {
        delete m_interface;
        m_interface = 0;
    }

    if (m_connection == NULL || m_path.isEmpty()) {
        setStatus(Incomplete);
        return;
    }

    if (!m_connection->isConnected()) {
        setError(m_connection->lastError());
        return;
    }

    m_interface = new DBusInterface(m_serviceName,
                                    m_path,
                                    m_interfaceName,
                                    *m_connection,
                                    this);
    setStatus(Ready);
}

void AsyncDBusProxy::doCall(const QString &method, const QList<QVariant> &args,
                            QObject *receiver,
                            const char *replySlot, const char *errorSlot)
{
    if (replySlot != NULL) {
        m_interface->callWithCallback(method, args,
                                      receiver, replySlot, errorSlot);
    } else {
        m_interface->callWithArgumentList(QDBus::NoBlock, method, args);
    }
}

void AsyncDBusProxy::sendErrorReply(QObject *receiver, const char *slot)
{
    int indexOfMethod = receiver->metaObject()->indexOfMethod(
                                  QMetaObject::normalizedSignature(slot + 1));
    QMetaMethod method = receiver->metaObject()->method(indexOfMethod);

    method.invoke(receiver, Q_ARG(QDBusError, m_lastError));
}

void AsyncDBusProxy::setConnection(const QDBusConnection &connection)
{
    delete m_connection;
    m_connection = new QDBusConnection(connection);
    update();
}

void AsyncDBusProxy::setObjectPath(const QDBusObjectPath &objectPath)
{
    Q_ASSERT(m_path.isEmpty() || objectPath.path().isEmpty());
    m_path = objectPath.path();
    update();
}

void AsyncDBusProxy::setError(const QDBusError &error)
{
    TRACE() << error;
    m_lastError = error;
    setStatus(Invalid);
}

int AsyncDBusProxy::queueCall(const QString &method,
                              const QList<QVariant> &args,
                              const char *replySlot,
                              const char *errorSlot)
{
    return queueCall(method, args, m_clientObject, replySlot, errorSlot);
}

int AsyncDBusProxy::queueCall(const QString &method,
                              const QList<QVariant> &args,
                              QObject *receiver,
                              const char *replySlot,
                              const char *errorSlot)
{
    TRACE() << method << m_status;
    if (m_status == Ready) {
        doCall(method, args, receiver, replySlot, errorSlot);
        return 0;
    } else if (m_status == Incomplete) {
        Operation *op = new Operation(method, args,
                                      receiver, replySlot, errorSlot,
                                      m_nextCallId++);
        m_operationsQueue.enqueue(op);
        return op->m_id;
    } else {
        sendErrorReply(receiver, errorSlot);
        return -1;
    }
}

bool AsyncDBusProxy::connect(const char *name,
                             QObject *receiver,
                             const char *slot)
{
    if (m_status == Ready) {
        return m_interface->connect(name, receiver, slot);
    } else if (m_status == Incomplete) {
        Connection *connection = new Connection(name, receiver, slot);
        m_connectionsQueue.enqueue(connection);
        return true;
    } else {
        return false;
    }
}

bool AsyncDBusProxy::cancelCall(int id)
{
    for (int i = 0; i < m_operationsQueue.count(); i++) {
        Operation *op = m_operationsQueue.at(i);
        if (op->m_id == id) {
            delete op;
            m_operationsQueue.removeAt(i);
            return true;
        }
    }
    return false;
}
