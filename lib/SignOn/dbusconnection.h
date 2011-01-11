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

#include <QDBusConnection>

namespace SignOn {

    /* This is a workaround for QDBusConnection::SessionBus as it is not thread safe
       - it can create dangling dbus connection refs.
       Use this class as an alternative to QDBusConnection to get a thread private
       dbus connection.
    */
    class DBusConnection
    {
    public:
        // static function to retrieve a dbus connection to sessionBus
        static QDBusConnection sessionBus();

        // static function to retrieve a dbus connection to systemBus
        static QDBusConnection systemBus();

    private:
        DBusConnection() {}
        ~DBusConnection() {}
    };
}
