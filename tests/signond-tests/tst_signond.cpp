/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of signond
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

#include <QDebug>
#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <libqtdbusmock/DBusMock.h>
#include <signal.h>
#include <sys/types.h>

#include "signond/signoncommon.h"

#include "fake_signonui.h"

namespace QTest {
template<>
char *toString(const QSet<QString> &set)
{
    QByteArray ba = "QSet<QString>(";
    QStringList list = set.toList();
    ba += list.join(", ");
    ba += ")";
    return qstrdup(ba.data());
}
} // QTest namespace

using namespace QtDBusMock;

class SignondTest: public QObject
{
    Q_OBJECT

public:
    SignondTest();

private Q_SLOTS:
    void initTestCase();
    void cleanup();
    void testStart();
    void testQueryMethods();
    void testQueryMechanisms_data();
    void testQueryMechanisms();
    void testIdentityCreation();
    void testIdentityRemoval();
    void testIdentityReferences();
    void testAuthSessionMechanisms_data();
    void testAuthSessionMechanisms();
    void testAuthSessionProcess();
    void testAuthSessionProcessUi();

private:
    void setupEnvironment();
    bool signondIsRunning();
    bool killSignond();
    void clearBaseDir();
    const QDBusConnection &connection() { return m_dbus.sessionConnection(); }
    QDBusMessage methodCall(const QString &path, const QString &interface,
                            const QString &method) {
        return QDBusMessage::createMethodCall(SIGNOND_SERVICE, path,
                                              interface, method);
    }
    bool replyIsValid(const QDBusMessage &reply);
    QString createIdentity(const QVariantMap &data, uint *id = 0);

private:
    QTemporaryDir m_baseDir;
    QtDBusTest::DBusTestRunner m_dbus;
    QtDBusMock::DBusMock m_mock;
    FakeSignOnUi m_signonUi;
};

static bool mapIsSuperset(const QVariantMap &superSet, const QVariantMap &set)
{
    QMapIterator<QString, QVariant> it(set);
    while (it.hasNext()) {
        it.next();
        if (!superSet.contains(it.key())) {
            qDebug() << "Missing key" << it.key();
            return false;
        }
        if (superSet.value(it.key()) != it.value()) {
            qDebug() << it.key() << "is" << superSet.value(it.key()) << " expecting " << it.value();
            return false;
        }
    }

    return true;
}

SignondTest::SignondTest():
    QObject(0),
    m_dbus((setupEnvironment(), TEST_DBUS_CONFIG_FILE)),
    m_mock(m_dbus),
    m_signonUi(&m_mock)
{
    DBusMock::registerMetaTypes();
}

void SignondTest::setupEnvironment()
{
    QVERIFY(m_baseDir.isValid());
    QByteArray baseDirPath = m_baseDir.path().toUtf8();
    QDir baseDir(m_baseDir.path());

    qunsetenv("XDG_DATA_DIR");
    qputenv("BUILDDIR", BUILDDIR);
    qputenv("HOME", baseDirPath);
    qputenv("XDG_RUNTIME_DIR", baseDirPath + "/runtime-dir");
    baseDir.mkpath("runtime-dir");
    qputenv("SSO_STORAGE_PATH", baseDirPath);
    qputenv("SSO_USE_PEER_BUS", "0");
    qputenv("SSO_LOGGING_LEVEL", "2");
    qputenv("SSO_PLUGINS_DIR", BUILDDIR "/src/plugins/test");
    QByteArray ldLibraryPath = qgetenv("LD_LIBRARY_PATH");
    qputenv("LD_LIBRARY_PATH",
            BUILDDIR "/lib/plugins:"
            BUILDDIR "/lib/plugins/signon-plugins-common:"
            BUILDDIR "/lib/signond/SignOn:" +
            ldLibraryPath);
    QByteArray execPath = qgetenv("PATH");
    qputenv("PATH",
            BUILDDIR "/src/remotepluginprocess:" +
            execPath);

    /* Make sure we accidentally don't talk to the developer's signond running
     * in the session bus */
    qunsetenv("DBUS_SESSION_BUS_ADDRESS");
}

bool SignondTest::replyIsValid(const QDBusMessage &msg)
{
    if (msg.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "Error name:" << msg.errorName();
        qDebug() << "Error text:" << msg.errorMessage();
    }
    return msg.type() == QDBusMessage::ReplyMessage;
}

