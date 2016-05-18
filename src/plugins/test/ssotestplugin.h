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

#ifndef SSOTESTPLUGIN_H_
#define SSOTESTPLUGIN_H_

#include <QtCore>

#include "SignOn/sessiondata.h"
#include "SignOn/authpluginif.h"
#include "SignOn/uisessiondata.h"

namespace SsoTestPluginNS {

class SsoTestPlugin: public AuthPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(AuthPluginInterface)
public:
    SsoTestPlugin(QObject *parent = 0);
    virtual ~SsoTestPlugin();

public Q_SLOTS:
    QString type() const { return m_type; }
    QStringList mechanisms() const { return m_mechanisms; }
    void cancel();
    void process(const SignOn::SessionData &inData,
                 const QString &mechanism = 0);
    void userActionFinished(const SignOn::UiSessionData &data);

private:
    QString m_type;
    QStringList m_mechanisms;
    SignOn::SessionData m_data;
    QString m_mechanism;
    QTimer m_timer;
    int m_statusCounter;

private Q_SLOTS:
    void execProcess();
};

} //namespace SsoTestPluginNS

#endif /* SSOTESTPLUGIN_H_ */
