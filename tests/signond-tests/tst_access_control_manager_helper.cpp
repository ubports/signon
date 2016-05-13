/*
 * This file is part of signon
 *
 * Copyright (C) 2014-2016 Canonical Ltd.
 *
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

#include <QByteArray>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QSignalSpy>
#include <QTest>

#include <SignOn/AbstractAccessControlManager>
#include "accesscontrolmanagerhelper.h"
#include "credentialsaccessmanager.h"
#include "credentialsdb.h"

using namespace SignOn;
using namespace SignonDaemonNS;

// mock AbstractAccessControlManager {
class AcmPlugin: public SignOn::AbstractAccessControlManager
{
    Q_OBJECT

public:
    AcmPlugin(QObject *parent = 0):
        SignOn::AbstractAccessControlManager(parent) {}
    ~AcmPlugin() {}

    bool isPeerAllowedToAccess(const QDBusConnection &peerConnection,
                               const QDBusMessage &peerMessage,
                               const QString &securityContext) {
        QStringList appPermissions =
            m_permissions.value(appIdOfPeer(peerConnection, peerMessage));
        return appPermissions.contains(securityContext);
    }

    QString appIdOfPeer(const QDBusConnection &peerConnection,
                        const QDBusMessage &peerMessage) {
        Q_UNUSED(peerConnection);
        return peerMessage.service();
    }

    QString keychainWidgetAppId() { return m_keychainWidgetAppId; }

    SignOn::AccessReply *handleRequest(const SignOn::AccessRequest &request) {
        Q_UNUSED(request);
        return 0;
    }

private:
    friend class AccessControlManagerHelperTest;
    QMap<QString,QStringList> m_permissions;
    QString m_keychainWidgetAppId;
};
// } mock AbstractAccessControlManager

class AccessControlManagerHelperTest: public QObject
{
    Q_OBJECT

public:
    AccessControlManagerHelperTest();

private Q_SLOTS:
    void init();
    void testOwnership_data();
    void testOwnership();
    void testIdentityAccess_data();
    void testIdentityAccess();

public:
    static AccessControlManagerHelperTest *instance() { return m_instance; }
    SignonDaemonNS::CredentialsDB *credentialsDB() { return &m_db; }

private:
    void setDbOwners(const QStringList &owners) {
        if (owners.contains("db-error")) {
            m_dbOwners = QStringList();
            m_dbLastError = CredentialsDBError("DB error!",
                                               CredentialsDBError::ConnectionError);
        } else {
            m_dbOwners = owners;
        }
    }

    void setDbAcl(const QStringList &acl) {
        if (acl.contains("db-error")) {
            m_dbAcl = QStringList();
            m_dbLastError = CredentialsDBError("DB error!",
                                               CredentialsDBError::ConnectionError);
        } else {
            m_dbAcl = acl;
        }
    }

private:
    friend class SignonDaemonNS::CredentialsDB;
    static AccessControlManagerHelperTest *m_instance;
    AcmPlugin m_acmPlugin;
    SignonDaemonNS::CredentialsDB m_db;
    SignOn::CredentialsDBError m_dbLastError;
    QStringList m_dbAcl;
    QStringList m_dbOwners;
    QDBusConnection m_conn;
};

AccessControlManagerHelperTest *AccessControlManagerHelperTest::m_instance = 0;

namespace SignonDaemonNS {
// mock CredentialsDB {
CredentialsDB::CredentialsDB(const QString &metaDataDbName,
                             SignOn::AbstractSecretsStorage *secretsStorage):
    QObject()
{
    Q_UNUSED(metaDataDbName);
    Q_UNUSED(secretsStorage);
}

CredentialsDB::~CredentialsDB()
{
}

SignOn::CredentialsDBError CredentialsDB::lastError() const
{
    return AccessControlManagerHelperTest::instance()->m_dbLastError;
}

QStringList CredentialsDB::accessControlList(const quint32 identityId)
{
    Q_UNUSED(identityId);
    return AccessControlManagerHelperTest::instance()->m_dbAcl;
}

QStringList CredentialsDB::ownerList(const quint32 identityId)
{
    Q_UNUSED(identityId);
    return AccessControlManagerHelperTest::instance()->m_dbOwners;
}
// } mock CredentialsDB

// mock CredentialsAccessManager {
CredentialsDB *CredentialsAccessManager::credentialsDB() const {
    return AccessControlManagerHelperTest::instance()->credentialsDB();
}
CredentialsAccessManager *CredentialsAccessManager::instance() {
    return 0;
}
} // namespace
// } mock CredentialsAccessManager

AccessControlManagerHelperTest::AccessControlManagerHelperTest():
    QObject(),
    m_db(QString(), 0),
    m_conn(QLatin1String("test-connection"))
{
    m_instance = this;
}

void AccessControlManagerHelperTest::init()
{
    m_dbOwners = QStringList();
    m_dbAcl = QStringList();
    m_dbLastError = CredentialsDBError();
}

void AccessControlManagerHelperTest::testOwnership_data()
{
    QTest::addColumn<QString>("peer");
    QTest::addColumn<QStringList>("ownerList");
    QTest::addColumn<int>("expectedOwnership");

    QTest::newRow("DB error") <<
        "tom" <<
        (QStringList() << "db-error") <<
        int(AccessControlManagerHelper::ApplicationIsNotOwner);

    QTest::newRow("empty") <<
        "tom" <<
        QStringList() <<
        int(AccessControlManagerHelper::IdentityDoesNotHaveOwner);

    QTest::newRow("is only owner") <<
        "tom" <<
        (QStringList() << "tom") <<
        int(AccessControlManagerHelper::ApplicationIsOwner);

    QTest::newRow("is co-owner") <<
        "tom" <<
        (QStringList() << "Bob" << "tom" << "harry") <<
        int(AccessControlManagerHelper::ApplicationIsOwner);
}

void AccessControlManagerHelperTest::testOwnership()
{
    QFETCH(QString, peer);
    QFETCH(QStringList, ownerList);
    QFETCH(int, expectedOwnership);

    setDbOwners(ownerList);

    m_acmPlugin.m_permissions["tom"] = QStringList() << "tom" << "Tom";

    /* forge a QDBusMessage */
    QDBusMessage msg =
        QDBusMessage::createMethodCall(peer, "/", "interface", "hi");

    SignonDaemonNS::AccessControlManagerHelper helper(&m_acmPlugin);

    AccessControlManagerHelper::IdentityOwnership ownership =
        helper.isPeerOwnerOfIdentity(m_conn, msg, 3);

    QCOMPARE(int(ownership), expectedOwnership);
}

