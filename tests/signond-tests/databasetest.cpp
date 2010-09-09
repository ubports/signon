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
    QVERIFY(conf.value(QLatin1String("Database Name")) == dbFile);
}

void TestDatabase::createTableStructureTest()
{
    QVERIFY(!m_db->hasTableStructure());
    m_db->createTableStructure();
    QMap<QString, QString> conf = m_db->sqlDBConfiguration();
    QVERIFY(!conf.value(QLatin1String("Tables")).isEmpty());
    QVERIFY(m_db->hasTableStructure());
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

    list = m_db->queryList(QString::fromLatin1(
            "SELECT realm FROM REALMS WHERE identity_id = 81"));
    QVERIFY(list.count() == 0);
}

void TestDatabase::insertMethodsTest()
{
    //test empty list
    QMap<QString, QStringList> methods;
    m_db->insertMethods(methods);
    QStringList list = m_db->queryList(QString::fromLatin1(
            "SELECT method FROM METHODS"));
    QVERIFY(list.count() == 0);
    list = m_db->queryList(QString::fromLatin1(
            "SELECT mechanism FROM MECHANISMS"));
    QVERIFY(list.count() == 0);

    //test real list
    QStringList mechs = QStringList() << QLatin1String("M1")<< QLatin1String("M2");
    methods.insert(QLatin1String("Test"), mechs);
    methods.insert(QLatin1String("Test2"), mechs);
    m_db->insertMethods(methods);
    list = m_db->queryList(QString::fromLatin1(
            "SELECT method FROM METHODS"));
    qDebug() << list;
    QVERIFY(list.contains(QLatin1String("Test")));
    QVERIFY(list.contains(QLatin1String("Test2")));
    QVERIFY(list.count() == 2);

    list = m_db->queryList(QString::fromLatin1(
            "SELECT mechanism FROM MECHANISMS"));
    qDebug() << list;
    QVERIFY(list.contains(QLatin1String("M1")));
    QVERIFY(list.contains(QLatin1String("M2")));
    QVERIFY(list.count() == 2);
}

void TestDatabase::cleanUpTablesTest()
{
    QMap<QString, QStringList> methods;
    m_db->cleanUpTables();
    QStringList mechs = QStringList() << QLatin1String("M1")<< QLatin1String("M2");
    methods.insert(QLatin1String("Test"), mechs);

    QString queryStr = QString::fromLatin1(
                        "INSERT INTO TOKENS (token) "
                        "SELECT '%1' WHERE NOT EXISTS "
                        "(SELECT id FROM TOKENS WHERE token = '%1')")
                        .arg(QLatin1String("token"));
    m_db->exec(queryStr);

    m_db->cleanUpTables();
    QStringList list = m_db->queryList(QString::fromLatin1(
            "SELECT method FROM METHODS"));
    QVERIFY(list.count() == 0);
    list = m_db->queryList(QString::fromLatin1(
            "SELECT mechanism FROM MECHANISMS"));
    QVERIFY(list.count() == 0);
    list = m_db->queryList(QString::fromLatin1(
            "SELECT token FROM TOKENS"));
    QVERIFY(list.count() == 0);
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
    m_db->clear();
    QMap<QString, QString> filter;
    QList<SignonIdentityInfo> creds = m_db->credentials(filter);
    QVERIFY(creds.count() == 0);
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
    info.m_validated = true;
    info.m_refCount = 5;

    id = m_db->insertCredentials(info, true);
    creds = m_db->credentials(filter);
    QVERIFY(creds.count() == 1);
    id = m_db->insertCredentials(info, true);
    creds = m_db->credentials(filter);
    QVERIFY(creds.count() == 2);
    foreach(SignonIdentityInfo info, creds) {
        qDebug() << info.m_id << info.m_caption;
    }
    //TODO check filtering when implemented
}

