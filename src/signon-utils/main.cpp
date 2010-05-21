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

#include <signond/signoncommon.h>


void dbusCall(const QLatin1String &method, const QList<QVariant> &args)
{
    QDBusInterface dbus_iface(SIGNOND_SERVICE,
                              SIGNOND_DAEMON_OBJECTPATH,
                              SIGNOND_DAEMON_INTERFACE,
                              SIGNOND_BUS);

    QDBusMessage reply = dbus_iface.callWithArgumentList(QDBus::Block, method, args);

    switch(reply.type()) {
        case QDBusMessage::ReplyMessage:
            fprintf(stderr, "\nCommand successfully executed.\n");
            break;
        case QDBusMessage::ErrorMessage:
            fprintf(stderr, "\nCommand execution failed.\n");
            fprintf(stderr, reply.errorName().toLatin1().data());
            fprintf(stderr, reply.errorMessage().toLatin1().data());
            break;
        case QDBusMessage::InvalidMessage:
            fprintf(stderr, "\nInvalid reply from Signon daemon.\n");
            break;
        default:
            fprintf(stderr, "\nUnknown error.\n");
    }
}

void setDeviceLockCode(const QByteArray &oldLockCode, const QByteArray &newLockCode)
{
    QList<QVariant> args = QList<QVariant>() << oldLockCode << newLockCode;
    dbusCall(QLatin1String("setDeviceLockCode"), args);
}

void initSecureStorage(const QByteArray &unlockData)
{
    qDebug() << "initSecureStorage" << unlockData;
    QList<QVariant> args = QList<QVariant>() << unlockData;
    dbusCall(QLatin1String("initSecureStorage"), args);
}

void remoteLock(const QByteArray &lockCode)
{
    QList<QVariant> args = QList<QVariant>() << lockCode;
    dbusCall(QLatin1String("remoteLock"), args);
}

void showHelp()
{
    fprintf(stderr, "\nUsage: signon-utils [option] [params] ...\n\n");
    fprintf(stderr, "Option           Params                       Meaning\n");
    fprintf(stderr, "----------------------------------------------------------------------\n");
    fprintf(stderr, "--lock-code      newLockCode, oldLockCode     Device lock code change.\n");
    fprintf(stderr, "--init-storage   unlockData                   Initialize secure storage.\n");
    fprintf(stderr, "--remote-lock    lockData                     Remote database lock.\n");
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

        if (args[idx] == QLatin1String("--init-storage")) {
            if (expectedArgsOk(args, idx, 1))
                initSecureStorage(args[idx+1].toUtf8());
            else
                showError("--init-storage");
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
