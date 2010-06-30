/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
 * Contact: Aurel Popirtac <ext-aurel.popirtac@nokia.com>
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

#include "accesscodehandler.h"
#include "signond-common.h"

using namespace SignonDaemonNS;

#ifndef SIGNON_USES_CELLULAR_QT

AccessCodeHandler::AccessCodeHandler(QObject *parent)
                                    : QObject(parent)
                                    , m_code ("1234")
{
}

AccessCodeHandler::~AccessCodeHandler()
{
}

bool AccessCodeHandler::isValid()
{
    return true;
}

void AccessCodeHandler::querySim()
{
    return;
}

#else

QString simStatusAsStr(const SIMStatus::Status status)
{
    QString statusStr;
    switch (status) {
    case SIMStatus::UnknownStatus: statusStr = QLatin1String("UnknownStatus"); break;
    case SIMStatus::Ok: statusStr = QLatin1String("Ok"); break;
    case SIMStatus::NoSIM: statusStr = QLatin1String("NoSIM"); break;
    case SIMStatus::PermanentlyBlocked: statusStr = QLatin1String("PermanentlyBlocked"); break;
    case SIMStatus::NotReady: statusStr = QLatin1String("NotReady"); break;
    case SIMStatus::PINRequired: statusStr = QLatin1String("PINRequired"); break;
    case SIMStatus::PUKRequired: statusStr = QLatin1String("PUKRequired"); break;
    case SIMStatus::Rejected: statusStr = QLatin1String("Rejected"); break;
    case SIMStatus::SIMLockRejected: statusStr = QLatin1String("SIMLockRejected"); break;
    default: statusStr = QLatin1String("Not Handled.");
    }
    return statusStr;
}

AccessCodeHandler::AccessCodeHandler(QObject *parent)
    : QObject(parent),
      m_code(QByteArray()),
      m_lastSimStatus(SIMStatus::UnknownStatus),
      m_simIdentity(new SIMIdentity(this)),
      m_simStatus(new SIMStatus(this))
{
    connect(m_simIdentity,
            SIGNAL(iccidComplete(QString, SIMError)),
            SLOT(simIccidComplete(QString, SIMError)));

    connect(m_simStatus,
            SIGNAL(statusChanged(SIMStatus::Status)),
            SLOT(simStatusChanged(SIMStatus::Status)));
}

AccessCodeHandler::~AccessCodeHandler()
{
}

void AccessCodeHandler::simIccidComplete(QString iccid, SIMError error)
{
    //TODO handle stuff here - cases of sim available vs sim changed
    if (error == Cellular::SIM::NoError) {
        QByteArray iccidBa = iccid.toLocal8Bit();

        if(codeAvailable() && (iccidBa != m_code)) {
            m_code = iccid.toLocal8Bit();
            emit simChanged(m_code);
        } else {
            m_code = iccid.toLocal8Bit();
            emit simAvailable(m_code);
        }

        TRACE() << "ICC-ID:[" << m_code << "]";
    } else {
        TRACE() << "Error occurred while querying icc-id. Code:" << error;
    }
}

void AccessCodeHandler::simStatusChanged(SIMStatus::Status status)
{
    TRACE() << simStatusAsStr(status);

    //Todo- if possible think of a more stable solution.
    if ((m_lastSimStatus == SIMStatus::NoSIM || m_lastSimStatus == SIMStatus::NotReady)
        && (status == SIMStatus::Ok)) {
        querySim();
    }

    m_lastSimStatus = status;
}

bool AccessCodeHandler::isValid()
{
    return (m_simIdentity->isValid() && m_simStatus->isValid());
}

void AccessCodeHandler::querySim()
{
    TRACE() << "Querying SIM.";
    m_simIdentity->iccid();
}


#endif

bool AccessCodeHandler::codeAvailable()
{
    return !m_code.isNull();
}

QByteArray AccessCodeHandler::currentCode() const
{
    return m_code;
}


