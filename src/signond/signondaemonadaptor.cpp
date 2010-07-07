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

#include "signondaemonadaptor.h"
#include "signondisposable.h"
#include "accesscontrolmanager.h"

namespace SignonDaemonNS {

    SignonDaemonAdaptor::SignonDaemonAdaptor(SignonDaemon *parent)
        : QDBusAbstractAdaptor(parent),
          m_parent(parent)
    {
        setAutoRelaySignals(false);
    }

    SignonDaemonAdaptor::~SignonDaemonAdaptor()
    {}

    bool SignonDaemonAdaptor::initSecureStorage(const QByteArray &lockCode)
    {
        return m_parent->initSecureStorage(lockCode);
    }

    void SignonDaemonAdaptor::registerNewIdentity(QDBusObjectPath &objectPath)
    {
        SignonDisposable::destroyUnused();

        m_parent->registerNewIdentity(objectPath);
    }

    void SignonDaemonAdaptor::securityErrorReply(const char *failedMethodName)
    {
        QString errMsg;
        QTextStream(&errMsg) << SIGNOND_PERMISSION_DENIED_ERR_STR
                             << "Method:"
                             << failedMethodName;

        QDBusMessage errReply =
                    parentDBusContext().message().createErrorReply(
                                            SIGNOND_PERMISSION_DENIED_ERR_NAME,
                                            errMsg);
        SIGNOND_BUS.send(errReply);
        TRACE() << "\nMethod FAILED Access Control check:\n" << failedMethodName;
    }

    void SignonDaemonAdaptor::registerStoredIdentity(const quint32 id, QDBusObjectPath &objectPath, QList<QVariant> &identityData)
    {
        SignonDisposable::destroyUnused();

        if (!AccessControlManager::isPeerAllowedToUseIdentity(
                                        parentDBusContext(), id)) {
            securityErrorReply(__func__);
            return;
        }

        m_parent->registerStoredIdentity(id, objectPath, identityData);
    }

    QStringList SignonDaemonAdaptor::queryMethods()
    {
        return m_parent->queryMethods();
    }

    QString SignonDaemonAdaptor::getAuthSessionObjectPath(const quint32 id, const QString &type)
    {
        SignonDisposable::destroyUnused();
        TRACE();

        /* Access Control */
        if (id != SIGNOND_NEW_IDENTITY) {
            if (!AccessControlManager::isPeerAllowedToUseAuthSession(
                                            parentDBusContext(), id)) {
                securityErrorReply(__func__);
                return QString();
            }
        }

        TRACE() << "ACM passed, creating AuthSession object";
        return m_parent->getAuthSessionObjectPath(id, type);
    }

    QStringList SignonDaemonAdaptor::queryMechanisms(const QString &method)
    {
        return m_parent->queryMechanisms(method);
    }

    QList<QVariant> SignonDaemonAdaptor::queryIdentities(const QMap<QString, QVariant> &filter)
    {
        /* Access Control */
        if (!AccessControlManager::isPeerKeychainWidget(parentDBusContext())) {
            securityErrorReply(__func__);
            return QList<QVariant>();
        }

        return m_parent->queryIdentities(filter);
    }

    bool SignonDaemonAdaptor::clear()
    {
        /* Access Control */
        if (!AccessControlManager::isPeerKeychainWidget(parentDBusContext())) {
            securityErrorReply(__func__);
            return false;
        }

        return m_parent->clear();
    }

    bool SignonDaemonAdaptor::setDeviceLockCode(const QByteArray &oldLockCode,
                                                const QByteArray &newLockCode)
    {
        /* TODO - this should be access controlled, too. Have to identify the
                  caller process/es. */
        return m_parent->setDeviceLockCode(oldLockCode, newLockCode);
    }

    bool SignonDaemonAdaptor::remoteLock(const QByteArray &lockCode)
    {
        /* TODO - this should be access controlled, too. Have to identify the
                  caller process/es. */
        return m_parent->remoteLock(lockCode);
    }

} //namespace SignonDaemonNS
