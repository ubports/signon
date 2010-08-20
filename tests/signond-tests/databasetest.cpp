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
#include "databasetest.h"
#include <signond/signoncommon.h>
#include <QDBusMessage>

#include "credentialsdb.h"
#include "signonidentityinfo.cpp"

const QString dbFile = QLatin1String("/tmp/signon_test.db");

void TestDatabase::initTestCase()
{
    QFile::remove(dbFile);
    m_db = new CredentialsDB(dbFile);
    QVERIFY(m_db != 0);
}

void TestDatabase::cleanupTestCase()
{
    delete m_db;
    m_db = NULL;
    //remove database file
    //QFile::remove(dbFile);
}

void TestDatabase::init()
{
}

void TestDatabase::cleanup()
{
}

void TestDatabase::sqlDBConfigurationTest()
{
    QMap<QString, QString> conf = m_db->sqlDBConfiguration();
    qDebug() << conf;
    QVERIFY(conf.value(QLatin1String("Database Name")) == QLatin1String("/tmp/signon_test.db"));
}

void TestDatabase::createTableStructureTest()
{
    QVERIFY(!m_db->hasTableStructure());
    m_db->createTableStructure();
    QMap<QString, QString> conf = m_db->sqlDBConfiguration();
    QVERIFY(!conf.value(QLatin1String("Tables")).isEmpty());
    QVERIFY(m_db->hasTableStructure());
}

void TestDatabase::insertMethodsTest()
{
    QMap<QString, QStringList> methods;
    m_db->insertMethods(10, methods);
    QStringList methodsList = m_db->methods(10);
    QVERIFY(methodsList.count() == 0);
    QStringList mechs = QStringList() << QLatin1String("M1")<< QLatin1String("M2");
    methods.insert(QLatin1String("Test"), mechs);
    methods.insert(QLatin1String("Test2"), mechs);
    m_db->insertMethods(11, methods);
    methodsList = m_db->methods(11);
    QVERIFY(methodsList.count() == 2);
    QVERIFY(methodsList.contains(QLatin1String("Test")));
}

void TestDatabase::removeMethodsTest()
{
    QStringList methodsList = m_db->methods(10);
    QVERIFY(methodsList.count() == 0);
    m_db->removeMethods(10);
    methodsList = m_db->methods(10);
    QVERIFY(methodsList.count() == 0);

    methodsList = m_db->methods(11);
    QVERIFY(methodsList.count() == 2);
    m_db->removeMethods(11);
    methodsList = m_db->methods(11);
    QVERIFY(methodsList.count() == 0);
}

void TestDatabase::insertListTest()
{
    QStringList list;
    m_db->startTransaction();
    m_db->insertList(list,QString::fromLatin1("INSERT INTO REALMS(identity_id, realm) "
                                   ), 67);
    m_db->commit();
    QStringList listRes = m_db->queryList(QString::fromLatin1(
            "SELECT realm FROM REALMS WHERE identity_id = 67"));
    QVERIFY(list == listRes);
    list.append(QLatin1String("Test"));
    list.append(QLatin1String("Test2"));

    m_db->startTransaction();
    m_db->insertList(list,QString::fromLatin1("INSERT INTO REALMS(identity_id, realm) "
                                   ), 68);
    m_db->commit();
    listRes = m_db->queryList(QString::fromLatin1(
            "SELECT realm FROM REALMS WHERE identity_id = 68"));
    QVERIFY(list == listRes);
}

void TestDatabase::removeListTest()
{
    QStringList listRes = m_db->queryList(QString::fromLatin1(
            "SELECT realm FROM REALMS WHERE identity_id = 68"));
    QVERIFY(listRes.count() == 2);
    m_db->removeList(QString::fromLatin1("DELETE FROM REALMS WHERE identity_id = %1"  ).arg(68));
    listRes = m_db->queryList(QString::fromLatin1(
            "SELECT realm FROM REALMS WHERE identity_id = 68"));
    QVERIFY(listRes.count() == 0);
}

