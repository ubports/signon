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

/*!
 * @file simdatahandler.h
 * @brief Definition of the SIM data handler object.
 */

#ifndef SIMDATAHANDLER_H
#define SIMDATAHANDLER_H

#include <QObject>

#ifdef SIGNON_USES_CELLULAR_QT
    #include <SIM>
    using namespace Cellular::SIM;
#endif


namespace SignonDaemonNS {

    /*!
     * @class SimDataHandler
     * SimDataHandler handles aqcuaring data from the device SIM card
     * which is used by the CryptoManager as a key for the mounting of the encrypted file system.
     * @ingroup Accounts_and_SSO_Framework
     * SimDataHandler inherits QObject.
     * @sa CryptoManager
     */
    class SimDataHandler : public QObject
    {
        Q_OBJECT

    public:
        /*!
          * Constructs a SimDataHandler object with the given parent.
          * @param parent the parent object
          * @sa SimDataHandler(const CodeType &type, QObject *parent = 0)
          */
        SimDataHandler(QObject *parent = 0);

        /*!
          * Destructor, releases allocated resources.
          */
        virtual ~SimDataHandler();

        /*!
          * @returns true uppon success. Error handling not supported yet.
          */
        bool isValid();

        /*!
          * Queries the SIM for authentication info
          * @sa simAvailable(const QByteArray &simData) is emitted if the query is successful.
          * @sa error() is emitted otherwise.
          */
        void querySim();

    Q_SIGNALS:
        /*!
            Is emitted when a the SIM data is available.
            Can be triggered because of a successful explicit querySim call,
            or automatically in the case the SIM has been changed.
            @param simData, the SIM's data.
        */
        void simAvailable(const QByteArray &simData);

        /*!
            Emitted when SIM was removed.
         */
        void simRemoved();

        /*!
            Emitted when SIM challenge fails.
         */
        void error();

#ifdef SIGNON_USES_CELLULAR_QT
    private Q_SLOTS:
        void authComplete(
            QByteArray res,
            QByteArray cipheringKey,
            QByteArray eapCipheringKey,
            QByteArray eapIntegrityKey,
            SIMError error);

        void simStatusChanged(SIMStatus::Status status);

    private:
        void refreshSimIdentity();
#endif

    private:
        QByteArray m_dataBuffer;
        bool m_simChallengeComplete;

#ifdef SIGNON_USES_CELLULAR_QT
        SIMStatus::Status m_lastSimStatus;
        SIMIdentity *m_simIdentity;
        SIMStatus *m_simStatus;
#endif
    };

} // namespace SignonDaemonNS

#endif // SIMDATAHANDLER_H
