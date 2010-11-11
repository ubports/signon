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

#include "signonidentityadaptor.h"

#include "signonidentity.h"
#include "accesscontrolmanager.h"

namespace SignonDaemonNS {

    SignonIdentityAdaptor::SignonIdentityAdaptor(SignonIdentity *parent)
        : QDBusAbstractAdaptor(parent),
          m_parent(parent)
    {
        setAutoRelaySignals(true);
    }

    SignonIdentityAdaptor::~SignonIdentityAdaptor()
    {}

    void SignonIdentityAdaptor::securityErrorReply(const char *failedMethodName)
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

    quint32 SignonIdentityAdaptor::requestCredentialsUpdate(const QString &msg)
    {
        /* Access Control */
        if (!AccessControlManager::isPeerAllowedToUseIdentity(
                                        parentDBusContext(), m_parent->id())) {
            /*TODO - uncomment this and remove trace line after NB#196033 is fixed.
            securityErrorReply(__func__);
            return 0;
            */
            qWarning() << Q_FUNC_INFO << accessControlTmpWarningMessage;
        }
        QDBusMessage m_message = parentDBusContext().message();
        m_message.setDelayedReply(true);
        QDBusMessage delayReply = m_message.createReply();
        SIGNOND_BUS.send(delayReply);

        return m_parent->requestCredentialsUpdate(msg);
    }

    QList<QVariant> SignonIdentityAdaptor::queryInfo()
    {
        /* Access Control */
        if (!AccessControlManager::isPeerAllowedToUseIdentity(
                                        parentDBusContext(), m_parent->id())) {
            /*TODO - uncomment this and remove trace line after NB#196033 is fixed.
            securityErrorReply(__func__);
            return QList<QVariant>();
            */
            qWarning() << Q_FUNC_INFO << accessControlTmpWarningMessage;
        }

        return m_parent->queryInfo();
    }

    quint32 SignonIdentityAdaptor::addReference(const QString &reference)
    {
        /* Access Control */
        if (!AccessControlManager::isPeerAllowedToUseIdentity(
                                        parentDBusContext(), m_parent->id())) {
            /*TODO - uncomment this and remove trace line after NB#196033 is fixed.
            securityErrorReply(__func__);
            return 0;
            */
            qWarning() << Q_FUNC_INFO << accessControlTmpWarningMessage;
        }

        return m_parent->addReference(reference) ? 1 : 0;
    }

    quint32 SignonIdentityAdaptor::removeReference(const QString &reference)
    {
        /* Access Control */
        if (!AccessControlManager::isPeerAllowedToUseIdentity(
                                        parentDBusContext(), m_parent->id())) {
            /*TODO - uncomment this and remove trace line after NB#196033 is fixed.
            securityErrorReply(__func__);
            return 0;
            */
            qWarning() << Q_FUNC_INFO << accessControlTmpWarningMessage;
        }

        return m_parent->addReference(reference) ? 1 : 0;
    }


    bool SignonIdentityAdaptor::verifyUser(const QString &message)
    {
        /* Access Control */
        if (!AccessControlManager::isPeerAllowedToUseIdentity(
                                        parentDBusContext(), m_parent->id())) {
            /*TODO - uncomment this and remove trace line after NB#196033 is fixed.
            securityErrorReply(__func__);
            return false;
            */
            qWarning() << Q_FUNC_INFO << accessControlTmpWarningMessage;
        }

        return m_parent->verifyUser(message);
    }

    bool SignonIdentityAdaptor::verifySecret(const QString &secret)
    {
        /* Access Control */
        if (!AccessControlManager::isPeerAllowedToUseIdentity(
                                        parentDBusContext(), m_parent->id())) {
            /*TODO - uncomment this and remove trace line after NB#196033 is fixed.
            securityErrorReply(__func__);
            return false;
            */
            qWarning() << Q_FUNC_INFO << accessControlTmpWarningMessage;
        }

        return m_parent->verifySecret(secret);
    }

    void SignonIdentityAdaptor::remove()
    {
        /* Access Control */
        AccessControlManager::IdentityOwnership ownership =
                AccessControlManager::isPeerOwnerOfIdentity(
                            parentDBusContext(), m_parent->id());

        if (ownership != AccessControlManager::IdentityDoesNotHaveOwner) {
            //Identity has an owner
            if (ownership == AccessControlManager::ApplicationIsNotOwner
                && !AccessControlManager::isPeerKeychainWidget(parentDBusContext())) {
                /*TODO - uncomment this and remove trace line after NB#196033 is fixed.
                securityErrorReply(__func__);
                return;
                */
                qWarning() << Q_FUNC_INFO << accessControlTmpWarningMessage;
            }
        }

        m_parent->remove();
    }

    bool SignonIdentityAdaptor::signOut()
    {
        /* Access Control */
        if (!AccessControlManager::isPeerAllowedToUseIdentity(
                                        parentDBusContext(), m_parent->id())) {
            /*TODO - uncomment this and remove trace line after NB#196033 is fixed.
            securityErrorReply(__func__);
            return false;
            */
            qWarning() << Q_FUNC_INFO << accessControlTmpWarningMessage;
        }

        return m_parent->signOut();
    }

    quint32 SignonIdentityAdaptor::storeCredentials(const quint32 id,
                                                    const QString &userName,
                                                    const QString &secret,
                                                    const bool storeSecret,
                                                    const QMap<QString, QVariant> &methods,
                                                    const QString &caption,
                                                    const QStringList &realms,
                                                    const QStringList &accessControlList,
                                                    const int type)
    {
        /* Access Control */
        if (id != SIGNOND_NEW_IDENTITY) {
            AccessControlManager::IdentityOwnership ownership =
                    AccessControlManager::isPeerOwnerOfIdentity(
                                                parentDBusContext(), m_parent->id());

            if (ownership != AccessControlManager::IdentityDoesNotHaveOwner) {
                //Identity has an owner
                if (ownership == AccessControlManager::ApplicationIsNotOwner
                    && !AccessControlManager::isPeerKeychainWidget(parentDBusContext())) {
                    /*TODO - uncomment this and remove trace line after NB#196033 is fixed.
                    securityErrorReply(__func__);
                    return 0;
                    */
                    qWarning() << Q_FUNC_INFO << accessControlTmpWarningMessage;
                }
            }
        }

        return m_parent->storeCredentials(id,
                                          userName,
                                          secret,
                                          storeSecret,
                                          methods,
                                          caption,
                                          realms,
                                          accessControlList,
                                          type);
    }

} //namespace SignonDaemonNS
