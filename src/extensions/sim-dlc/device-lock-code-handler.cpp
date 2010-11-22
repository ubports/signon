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

#include "device-lock-code-handler.h"
#include "debug.h"

#include <devicelock/devicelock.h>

#include <QDBusInterface>
#include <QDBusArgument>

using namespace DeviceLock;

#define SIGNON_DEVICE_LOCK_INTERFACE "com.nokia.devicelock"
#define SIGNOND_MAX_TIMEOUT 0x7FFFFFFF

DeviceLockCodeHandler::DeviceLockCodeHandler(QObject *parent)
    : QObject(parent)
{

    m_dbusInterface = new QDBusInterface(QLatin1String(DEVLOCK_SERVICE),
                                         QLatin1String(DEVLOCK_PATH),
                                         QLatin1String(SIGNON_DEVICE_LOCK_INTERFACE),
                                         QDBusConnection::systemBus(),
                                         this);
    if (!m_dbusInterface->isValid())
        BLAME() << "Device lock DBUS interface invalid.";
}

DeviceLockCodeHandler::~DeviceLockCodeHandler()
{
}

bool DeviceLockCodeHandler::callWithTimeout(const QString &operation,
                                            const char *replySlot,
                                            const QList<QVariant> &args)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(m_dbusInterface->service(),
                                                      m_dbusInterface->path(),
                                                      m_dbusInterface->interface(),
                                                      operation);
    if (!args.isEmpty())
        msg.setArguments(args);

    return m_dbusInterface->connection().callWithCallback(msg,
                                                          this,
                                                          replySlot,
                                                          SLOT(errorReply(const QDBusError&)),
                                                          SIGNOND_MAX_TIMEOUT);
}

void DeviceLockCodeHandler::configureLockCode()
{
    bool result = false;

    ProvisioningSettingsMap settings;
    settings.insert(DeviceLockEnums::DevicePasswordRequired,
                    QVariant(true));

    QDBusArgument arg;
    arg << settings;

    QList<QVariant> args = QList<QVariant>()
        << QString()
        << qVariantFromValue(arg);

    if ((!m_dbusInterface->isValid()) && m_dbusInterface->lastError().isValid())
        result = callWithTimeout(QString::fromLatin1("updateProvisioningSettings"),
                                 SLOT(updateProvisioningSettingsReply(bool)),
                                 args);
    else
        result = m_dbusInterface->callWithCallback(
                                            QString::fromLatin1("updateProvisioningSettings"),
                                            args,
                                            this,
                                            SLOT(updateProvisioningSettingsReply(bool)),
                                            SLOT(errorReply(const QDBusError &)));
    if (!result) {
        BLAME() << "Error in setting and querying device lock code.";
    }
}

void DeviceLockCodeHandler::queryLockCode()
{
    bool result = false;

    QList<QVariant> args = QList<QVariant>()
        << static_cast<int>(DeviceLockEnums::Device)
        << static_cast<int>(DeviceLockEnums::PasswordPrompt);

    if ((!m_dbusInterface->isValid()) && m_dbusInterface->lastError().isValid())
        result = callWithTimeout(QString::fromLatin1("setState"),
                                 SLOT(setStateReply(bool)),
                                 args);
    else
        result = m_dbusInterface->callWithCallback(
                                            QString::fromLatin1("setState"),
                                            args,
                                            this,
                                            SLOT(setStateReply(bool)),
                                            SLOT(errorReply(const QDBusError &)));
    if (!result) {
        BLAME() << "Error in querying device lock code.";
    }
}

void DeviceLockCodeHandler::setStateReply(bool result)
{
    if (!result) {
        TRACE() << "Could not query the device lock code,"
                   " attempting to configure it.";
        configureLockCode();
    }
}

void DeviceLockCodeHandler::updateProvisioningSettingsReply(bool result)
{
    TRACE() << "Could not configure the device lock code.";
}

void DeviceLockCodeHandler::errorReply(const QDBusError &error)
{
    TRACE() << error.type() << error.name() << error.message();
}

bool DeviceLockCodeHandler::isLockCodeValid()
{
    return false;
}
