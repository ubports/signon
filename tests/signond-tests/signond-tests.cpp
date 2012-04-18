/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
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

#include "testpluginproxy.h"
#include "timeouts.h"
#include "backuptest.h"
#include "databasetest.h"

#ifdef CAM_UNIT_TESTS_FIXED
#include "credentialsaccessmanagertest.h"
#endif

#include <QCoreApplication>
#include <QtTest/QtTest>
#include <QtCore>

class SignondTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void runTimeoutTests();
    void runPluginProxyTests();
    void runCAMTests();
    void runBackupTests();
    void runDatabaseTests();

public:
    TestPluginProxy testPluginProxy;
    TimeoutsTest testTimeouts;
    TestBackup testBackup;
    TestDatabase testDatabase;
#ifdef CAM_UNIT_TESTS_FIXED
    CredentialsAccessManagerTest testCAM;
#endif
};

void SignondTest::runTimeoutTests()
{
    testTimeouts.runAllTests();
}

void SignondTest::runPluginProxyTests()
{
    testPluginProxy.runAllTests();
}

void SignondTest::runCAMTests()
{
#if !defined(CAM_UNIT_TESTS_FIXED)
    QSKIP("This test requires fixes in CAM", SkipSingle);
#else
    testCAM.runAllTests();
#endif
}

void SignondTest::runBackupTests()
{
    QSKIP("Backup has been changed", SkipSingle);
    testBackup.runAllTests();
}

void SignondTest::runDatabaseTests()
{
    testDatabase.runAllTests();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    SignondTest signondTest;
    QTest::qExec(&signondTest, argc, argv);
}

#include "signond-tests.moc"

