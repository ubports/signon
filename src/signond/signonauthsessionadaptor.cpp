/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
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

#include "signonauthsessionadaptor.h"
#include "accesscontrolmanager.h"

namespace SignonDaemonNS {

    SignonAuthSessionAdaptor::SignonAuthSessionAdaptor(SignonAuthSession *parent) : QDBusAbstractAdaptor(parent)
    {
        setAutoRelaySignals(true);
    }

    SignonAuthSessionAdaptor::~SignonAuthSessionAdaptor()
    {
    }

    void SignonAuthSessionAdaptor::errorReply(const QString &name,
                                              const QString &message)
    {
        QDBusMessage errReply =
            static_cast<QDBusContext *>(parent())->message().createErrorReply(name, message);
        SIGNOND_BUS.send(errReply);
    }

    QStringList SignonAuthSessionAdaptor::queryAvailableMechanisms(const QStringList &wantedMechanisms)
    {
        TRACE();

        QDBusContext &dbusContext = *static_cast<QDBusContext *>(parent());
        if (AccessControlManager::pidOfPeer(dbusContext) != parent()->ownerPid()) {
            TRACE() << "queryAvailableMechanisms called from peer that doesn't own the AuthSession object\n";
            QString errMsg;
            QTextStream(&errMsg) << SIGNOND_PERMISSION_DENIED_ERR_STR
                                 << " Authentication session owned by other process.";
            errorReply(SIGNOND_PERMISSION_DENIED_ERR_NAME, errMsg);
            return QStringList();
        }

        return parent()->queryAvailableMechanisms(wantedMechanisms);
    }

    QVariantMap SignonAuthSessionAdaptor::process(const QVariantMap &sessionDataVa, const QString &mechanism)
    {
        TRACE();

        QDBusContext &dbusContext = *static_cast<QDBusContext *>(parent());
        if (AccessControlManager::pidOfPeer(dbusContext) != parent()->ownerPid()) {
            TRACE() << "process called from peer that doesn't own the AuthSession object\n";
            QString errMsg;
            QTextStream(&errMsg) << SIGNOND_PERMISSION_DENIED_ERR_STR
                                 << " Authentication session owned by other process.";
            errorReply(SIGNOND_PERMISSION_DENIED_ERR_NAME, errMsg);
            return QVariantMap();
        }

        return parent()->process(sessionDataVa, mechanism);
    }

    void SignonAuthSessionAdaptor::cancel()
    {
        TRACE();

        QDBusContext &dbusContext = *static_cast<QDBusContext *>(parent());
        if (AccessControlManager::pidOfPeer(dbusContext) != parent()->ownerPid()) {
            TRACE() << "cancel called from peer that doesn't own the AuthSession object\n";
            return;
        }

        parent()->cancel();
    }

    void SignonAuthSessionAdaptor::setId(quint32 id)
    {
        TRACE();

        QDBusContext &dbusContext = *static_cast<QDBusContext *>(parent());
        if (AccessControlManager::pidOfPeer(dbusContext) != parent()->ownerPid()) {
            TRACE() << "setId called from peer that doesn't own the AuthSession object\n";
            return;
        }

        parent()->setId(id);
    }

    void SignonAuthSessionAdaptor::objectUnref()
    {
        TRACE();

        QDBusContext &dbusContext = *static_cast<QDBusContext *>(parent());
        if (AccessControlManager::pidOfPeer(dbusContext) != parent()->ownerPid()) {
            TRACE() << "objectUnref called from peer that doesn't own the AuthSession object\n";
            return;
        }

        parent()->objectUnref();
    }

} //namespace SignonDaemonNS