bool SignondTest::signondIsRunning()
{
    return connection().
        interface()->isServiceRegistered(SIGNOND_SERVICE).value();
}

bool SignondTest::killSignond()
{
    uint pid = connection().interface()->servicePid(SIGNOND_SERVICE).value();
    if (pid == 0) return true;
    return kill(pid, SIGTERM) == 0 || errno == ESRCH;
}

void SignondTest::clearBaseDir()
{
    QDir baseDir(m_baseDir.path());
    baseDir.removeRecursively();
    baseDir.mkpath(".");
}

QString SignondTest::createIdentity(const QVariantMap &data, uint *id)
{
    QDBusMessage msg = methodCall(SIGNOND_DAEMON_OBJECTPATH,
                                  SIGNOND_DAEMON_INTERFACE,
                                  "registerNewIdentity");
    QDBusMessage reply = connection().call(msg);
    if (!replyIsValid(reply)) return QString();

    QString objectPath =
        reply.arguments()[0].value<QDBusObjectPath>().path();

    msg = methodCall(objectPath, SIGNOND_IDENTITY_INTERFACE, "store");
    msg << data;
    reply = connection().call(msg);
    if (!replyIsValid(reply)) return QString();

    if (id) {
        *id = reply.arguments()[0].toUInt();
    }
    return objectPath;
}

void SignondTest::initTestCase()
{
    m_dbus.startServices();
}

void SignondTest::cleanup()
{
    if (QTest::currentTestFailed()) {
        m_baseDir.setAutoRemove(false);
        qDebug() << "Base dir:" << m_baseDir.path();
    }
}

void SignondTest::testStart()
{
    QVERIFY(killSignond());
    QTRY_VERIFY(!signondIsRunning());

    QDBusMessage reply =
        connection().interface()->call("StartServiceByName",
                                       SIGNOND_SERVICE, uint(0));
    QVERIFY(replyIsValid(reply));
    QTRY_VERIFY(signondIsRunning());
}

void SignondTest::testQueryMethods()
{
    QDBusMessage msg = methodCall(SIGNOND_DAEMON_OBJECTPATH,
                                  SIGNOND_DAEMON_INTERFACE,
                                  "queryMethods");
    QDBusMessage reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));
    QCOMPARE(reply.arguments().count(), 1);
    QStringList methods = reply.arguments()[0].toStringList();
    QStringList expectedMethods { "ssotest", "ssotest2" };
    QCOMPARE(methods.toSet(), expectedMethods.toSet());
}

void SignondTest::testQueryMechanisms_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QStringList>("expectedMechanisms");

    QTest::newRow("ssotest") <<
        "ssotest" << QStringList { "mech1", "mech2", "mech3", "BLOB" };
    QTest::newRow("ssotest2") <<
        "ssotest2" << QStringList { "mech1", "mech2", "mech3" };
}

void SignondTest::testQueryMechanisms()
{
    QFETCH(QString, method);
    QFETCH(QStringList, expectedMechanisms);

    QDBusMessage msg = methodCall(SIGNOND_DAEMON_OBJECTPATH,
                                  SIGNOND_DAEMON_INTERFACE,
                                  "queryMechanisms");
    msg << method;
    QDBusMessage reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QCOMPARE(reply.arguments().count(), 1);
    QStringList mechanisms = reply.arguments()[0].toStringList();
    QCOMPARE(mechanisms.toSet(), expectedMechanisms.toSet());
}