void AccessControlManagerHelperTest::testIdentityAccess_data()
{
    QTest::addColumn<QString>("peer");
    QTest::addColumn<QStringList>("ownerList");
    QTest::addColumn<QStringList>("acl");
    QTest::addColumn<bool>("expectedIsAllowed");

    QTest::newRow("DB error") <<
        "tom" <<
        (QStringList() << "tom") <<
        (QStringList() << "db-error") <<
        false;

    QTest::newRow("is owner, ACL empty") <<
        "tom" <<
        (QStringList() << "tom") <<
        QStringList() <<
        true;

    QTest::newRow("is owner, not in ACL") <<
        "tom" <<
        (QStringList() << "tom") <<
        (QStringList() << "bob") <<
        true;

    QTest::newRow("is owner, in ACL") <<
        "tom" <<
        (QStringList() << "tom") <<
        (QStringList() << "bob" << "tom" << "harry") <<
        true;

    QTest::newRow("is owner, ACL=*") <<
        "tom" <<
        (QStringList() << "tom") <<
        (QStringList() << "*") <<
        true;

    QTest::newRow("not owner, ACL empty") <<
        "tom" <<
        (QStringList() << "bob") <<
        QStringList() <<
        false;

    QTest::newRow("not owner, not in ACL") <<
        "tom" <<
        (QStringList() << "bob") <<
        (QStringList() << "bob") <<
        false;

    QTest::newRow("not owner, in ACL") <<
        "tom" <<
        (QStringList() << "bob") <<
        (QStringList() << "bob" << "tom" << "harry") <<
        true;

    QTest::newRow("not owner, ACL=*") <<
        "tom" <<
        (QStringList() << "bob") <<
        (QStringList() << "*") <<
        true;
}

void AccessControlManagerHelperTest::testIdentityAccess()
{
    QFETCH(QString, peer);
    QFETCH(QStringList, ownerList);
    QFETCH(QStringList, acl);
    QFETCH(bool, expectedIsAllowed);

    setDbOwners(ownerList);
    setDbAcl(acl);

    m_acmPlugin.m_permissions["tom"] = QStringList() << "tom" << "Tom";

    /* forge a QDBusMessage */
    QDBusMessage msg =
        QDBusMessage::createMethodCall(peer, "/", "interface", "hi");

    SignonDaemonNS::AccessControlManagerHelper helper(&m_acmPlugin);

    bool isAllowed = helper.isPeerAllowedToUseIdentity(m_conn, msg, 3);

    QCOMPARE(isAllowed, expectedIsAllowed);
}

QTEST_MAIN(AccessControlManagerHelperTest)
#include "tst_access_control_manager_helper.moc"