void TestDatabase::queryListTest()
{
    QString queryStr;
    queryStr = QString::fromLatin1(
            "INSERT INTO REALMS (identity_id, realm) "
            "VALUES('%1', '%2')")
            .arg(80).arg(QLatin1String("a"));
    QSqlQuery insertQuery = m_db->exec(queryStr);
    queryStr = QString::fromLatin1(
            "INSERT INTO REALMS (identity_id, realm) "
            "VALUES('%1', '%2')")
            .arg(80).arg(QLatin1String("b"));
    insertQuery = m_db->exec(queryStr);
    QStringList list = m_db->queryList(QString::fromLatin1(
            "SELECT realm FROM REALMS WHERE identity_id = 80"));
    QVERIFY(list.contains(QLatin1String("a")));
    QVERIFY(list.contains(QLatin1String("b")));
    QVERIFY(list.count() == 2);
}

void TestDatabase::insertCredentialsTest()
{
    SignonIdentityInfo info;
    SignonIdentityInfo retInfo;
    quint32 id;

    //insert empty
    id = m_db->insertCredentials(info, false);
    retInfo = m_db->credentials(id, false);
    QVERIFY(id != info.m_id);
    info.m_id = id;
    QVERIFY(retInfo == info);

    //insert complete
    info.m_caption = QLatin1String("Caption");
    info.m_userName = QLatin1String("User");
    info.m_password = QLatin1String("Pass");
    info.m_realms = QStringList() << QLatin1String("Realm1.com") << QLatin1String("Realm2.com") << QLatin1String("Realm3.com") ;
    QMap<MethodName,MechanismsList> methods;
    QStringList mechs = QStringList() << QString::fromLatin1("Mech1") << QString::fromLatin1("Mech2") ;
    methods.insert(QLatin1String("Method1"), mechs);
    methods.insert(QLatin1String("Method2"), mechs);
    methods.insert(QLatin1String("Method3"), QStringList());
    info.m_methods = methods;
    info.m_accessControlList = QStringList() << QLatin1String("AID::12345678") << QLatin1String("AID::87654321") << QLatin1String("test::property") ;
    info.m_type = 3;

    id = m_db->insertCredentials(info, true);
    retInfo = m_db->credentials(id, false);
    QVERIFY(id != info.m_id);
    info.m_id = id;
    QVERIFY(info.m_password != retInfo.m_password);
    retInfo.m_password = info.m_password;

    QVERIFY(retInfo == info);

    //with password
    id = m_db->insertCredentials(info, true);
    retInfo = m_db->credentials(id, true);
    QVERIFY(id != info.m_id);
    info.m_id = id;
    QVERIFY(retInfo == info);
}

void TestDatabase::updateCredentialsTest()
{
    SignonIdentityInfo info;
    SignonIdentityInfo updateInfo;
    SignonIdentityInfo retInfo;
    quint32 id;

    //insert complete
    info.m_caption = QLatin1String("Caption");
    info.m_userName = QLatin1String("User");
    info.m_password = QLatin1String("Pass");
    info.m_realms = QStringList() << QLatin1String("Realm1.com") << QLatin1String("Realm2.com") << QLatin1String("Realm3.com") ;
    QMap<MethodName,MechanismsList> methods;
    QStringList mechs = QStringList() << QString::fromLatin1("Mech1") << QString::fromLatin1("Mech2") ;
    methods.insert(QLatin1String("Method1"), mechs);
    methods.insert(QLatin1String("Method2"), mechs);
    methods.insert(QLatin1String("Method3"), QStringList());
    info.m_methods = methods;
    info.m_accessControlList = QStringList() << QLatin1String("AID::12345678") << QLatin1String("AID::87654321") << QLatin1String("test::property") ;

    id = m_db->insertCredentials(info, true);
    retInfo = m_db->credentials(id, true);
    QVERIFY(id != info.m_id);
    info.m_id = id;
    QVERIFY(retInfo == info);

    //update complete
    updateInfo.m_caption = QLatin1String("Updated Caption");
    updateInfo.m_userName = QLatin1String("UpUser");
    updateInfo.m_password = QLatin1String("UpdatedPass");
    updateInfo.m_realms = QStringList() << QLatin1String("URealm1.com") << QLatin1String("URealm2.com") << QLatin1String("Realm3.com") ;
    QMap<MethodName,MechanismsList> umethods;
    QStringList umechs = QStringList() << QString::fromLatin1("UMech1") << QString::fromLatin1("Mech2") ;
    umethods.insert(QLatin1String("Method1"), umechs);
    umethods.insert(QLatin1String("UMethod2"), umechs);
    umethods.insert(QLatin1String("Method3"), QStringList());
    updateInfo.m_methods = umethods;
    updateInfo.m_accessControlList = QStringList() << QLatin1String("AID::12345678") << QLatin1String("AID::87654321") << QLatin1String("test::property") ;
    updateInfo.m_id = id;
    updateInfo.m_type = 2;

    QVERIFY(m_db->updateCredentials(updateInfo, true));

    retInfo = m_db->credentials(id, true);

QVERIFY (updateInfo.m_id == retInfo.m_id);
QVERIFY (updateInfo.m_userName == retInfo.m_userName);
QVERIFY (updateInfo.m_password == retInfo.m_password);
QVERIFY (updateInfo.m_caption == retInfo.m_caption);
QVERIFY (updateInfo.m_realms == retInfo.m_realms);
QVERIFY (updateInfo.m_accessControlList == retInfo.m_accessControlList);
QVERIFY (updateInfo.m_type == retInfo.m_type);
QVERIFY (updateInfo.m_methods == retInfo.m_methods);

    QVERIFY(!(retInfo == info));
    QVERIFY((retInfo ==updateInfo));

}

