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

#include "testpluginproxy.h"
#include "timeouts.h"

#ifdef CAM_UNIT_TESTS_FIXED
#include "credentialsaccessmanagertest.h"
#endif

#include <QCoreApplication>
#include <QtTest/QtTest>
#include <QtCore>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

#if CAM_UNIT_TESTS_FIXED
    CredentialsAccessManagerTest testCAM;
    QTest::qExec(&testCAM, argc, argv);
#endif

    TestPluginProxy testPluginProxy;
    QTest::qExec(&testPluginProxy, argc, argv);

    TimeoutsTest timeoutsTest;
    QTest::qExec(&timeoutsTest, argc, argv);
}

