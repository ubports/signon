/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
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


#include <QtTest/QtTest>
#include <sasl/sasl.h>

#include "saslplugin.h"
#include "sasldata.h"
#include "saslplugin.cpp"

#include "saslplugintest.h"

using namespace SaslPluginNS;


#define TEST_START qDebug("\n\n\n\n ----------------- %s ----------------\n\n",  __func__);

#define TEST_DONE  qDebug("\n\n ----------------- %s DONE ----------------\n\n",  __func__);

void SaslPluginTest::initTestCase()
{
    TEST_START
    qRegisterMetaType<SignOn::SessionData>();
    qRegisterMetaType<AuthPluginError>();
    TEST_DONE
}

void SaslPluginTest::cleanupTestCase()
{
    TEST_START

    sasl_done();
    TEST_DONE
}

//prepare each test by creating new plugin
void SaslPluginTest::init()
{
    m_testPlugin = new SaslPlugin();
}

//finnish each test by deleting plugin
void SaslPluginTest::cleanup()
{
    delete m_testPlugin;
    m_testPlugin=NULL;
}

//slot for receiving result
void SaslPluginTest::result(const SignOn::SessionData& data)
{
    qDebug() << "got result";
    m_response = data;
    m_loop.exit();
}

//slot for receiving error
void SaslPluginTest::pluginError(AuthPluginError error)
{
    qDebug() << "got error";
    m_error = error;
    m_loop.exit();
}

//test cases

void SaslPluginTest::testPlugin()
{
    TEST_START

    qDebug() << "Checking plugin integrity.";
    QVERIFY(m_testPlugin);

    TEST_DONE
}

void SaslPluginTest::testPluginType()
{
    TEST_START

    qDebug() << "Checking plugin type.";
    QCOMPARE(m_testPlugin->type(), QString("sasl"));

    TEST_DONE
}

void SaslPluginTest::testPluginMechanisms()
{
    TEST_START

    qDebug() << "Checking plugin mechanisms.";
    QStringList mechs = m_testPlugin->mechanisms();
    QVERIFY(!mechs.isEmpty());
    QVERIFY(mechs.contains(QString("PLAIN")));
    qDebug() << mechs;

    TEST_DONE
}

void SaslPluginTest::testPluginCancel()
{
    TEST_START
    //no functionality to test
    TEST_DONE
}

void SaslPluginTest::testPluginProcess()
{
    TEST_START

    SaslData info;

    QObject::connect(m_testPlugin, SIGNAL(result(const SignOn::SessionData&)),
                  this,  SLOT(result(const SignOn::SessionData&)),Qt::QueuedConnection);
    QObject::connect(m_testPlugin, SIGNAL(error(AuthPluginError)),
                  this,  SLOT(pluginError(AuthPluginError)),Qt::QueuedConnection);
    QTimer::singleShot(10*1000, &m_loop, SLOT(quit()));

    //try without mechanism
    m_testPlugin->process(info);
    m_loop.exec();

    QVERIFY(m_error == PLUGIN_ERROR_MECHANISM_NOT_SUPPORTED);

    //try without username
    m_testPlugin->process(info, QString("ANONYMOUS"));
    m_loop.exec();
    QVERIFY(m_error == PLUGIN_ERROR_MISSING_DATA);

    //try with wron state
    info.setstate(SaslData::CONTINUE);
    info.setUserName(QString("test"));
    m_testPlugin->process(info, QString("ANONYMOUS"));
    m_loop.exec();
    QVERIFY(m_error == PLUGIN_ERROR_INVALID_STATE);

    //rest of process is tested with real authentication mechanisms
    TEST_DONE
}

void SaslPluginTest::testPluginChallengePlain()
{
    TEST_START

    SaslData info;

    QObject::connect(m_testPlugin, SIGNAL(result(const SignOn::SessionData&)),
                  this,  SLOT(result(const SignOn::SessionData&)),Qt::QueuedConnection);
    QObject::connect(m_testPlugin, SIGNAL(error(AuthPluginError)),
                  this,  SLOT(pluginError(AuthPluginError)),Qt::QueuedConnection);
    QTimer::singleShot(10*1000, &m_loop, SLOT(quit()));

    info.setUserName(QString("idmtestuser"));
    info.setSecret(QString("abc123"));
    info.setAuthname(QString("authn"));

    //create connection to server to get initial challenge
    SaslServer* server = new SaslServer();
    QByteArray reply;
    server->init(QString("PLAIN"),reply);

    //give challenge to plugin
    m_testPlugin->process(info, QString("PLAIN"));
    m_loop.exec();

    //test response here
    SaslData result =  m_response.data<SaslData>();
    QByteArray token=result.Response();
    token.replace('\0',':');
    qDebug() << token;
    QCOMPARE(result.Response(), QByteArray("idmtestuser\0authn\0abc123",11+5+6+2));

   //check that authentication server is happy about answer
    int retval=server->step(result.Response());
   QVERIFY(retval== SASL_NOUSER);

   delete server;

   TEST_DONE
}