void TestDatabase::removeCredentialsTest()
{
    SignonIdentityInfo info;
    SignonIdentityInfo retInfo;
    quint32 id;

    //insert complete
    info.m_caption = QLatin1String("Caption");
    info.m_userName = QLatin1String("User");
    info.m_password = QLatin1String("Pass");
    info.m_realms = QStringList() << QLatin1String("Realm1.com") << QLatin1String("Realm2.com") << QLatin1String("Realm3.com") ;
    QMap<MethodName,MechanismsList> methods;
    QStringList mechs = QStringList() << QString::fromLatin1("Mech1") << QString::fromLatin1("Mech2") ;
    methods.insert(QLatin1String("Method1"), mechs);
    methods.insert(QLatin1String("Method2"), mechs);
    methods.insert(QLatin1String("Method3"), QStringList());
    info.m_methods = methods;
    info.m_accessControlList = QStringList() << QLatin1String("AID::12345678") << QLatin1String("AID::87654321") << QLatin1String("test::property") ;

    id = m_db->insertCredentials(info, true);
    retInfo = m_db->credentials(id, true);
    QVERIFY(id != info.m_id);
    info.m_id = id;
    QVERIFY(retInfo == info);

    QVERIFY(m_db->removeCredentials(id));

    retInfo = m_db->credentials(id, true);
    QVERIFY(!(retInfo == info));
    QVERIFY(retInfo.m_id == 0);
}

void TestDatabase::checkPasswordTest()
{
    SignonIdentityInfo info;
    quint32 id;

    //insert complete
    info.m_caption = QLatin1String("Caption");
    info.m_userName = QLatin1String("User");
    info.m_password = QLatin1String("Pass");
    info.m_realms = QStringList() << QLatin1String("Realm1.com") << QLatin1String("Realm2.com") << QLatin1String("Realm3.com") ;
    QMap<MethodName,MechanismsList> methods;
    QStringList mechs = QStringList() << QString::fromLatin1("Mech1") << QString::fromLatin1("Mech2") ;
    methods.insert(QLatin1String("Method1"), mechs);
    methods.insert(QLatin1String("Method2"), mechs);
    methods.insert(QLatin1String("Method3"), QStringList());
    info.m_methods = methods;
    info.m_accessControlList = QStringList() << QLatin1String("AID::12345678") << QLatin1String("AID::87654321") << QLatin1String("test::property") ;

    id = m_db->insertCredentials(info, true);

    QVERIFY(m_db->checkPassword(id, info.m_userName, info.m_password));
    QVERIFY(!m_db->checkPassword(id, info.m_userName, QLatin1String("PassWd")));
    QVERIFY(!m_db->checkPassword(id, QLatin1String("User2"), info.m_password));
}

void TestDatabase::credentialsTest()
{
}

