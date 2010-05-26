/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * This file is part of Harmattan Signon Daemon test suite.
 *
 * Copyright (C) 2009 Nokia Corporation. All rights reserved.
 *
 * Contact: Andrei Laperie <andrei.laperie@nokia.com>
 *
 * This software, including documentation, is protected by copyright
 * controlled by Nokia Corporation. All rights are reserved.  Copying,
 * including reproducing, storing, adapting or translating, any or all
 * of this material requires the prior written consent of Nokia
 * Corporation.  This material also contains confidential information
 * which may not be disclosed to others without the prior written
 * consent of Nokia.
 */

#ifndef CREDENTIALS_ACCESS_MANAGER_TEST
#define CREDENTIALS_ACCESS_MANAGER_TEST

#include "credentialsaccessmanager.h"

/* TODO - update this test when all the functionality is implemented
          (SIM & Device Lock Code).
*/
using namespace SignonDaemonNS;

class CredentialsAccessManagerTest : public QObject
{
    Q_OBJECT

private slots:

#if defined(SSO_CI_TESTMANAGEMENT)
    void runAllTests();
#else
    private slots:
#endif

    void initTestCase();
    void cleanupTestCase();

    //test cases
    void createCredentialsSystem();
    void openCredentialsSystem();

    void testCredentialsDatabase();

    void closeCredentialsSystem();
    void deleteCredentialsSystem();
    //end test cases

private:
    CredentialsAccessManager *m_pManager;
};

#endif //CREDENTIALS_ACCESS_MANAGER_TEST
