/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 * Copyright (C) 2012-2016 Canonical Ltd.
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

#include <QMutex>
#include <QMutexLocker>
#include <unistd.h>

#include "ssotestplugin.h"

#include "SignOn/signonplugincommon.h"

using namespace SignOn;

namespace SsoTestPluginNS {

SsoTestPlugin::SsoTestPlugin(QObject *parent):
    AuthPluginInterface(parent)
{
    TRACE();

    m_type = QLatin1String("ssotest");
    m_mechanisms = QStringList(QLatin1String("mech1"));
    m_mechanisms += QLatin1String("mech2");
    m_mechanisms += QLatin1String("mech3");
    m_mechanisms += QLatin1String("BLOB");

    qRegisterMetaType<SignOn::SessionData>("SignOn::SessionData");

    QObject::connect(&m_timer, SIGNAL(timeout()), this, SLOT(execProcess()));
}

SsoTestPlugin::~SsoTestPlugin()
{
}

void SsoTestPlugin::cancel()
{
    TRACE() << "Operation is canceled";
    emit error(Error(Error::SessionCanceled,
                     QLatin1String("The operation is canceled")));
    m_timer.stop();
}

/*
 * dummy plugin is used for testing purposes only
 */
void SsoTestPlugin::process(const SignOn::SessionData &inData,
                            const QString &mechanism)
{
    if (!mechanisms().contains(mechanism)) {
        QString message = QLatin1String("The given mechanism is unavailable");
        TRACE() << message;
        emit error(Error(Error::MechanismNotAvailable, message));
        return;
    }

    m_data = inData;
    m_mechanism = mechanism;
    m_statusCounter = 0;

    m_timer.setInterval(100);
    m_timer.setSingleShot(false);
    m_timer.start();
}

void SsoTestPlugin::execProcess()
{
    m_statusCounter++;
    emit statusChanged(PLUGIN_STATE_WAITING,
                       QLatin1String("hello from the test plugin"));
    if (m_statusCounter < 10) return;

    m_timer.stop();

    SignOn::SessionData outData(m_data);
    outData.setRealm("testRealm_after_test");

    if (m_mechanism == QLatin1String("BLOB")) {
        emit result(outData);
        return;
    }

    foreach(QString key, outData.propertyNames())
        TRACE() << key << ": " << outData.getProperty(key);

    if (m_mechanism == QLatin1String("mech1")) {
        emit result(outData);
        return;
    }

    if (m_mechanism == QLatin1String("mech2")) {
        SignOn::UiSessionData data;
        data.setQueryPassword(true);
        emit userActionRequired(data);
        return;
    }

    emit result(outData);
}

void SsoTestPlugin::userActionFinished(const SignOn::UiSessionData &data)
{
    TRACE();

    if (data.QueryErrorCode() == QUERY_ERROR_NONE) {
        SignOn::SessionData response;
        response.setUserName(data.UserName());
        response.setSecret(data.Secret());
        emit result(response);
        return;
    }

    if (data.QueryErrorCode() == QUERY_ERROR_FORBIDDEN)
        emit error(Error(Error::NotAuthorized,
                   QLatin1String("userActionFinished forbidden ")));
    else
        emit error(Error(Error::UserInteraction,
                   QLatin1String("userActionFinished error: ")
                   + QString::number(data.QueryErrorCode())));

    return;
}

SIGNON_DECL_AUTH_PLUGIN(SsoTestPlugin)

} //namespace SsoTestPluginNS
