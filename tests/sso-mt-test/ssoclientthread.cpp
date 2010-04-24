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

#include "ssoclientthread.h"
#include "ssotestclient.h"

#include <QTime>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>

static int finishedThreads = 0;
static SsoClientThread* primary = NULL;

SsoClientThread::SsoClientThread(SsoTestClient::TestType type)
    : m_pTestClient(NULL), m_testType(type)
{
    static int id = 0;
    connect(this, SIGNAL(reallyStarted()), this, SLOT(startTests()), Qt::DirectConnection);

    m_id = id++;

    if(!m_id)
        primary = this;
}

SsoClientThread::~SsoClientThread()
{
}

bool SsoClientThread::isPrimary()
{
    return (m_id == 0);
}

#include <QProcess>

void SsoClientThread::run()
{
    qDebug() << QString("\n\nSSO Client thread started - %1\n\n").arg(QThread::currentThreadId());

    //todo results will have to be exported to xml file and interpreted.
    QProcess process;

    /*
     *
     * THIS IS FOR LOCAL USAGE ONLY
     * */
    process.start("./testsinglesignon", QStringList()
                  << "singleTestClient"
                  << "-o"
                  << QString("filename%1").arg(QThread::currentThreadId()), QIODevice::ReadWrite);

    if(!process.waitForStarted())
        qDebug() << "Could not wait for started";

    if(process.state() != QProcess::Running)
        qDebug() << "Wroong!!!";

    process.waitForFinished(10 * 60 * 1000);
    qDebug() << "Out of thread.";
}

void SsoClientThread::startTests()
{
    if(!m_pTestClient)
    {
        qCritical() << "Start the SsoClientThread first.";
        return;
    }

    QFlags<SsoTestClient::TestType> flags(m_testType);

    if(flags.testFlag(SsoTestClient::AllTests))
    {
        connect(this, SIGNAL(runTests()), m_pTestClient, SLOT(runAllTests()));
    }
    else
    {
        if(flags.testFlag(SsoTestClient::QueryServices))
            connect(this, SIGNAL(runTests()), m_pTestClient, SLOT(runQueryServiceTests()));

        if(flags.testFlag(SsoTestClient::QueryIdentities))
            connect(this, SIGNAL(runTests()), m_pTestClient, SLOT(runQueryIdentityTests()));

        if(flags.testFlag(SsoTestClient::IdentityStorage))
            connect(this, SIGNAL(runTests()), m_pTestClient, SLOT(runStoreTests()));
    }

    emit runTests();
}

void SsoClientThread::stopAllExceptPrimary()
{
    finishedThreads++;

    if(m_id)
        quit();

    if(finishedThreads == NUMBER_OF_TEST_CLIENTS)
    {
        primary->quit();
    }
}
