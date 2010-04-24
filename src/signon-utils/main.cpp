/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
 * Contact: Lucian Horga <ext-lucian.horga@nokia.com>
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

/*
    Calling the following signond functions:

    bool setDeviceLockCode(const QByteArray &oldLockCode,
                               const QByteArray &newLockCode);

    bool setSim(const QByteArray& simData,
                const QByteArray& checkData);

    bool remoteLock(const QByteArray &lockCode)
*/

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtDBus/QtDBus>

#include "SignOn/signoncommon.h"

bool setDeviceLockCode(const QByteArray &oldLockCode, const QByteArray &newLockCode)
{
    QDBusInterface dbus_iface(SIGNOND_SERVICE,
                              SIGNOND_DAEMON_OBJECTPATH,
                              SIGNOND_DAEMON_INTERFACE,
                              SIGNOND_BUS);

    dbus_iface.call(QLatin1String("setDeviceLockCode"), oldLockCode, newLockCode);

    return true;
}

bool setSim(const QByteArray &simData, const QByteArray &checkData)
{
    QDBusInterface dbus_iface(SIGNOND_SERVICE,
                              SIGNOND_DAEMON_OBJECTPATH,
                              SIGNOND_DAEMON_INTERFACE,
                              SIGNOND_BUS);

    dbus_iface.call(QLatin1String("setSIM"), simData, checkData);

    return true;
}

bool remoteLock(const QByteArray &lockCode)
{
    QDBusInterface dbus_iface(SIGNOND_SERVICE,
                              SIGNOND_DAEMON_OBJECTPATH,
                              SIGNOND_DAEMON_INTERFACE,
                              SIGNOND_BUS);

    dbus_iface.call(QLatin1String("remoteLock"), lockCode);

    return true;
}

void showHelp()
{
    fprintf(stderr, "\nUsage: signon-utils [option] [params] ...\n\n");
    fprintf(stderr, "Option           Params                       Meaning\n");
    fprintf(stderr, "----------------------------------------------------------------------\n");
    fprintf(stderr, "--lock-code      oldLockCode, newLockCode     Device lock code change.\n");
    fprintf(stderr, "--sim-change     param                        SIM card change.\n");
    fprintf(stderr, "--remote-lock    param                        Remote database lock/wipe, I guess.\n");
    fprintf(stderr, "--help           none                         Shows this message. \n\n");
}

void showError(const char *command)
{
    fprintf(stderr, "Wrong parameter count for command %s. Run with --help for details.\n", command);
}

bool expectedArgsOk(const QStringList &args, int currentIdx, int expectedArgsCount)
{
    /* Nothing to check */
    if (expectedArgsCount == 0)
        return true;

    /* Not enough arguments */
    if (args.count() <= currentIdx + expectedArgsCount)
        return false;

    /* While parsing arguments, illegal string encountered */
    for (int idx = currentIdx + 1; idx <= currentIdx + expectedArgsCount; idx++)
        if (args[idx].contains(QLatin1String("-")) || args[idx].contains(QLatin1String("--")))
            return false;

    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (!SIGNOND_BUS.isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
                "To start it, run:\n"
                "\teval `dbus-launch --auto-syntax`\n");
        return 0;
    }

    if (app.arguments().count() == 1) {
        showHelp();
        return 0;
    }

    if (app.arguments().count() < 2 ||
        app.arguments().at(1) == QLatin1String("--help")) {
        showHelp();
        return 0;
    }

    QStringList args = app.arguments();
    for (int idx = 0; idx < args.count(); idx++) {
        if (args[idx] == QLatin1String("--lock-code")) {
            if (expectedArgsOk(args, idx, 2))
                setDeviceLockCode( args[idx+1].toUtf8(), args[idx+2].toUtf8());
            else
                showError("--lock-code");
        }

        if (args[idx] == QLatin1String("--sim-change")) {
            if (expectedArgsOk(args, idx, 1))
                setSim(args[idx+1].toUtf8(), args[idx+2].toUtf8());
            else
                showError("--sim-change");
        }

        if (args[idx] == QLatin1String("--remote-lock")) {
            if (expectedArgsOk(args, idx, 1))
                remoteLock(args[idx+1].toUtf8());
            else
                showError("--remote-lock");
        }
    }

    return 0;
}
