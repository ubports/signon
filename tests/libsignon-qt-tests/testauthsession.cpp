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

#ifndef TESTAUTHSESSION_CPP_
#define TESTAUTHSESSION_CPP_

#include "testauthsession.h"
#include "testthread.h"
#include "SignOn/identity.h"

#define SSO_TEST_CREATE_AUTH_SESSION(__session__, __method__) \
    do {                                                            \
        Identity *id = Identity::newIdentity(IdentityInfo(), this); \
        __session__ = id->createSession(QLatin1String(__method__)); \
    } while(0)


static AuthSession *g_currentSession = NULL;
static QStringList g_processReplyRealmsList;
static int g_bigStringSize = 50000;
static int g_bigStringReplySize = 0;

void TestAuthSession::initTestCase()
{
    qDebug() << "HI!";
}

void TestAuthSession::cleanupTestCase()
{
    qDebug() << "BYE!";
}

void TestAuthSession::queryMechanisms_existing_method()
{
    AuthSession *as;
    SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");

    QStringList wantedMechs;

    QSignalSpy spy(as, SIGNAL(mechanismsAvailable(const QStringList&)));
    QSignalSpy errorCounter(as, SIGNAL(error(const SignOn::Error &)));
    QEventLoop loop;

    QObject::connect(as, SIGNAL(mechanismsAvailable(const QStringList&)), &loop, SLOT(quit()));
    QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));
    QTimer::singleShot(10*1000, &loop, SLOT(quit()));

    as->queryAvailableMechanisms(wantedMechs);

    if(!errorCounter.count())
        loop.exec();

    errorCounter.clear();

    QCOMPARE(spy.count(), 1);
    QStringList result = spy.at(0).at(0).toStringList();
    QCOMPARE(result.size(), 4);

    wantedMechs += "mech1";

    QTimer::singleShot(10*1000, &loop, SLOT(quit()));

    as->queryAvailableMechanisms(wantedMechs);

    if(!errorCounter.count())
        loop.exec();

    errorCounter.clear();

    QCOMPARE(spy.count(), 2);
    result = spy.at(1).at(0).toStringList();
    QCOMPARE(result.size(), 1);

    wantedMechs += "mech2";

    QTimer::singleShot(10*1000, &loop, SLOT(quit()));

    as->queryAvailableMechanisms(wantedMechs);

    if(!errorCounter.count())
        loop.exec();

    errorCounter.clear();

    QCOMPARE(spy.count(), 3);
    result = spy.at(2).at(0).toStringList();
    QCOMPARE(result.size(), 2);

    wantedMechs = QStringList("fake");

    QTimer::singleShot(10*1000, &loop, SLOT(quit()));

    as->queryAvailableMechanisms(wantedMechs);

    if(!errorCounter.count())
        loop.exec();

    errorCounter.clear();

    QCOMPARE(spy.count(), 4);
    result = spy.at(3).at(0).toStringList();
    QCOMPARE(result.size(), 0);
 }

 void TestAuthSession::queryMechanisms_nonexisting_method()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "nonexisting");

     QStringList wantedMechs;

     QSignalSpy spy(as, SIGNAL(error(const SignOn::Error &)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(mechanismsAvailable(const QStringList&)), &loop, SLOT(quit()));
     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     as->queryAvailableMechanisms(wantedMechs);
     loop.exec();

     QCOMPARE(spy.count(), 1);
 }

 void TestAuthSession::process_with_new_identity()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");

     g_processReplyRealmsList.clear();
     connect(as, SIGNAL(response(const SignOn::SessionData &)), this, SLOT(response(const SignOn::SessionData &)));

     QSignalSpy spy(as, SIGNAL(response(const SignOn::SessionData &)));
     QSignalSpy errorCounter(as, SIGNAL(error(const SignOn::Error &)));
     QSignalSpy stateCounter(as, SIGNAL(stateChanged(AuthSession::AuthSessionState, const QString&)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(response(const SignOn::SessionData &)), &loop, SLOT(quit()));
     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     SessionData inData;

     inData.setSecret("testSecret");
     inData.setUserName("testUsername");

     as->process(inData, "mech1");
     if(!errorCounter.count())
         loop.exec();

     QCOMPARE(stateCounter.count(), 12);
     stateCounter.clear();
     errorCounter.clear();

     as->process(inData, "mech1");
     if(!errorCounter.count())
         loop.exec();

     QCOMPARE(stateCounter.count(), 12);
     stateCounter.clear();
     errorCounter.clear();

     as->process(inData, "mech1");
     if(!errorCounter.count())
         loop.exec();

     QCOMPARE(stateCounter.count(), 12);
     stateCounter.clear();
     errorCounter.clear();

     as->process(inData, "mech1");
     if(!errorCounter.count())
         loop.exec();

     QCOMPARE(stateCounter.count(), 12);
     stateCounter.clear();
     errorCounter.clear();

     QCOMPARE(spy.count(), 4);

     QVERIFY(g_processReplyRealmsList.at(0) == "testRealm_after_test");
     QVERIFY(g_processReplyRealmsList.at(1) == "testRealm_after_test");
     QVERIFY(g_processReplyRealmsList.at(2) == "testRealm_after_test");
     QVERIFY(g_processReplyRealmsList.at(3) == "testRealm_after_test");
 }

 void TestAuthSession::process_with_existing_identity()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");

     g_processReplyRealmsList.clear();
     connect(as, SIGNAL(response(const SignOn::SessionData &)), this, SLOT(response(const SignOn::SessionData &)));

     QSignalSpy errorCounter(as, SIGNAL(error(const SignOn::Error &)));
     QSignalSpy stateCounter(as, SIGNAL(stateChanged(AuthSession::AuthSessionState, const QString&)));
     QSignalSpy spy(as, SIGNAL(response(const SignOn::SessionData&)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(response(const SignOn::SessionData&)), &loop, SLOT(quit()));
     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     SessionData inData;

     inData.setSecret("testSecret");
     inData.setUserName("testUsername");

     as->process(inData, "mech1");
     if(!errorCounter.count())
         loop.exec();
     QCOMPARE(stateCounter.count(), 12);
     stateCounter.clear();
     errorCounter.clear();

     as->process(inData, "mech1");
     if(!errorCounter.count())
         loop.exec();
     QCOMPARE(stateCounter.count(), 12);
     stateCounter.clear();
     errorCounter.clear();

     as->process(inData, "mech1");
     if(!errorCounter.count())
         loop.exec();
     QCOMPARE(stateCounter.count(), 12);
     stateCounter.clear();
     errorCounter.clear();

     as->process(inData, "mech1");
     if(!errorCounter.count())
         loop.exec();
     QCOMPARE(stateCounter.count(), 12);
     stateCounter.clear();
     errorCounter.clear();

     QCOMPARE(spy.count(), 4);

     QVERIFY(g_processReplyRealmsList.at(0) == "testRealm_after_test");
     QVERIFY(g_processReplyRealmsList.at(1) == "testRealm_after_test");
     QVERIFY(g_processReplyRealmsList.at(2) == "testRealm_after_test");
     QVERIFY(g_processReplyRealmsList.at(3) == "testRealm_after_test");

 }

 void TestAuthSession::process_with_nonexisting_type()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "nonexisting");

     QSignalSpy spyResponse(as, SIGNAL(response(const SignOn::SessionData &)));
     QSignalSpy spyError(as, SIGNAL(error(const SignOn::Error &)));
     QSignalSpy stateCounter(as, SIGNAL(stateChanged(AuthSession::AuthSessionState, const QString&)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()),  Qt::QueuedConnection);
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     SessionData inData;

     inData.setSecret("testSecret");
     inData.setUserName("testUsername");

     as->process(inData, "mech1");
     loop.exec();

     as->process(inData, "mech1");
     loop.exec();

     as->process(inData, "mech1");
     loop.exec();

     as->process(inData, "mech1");
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 4);
     QCOMPARE(stateCounter.count(), 0);
 }


 void TestAuthSession::process_with_nonexisting_method()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");

     QSignalSpy spyResponse(as, SIGNAL(response(const SignOn::SessionData&)));
     QSignalSpy spyError(as, SIGNAL(error(const SignOn::Error &)));
     QSignalSpy stateCounter(as, SIGNAL(stateChanged(AuthSession::AuthSessionState, const QString&)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()),  Qt::QueuedConnection);
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     SessionData inData;

     inData.setSecret("testSecret");
     inData.setUserName("testUsername");

     as->process(inData, "nonexisting");
     loop.exec();

     as->process(inData, "nonexisting");
     loop.exec();

     as->process(inData, "nonexisting");
     loop.exec();

     as->process(inData, "nonexisting");
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 4);
     QCOMPARE(stateCounter.count(), 8);
 }


 void TestAuthSession::process_many_times_after_auth()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");

     QSignalSpy spyResponse(as, SIGNAL(response(const SignOn::SessionData&)));
     QSignalSpy spyError(as, SIGNAL(error(const SignOn::Error &)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));
     QObject::connect(as, SIGNAL(response(const SignOn::SessionData&)), &loop, SLOT(quit()));
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     SessionData inData;

     inData.setSecret("testSecret");
     inData.setUserName("testUsername");

     as->process(inData, "mech1");
     loop.exec();
     QCOMPARE(spyResponse.count(), 1);

     as->process(inData, "mech1");
     as->process(inData, "mech1");
     as->process(inData, "mech1");
     as->process(inData, "mech1");

     loop.exec();

     QCOMPARE(spyResponse.count(), 2);
     QCOMPARE(spyError.count(), 3);

     as->process(inData, "mech1");
     as->process(inData, "mech1");
     as->process(inData, "mech1");
     as->process(inData, "mech1");

     loop.exec();

     QCOMPARE(spyResponse.count(), 3);
     QCOMPARE(spyError.count(), 6);

     as->process(inData, "mech1");
     as->process(inData, "mech1");
     as->process(inData, "mech1");
     as->process(inData, "mech1");

     loop.exec();

     QCOMPARE(spyResponse.count(), 4);
     QCOMPARE(spyError.count(), 9);
 }

 void TestAuthSession::process_many_times_before_auth()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");

     QSignalSpy spyResponse(as, SIGNAL(response(const SignOn::SessionData&)));
     QSignalSpy spyError(as, SIGNAL(error(const SignOn::Error &)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(response(const SignOn::SessionData&)), &loop, SLOT(quit()));
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     SessionData inData;

     inData.setSecret("testSecret");
     inData.setUserName("testUsername");

     as->process(inData, "mech1");
     as->process(inData, "mech1");
     as->process(inData, "mech1");
     as->process(inData, "mech1");

     loop.exec();

     QCOMPARE(spyError.count(), 3);
     QCOMPARE(spyResponse.count(), 1);
 }


 void TestAuthSession::process_with_big_session_data()
 {
     QSKIP("This test requires fix", SkipSingle); //TODO once bug Bug#222200 is fixed, this test case can be enabled
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");

     QSignalSpy spyResponse(as, SIGNAL(response(const SignOn::SessionData&)));
     QSignalSpy spyError(as, SIGNAL(error(const SignOn::Error &)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(response(const SignOn::SessionData&)),
                      this, SLOT(response(const SignOn::SessionData&)));
     QObject::connect(as, SIGNAL(response(const SignOn::SessionData&)), &loop, SLOT(quit()));
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     SessionData inData;

     inData.setSecret("testSecret");

     QString bigString;
     bigString.fill(QChar('A'), g_bigStringSize);
     inData.setCaption(bigString);

     as->process(inData, "BLOB");

     loop.exec();

     QCOMPARE(spyError.count(), 0);
     QCOMPARE(spyResponse.count(), 1);
     QCOMPARE(g_bigStringReplySize, g_bigStringSize);
 }

 void TestAuthSession::cancel_immidiately()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");

     QSignalSpy spyResponse(as, SIGNAL(response(const SignOn::SessionData&)));
     QSignalSpy spyError(as, SIGNAL(error(const SignOn::Error &)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));
     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()),  Qt::QueuedConnection);
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     SessionData inData;

     inData.setSecret("testSecret");
     inData.setUserName("testUsername");

     as->process(inData, "mech1");
     as->cancel();
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 1);

     as->process(inData, "mech1");
     as->cancel();
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 2);

     as->process(inData, "mech1");
     as->cancel();
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 3);

     as->process(inData, "mech1");
     as->cancel();
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 4);
 }

 void TestAuthSession::cancel_with_delay()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");
     g_currentSession = as;

     QSignalSpy spyResponse(as, SIGNAL(response(const SignOn::SessionData&)));
     QSignalSpy spyError(as, SIGNAL(error(const SignOn::Error &)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));
     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()),  Qt::DirectConnection);
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     SessionData inData;

     inData.setSecret("testSecret");
     inData.setUserName("testUsername");

     as->process(inData, "mech1");
     QTimer::singleShot(0.1*1000, this, SLOT(cancel()));
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 1);

     as->process(inData, "mech1");
     QTimer::singleShot(0.1*1000, this, SLOT(cancel()));
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 2);

     as->process(inData, "mech1");
     QTimer::singleShot(0.1*1000, this, SLOT(cancel()));
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 3);

     as->process(inData, "mech1");
     QTimer::singleShot(0.1*1000, this, SLOT(cancel()));
     loop.exec();

     QCOMPARE(spyResponse.count(), 0);
     QCOMPARE(spyError.count(), 4);
 }

 void TestAuthSession::cancel_without_process()
 {
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");
     g_currentSession = as;

     QSignalSpy spyError(as, SIGNAL(error(const SignOn::Error &)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()),  Qt::QueuedConnection);
     QTimer::singleShot(10*1000, &loop, SLOT(quit()));

     QTimer::singleShot(1*1000, &loop, SLOT(quit()));
     as->cancel();
     loop.exec();

     QCOMPARE(spyError.count(), 0);

     QTimer::singleShot(1*1000, &loop, SLOT(quit()));
     QTimer::singleShot(0.1*1000, this, SLOT(cancel()));
     loop.exec();

     QCOMPARE(spyError.count(), 0);

     QTimer::singleShot(0.1*1000, this, SLOT(cancel()));
     QTimer::singleShot(1*1000, &loop, SLOT(quit()));
     loop.exec();

     QCOMPARE(spyError.count(), 0);

     SessionData inData;

     inData.setSecret("testSecret");
     inData.setUserName("testUsername");

     as->process(inData, "mech1");
     as->cancel();
     as->cancel();
     as->cancel();
     loop.exec();

     QCOMPARE(spyError.count(), 1);
 }

 void TestAuthSession::handle_destroyed_signal()
 {
     QSKIP("testing in sb", SkipSingle);
     AuthSession *as;
     SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest");
     g_currentSession = as;

     QSignalSpy spy(as, SIGNAL(mechanismsAvailable(const QStringList&)));
     QSignalSpy errorCounter(as, SIGNAL(error(const SignOn::Error &)));
     QEventLoop loop;

     QObject::connect(as, SIGNAL(mechanismsAvailable(const QStringList&)), &loop, SLOT(quit()));
     QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));

     /*
      * 5 minutes + 10 seconds
      * */
     QTimer::singleShot(5 * 62 *1000, &loop, SLOT(quit()));
     loop.exec();

     AuthSession *as2;
     SSO_TEST_CREATE_AUTH_SESSION(as2, "ssotest");

     QTimer::singleShot(5 * 1000, &loop, SLOT(quit()));
     loop.exec();

     QStringList wantedMechs;
     as->queryAvailableMechanisms(wantedMechs);

     if(!errorCounter.count())
         loop.exec();

     QCOMPARE(spy.count(), 1);
     QStringList result = spy.at(0).at(0).toStringList();
     QCOMPARE(result.size(), 4);
 }

 void TestAuthSession::multi_thread_test()
 {
     //execute a SignOn call in a separate thread
     TestThread thread;
     thread.start();
     thread.wait(g_testThreadTimeout + 1000);

     //do the same in this thread - this test succeeds if the
     //following succeeds
     process_with_new_identity();
 }

 void TestAuthSession::cancel()
 {
     g_currentSession->cancel();
 }

 void TestAuthSession::response(const SignOn::SessionData &data)
 {
     g_processReplyRealmsList << data.Realm();
     g_bigStringReplySize = data.Caption().size();
 }


 #ifdef SSOUI_TESTS_ENABLED

  void TestAuthSession::processUi_with_existing_identity()
  {
       AuthSession *as;
       SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest2");

       QSignalSpy errorCounter(as, SIGNAL(error(const SignOn::Error &)));
       QSignalSpy stateCounter(as, SIGNAL(stateChanged(AuthSession::AuthSessionState, const QString&)));
       QSignalSpy spy(as, SIGNAL(response(const SessionData&)));
       QEventLoop loop;

       QObject::connect(as, SIGNAL(response(const SessionData&)), &loop, SLOT(quit()));
       QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));
       QTimer::singleShot(500*1000, &loop, SLOT(quit()));

       /*
        * chain of UiSessionData
        * */
       QStringList chainOfStates;

       SsoTest2PluginNS::SsoTest2Data testData;

       chainOfStates << "Browser" <<"Login" << "Captcha" << "LoginAndCaptcha";
       chainOfStates << "Browser" <<"Login" << "Captcha" << "LoginAndCaptcha";
       chainOfStates << "Browser" <<"Login" << "Captcha" << "LoginAndCaptcha";
       chainOfStates << "Browser" <<"Login" << "Captcha" << "LoginAndCaptcha";

       testData.setChainOfStates(chainOfStates);
       testData.setCurrentState(0);

       as->process(testData, "mech1");

       if(!errorCounter.count())
           loop.exec();

       QCOMPARE(spy.count(), 1);
       if(errorCounter.count())
           TRACE() << errorCounter.at(0).at(1).toString();

       QCOMPARE(errorCounter.count(), 0);


       SignOn::SessionData outData = qVariantValue<SignOn::SessionData>(spy.at(0).at(0));
       SsoTest2PluginNS::SsoTest2Data resultData = outData.data<SsoTest2PluginNS::SsoTest2Data>();

       foreach(QString result, resultData.ChainOfResults())
           QCOMPARE(result, QString("OK"));
   }

  void TestAuthSession::processUi_and_cancel()
  {
       AuthSession *as;
       SSO_TEST_CREATE_AUTH_SESSION(as, "ssotest2");
       g_currentSession = as;

       QSignalSpy errorCounter(as, SIGNAL(error(const SignOn::Error &)));
       QSignalSpy stateCounter(as, SIGNAL(stateChanged(AuthSession::AuthSessionState, const QString&)));
       QSignalSpy spy(as, SIGNAL(response(const SessionData&)));
       QEventLoop loop;

       QObject::connect(as, SIGNAL(response(const SessionData&)), &loop, SLOT(quit()));
       QObject::connect(as, SIGNAL(error(const SignOn::Error &)), &loop, SLOT(quit()));
       QTimer::singleShot(500*1000, &loop, SLOT(quit()));

       /*
        * chain of UiSessionData
        * */
       QStringList chainOfStates;

       SsoTest2PluginNS::SsoTest2Data testData;

       chainOfStates << "Browser" <<"Login" << "Captcha" << "LoginAndCaptcha";
       chainOfStates << "Browser" <<"Login" << "Captcha" << "LoginAndCaptcha";
       chainOfStates << "Browser" <<"Login" << "Captcha" << "LoginAndCaptcha";
       chainOfStates << "Browser" <<"Login" << "Captcha" << "LoginAndCaptcha";

       testData.setChainOfStates(chainOfStates);
       testData.setCurrentState(0);

       as->process(testData, "mech1");
       QTimer::singleShot(3*1000, this, SLOT(cancel()));

       if(!errorCounter.count())
           loop.exec();

       QCOMPARE(spy.count(), 0);
       QCOMPARE(errorCounter.count(), 1);
   }

 #endif

 #if !defined(SSO_CI_TESTMANAGEMENT) && !defined(SSOTESTCLIENT_USES_AUTHSESSION)
      QTEST_MAIN(TestAuthSession)
 #endif

 #endif //TESTAUTHSESSION_CPP_
