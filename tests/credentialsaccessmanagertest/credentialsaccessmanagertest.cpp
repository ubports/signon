/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
 * Contact: Aurel Popirtac <ext-Aurel.Popirtac@nokia.com>
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


#include <QtTest/QtTest>
#include "credentialsaccessmanagertest.h"

#include "credentialsaccessmanager.h"

using namespace SignonDaemonNS;

#define TEST_START qDebug("\n\n\n\n ----------------- %s ----------------\n\n",  __func__);

#define TEST_DONE  qDebug("\n\n ----------------- %s PASS ----------------\n\n",  __func__);


void CredentialsAccessManagerTest::initTestCase()
{
    TEST_START
    m_pManager = CredentialsAccessManager::instance();

    QVERIFY(m_pManager != NULL);
    QCOMPARE(m_pManager->lastError(), SignonDaemonNS::NoError);

    //otherwise using currrent default configuration
    CAMConfiguration config;
    config.m_useEncryption = true;
    config.m_dbFileSystemPath = QLatin1String("/home/user/MyDocs/signonfs");
    config.m_encryptionPassphrase = "1234";

    QVERIFY(m_pManager->init(config));

    QCOMPARE(m_pManager->lastError(), SignonDaemonNS::NoError);
    TEST_DONE
}

void CredentialsAccessManagerTest::cleanupTestCase()
{
    TEST_START
    if(m_pManager)
        delete m_pManager;
    TEST_DONE
}

//test cases

void CredentialsAccessManagerTest::createCredentialsSystem()
{
    TEST_START
    QSKIP("This test requires to be reconsidered.", SkipSingle);

    QVERIFY(m_pManager->openCredentialsSystem());

    QCOMPARE(m_pManager->lastError(), SignonDaemonNS::NoError);

    QVERIFY(m_pManager->credentialsSystemOpened());

    QVERIFY(m_pManager->closeCredentialsSystem());

    QCOMPARE(m_pManager->lastError(), SignonDaemonNS::NoError);

    QVERIFY(m_pManager->credentialsSystemOpened() == false);
    TEST_DONE
}

void CredentialsAccessManagerTest::openCredentialsSystem()
{
    TEST_START
    QSKIP("This test requires to be reconsidered.", SkipSingle);

    QVERIFY(m_pManager->openCredentialsSystem());

    QVERIFY(m_pManager->credentialsSystemOpened());

    QCOMPARE(m_pManager->lastError(), SignonDaemonNS::NoError);
    TEST_DONE
}

void CredentialsAccessManagerTest::testCredentialsDatabase()
{
    TEST_START

    CredentialsDB *db = m_pManager->credentialsDB();
    QVERIFY(db != NULL);

    if (!db->clear())
        qDebug() << "Clearing of credentials db failed. Test could fail.";

    // TODO - test with a greater number of crendetials.
    int numberOfStoredCredentials = 500;

    //Prepare Identity data
    QMap<QString, QStringList> mapList;
    for(int i = 0; i < 5; ++i)
        mapList.insert("my_so_called_method",
                       QStringList() << "mech1" << "mech2" << "mech3" << "mech4" << "mech5");
    QMap<QString, QVariant> methods =
            SignonIdentityInfo::mapListToMapVariant(mapList);

    QLatin1String caption("Long text about strawberries and old stories"
                          "about stupid things that make us feel good.");

    QStringList realms = QStringList() << "http://www.something.com/dsa=43fds=test"
                                       << "http://www.something.com/dsa=43fds=test"
                                       << "http://www.something.com/dsa=43fds=test"
                                       << "http://ww555008w.something.com/dsa=43fds=test"
                                       << "http://www.something.com/dsa=43fds=test"
                                       << "http://www.something.com/dsa=43fds=test";
    QStringList accessControlList = QStringList() << "PREF::package.token.gummybear"
                                                  << "PREF::package.token.gummybear"
                                                  << "PREF::package.token.beer"
                                                  << "PREF::package.token.stuff"
                                                  << "PREF::package.token.application"
                                                  << "PREF::package.token.wonders"
                                                  << "PREF::package.token.ventilation";
    SignonIdentityInfo info(0, QLatin1String("username"), QLatin1String("passsssword1231454"), true,
                            methods, caption, realms);

    info.m_accessControlList = accessControlList;

    //store 200 identities
    for(int j = 0; j < numberOfStoredCredentials; ++j) {
        QVERIFY2(db->insertCredentials(info, true) > 0,
                 QString(QLatin1String("Number of successfully inserted credentials:")
                 + QString::number(j)).toUtf8().constData());
    }

    QList<SignonIdentityInfo> result = db->credentials(QMap<QString, QString>());
    QVERIFY(result.count() == numberOfStoredCredentials);

    TEST_DONE
}

void CredentialsAccessManagerTest::closeCredentialsSystem()
{
    TEST_START
    QVERIFY(m_pManager->closeCredentialsSystem());
    QCOMPARE(m_pManager->lastError(), SignonDaemonNS::NoError);
    QVERIFY(m_pManager->credentialsSystemOpened() == false);
    TEST_DONE
}

void CredentialsAccessManagerTest::deleteCredentialsSystem()
{
    TEST_START
    //TODO
    TEST_DONE
}
#if defined(SSO_CI_TESTMANAGEMENT)
    void CredentialsAccessManagerTest::runAllTests()
    {
        initTestCase();
        createCredentialsSystem();
        openCredentialsSystem();
        testCredentialsDatabase();
        closeCredentialsSystem();
        deleteCredentialsSystem();
        cleanupTestCase();
    }
#else
    QTEST_MAIN(CredentialsAccessManagerTest)
#endif