void TestDatabase::insertCredentialsTest()
{
    SignonIdentityInfo info;
    SignonIdentityInfo info2;
   SignonIdentityInfo retInfo;
    quint32 id;

    //insert empty
    id = m_db->insertCredentials(info, false);
    retInfo = m_db->credentials(id, false);
    QVERIFY(id != info.m_id);
    info.m_id = id;
    QVERIFY(retInfo == info);

    //insert with empty acl
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
    info.m_validated = true;
    info.m_refCount = 5;

    id = m_db->insertCredentials(info, false);
    retInfo = m_db->credentials(id, false);
    QVERIFY(id != info.m_id);
    info.m_id = id;
    retInfo.m_password = info.m_password;
    QVERIFY(retInfo == info);

    //insert with empty methods
    info2.m_caption = QLatin1String("Caption");
    info2.m_userName = QLatin1String("User");
    info2.m_password = QLatin1String("Pass");
    info2.m_realms = QStringList() << QLatin1String("Realm1.com") << QLatin1String("Realm2.com") << QLatin1String("Realm3.com") ;
    QMap<MethodName,MechanismsList> methods2;
    info2.m_methods = methods2;

    id = m_db->insertCredentials(info2, false);
    retInfo = m_db->credentials(id, false);
    QVERIFY(id != info2.m_id);
    info2.m_id = id;
    retInfo.m_password = info2.m_password;
    QVERIFY(retInfo == info2);

    //insert complete
    info.m_caption = QLatin1String("Caption");
    info.m_userName = QLatin1String("User");
    info.m_password = QLatin1String("Pass");
    info.m_realms = QStringList() << QLatin1String("Realm1.com") << QLatin1String("Realm2.com") << QLatin1String("Realm3.com") ;
    mechs = QStringList() << QString::fromLatin1("Mech1") << QString::fromLatin1("Mech2") ;
    methods.insert(QLatin1String("Method1"), mechs);
    methods.insert(QLatin1String("Method2"), mechs);
    methods.insert(QLatin1String("Method3"), QStringList());
    info.m_methods = methods;
    info.m_accessControlList = QStringList() << QLatin1String("AID::12345678") << QLatin1String("AID::87654321") << QLatin1String("test::property") ;
    info.m_type = 3;
    info.m_validated = true;
    info.m_refCount = 5;

    id = m_db->insertCredentials(info, true);
    retInfo = m_db->credentials(id, false);
    QVERIFY(id != info.m_id);
    info.m_id = id;
    QVERIFY(info.m_password != retInfo.m_password);
    retInfo.m_password = info.m_password;

qDebug() << info.m_methods;
qDebug() << retInfo.m_methods;
QVERIFY (info.m_methods == retInfo.m_methods);

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
    info.m_validated = true;
    info.m_refCount = 5;

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
    updateInfo.m_accessControlList = QStringList() << QLatin1String("UID::12345678") << QLatin1String("UID::87654321") << QLatin1String("test::property") ;
    updateInfo.m_id = id;
    updateInfo.m_type = 2;
    updateInfo.m_validated = false;
    updateInfo.m_refCount = 4;

    QVERIFY(m_db->updateCredentials(updateInfo, true));

    retInfo = m_db->credentials(id, true);

QVERIFY (updateInfo.m_id == retInfo.m_id);
QVERIFY (updateInfo.m_userName == retInfo.m_userName);
QVERIFY (updateInfo.m_password == retInfo.m_password);
QVERIFY (updateInfo.m_caption == retInfo.m_caption);

    QVERIFY(!(retInfo == info));
    QVERIFY((retInfo == updateInfo));
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
    info.m_validated = true;
    info.m_refCount = 5;

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

void TestDatabase::clearTest()
{
    m_db->clear();
    QSqlQuery query = m_db->exec(QLatin1String("SELECT * FROM credentials"));
    QVERIFY(!query.first());
}

void TestDatabase::dataTest()
{
    SignonIdentityInfo info;
    quint32 id;
    QString method = QLatin1String("Method1");
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

    bool ret = m_db->storeData(id, method, QVariantMap());
    QVERIFY(ret);
    QVariantMap result = m_db->loadData(id, method);
    QVERIFY(result.isEmpty());
    QVariantMap data;
    data.insert(QLatin1String("token"), QLatin1String("tokenval"));
    ret = m_db->storeData(id, method, data);
    QVERIFY(ret);
    result = m_db->loadData(id, method);
    qDebug() << result;
    QVERIFY(result == data);

    data.insert(QLatin1String("token"), QLatin1String("tokenvalupdated"));
    data.insert(QLatin1String("token2"), QLatin1String("tokenval2"));
    ret = m_db->storeData(id, method, data);
    QVERIFY(ret);
    result = m_db->loadData(id, method);
    QVERIFY(result == data);


    data.insert(QLatin1String("token"), QVariant());
    data.insert(QLatin1String("token2"), QVariant());
    ret = m_db->storeData(id, method, data);
    QVERIFY(ret);
    result = m_db->loadData(id, method);
    qDebug() << data;
    QVERIFY(result.isEmpty());


    ret = m_db->storeData(id, method, QVariantMap());
    QVERIFY(ret);
    result = m_db->loadData(id, method);
    qDebug() << data;
    QVERIFY(result.isEmpty());


    data.clear();
    for ( int i = 1000; i <1000+(SSO_MAX_TOKEN_STORAGE/10) +1 ; i++) {
        data.insert(QString::fromLatin1("t%1").arg(i), QLatin1String("12345"));
    }
    ret = m_db->storeData(id, method, data);
    QVERIFY(!ret);
    result = m_db->loadData(id, method);
    QVERIFY(result != data);


    data.clear();
    QVariantMap map;
    map.insert("key1",QLatin1String("string"));
    map.insert("key2",qint32(12));
    data.insert("key", map);
    ret = m_db->storeData(0, method, data);
    QVERIFY(ret);
    result = m_db->loadData(0, method);
    QVERIFY(result == data);

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
    queryListTest();
    cleanup();

    init();
    insertMethodsTest();
    cleanup();

    init();
    cleanUpTablesTest();
    cleanup();

    init();
    methodsTest();
    cleanup();

    init();
    checkPasswordTest();
    cleanup();

    init();
    credentialsTest();
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
    clearTest();
    cleanup();

    init();
    dataTest();
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
