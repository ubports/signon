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

#ifndef KEYMANAGER_H
#define KEYMANAGER_H

#include <SignOn/AbstractKeyManager>

class DeviceLockCodeHandler;

class KeyManager: public SignOn::AbstractKeyManager
{
    Q_OBJECT

public:
    KeyManager(QObject *parent = 0);
    ~KeyManager();

    // reimplemented virtual methods
    void setup();
    void authorizeKey(const SignOn::Key &key,
                      const QString &message = QString());
    void queryKeys();

private Q_SLOTS:
    void onLockCodeSet(const QByteArray lockCode,
                       const QByteArray oldLockCode);

private:
    DeviceLockCodeHandler *dlcHandler;
    SignOn::Key newDeviceLockKey;
    SignOn::Key keyToBeRemoved;
    QList<SignOn::Key> keysToBeAuthorized;
};

#endif // KEYMANAGER_H

