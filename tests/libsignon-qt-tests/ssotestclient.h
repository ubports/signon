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

#ifndef SSOTESTCLIENT_H
#define SSOTESTCLIENT_H

#include <QObject>

//#define SSOUI_TESTS_ENABLED

#include "testauthsession.h"
#include "testidentityresult.h"
#include "testauthserviceresult.h"

#include "SignOn/signonerror.h"

#ifdef SSO_CI_TESTMANAGEMENT
    #define SSOTESTCLIENT_USES_AUTHSESSION
#endif

class SsoTestClient : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    /*
     * AuthService tests
     */
    void queryIdentities();
    void queryMethods();
    void queryMechanisms();
    void clear();

    /*
     * Identity tests
     */
    void queryAvailableMetods();
    void storeCredentials();
    void requestCredentialsUpdate();
    void queryInfo();
    void addReference();
    void removeReference();
    void verifyUser();
    void verifySecret();
    void signOut();
    void remove();
    void storeCredentialsWithoutAuthMethodsTest();
    void sessionTest();
    void multipleRemove();
    void removeStoreRemove();

#ifdef SSOTESTCLIENT_USES_AUTHSESSION
    /*
     * AuthSession tests
     */
    void multiThreadTest();
    void queryMechanisms_existing_method();
    void queryMechanisms_nonexisting_method();
    void queryAuthPluginACL();
    void process_with_new_identity();
    void process_with_existing_identity();
    void process_with_nonexisting_type();
    void process_with_nonexisting_method();
    void process_with_unauthorized_method();
    void process_from_other_process();
    void process_many_times_after_auth();
    void process_many_times_before_auth();
    void process_with_big_session_data();
    void cancel_immidiately();
    void cancel_with_delay();
    void cancel_without_process();
    void handle_destroyed_signal();
#endif

private:
    void clearDB();

    void initAuthServiceTest();

    void queryIdentitiesWithFilter();

#ifdef SSOUI_TESTS_ENABLED
    void processUi_with_existing_identity();
    void processUi_and_cancel();
#endif
    /*
     * Subtests
     * */
    bool testUpdatingCredentials(bool addMethods = true);
    bool testAddingNewCredentials(bool addMethods = true);

    /*
     * Helpers
     * */
    //deprecated
    static QString authErrCodeAsStr(const AuthService::ServiceError);
    //deprecated
    static QString idErrCodeAsStr(const Identity::IdentityError);
    static QString errCodeAsStr(const Error::ErrorType);
    bool storeCredentialsPrivate(const SignOn::IdentityInfo &info);
    QString pluginsDir() const;

protected Q_SLOTS:
    void response(const SignOn::SessionData &data);

private:
    TestAuthSession testAuthSession;
    QStringList m_tokenList;
    int m_expectedNumberOfMethods;
    QStringList m_expectedMechanisms;
    int m_numberOfInsertedCredentials;

    quint32 m_storedIdentityId;
    IdentityInfo m_storedIdentityInfo;
    QString m_methodToQueryMechanisms;

    TestIdentityResult m_identityResult;
    TestAuthServiceResult m_serviceResult;
};

#endif // SSOTESTCLIENT_H