void SignondTest::testIdentityCreation()
{
    QDBusMessage msg = methodCall(SIGNOND_DAEMON_OBJECTPATH,
                                  SIGNOND_DAEMON_INTERFACE,
                                  "registerNewIdentity");
    QDBusMessage reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QCOMPARE(reply.arguments().count(), 1);

    QString objectPath =
        reply.arguments()[0].value<QDBusObjectPath>().path();
    QVERIFY(objectPath.startsWith('/'));

    QVariantMap identityData {
        { SIGNOND_IDENTITY_INFO_USERNAME, "John" },
        { SIGNOND_IDENTITY_INFO_CAPTION, "John's account" },
        { SIGNOND_IDENTITY_INFO_ACL, QStringList { "*" } },
    };
    msg = methodCall(objectPath, SIGNOND_IDENTITY_INTERFACE, "store");
    msg << identityData;
    reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    uint id = reply.arguments()[0].toUInt();
    QVERIFY(id != 0);

    /* Read the current identity info */
    msg = methodCall(objectPath, SIGNOND_IDENTITY_INTERFACE, "getInfo");
    reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QVariantMap storedData = QDBusReply<QVariantMap>(reply).value();
    QVERIFY(mapIsSuperset(storedData, identityData));

    /* Reload the identity */
    msg = methodCall(SIGNOND_DAEMON_OBJECTPATH, SIGNOND_DAEMON_INTERFACE,
                     "getIdentity");
    msg << id;
    reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QCOMPARE(reply.arguments().count(), 2);
    objectPath = reply.arguments()[0].value<QDBusObjectPath>().path();
    QVERIFY(objectPath.startsWith('/'));
    storedData =
        qdbus_cast<QVariantMap>(reply.arguments()[1].value<QDBusArgument>());
    QVERIFY(mapIsSuperset(storedData, identityData));

    /* Query identities */
    msg = methodCall(SIGNOND_DAEMON_OBJECTPATH, SIGNOND_DAEMON_INTERFACE,
                     "queryIdentities");
    msg << QVariantMap();
    reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QCOMPARE(reply.arguments().count(), 1);
    QList<QVariantMap> identities =
        qdbus_cast<QList<QVariantMap>>(reply.arguments()[0].value<QDBusArgument>());
    QCOMPARE(identities.count(), 1);
    storedData = identities[0];
    QVERIFY(mapIsSuperset(storedData, identityData));
}

void SignondTest::testIdentityRemoval()
{
    m_signonUi.mockedService().ClearCalls().waitForFinished();

    QVariantMap identityData {
        { SIGNOND_IDENTITY_INFO_USERNAME, "John Deleteme" },
        { SIGNOND_IDENTITY_INFO_CAPTION, "John's account" },
        { SIGNOND_IDENTITY_INFO_ACL, QStringList { "*" } },
    };
    uint id;
    QString objectPath = createIdentity(identityData, &id);
    QVERIFY(objectPath.startsWith('/'));
    QVERIFY(id > 0);

    QDBusMessage msg =
        methodCall(objectPath, SIGNOND_IDENTITY_INTERFACE, "remove");
    QDBusMessage reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    /* Check that signonui has been asked to remove its data */
    QList<MethodCall> calls =
        m_signonUi.mockedService().GetMethodCalls("removeIdentityData");
    QCOMPARE(calls.count(), 1);
    MethodCall call = calls.first();
    QCOMPARE(call.args().count(), 1);
    QCOMPARE(call.args().first().toUInt(), id);

    /* Read the current identity info */
    msg = methodCall(objectPath, SIGNOND_IDENTITY_INTERFACE, "getInfo");
    reply = connection().call(msg);
    QCOMPARE(reply.type(), QDBusMessage::ErrorMessage);

    /* Load the identity again */
    msg = methodCall(SIGNOND_DAEMON_OBJECTPATH, SIGNOND_DAEMON_INTERFACE,
                     "getIdentity");
    msg << id;
    reply = connection().call(msg);
    QCOMPARE(reply.type(), QDBusMessage::ErrorMessage);
}

void SignondTest::testIdentityReferences()
{
    QVariantMap identityData {
        { SIGNOND_IDENTITY_INFO_USERNAME, "John" },
        { SIGNOND_IDENTITY_INFO_CAPTION, "John's account" },
        { SIGNOND_IDENTITY_INFO_ACL, QStringList { "*" } },
    };
    QString objectPath = createIdentity(identityData);

    /* Read the current identity info */
    QDBusMessage msg =
        methodCall(objectPath, SIGNOND_IDENTITY_INTERFACE, "getInfo");
    QDBusMessage reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QVariantMap storedData = QDBusReply<QVariantMap>(reply).value();
    QCOMPARE(storedData.value(SIGNOND_IDENTITY_INFO_REFCOUNT).toInt(), 0);

    /* Add a reference */
    msg = methodCall(objectPath, SIGNOND_IDENTITY_INTERFACE, "addReference");
    msg << "hello";
    reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    /* Check new refcount */
    msg = methodCall(objectPath, SIGNOND_IDENTITY_INTERFACE, "getInfo");
    reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));
    storedData = QDBusReply<QVariantMap>(reply).value();
    QEXPECT_FAIL("", "refcount not updated: https://gitlab.com/accounts-sso/signond/issues/1", Continue);
    QCOMPARE(storedData.value(SIGNOND_IDENTITY_INFO_REFCOUNT).toInt(), 1);
}

