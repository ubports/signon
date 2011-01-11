/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
 * Contact: Aurel Popirtac <ext-Aurel.Popirtac@nokia.com>
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