void TestDatabase::methodsTest()
{
    SignonIdentityInfo info;
    quint32 id;

    //insert complete
    info.m_caption = QLatin1String("Caption");
    info.m_userName = QLatin1String("User");
    info.m_password = QLatin1String("Pass");
    info.m_realms = QStringList() << QLatin1String("Realm1.com") << QLatin1String("Realm2.com") << QLatin1String("Realm3.com") ;
    QMap<MethodName,MechanismsList> methods;
    QStringList mechs = QStringList() << QString::fromLatin1("Mech1") << QString::fromLatin1("Mech2") ;
    methods.insert(QLatin1String("Method1"), mechs);
    methods.insert(QLatin1String("Method2"), mechs);
    methods.insert(QLatin1String("Method3"), QStringList());
    info.m_methods = methods;
    info.m_accessControlList = QStringList() << QLatin1String("AID::12345678") << QLatin1String("AID::87654321") << QLatin1String("test::property") ;

    id = m_db->insertCredentials(info, true);

    QStringList meths = m_db->methods(id);
    QVERIFY(meths.contains(QLatin1String("Method1")));
    QVERIFY(meths.contains(QLatin1String("Method2")));
    QVERIFY(meths.contains(QLatin1String("Method3")));
    QVERIFY(meths.count() == 3);
}

void TestDatabase::clearTest()
{
    m_db->clear();
    QSqlQuery query = m_db->exec(QLatin1String("SELECT * FROM credentials"));
    QVERIFY(!query.next());
}

void TestDatabase::accessControlListTest()
{
    SignonIdentityInfo info;
    quint32 id;

    //insert complete
    info.m_caption = QLatin1String("Caption");
    info.m_userName = QLatin1String("User");
    info.m_password = QLatin1String("Pass");
    info.m_realms = QStringList() << QLatin1String("Realm1.com") << QLatin1String("Realm2.com") << QLatin1String("Realm3.com") ;
    QMap<MethodName,MechanismsList> methods;
    QStringList mechs = QStringList() << QString::fromLatin1("Mech1") << QString::fromLatin1("Mech2") ;
    methods.insert(QLatin1String("Method1"), mechs);
    methods.insert(QLatin1String("Method2"), mechs);
    methods.insert(QLatin1String("Method3"), QStringList());
    info.m_methods = methods;
    info.m_accessControlList = QStringList() << QLatin1String("AID::12345678") << QLatin1String("AID::87654321") << QLatin1String("test::property") ;

    id = m_db->insertCredentials(info, true);

    QStringList acl = m_db->accessControlList(id);
    qDebug() << acl;
    QVERIFY(acl == info.m_accessControlList);
}

void TestDatabase::credentialsOwnerSecurityTokenTest()
{
    SignonIdentityInfo info;
    quint32 id;

    //insert complete
    info.m_caption = QLatin1String("Caption");
    info.m_userName = QLatin1String("User");
    info.m_password = QLatin1String("Pass");
    info.m_realms = QStringList() << QLatin1String("Realm1.com") << QLatin1String("Realm2.com") << QLatin1String("Realm3.com") ;
    QMap<MethodName,MechanismsList> methods;
    QStringList mechs = QStringList() << QString::fromLatin1("Mech1") << QString::fromLatin1("Mech2") ;
    methods.insert(QLatin1String("Method1"), mechs);
    methods.insert(QLatin1String("Method2"), mechs);
    methods.insert(QLatin1String("Method3"), QStringList());
    info.m_methods = methods;
    info.m_accessControlList = QStringList() << QLatin1String("AID::12345678") << QLatin1String("AID::87654321") << QLatin1String("test::property") ;

    id = m_db->insertCredentials(info, true);

    QString token = m_db->credentialsOwnerSecurityToken(id);
    qDebug() << token;
    QVERIFY(token == QLatin1String("AID::12345678"));
}

void TestDatabase::runAllTests()
{
    initTestCase();

    init();
    sqlDBConfigurationTest();
    cleanup();

    init();
    createTableStructureTest();
    cleanup();

    init();
    insertMethodsTest();
    cleanup();

    init();
    removeMethodsTest();
    cleanup();

    init();
    insertListTest();
    cleanup();

    init();
    removeListTest();
    cleanup();

    init();
    queryListTest();
    cleanup();

    init();
    insertCredentialsTest();
    cleanup();

    init();
    updateCredentialsTest();
    cleanup();

    init();
    removeCredentialsTest();
    cleanup();

    init();
    checkPasswordTest();
    cleanup();

    init();
    credentialsTest();
    cleanup();

    init();
    methodsTest();
    cleanup();

    init();
    clearTest();
    cleanup();

    init();
    accessControlListTest();
    cleanup();

    init();
    credentialsOwnerSecurityTokenTest();
    cleanup();

    cleanupTestCase();
}

#if !defined(SSO_CI_TESTMANAGEMENT)
    QTEST_MAIN(TestDatabase)
#endif
