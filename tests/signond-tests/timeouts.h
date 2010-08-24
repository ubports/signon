/*
 * This file is part of signon
 *
 * Copyright (C) 2010 Nokia Corporation.
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

#ifndef TIMEOUTS_TEST_H
#define TIMEOUTS_TEST_H

#include <SignOn/Identity>

#include <QtTest/QtTest>
#include <QtCore>

using namespace SignOn;

class TimeoutsTest: public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void credentialsStored(const quint32 id);
    void identityError(Identity::IdentityError code, const QString &message);

#if defined(SSO_CI_TESTMANAGEMENT)
     void runAllTests();
#endif

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void identityTimeout();
    void identityRegisterTwice();


signals:
    void finished();

private:
    bool triggerDisposableCleanup();
    bool identityAlive(const QString &path);

    QProcess *daemonProcess;
    bool completed;
};

#endif // TIMEOUTS_TEST_H