void SaslPluginTest::testPluginChallengeDigestMd5()
{
    TEST_START
    SaslData info;

    QObject::connect(m_testPlugin, SIGNAL(result(const SignOn::SessionData&)),
                  this,  SLOT(result(const SignOn::SessionData&)),Qt::QueuedConnection);
    QObject::connect(m_testPlugin, SIGNAL(error(AuthPluginError)),
                  this,  SLOT(pluginError(AuthPluginError)),Qt::QueuedConnection);
    QTimer::singleShot(10*1000, &m_loop, SLOT(quit()));

    info.setUserName(QString("idmtestuser"));
    info.setSecret(QString("abc123"));
    info.setAuthname(QString("authn"));
    info.setRealm(QString("realm"));
    info.setService(QByteArray("sample"));

    //create connection to server to get initial challenge
    SaslServer* server = new SaslServer();

    QByteArray challenge;
    server->init(QString("DIGEST-MD5"), challenge);
    qDebug() <<challenge;
    info.setChallenge(challenge);

    //give challenge to plugin
    m_testPlugin->process(info, QString("DIGEST-MD5"));
    m_loop.exec();

    //test response here
    SaslData result = m_response.data<SaslData>();
    qDebug() << result.Response();

    qDebug("verify response");
    int retval=server->step(result.Response());
   QVERIFY(retval== SASL_NOUSER);

    delete server;
    TEST_DONE
}

void SaslPluginTest::testPluginChallengeCramMd5()
{
    TEST_START
    SaslData info;

    QObject::connect(m_testPlugin, SIGNAL(result(const SignOn::SessionData&)),
                  this,  SLOT(result(const SignOn::SessionData&)),Qt::QueuedConnection);
    QObject::connect(m_testPlugin, SIGNAL(error(AuthPluginError)),
                  this,  SLOT(pluginError(AuthPluginError)),Qt::QueuedConnection);
    QTimer::singleShot(10*1000, &m_loop, SLOT(quit()));

    info.setUserName(QString("idmtestuser"));
    info.setSecret(QString("abc123"));
    info.setAuthname(QString("authn"));
    info.setRealm(QString("realm"));
    info.setService(QByteArray("sample"));

    //create connection to server to get initial challenge
    SaslServer *server = new SaslServer();

    QByteArray challenge;
    int serret = server->init(QString("CRAM-MD5"), challenge); //fails sometimes
    if (serret!=SASL_OK)
    {
        QSKIP("sasl server init for CRAM-MD5 failed",SkipSingle);
    }

    qDebug() <<challenge;
    info.setChallenge(challenge);

    //give challenge to plugin
    m_testPlugin->process(info, QString("CRAM-MD5"));
    m_loop.exec();

    //test response here
    SaslData result =  m_response.data<SaslData>();
    qDebug() << result.Response();

    qDebug("verify response");
    int retval=server->step(result.Response()); //fails, check server impl.
    QVERIFY(retval== SASL_NOUSER);

    delete server;

    TEST_DONE
}

//private funcs

    void SaslPluginTest::testPluginsasl_callback()
{
    TEST_START
    int ret=0;
    const char* res;
    unsigned len;
    ret=m_testPlugin->sasl_callback(NULL,0,&res,&len);
    QVERIFY(ret==SASL_BADPARAM);
    ret=m_testPlugin->sasl_callback(m_testPlugin,0,NULL,&len);
    QVERIFY(ret==SASL_BADPARAM);

    m_testPlugin->d->m_input.setUserName(QString("user"));

    ret=m_testPlugin->sasl_callback(m_testPlugin,SASL_CB_USER,&res,&len);
    QVERIFY(ret==SASL_OK);
    QCOMPARE(QByteArray(res),QByteArray("user"));
    QVERIFY(len == 4);

    m_testPlugin->d->m_input.setAuthname(QString("auth"));

    ret=m_testPlugin->sasl_callback(m_testPlugin,SASL_CB_AUTHNAME,&res,&len);
    QVERIFY(ret==SASL_OK);
    QCOMPARE(QByteArray(res),QByteArray("auth"));
    QVERIFY(len == 4);

    ret=m_testPlugin->sasl_callback(m_testPlugin,SASL_CB_LANGUAGE,&res,&len);
    QVERIFY(ret==SASL_OK);
    QVERIFY(res == NULL);
    QVERIFY(len == 0);

    ret=m_testPlugin->sasl_callback(m_testPlugin,45643,&res,&len);
    QVERIFY(ret==SASL_BADPARAM);

    TEST_DONE
}

