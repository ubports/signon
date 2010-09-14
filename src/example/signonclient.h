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
#ifndef SIGNONCLIENT_H
#define SIGNONCLIENT_H

#include "ui_signonclient.h"
#include "SignOn/AuthService"
#include "SignOn/Identity"


namespace SignOn {

class SignonClient : public QWidget
{
    Q_OBJECT

public:
    SignonClient(QWidget *parent = 0);

public slots:
    void response(const SignOn::SessionData &sessionData);
    void error(const SignOn::Error &error);
    void sessionError(const SignOn::Error &error);
    void credentialsStored(const quint32 id);

private slots:
    void methodsAvailable(const QStringList &mechs);
    void mechanismsAvailable(const QString &method, const QStringList &mechs);
    void identities(const QList<IdentityInfo> &identityList);

    void on_store_clicked();
    void on_query_clicked();
    void on_challenge_clicked();
    void on_google_clicked();

private:
    Ui::SignonClient ui;
    SignOn::AuthService *m_service;
    SignOn::Identity *m_identity;
    SignOn::IdentityInfo *m_info;
    SignOn::AuthSession *m_session;
    SignOn::SessionData *m_sessionData;
};

} //namespace SignOn
#endif //SIGNONCLIENT_H

