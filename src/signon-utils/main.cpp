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

    bool initSecureStorage(const QByteArray& lockCode);

    bool remoteLock(const QByteArray &lockCode)
*/

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtDBus/QtDBus>

#include <signond/signoncommon.h>


void dbusCall(const QLatin1String &method, const QList<QVariant> &args)
{
    QDBusInterface dbus_iface(QLatin1String("com.nokia.SingleSignOn.DeviceLock"),
                              QLatin1String("/com/nokia/SingleSignOn/DeviceLock"),
                              QLatin1String("com.nokia.SingleSignOn.DeviceLock"),
                              SIGNOND_BUS);

    QDBusMessage reply = dbus_iface.callWithArgumentList(QDBus::Block, method, args);

    switch (reply.type()) {
    case QDBusMessage::ReplyMessage:
        fputs("\nCommand successfully executed.\n", stderr);
        break;
    case QDBusMessage::ErrorMessage:
        fputs("\nCommand execution failed.\n", stderr);
        fputs(reply.errorName().toLatin1().data(), stderr);
        fputs(reply.errorMessage().toLatin1().data(), stderr);
        break;
    case QDBusMessage::InvalidMessage:
        fputs("\nInvalid reply from Signon daemon.\n", stderr);
        break;
    default:
        fputs("\nUnknown error.\n", stderr);
    }
}

void setDeviceLockCode(const QByteArray &newLockCode, const QByteArray &oldLockCode)
{
    QList<QVariant> args = QList<QVariant>() << newLockCode << oldLockCode;
    dbusCall(QLatin1String("setDeviceLockCode"), args);
}

void remoteLock(const QByteArray &lockCode)
{
    QList<QVariant> args = QList<QVariant>() << lockCode;
    dbusCall(QLatin1String("remoteDrop"), args);
}

void showHelp()
{
    fputs("\nUsage: signon-utils [option] [params] ...\n\n", stderr);
    fputs("Option           Params                       Meaning\n", stderr);
    fputs("----------------------------------------------------------------------\n", stderr);
    fputs("--lock-code      newLockCode, oldLockCode     Device lock code change.\n", stderr);
    fputs("--remote-drop    lockCode                     Remote database drop.\n", stderr);
    fputs("--help           none                         Shows this message.\n\n", stderr);
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
        fputs("Cannot connect to the D-Bus session bus.\n"
              "To start it, run:\n"
              "\teval `dbus-launch --auto-syntax`\n", stderr);
        return 0;
    }

    if (app.arguments().count() == 1) {
        showHelp();
        return 0;
    }

    QStringList args = app.arguments();
    for (int idx = 0; idx < args.count(); idx++) {
        if (args[idx] == QLatin1String("--lock-code")) {
            if (args.count() == 3) {
                if (expectedArgsOk(args, idx, 1))
                    setDeviceLockCode( args[idx+1].toUtf8(), QByteArray());
                else
                    showError("--lock-code");
            } else if(args.count() == 4) {
                if (expectedArgsOk(args, idx, 2))
                    setDeviceLockCode( args[idx+1].toUtf8(), args[idx+2].toUtf8());
                else
                    showError("--lock-code");
            }
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