void SaslPluginTest::testPluginsasl_get_realm()
{
    TEST_START
    int ret=0;
    const char* res;

    ret=m_testPlugin->sasl_get_realm(NULL, 0, NULL, NULL);
    QVERIFY(ret==SASL_FAIL);
    ret=m_testPlugin->sasl_get_realm(NULL, SASL_CB_GETREALM, NULL, NULL);
    QVERIFY(ret==SASL_BADPARAM);
    ret=m_testPlugin->sasl_get_realm(m_testPlugin, SASL_CB_GETREALM, NULL, NULL);
    QVERIFY(ret==SASL_BADPARAM);

    ret=m_testPlugin->sasl_get_realm(m_testPlugin, SASL_CB_GETREALM, NULL, &res);
    QVERIFY(ret==SASL_OK);

    m_testPlugin->d->m_input.setRealm(QString("real"));

    ret=m_testPlugin->sasl_get_realm(m_testPlugin, SASL_CB_GETREALM, NULL, &res);
    QVERIFY(ret==SASL_OK);
    QCOMPARE(QByteArray(res),QByteArray("real"));

    TEST_DONE
}

void SaslPluginTest::testPluginsasl_get_secret()
{
    TEST_START
    int ret=0;
    sasl_secret_t *secret=NULL;

    ret=m_testPlugin->sasl_get_secret(m_testPlugin->d->m_conn,NULL,0, &secret);
    QVERIFY(ret==SASL_BADPARAM);

    m_testPlugin->d->m_input.setSecret(QString("password"));

    ret=m_testPlugin->sasl_get_secret(m_testPlugin->d->m_conn,m_testPlugin,SASL_CB_PASS, NULL);
    QVERIFY(ret==SASL_BADPARAM);
    ret=m_testPlugin->sasl_get_secret(m_testPlugin->d->m_conn,m_testPlugin,SASL_CB_PASS, &secret);
    qDebug("err: %d",ret);
    QVERIFY(ret==SASL_OK);
    QCOMPARE(QByteArray("password"),QByteArray((const char*)secret->data,(int)secret->len));

    TEST_DONE
}

void SaslPluginTest::testPluginsasl_log()
{
    TEST_START
    int ret=0;

    ret=m_testPlugin->sasl_log(m_testPlugin, 0, NULL);
    QVERIFY(ret==SASL_BADPARAM);

    ret=m_testPlugin->sasl_log(m_testPlugin, 0, "test debug");
    QVERIFY(ret==SASL_OK);

    TEST_DONE
}

void SaslPluginTest::testPluginset_callbacks()
{
    TEST_START
    m_testPlugin->set_callbacks();
    sasl_callback_t *callback;
    callback = m_testPlugin->d->m_callbacks;
    QVERIFY(callback != NULL);
    QVERIFY(callback->id == SASL_CB_LOG);
    QVERIFY(callback->context == m_testPlugin);
    ++callback;
    QVERIFY(callback != NULL);
    QVERIFY(callback->id == SASL_CB_USER);
    QVERIFY(callback->context == m_testPlugin);
    ++callback;
    QVERIFY(callback != NULL);
    QVERIFY(callback->id == SASL_CB_AUTHNAME);
    QVERIFY(callback->context == m_testPlugin);
    ++callback;
    QVERIFY(callback != NULL);
    QVERIFY(callback->id == SASL_CB_PASS);
    QVERIFY(callback->context == m_testPlugin);
    ++callback;
    QVERIFY(callback != NULL);
    QVERIFY(callback->id == SASL_CB_GETREALM);
    QVERIFY(callback->context == m_testPlugin);
    ++callback;
    QVERIFY(callback != NULL);
    QVERIFY(callback->id == SASL_CB_LIST_END);
    QVERIFY(callback->context == NULL);

    TEST_DONE
}

void SaslPluginTest::testPlugincheck_and_fix_parameters()
{
    TEST_START
    bool ret;
    SaslData input;
    ret=m_testPlugin->check_and_fix_parameters(input);
    QVERIFY(ret==false); //m_user not defined
    QVERIFY(input.Service().isEmpty());
    input.setUserName(QString("user"));

    ret=m_testPlugin->check_and_fix_parameters(input);
    QVERIFY(ret); //user valid
    QCOMPARE(input.UserName(), QString("user"));
    QCOMPARE(input.Service(), QString("default"));
    QCOMPARE(input.Fqdn(), QString("default"));
    QCOMPARE(input.IpLocal(), QString("127.0.0.1"));
    QCOMPARE(input.IpRemote(), QString("127.0.0.1"));

    TEST_DONE
}

// sasl server implementation for response verification

