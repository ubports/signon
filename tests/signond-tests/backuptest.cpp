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
#include "backuptest.h"
#include <signond/signoncommon.h>
 #include <QDBusMessage>

void TestBackup::initTestCase()
{
    /*
    daemonProcess = new QProcess();
    daemonProcess->start("signond");
    daemonProcess->waitForStarted(10 * 1000);
    */
}

void TestBackup::cleanupTestCase()
{
/*
    daemonProcess->kill();
    daemonProcess->waitForFinished();
    delete daemonProcess;
*/
}
void TestBackup::init()
{
    //wait a bit between tests
    sleep(5);
}
void TestBackup::cleanup()
{
    sleep(1);
}

void TestBackup::backupTest()
{
    QDBusConnection conn (SIGNOND_BUS);
    QDBusMessage msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "prestart");
    QDBusMessage reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    //remove backup files
    QFile::remove("/home/user/.signon/signondb.bin");

    msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "backup");
    QList<QVariant> args;
    args.append(QStringList("settings"));
    msg.setArguments(args);
    reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    //check if backup file was created successfully
    QVERIFY(QFile::exists("/home/user/.signon/signondb.bin"));


    msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "close");
    reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

}
void TestBackup::restoreTest()
{
    QDBusConnection conn (SIGNOND_BUS);
    QDBusMessage msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "prestart");
    QDBusMessage reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "restore");
    QList<QVariant> args;
    args.append(QStringList("settings"));
    args.append(QString("harmattan"));
    args.append(QStringList());
    msg.setArguments(args);
    reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    //check if backup file was created successfully
    QVERIFY(QFile::remove("/home/user/.signon/signondb.bin"));
    QVERIFY(QFile::exists("/home/user/signon.db"));

    msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "close");
    reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

}

void TestBackup::backupNormalTest()
{

    daemonProcess = new QProcess();
    daemonProcess->start("signond");
    daemonProcess->waitForStarted(10 * 1000);

    QDBusConnection conn (SIGNOND_BUS);
    QDBusMessage msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "prestart");
    QDBusMessage reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    //remove backup files
    QFile::remove("/home/user/.signon/signondb.bin");

    msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "backup");
    QList<QVariant> args;
    args.append(QStringList("settings"));
    msg.setArguments(args);
    reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    //check if backup file was created successfully
    QVERIFY(QFile::exists("/home/user/.signon/signondb.bin"));


    msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "close");
    reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    //check that daemon is still running normally after backup close signal
    QVERIFY(daemonProcess->state()==QProcess::Running);
    daemonProcess->kill();
    daemonProcess->waitForFinished();
    delete daemonProcess;
    daemonProcess = NULL;
}
void TestBackup::restoreNormalTest()
{

    daemonProcess = new QProcess();
    daemonProcess->start("signond");
    daemonProcess->waitForStarted(10 * 1000);

    QDBusConnection conn (SIGNOND_BUS);
    QDBusMessage msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "prestart");
    QDBusMessage reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "restore");
    QList<QVariant> args;
    args.append(QStringList("settings"));
    args.append(QString("harmattan"));
    args.append(QStringList());
    msg.setArguments(args);
    reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    //check if backup file was created successfully
    QVERIFY(QFile::remove("/home/user/.signon/signondb.bin"));
    QVERIFY(QFile::exists("/home/user/signon.db"));

    msg = QDBusMessage::createMethodCall(SIGNOND_SERVICE + ".backup",
                                            SIGNOND_DAEMON_OBJECTPATH + "/backup",
                                            "com.nokia.backupclient",
                                            "close");
    reply = conn.call(msg, QDBus::Block, 10*1000);

    QVERIFY(reply.type() == QDBusMessage::ReplyMessage);

    //check that daemon is still running normally after backup close signal
    QVERIFY(daemonProcess->state()==QProcess::Running);
    daemonProcess->kill();
    daemonProcess->waitForFinished();
    delete daemonProcess;
    daemonProcess = NULL;

}

void TestBackup::runAllTests()
{
    initTestCase();

    init();
    backupTest();
    cleanup();

    init();
    restoreTest();
    cleanup();

    init();
    backupNormalTest();
    cleanup();

    init();
    restoreNormalTest();
    cleanup();

    cleanupTestCase();
}

#if !defined(SSO_CI_TESTMANAGEMENT)
    QTEST_MAIN(TestPluginProxy)
#endif
