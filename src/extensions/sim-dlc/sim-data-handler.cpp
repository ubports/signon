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

#include "sim-data-handler.h"
#include "debug.h"

#include <QDBusError>

#include <openssl/sha.h>

#define SIM_RAND_BASE_SIZE 15
#define SIM_AUTH_COUNT 3


/* ---  Helper function --- */

QByteArray sha256Digest(const QByteArray &source)
{
    SHA256_CTX ctx;
    u_int8_t results[SHA256_DIGEST_LENGTH];

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (u_int8_t *)source.constData(), source.length());
    SHA256_Final(results, &ctx);

    QByteArray result((char *)results, SHA256_DIGEST_LENGTH);
    return result;
}

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

SimDataHandler::SimDataHandler(QObject *parent)
    : QObject(parent),
      m_dataBuffer(QByteArray()),
      simData(QByteArray()),
      m_simChallengeComplete(true),
      m_randCounter(0),
      m_lastSimStatus(SIMStatus::UnknownStatus),
      m_simIdentity(0),
      m_simStatus(new SIMStatus(this))
{
    refreshSimIdentity();

    connect(m_simStatus,
            SIGNAL(statusChanged(SIMStatus::Status)),
            SLOT(simStatusChanged(SIMStatus::Status)));
    connect(m_simStatus,
            SIGNAL(statusComplete(SIMStatus::Status, SIMError)),
            SLOT(simStatusComplete(SIMStatus::Status, SIMError)));

    m_simStatus->status();
}

SimDataHandler::~SimDataHandler()

{
}

void SimDataHandler::refreshSimIdentity()
{
    if (m_simIdentity != 0)
        delete m_simIdentity;

    m_simIdentity = new SIMIdentity;
    connect(m_simIdentity,
            SIGNAL(authComplete(QByteArray, QByteArray, QByteArray, QByteArray, SIMError)),
            SLOT(authComplete(QByteArray, QByteArray, QByteArray, QByteArray, SIMError)));
}

void SimDataHandler::authComplete(QByteArray res,
                                  QByteArray cipheringKey,
                                  QByteArray eapCipheringKey,
                                  QByteArray eapIntegrityKey,
                                  SIMError err)
{
    Q_UNUSED(eapCipheringKey);
    Q_UNUSED(eapIntegrityKey);

    if (err == Cellular::SIM::NoError) {
        TRACE() << "Successful SIM challenge...";
        m_dataBuffer += res;
        m_dataBuffer += cipheringKey;

        if (!m_simChallengeComplete) {
            querySim();
        } else {
            QByteArray sha256DigestBa = sha256Digest(m_dataBuffer);
            m_dataBuffer.clear();
            TRACE() << sha256DigestBa.toHex();
            simData = sha256DigestBa;
            emit simAvailable(sha256DigestBa);
            refreshSimIdentity();
        }
    } else {
        BLAME() << "SIM chanllenge error occurred:" << err;
        m_simChallengeComplete = true;
        m_dataBuffer.clear();
        m_randCounter = 0;
        emit error();
        refreshSimIdentity();
    }
}

void SimDataHandler::simStatusChanged(SIMStatus::Status status)
{
    TRACE() << simStatusAsStr(status);

    if ((m_lastSimStatus != SIMStatus::Ok) && (status == SIMStatus::Ok)) {
        TRACE() << "SIM inserted.";
        querySim();
    }

    if ((m_lastSimStatus == SIMStatus::Ok) && (status != SIMStatus::Ok)) {
        TRACE() << "SIM removed.";
        QByteArray simDataTmp = simData;
        simData.clear();
        emit simRemoved(simDataTmp);
    }

    m_lastSimStatus = status;
}

void SimDataHandler::simStatusComplete(SIMStatus::Status status, SIMError err)
{
    if (err == Cellular::SIM::NoError) {
        m_lastSimStatus = status;
        TRACE() << "Initial SIM status check:" << simStatusAsStr(status);
    } else {
        TRACE() << "Error occurred at initial SIM status check:" << err;
    }
}

bool SimDataHandler::isValid()
{
    return (m_simIdentity->isValid() && m_simStatus->isValid());
}

bool SimDataHandler::isSimPresent()
{
    return (m_lastSimStatus == SIMStatus::Ok);
}

bool SimDataHandler::isSimActive()
{
    return isSimPresent() && !simData.isEmpty();
}

void SimDataHandler::querySim()
{
    m_simChallengeComplete = false;

    QByteArray ba(SIM_RAND_BASE_SIZE, '0');
    ba.append(QByteArray::number(m_randCounter));
    m_simIdentity->auth(ba);
    m_randCounter++;

    if (m_randCounter == SIM_AUTH_COUNT) {
        m_randCounter = 0;
        m_simChallengeComplete = true;
    }
}