int sasl_log(void *context,
            int priority,
            const char *message)
    {
        Q_UNUSED(context);
        Q_UNUSED(priority);
        if (! message)
            return SASL_BADPARAM;

        TRACE() << message;
        return SASL_OK;
    }

static sasl_callback_t callbacks[] = {
  {
    SASL_CB_LOG, (int(*)())(&sasl_log), NULL
  }, {
    SASL_CB_LIST_END, NULL, NULL
  }
};

void saslfail(int why, const char *what, const char *errstr)
{
  qDebug() << why << what << errstr;
}

#define SAMPLE_SEC_BUF_SIZE (2048)

char buf[SAMPLE_SEC_BUF_SIZE];

SaslServer::SaslServer()
{
    service="sample";
    localdomain="loc";
    userdomain="realm";
    iplocal="127.0.0.1";
    ipremote="127.0.0.1";
    searchpath=".";
    memset(&buf, 0L, SAMPLE_SEC_BUF_SIZE);
}

SaslServer::~SaslServer()
{
    sasl_dispose(&conn);
    //sasl_done(); //cannot be called, as plugin also runs this
}

int SaslServer::init(const QString& mech, QByteArray& challenge)
{
  const char *ext_authid = NULL;

  /* Init defaults... */
  memset(&secprops, 0L, sizeof(secprops));
  secprops.maxbufsize = SAMPLE_SEC_BUF_SIZE;
  secprops.max_ssf = UINT_MAX;

 result = sasl_server_init(callbacks, "sample");
  if (result != SASL_OK)
    saslfail(result, "Initializing libsasl", NULL);

  result = sasl_server_new(service,
                           localdomain,
                           userdomain,
                           iplocal,
                           ipremote,
                           NULL,
                           serverlast,
                           &conn);
  if (result != SASL_OK)
    saslfail(result, "Allocating sasl connection state", NULL);

  if (!mech.isEmpty()) {
    printf("Forcing use of mechanism %s\n", mech.toAscii().constData());
    data = strdup(mech.toAscii().constData());
    len = (unsigned) strlen(data);
    count = 1;
  } else {
    puts("Generating client mechanism list...");
    result = sasl_listmech(conn,
                           ext_authid,
                           NULL,
                           " ",
                           NULL,
                           &data,
                           &len,
                           (int*)&count);
    if (result != SASL_OK)
      saslfail(result, "Generating client mechanism list", NULL);
  }

  printf("Sending list of %d mechanism(s)\n", count);

  if(!mech.isEmpty()) {
      free((void *)data);
  }

 puts("Waiting for client mechanism...");
 strcpy(buf, mech.toAscii().constData());
 len=strlen(buf);

 if (!mech.isEmpty() && strcasecmp(mech.toAscii().constData(), buf))
 {
     qDebug("Client chose something other than the mandatory mechanism");
     return SASL_FAIL;
 }

 if (strlen(buf) < len) {
     qDebug(" Hmm, there's an initial response here ");
    data = buf + strlen(buf) + 1;
    len = len - strlen(buf) - 1;
  } else {
      qDebug("no initial data");
    data = NULL;
    len = 0;
  }
  qDebug("start");
   result = sasl_server_start(conn,
                             buf,
                             data,
                             len,
                             &data,
                             &len);
  if (result != SASL_OK && result != SASL_CONTINUE)
   {
    saslfail(result, "Starting SASL negotiation", sasl_errstring(result,NULL,NULL));
    return result;
}
  if(len) {
      qDebug("gotl data");
qDebug()<< data;
  challenge=QByteArray(data,len);
  qDebug()<< challenge;
}
  return result;
}

int SaslServer::step(const QByteArray &response)
{
    if (data) {
      puts("Sending response...");
    } else {
     qDebug("No data to send--something's wrong");
 }
    puts("Waiting for client reply...");
    QByteArray resp=response;
    resp.replace('\0',':');

    qDebug()<<resp;
    qDebug()<<response;
    qDebug()<<response.count();

    for(int i=0; i<response.count(); i++) buf[i]=(char)(response.constData()[i]);
    len=response.count();
    buf[len]=0;

    data = NULL;
    result = sasl_server_step(conn, buf, len,
                              &data, &len);
    if (result != SASL_OK && result != SASL_CONTINUE)
    {
      saslfail(result, "Performing SASL negotiation", sasl_errstring(result,NULL,NULL));
      return result;
  }

  if(result == SASL_CONTINUE) {
      qDebug("continue");
      return result;
  }

  puts("Negotiation complete");

  if(serverlast&&data) {
      printf("might need additional send:\n");
  }

  result = sasl_getprop(conn, SASL_USERNAME, (const void **)&data);

return SASL_OK;
}

//end test cases

QTEST_MAIN(SaslPluginTest)