void SignondTest::testAuthSessionMechanisms_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QStringList>("wantedMechanisms");
    QTest::addColumn<QStringList>("expectedMechanisms");

    QTest::newRow("ssotest - any") <<
        "ssotest" <<
        QStringList {} <<
        QStringList { "mech1", "mech2", "mech3", "BLOB" };
    QTest::newRow("ssotest - subset") <<
        "ssotest" <<
        QStringList { "BLOB", "mech1" } <<
        QStringList { "BLOB", "mech1" };
    QTest::newRow("ssotest2 - any") <<
        "ssotest2" <<
        QStringList {} <<
        QStringList { "mech1", "mech2", "mech3" };
    QTest::newRow("ssotest2 - mix") <<
        "ssotest2" <<
        QStringList { "BLOB", "mech1" } <<
        QStringList { "mech1" };
}

void SignondTest::testAuthSessionMechanisms()
{
    QFETCH(QString, method);
    QFETCH(QStringList, wantedMechanisms);
    QFETCH(QStringList, expectedMechanisms);

    QDBusMessage msg = methodCall(SIGNOND_DAEMON_OBJECTPATH,
                                  SIGNOND_DAEMON_INTERFACE,
                                  "getAuthSessionObjectPath");
    msg << uint(0);
    msg << method;
    QDBusMessage reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QCOMPARE(reply.arguments().count(), 1);

    QString objectPath = reply.arguments()[0].toString();
    QVERIFY(objectPath.startsWith('/'));

    /* Check the available mechanisms */
    msg = methodCall(objectPath, SIGNOND_AUTH_SESSION_INTERFACE,
                     "queryAvailableMechanisms");
    msg << wantedMechanisms;
    reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QStringList mechanisms = QDBusReply<QStringList>(reply).value();
    QCOMPARE(mechanisms.toSet(), expectedMechanisms.toSet());
}

void SignondTest::testAuthSessionProcess()
{
    QDBusMessage msg = methodCall(SIGNOND_DAEMON_OBJECTPATH,
                                  SIGNOND_DAEMON_INTERFACE,
                                  "getAuthSessionObjectPath");
    msg << uint(0);
    msg << QString("ssotest");
    QDBusMessage reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QCOMPARE(reply.arguments().count(), 1);

    QString objectPath = reply.arguments()[0].toString();
    QVERIFY(objectPath.startsWith('/'));

    QVariantMap sessionData {
        { "Some key", "its value" },
        { "height", 123 },
    };
    /* Read the current identity info */
    msg = methodCall(objectPath, SIGNOND_AUTH_SESSION_INTERFACE, "process");
    msg << sessionData;
    msg << QString("mech1");
    reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QVariantMap response = QDBusReply<QVariantMap>(reply).value();
    QVariantMap expectedResponse = sessionData;
    expectedResponse["Realm"] = "testRealm_after_test";
    QCOMPARE(response, expectedResponse);
}

void SignondTest::testAuthSessionProcessUi()
{
    QDBusMessage msg = methodCall(SIGNOND_DAEMON_OBJECTPATH,
                                  SIGNOND_DAEMON_INTERFACE,
                                  "getAuthSessionObjectPath");
    msg << uint(0);
    msg << QString("ssotest");
    QDBusMessage reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));
    QString objectPath = reply.arguments()[0].toString();
    QVERIFY(objectPath.startsWith('/'));

    /* prepare SignOnUi */
    m_signonUi.mockedService().ClearCalls().waitForFinished();
    QVariantMap uiReply {
        { "data",
            QVariantMap {
                { "UserName", "the user" },
                { "Secret", "s3c'r3t" },
                { "QueryErrorCode", 0 },
            }
        },
    };
    m_signonUi.setNextReply(uiReply);

    /* Start the authentication, using a mechanism requiring UI interaction */
    QVariantMap sessionData {
        { "Some key", "its value" },
        { "height", 123 },
    };
    msg = methodCall(objectPath, SIGNOND_AUTH_SESSION_INTERFACE, "process");
    msg << sessionData;
    msg << QString("mech2");
    reply = connection().call(msg);
    QVERIFY(replyIsValid(reply));

    QVariantMap response = QDBusReply<QVariantMap>(reply).value();
    QVariantMap expectedResponse {
        { "UserName", "the user" },
    };
    QCOMPARE(response, expectedResponse);
}

QTEST_GUILESS_MAIN(SignondTest);

#include "tst_signond.moc"
