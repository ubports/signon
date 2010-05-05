/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
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

#include <QMutex>
#include <QMutexLocker>
#include "ssotestplugin.h"

#include "SignOn/signonplugincommon.h"

using namespace SignOn;

namespace SsoTestPluginNS {

    static QMutex mutex;
    static bool is_cancelled = false;

    SsoTestPlugin::SsoTestPlugin(QObject *parent) : AuthPluginInterface(parent)
    {
        TRACE();

        m_type = QLatin1String("ssotest");
        m_mechanisms = QStringList(QLatin1String("mech1"));
        m_mechanisms += QLatin1String("mech2");
        m_mechanisms += QLatin1String("mech3");

        qRegisterMetaType<SignOn::SessionData>("SignOn::SessionData");
        qRegisterMetaType<AuthPluginError>("AuthPluginError");
    }

    SsoTestPlugin::~SsoTestPlugin()
    {

    }

    void SsoTestPlugin::cancel()
    {
        TRACE();
        QMutexLocker locker(&mutex);
        is_cancelled = true;
    }
    /*
     * dummy plugin is used for testing purposes only
     * */
    void SsoTestPlugin::process(const SignOn::SessionData &inData, const QString &mechanism)
    {
        if (!mechanisms().contains(mechanism)) {
            QString message = QLatin1String("The given mechanism is unavailable");
            TRACE() << message;
            emit error(PLUGIN_ERROR_MECHANISM_NOT_SUPPORTED, message);
            return;
        }

        QMetaObject::invokeMethod(this,
                                  "execProcess",
                                  Qt::QueuedConnection,
                                  Q_ARG(SignOn::SessionData, inData),
                                  Q_ARG(QString, mechanism));
    }

    void SsoTestPlugin::execProcess(const SignOn::SessionData &inData, const QString &mechanism)
    {
        SignOn::SessionData outData(inData);
        outData.setRealm("testRealm_after_test");

        for (int i = 0; i < 10; i++)
            if (!is_cancelled) {
                TRACE() << "Signal is sent";
                emit statusChanged(PLUGIN_STATE_WAITING, QLatin1String("hello from the test plugin"));
                usleep(0.1 * 1000000);
            }

        if (is_cancelled) {
            TRACE() << "Operation is cancelled";
            QMutexLocker locker(&mutex);
            is_cancelled = false;
            emit error(PLUGIN_ERROR_OPERATION_FAILED, QLatin1String("The operation is canceled"));
            return;
        }

        foreach(QString key, outData.propertyNames())
            TRACE() << key << ": " << outData.getProperty(key);

        if (mechanism == QLatin1String("mech2")) {
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
            emit error(PLUGIN_ERROR_NOT_AUTHORIZED,
                       QLatin1String("userActionFinished forbidden "));
        else
            emit error(PLUGIN_ERROR_USER_INTERACTION,
                       QLatin1String("userActionFinished error: ")
                       + QString::number(data.QueryErrorCode()));

        return;
    }

    SIGNON_DECL_AUTH_PLUGIN(SsoTestPlugin)
} //namespace SsoTestPluginNS


