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

#ifndef SSO_FAKE_SIGNONUI_H
#define SSO_FAKE_SIGNONUI_H

#include <QVariantMap>
#include <libqtdbusmock/DBusMock.h>

class FakeSignOnUi
{
public:
    FakeSignOnUi(QtDBusMock::DBusMock *mock): m_mock(mock) {
        m_mock->registerTemplate("com.nokia.singlesignonui",
                                 SIGNONUI_MOCK_TEMPLATE,
                                 QDBusConnection::SessionBus);
    }

    void setNextReply(const QVariantMap &reply) {
        mockedService().call("SetNextReply", reply);
    }

    OrgFreedesktopDBusMockInterface &mockedService() {
        return m_mock->mockInterface("com.nokia.singlesignonui",
                                     "/SignonUi",
                                     "com.nokia.singlesignonui",
                                     QDBusConnection::SessionBus);
    }

private:
    QtDBusMock::DBusMock *m_mock;
};

#endif // SSO_FAKE_SIGNONUI_H
