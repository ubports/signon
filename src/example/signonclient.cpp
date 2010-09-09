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

#include <QtGui>
#include <QDebug>
#include <QMessageBox>

#include <SignOn/SessionData>
#include "signonclient.h"
#include "exampledata.h"

using namespace SignOn;
namespace SignOn {
SignonClient::SignonClient(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    m_service = new SignOn::AuthService();
    m_identity = NULL;
    m_session = NULL;
    connect(m_service, SIGNAL(methodsAvailable(const QStringList&)),
            this, SLOT(methodsAvailable(const QStringList&)));
    connect(m_service, SIGNAL(mechanismsAvailable(const QString &, const QStringList&)),
            this, SLOT( mechanismsAvailable(const QString &,  const QStringList&)));
    connect(m_service, SIGNAL(identities(const QList<IdentityInfo>& )),
            this, SLOT(identities(const QList<IdentityInfo>& )));

    qRegisterMetaType<SignOn::SessionData>("SignOn::SessionData");
    qRegisterMetaType<AuthSession::AuthSessionError>("AuthSession::AuthSessionError");
}

void SignonClient::methodsAvailable(const QStringList &mechs)
{
    qDebug("methodsAvailable");
    for (int i = 0; i < mechs.size(); ++i) {
        qDebug() << mechs.at(i).toLocal8Bit().constData() << endl;
        m_service->queryMechanisms(mechs.at(i));
    }
}

void SignonClient::mechanismsAvailable(const QString &method, const QStringList &mechs)
{
    qDebug("mechanismsAvailable");
    qDebug() << method;
    for (int i = 0; i < mechs.size(); ++i) {
        qDebug() << mechs.at(i).toLocal8Bit().constData() << endl;
    }
}

void SignonClient::identities(const QList<IdentityInfo> &identityList)
{
    qDebug("identities");
     for (int i = 0; i < identityList.size(); ++i) {
         qDebug() << identityList.at(i).caption().toLocal8Bit().constData() << endl;
    }
 }

void SignonClient::response(const SessionData &sessionData)
{
    Q_UNUSED(sessionData);
    qDebug("response");
}

void SignonClient::error(SignOn::Identity::IdentityError code, const QString &message)
{
    qDebug("identity Err: %d", code);
    qDebug() << message;
}

void SignonClient::sessionError(AuthSession::AuthSessionError code, const QString &message)
{
    qDebug("session Err: %d", code);
    qDebug() << message;
}

void SignonClient::credentialsStored(const quint32 id)
{
    qDebug() << "stored id: " << id;
    QString message;
    message.setNum(id);
}

void SignonClient::on_store_clicked()
{
    qDebug("on_store_clicked");
    if ( m_identity ) delete m_identity;
    QMap<MethodName,MechanismsList> methods;

    QStringList mechs = QStringList() << QString::fromLatin1("ClientLogin")
                        << QString::fromLatin1("Example") ;
    methods.insert(QLatin1String("google"), mechs);

    //example method to be able to use example plugin
    methods.insert(QLatin1String("example"), QStringList());

    int randomNumber = qrand() % 100;
    m_info = new IdentityInfo(QLatin1String("test_caption")
                              + QString().number(randomNumber),
                              QLatin1String("test_username")
                              + QString().number(randomNumber), methods);
    m_info->setSecret(QLatin1String("test_secret"));

    QStringList realms = QStringList() << QString::fromLatin1("google.com")
                         << QString::fromLatin1("example.com")
                         << QString::fromLatin1("example2.com");
    m_info->setRealms(realms);

    QStringList acl = QStringList() << QString::fromLatin1("AID::12345678")
                      << QString::fromLatin1("AID::87654321")
                      << QString::fromLatin1("signon::example");
    m_info->setAccessControlList(acl);

    int randomType = qrand() % 4;
    switch (randomType) {
    case 0:
        m_info->setType(IdentityInfo::Other);
        break;
    case 1:
        m_info->setType(IdentityInfo::Application);
        break;
    case 2:
        m_info->setType(IdentityInfo::Web);
        break;
    case 3:
        m_info->setType(IdentityInfo::Network);
        break;
    }

    m_identity = Identity::newIdentity(*m_info);

    connect(m_identity, SIGNAL(credentialsStored(const quint32)),
            this, SLOT(credentialsStored(const quint32)));

    connect(m_identity, SIGNAL(error(Identity::IdentityError , const QString& )),
            this, SLOT(error(Identity::IdentityError, const QString& )));

    m_identity->storeCredentials();
}

void SignonClient::on_query_clicked()
{
    qDebug("on_query_clicked");
    m_service->queryMethods();
    m_service->queryIdentities();
}

void SignonClient::on_challenge_clicked()
{
    qDebug("on_challenge_clicked");
    if (!m_identity) {
        error(SignOn::Identity::CanceledError,QLatin1String("Identity not created"));
        return;
    }
    ExampleData data;
    data.setSecret("test");

    data.setSecret("secret");
//    data.setUserName("url");
    data.setExample("http://www.flickr.com/");

    if (!m_session) {
        m_session=m_identity->createSession(QLatin1String("example"));

    connect(m_session, SIGNAL(response(const SessionData&)),
            this, SLOT(response(const SessionData&)));

    connect(m_session, SIGNAL(error(AuthSession::AuthSessionError , const QString& )),
            this, SLOT(sessionError(AuthSession::AuthSessionError , const QString& )));
    }

    m_session->process(data, QLatin1String("example"));

}

void SignonClient::on_google_clicked()
{
    qDebug("on_google_clicked");
    if (!m_identity) {
        error(SignOn::Identity::CanceledError,QLatin1String("Identity not created"));
        return;
    }
    SignOn::SessionData data;

    data.setSecret("test");
    data.setUserName("user@google.com");

    if (!m_session) {
        m_session=m_identity->createSession(QLatin1String("google"));

        connect(m_session, SIGNAL(response(const SignOn::SessionData&)),
            this, SLOT(response(const SignOn::SessionData&)));

        connect(m_session, SIGNAL(error(AuthSession::AuthSessionError , const QString& )),
            this, SLOT(sessionError(AuthSession::AuthSessionError , const QString& )));
    }

    m_session->process(data , QLatin1String("ClientLogin"));
}
}
