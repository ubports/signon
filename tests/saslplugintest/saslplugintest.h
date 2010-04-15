/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#ifndef SASL_PLUGINS_TEST
#define SASL_PLUGINS_TEST

#include <sasl/sasl.h>
#include <QString>
#include "saslplugin.h"
#include "SignOn/authpluginif.h"

using namespace SaslPluginNS;

class SaslPluginTest : public QObject
{
    Q_OBJECT

public slots:
    void result(const SignOn::SessionData& data);
    void pluginError(AuthPluginError error);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    //test cases
    void testPlugin();
    void testPluginType();
    void testPluginMechanisms();
    void testPluginCancel();
    void testPluginProcess();
    void testPluginChallengePlain();
    void testPluginChallengeDigestMd5();
    void testPluginChallengeCramMd5();
    void testPluginsasl_callback();
    void testPluginsasl_get_realm();
    void testPluginsasl_get_secret();
    void testPluginsasl_log();
    void testPluginset_callbacks();
    void testPlugincheck_and_fix_parameters();
    //end test cases

private:
    SaslPlugin* m_testPlugin;
    AuthPluginError m_error;
    SignOn::SessionData m_response;
    QEventLoop m_loop;
};

class SaslServer {
public:
    SaslServer();
    ~SaslServer();
    int  init(const QString& mech, QByteArray &challenge);
    int step(const QByteArray &response);

    int c;
    int errflag ;
    int result;
    sasl_security_properties_t secprops;
    sasl_ssf_t extssf ;
    char *options, *value;
    unsigned len, count;
    const char *data;
    int serverlast ;
    sasl_ssf_t *ssf;

    const char *mech,
    *iplocal ,
    *ipremote ,
    *searchpath,
    *service ,
    *localdomain ,
    *userdomain ;
    sasl_conn_t *conn ;
};

#endif //SASL_PLUGINS_TEST
