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

#ifndef TESTAUTHSESSION_H_
#define TESTAUTHSESSION_H_

#include <QtTest/QtTest>
#include <QtCore>

#include <sys/types.h>
#include <errno.h>

#include <signal.h>

#include "signond/signoncommon.h"
#include "SignOn/authservice.h"

//#ifdef SSOUI_TESTS_ENABLED
//#undef SSOUI_TESTS_ENABLED
//#endif

/*
  * As the autotesting framework is not implemented yet
  * so we need to disable this test for now.
  *
  * */
//#define SSOUI_TESTS_ENABLED


#ifdef SSOUI_TESTS_ENABLED
    #include "ssotest2data.h"
    #include "SignOn/uisessiondata.h"
#endif

/*
 * here we test the implementation because of difficulties of having
 *  access to protected/private functions
 * */
#include "SignOn/authsessionimpl.h"

using namespace SignOn;

/*
 * test timeout 10 seconds
 * */
#define test_timeout 10000

class TestAuthSession: public QObject
{
     Q_OBJECT

#if defined(SSO_CI_TESTMANAGEMENT) || defined(SSOTESTCLIENT_USES_AUTHSESSION)
    public Q_SLOTS:
#else
    private Q_SLOTS:
#endif
      /*
      * Start the signon daemon
      * */
     void initTestCase();

     /*
      * End the signon daemon
      * */
     void cleanupTestCase();

     /*
      * UIless
      * AuthSession API related test cases
      * */

     void queryMechanisms_existing_method();
     void queryMechanisms_nonexisting_method();


     void process_with_new_identity();
     void process_with_existing_identity();
     void process_with_nonexisting_type();
     void process_with_nonexisting_method();
     void process_many_times_after_auth();
     void process_many_times_before_auth();
     void process_with_big_session_data();

     void cancel_immidiately();
     void cancel_with_delay();
     void cancel_without_process();

     void handle_destroyed_signal();

     void multi_thread_test();

#ifdef SSOUI_TESTS_ENABLED
     void processUi_with_existing_identity();
     void processUi_and_cancel();
#endif
    private Q_SLOTS:
        void cancel();
        void response(const SignOn::SessionData &data);
 };

#endif
