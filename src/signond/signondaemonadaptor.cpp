/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 * Copyright (C) 2011 Intel Corporation.
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Contact: Aurel Popirtac <ext-aurel.popirtac@nokia.com>
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
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

#include "signondaemonadaptor.h"
#include "signondisposable.h"
#include "accesscontrolmanagerhelper.h"

namespace SignonDaemonNS {

SignonDaemonAdaptor::SignonDaemonAdaptor(SignonDaemon *parent):
    QDBusAbstractAdaptor(parent),
    m_parent(parent)
{
    setAutoRelaySignals(false);
}

SignonDaemonAdaptor::~SignonDaemonAdaptor()
{
}

void SignonDaemonAdaptor::registerNewIdentity(QDBusObjectPath &objectPath)
{
    QObject *identity = m_parent->registerNewIdentity();
    objectPath = registerObject(parentDBusContext().connection(), identity);

    SignonDisposable::destroyUnused();
}

void SignonDaemonAdaptor::securityErrorReply()
{
    securityErrorReply(parentDBusContext().connection(),
                       parentDBusContext().message());
}

void SignonDaemonAdaptor::securityErrorReply(const QDBusConnection &conn,
                                             const QDBusMessage &msg)
{
    QString errMsg;
    QTextStream(&errMsg) << SIGNOND_PERMISSION_DENIED_ERR_STR
                         << "Method:"
                         << msg.member();

    msg.setDelayedReply(true);
    QDBusMessage errReply =
                msg.createErrorReply(SIGNOND_PERMISSION_DENIED_ERR_NAME,
                                     errMsg);
    conn.send(errReply);
    TRACE() << "Method FAILED Access Control check:" << msg.member();
}

bool SignonDaemonAdaptor::handleLastError(const QDBusConnection &conn,
                                          const QDBusMessage &msg)
{
    if (!m_parent->lastErrorIsValid()) return false;

    msg.setDelayedReply(true);
    QDBusMessage errReply =
                msg.createErrorReply(m_parent->lastErrorName(),
                                     m_parent->lastErrorMessage());
    conn.send(errReply);
    return true;
}

QDBusObjectPath
SignonDaemonAdaptor::registerObject(const QDBusConnection &connection,
                                    QObject *object)
{
    QString path = object->objectName();

    if (connection.objectRegisteredAt(path) != object) {
        QDBusConnection conn(connection);
        if (!conn.registerObject(path, object,
                                 QDBusConnection::ExportAdaptors)) {
            BLAME() << "Object registration failed:" << object <<
                conn.lastError();
        }
    }
    return QDBusObjectPath(path);
}

void SignonDaemonAdaptor::getIdentity(const quint32 id,
                                      QDBusObjectPath &objectPath,
                                      QVariantMap &identityData)
{
    AccessControlManagerHelper *acm = AccessControlManagerHelper::instance();
    QDBusMessage msg = parentDBusContext().message();
    QDBusConnection conn = parentDBusContext().connection();
    if (!acm->isPeerAllowedToUseIdentity(conn, msg, id)) {
        securityErrorReply();
        return;
    }

    QObject *identity = m_parent->getIdentity(id, identityData);
    if (handleLastError(conn, msg)) return;

    objectPath = registerObject(conn, identity);

    SignonDisposable::destroyUnused();
}

QStringList SignonDaemonAdaptor::queryMethods()
{
    return m_parent->queryMethods();
}

QString SignonDaemonAdaptor::getAuthSessionObjectPath(const quint32 id,
                                                      const QString &type)
{
    SignonDisposable::destroyUnused();

    AccessControlManagerHelper *acm = AccessControlManagerHelper::instance();
    QDBusMessage msg = parentDBusContext().message();
    QDBusConnection conn = parentDBusContext().connection();

    /* Access Control */
    if (id != SIGNOND_NEW_IDENTITY) {
        if (!acm->isPeerAllowedToUseAuthSession(conn, msg, id)) {
            securityErrorReply();
            return QString();
        }
    }

    TRACE() << "ACM passed, creating AuthSession object";
    pid_t ownerPid = acm->pidOfPeer(conn, msg);
    QObject *authSession = m_parent->getAuthSession(id, type, ownerPid);
    if (handleLastError(conn, msg)) return QString();

    QDBusObjectPath objectPath = registerObject(conn, authSession);
    return objectPath.path();
}

QStringList SignonDaemonAdaptor::queryMechanisms(const QString &method)
{
    QStringList mechanisms = m_parent->queryMechanisms(method);
    if (handleLastError(parentDBusContext().connection(),
                        parentDBusContext().message())) {
        return QStringList();
    }

    return mechanisms;
}

void SignonDaemonAdaptor::queryIdentities(const QVariantMap &filter)
{
    /* Access Control */
    QDBusMessage msg = parentDBusContext().message();
    QDBusConnection conn = parentDBusContext().connection();
    if (!AccessControlManagerHelper::instance()->isPeerKeychainWidget(conn,
                                                                      msg)) {
        securityErrorReply();
        return;
    }

    msg.setDelayedReply(true);
    MapList identities = m_parent->queryIdentities(filter);
    if (handleLastError(conn, msg)) return;

    QDBusMessage reply = msg.createReply(QVariant::fromValue(identities));
    conn.send(reply);
}

bool SignonDaemonAdaptor::clear()
{
    /* Access Control */
    QDBusMessage msg = parentDBusContext().message();
    QDBusConnection conn = parentDBusContext().connection();
    if (!AccessControlManagerHelper::instance()->isPeerKeychainWidget(conn,
                                                                      msg)) {
        securityErrorReply();
        return false;
    }

    bool ok = m_parent->clear();
    if (handleLastError(conn, msg)) return false;

    return ok;
}

} //namespace SignonDaemonNS
