/*
 * This file is part of signond
 *
 * Copyright (C) 2016 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QTextStream>
#include <QTimer>

#include "signond/signoncommon.h"

class AuthSession: public QObject
{
    Q_OBJECT

public:
    AuthSession(const QString &path);
    ~AuthSession();

public Q_SLOTS:
    void process(const QVariantMap &sessionData);
    void onResponse(const QVariantMap &response);
    void onError(const QDBusError &error);

private:
    QTextStream m_out;
    QDBusInterface *m_dbus;
};

AuthSession::AuthSession(const QString &path):
    QObject(),
    m_out(stdout)
{
    m_dbus = new QDBusInterface(SIGNOND_SERVICE,
                                path,
                                QLatin1String(SIGNOND_AUTH_SESSION_INTERFACE),
                                QDBusConnection::sessionBus());
}

AuthSession::~AuthSession()
{
    delete m_dbus;
}

void AuthSession::process(const QVariantMap &sessionData)
{
    QVariantList arguments;
    arguments += sessionData;
    arguments += QStringLiteral("mech1");

    QDBusMessage msg = QDBusMessage::createMethodCall(m_dbus->service(),
                                                      m_dbus->path(),
                                                      m_dbus->interface(),
                                                      QStringLiteral("process"));
    msg.setArguments(arguments);

    m_dbus->connection().callWithCallback(msg, this,
                                          SLOT(onResponse(const QVariantMap&)),
                                          SLOT(onError(const QDBusError&)),
                                          SIGNOND_MAX_TIMEOUT);
}

void AuthSession::onResponse(const QVariantMap &response)
{
    // The called doesn't really care about the response value
    Q_UNUSED(response);
    m_out << "Response:";
    QCoreApplication::quit();
}

void AuthSession::onError(const QDBusError &error)
{
    m_out << "Error:" << error.name();
    QCoreApplication::quit();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QString sessionPath;

    QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.count(); i++) {
        if (args[i] == "--sessionPath") {
            sessionPath = args[++i];
        }
    }

    AuthSession authSession(sessionPath);

    QVariantMap sessionData {
        { "Secret", QStringLiteral("testSecret") },
        { "UserName", QStringLiteral("testUsername") },
    };
    authSession.process(sessionData);

    QTimer::singleShot(10*1000, &app, SLOT(quit()));
    app.exec();

    return 0;
}

#include "session_tool.moc"
