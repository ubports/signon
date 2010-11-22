/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of signon
 *
 * Copyright (C) 2010 Nokia Corporation.
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

#include "debug.h"
#include "device-lock-code-handler.h"
#include "key-manager.h"

using namespace SignOn;

KeyManager::KeyManager(QObject *parent):
    AbstractKeyManager(parent)
{
    qDebug() << Q_FUNC_INFO << "Constructed";
}

KeyManager::~KeyManager()
{
}

void KeyManager::setup()
{
    qDebug() << Q_FUNC_INFO << "Initialized";

    dlcHandler = new DeviceLockCodeHandler(this);
    connect(dlcHandler,
            SIGNAL(lockCodeSet(const QByteArray, const QByteArray)),
            SLOT(onLockCodeSet(const QByteArray, const QByteArray)));
}

void KeyManager::authorizeKey(const Key &key, const QString &message)
{
    Q_UNUSED(message);

    TRACE() << "key:" << key.toHex();

    if (key.isEmpty()) {
        emit keyAuthorized(key, false);
        return;
    }

    /* were we expecting this request?  If so, just authorized the key and if
     * an old one had to be removed, do it.
     */
    if (key == newDeviceLockKey) {
        emit keyAuthorized(key, true);
        newDeviceLockKey = QByteArray();

        if (!keyToBeRemoved.isEmpty()) {
            emit keyRemoved(keyToBeRemoved);
            keyToBeRemoved = QByteArray();
        }
        return;
    }

    /* Pop up a dialog to authorize the key; we'll get the answer in the
     * onLockCodeSet slot */
    keysToBeAuthorized << key;
    Q_ASSERT(dlcHandler != 0);
    dlcHandler->queryLockCode();
}

void KeyManager::queryKeys()
{
    TRACE();

    /* Pop up a dialog to authorize the key; we'll get the answer in the
     * onLockCodeSet slot */
    Q_ASSERT(dlcHandler != 0);
    dlcHandler->queryLockCode();
}

void KeyManager::onLockCodeSet(const QByteArray lockCode,
                               const QByteArray oldLockCode)
{
    TRACE() << "new:" << lockCode.toHex() << "old:" << oldLockCode.toHex();

    if (!oldLockCode.isEmpty()) {
        /* the device lock code has changed: we should remove the old key; but
         * we'll do that only after the new key has been authorized. So, for
         * the moment just keep the old key in memory
         */
        keyToBeRemoved = oldLockCode;
    }

    if (!lockCode.isEmpty()) {
        /* we tell signond about the new key; signond will most likely ask us
         * to authorize the key; so we keep it in memory, till the
         * authorizeKey() method is called.
         */
        newDeviceLockKey = lockCode;
        emit keyInserted(lockCode);
    }

    /* If we have some keys to authorize, do so */
    bool authorized = !lockCode.isEmpty();
    foreach (Key key, keysToBeAuthorized) {
        emit keyAuthorized(key, authorized);
    }
    keysToBeAuthorized.clear();
}

