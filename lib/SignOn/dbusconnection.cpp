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

#include "dbusconnection.h"
#include "libsignoncommon.h"

#include <QThreadStorage>

namespace SignOn
{
    /*
       Private DBusConnection object that Auto disconnects when the thread storage
       for this object is being released.
    */
    class AutoDBusConnection : public QDBusConnection
    {
    public:
        AutoDBusConnection(const QDBusConnection &connection)
            : QDBusConnection(connection)
        {
            TRACE();
        }

        ~AutoDBusConnection()
        {
            disconnectFromBus(name());
        }
    };

    // Thread Specific Storage to store the dbus connection
    // to the session bus for this thread
    QThreadStorage<AutoDBusConnection*> sessionBusConnection;

    // Thread Specific Storage to store the dbus connection
    // to the system bus for this thread
    QThreadStorage<AutoDBusConnection*> systemBusConnection;

    // Connection counters for distinct connection names in different threads.
    QAtomicInt sessionBusCounter = 0;
    QAtomicInt systemBusCounter = 0;

    QDBusConnection DBusConnection::sessionBus()
    {
        /*
           Create a separate D-Bus connection to Session Bus for each thread.
           Use AutoDBusConnection so that the bus gets disconnected when the
           thread storage is deleted.
        */
        if (!sessionBusConnection.hasLocalData()) {
            QString connectionName = QString::number(
                sessionBusCounter.fetchAndAddRelaxed(1)).prepend(
                    QLatin1String("signon-sync-session-bus-"));

            sessionBusConnection.setLocalData(new AutoDBusConnection(
                QDBusConnection::connectToBus(
                    QDBusConnection::SessionBus,connectionName)));

            //TODO - Remove this trace after Christmas 2010
            QDBusConnection dbus = *sessionBusConnection.localData();
            TRACE() << "DBus Connection Address:" << &dbus
                    << "\nDBus Connection Name:" << dbus.name()
                    << "\nDBus is Connected:" << dbus.isConnected()
                    << "\nDBus Connection ID:" << dbus.baseService();
        }

        return *sessionBusConnection.localData();
    }

    QDBusConnection DBusConnection::systemBus()
    {
        /*
           Create a separate D-Bus connection to System Bus for each thread.
           Use AutoDBusConnection so that the bus gets disconnected when the
           thread storage is deleted.
         */
        if (!systemBusConnection.hasLocalData()) {
            QString connectionName = QString::number(
                systemBusCounter.fetchAndAddRelaxed(1)).prepend(
                    QLatin1String("signon-sync-system-bus-"));

            systemBusConnection.setLocalData(new AutoDBusConnection(
                QDBusConnection::connectToBus(
                    QDBusConnection::SystemBus, connectionName)));


            //TODO - Remove this trace after Christmas 2010
            QDBusConnection dbus = *systemBusConnection.localData();
            TRACE() << "DBus Connection Address:" << &dbus
                    << "DBus Connection Name:" << dbus.name()
                    << "DBus is Connected:" << dbus.isConnected()
                    << "DBus Connection ID:" << dbus.baseService();
        }

        return *systemBusConnection.localData();
    }

} // namespace SignOn
