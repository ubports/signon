/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

/*!
 * @file devicelockhandler.h
 * @brief Definition of the device lock code handler object.
 */

#ifndef DEVICELOCKCODEHANDLER_H
#define DEVICELOCKCODEHANDLER_H

#include <QObject>
#include <QtDBus>

class QDBusInterface;
class DeviceLockCodeHandler;

class DeviceLockCodeHandlerAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.nokia.SingleSignOn.DeviceLock")

public:
    DeviceLockCodeHandlerAdaptor(DeviceLockCodeHandler *parent);
    virtual ~DeviceLockCodeHandlerAdaptor() {};

public Q_SLOTS:
    bool setDeviceLockCode(const QByteArray &lockCode,
                           const QByteArray &oldLockCode);
private:
    DeviceLockCodeHandler *parent;
};

/*!
 * @class DeviceLockCodeHandler
 * DeviceLockCodeHandler handles getting the device lock code
 * which is used by the CryptoManager as a master key for the mounting
 * of the encrypted file system.
 * @ingroup Accounts_and_SSO_Framework
 * @sa CryptoManager
 */
class DeviceLockCodeHandler : public QObject
{
    Q_OBJECT

public:
    /*!
      * Constructs a DeviceLockCodeHandler object with the given parent.
      * @param parent the parent object
      */
    DeviceLockCodeHandler(QObject *parent = 0);

    /*!
      * Destructor, releases allocated resources.
      */
    ~DeviceLockCodeHandler();

    /*!
      * Displays the Device Lock query UI.
      * If the query is successful, `SignonDaemon::setDeviceLockCode(...)` will
      * be triggered by the devicelock daemon.
      * @sa SignonDaemon
      */
    void queryLockCode();

    /*!
      * @todo check if this is needed.
      */
    bool isLockCodeValid();

Q_SIGNALS:
    void lockCode(const QByteArray &);
    void lockCodeSet(const QByteArray lockCode,
                     const QByteArray oldLockCode);

public Q_SLOTS:
    // Interface method to set the device lock code
    bool setDeviceLockCode(const QByteArray &lockCode,
                           const QByteArray &oldLockCode);

private Q_SLOTS:
    void setStateReply(bool result);
    void updateProvisioningSettingsReply(bool result);
    void errorReply(const QDBusError &error);

private:
    void registerDBusService();
    void configureLockCode();
    bool callWithTimeout(const QString &operation,
                         const char *replySlot,
                         const QList<QVariant> &args = QList<QVariant>());
private:
    QDBusInterface *m_dbusInterface;
};

#endif // DEVICELOCKCODEHANDLER_H
