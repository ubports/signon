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
 * @file accesscodehandler.h
 * @brief Definition of the access code (sim, lock code) handler object.
 */

#ifndef ACCESSCODEHANDLER_H
#define ACCESSCODEHANDLER_H

#include <QObject>
#include <QDBusError>

#include <SIM>

using namespace Cellular::SIM;

namespace SignonDaemonNS {

    /*!
     * @class AccessCodeHandler
     * AccessCodeHandler handles getting the access code (SIM data only for the moment)
     * which is used by the CryptoManager as a key for the mounting of the encrypted file system.
     * @ingroup Accounts_and_SSO_Framework
     * AccessCodeHandler inherits QObject.
     * @sa CryptoManager
     * @todo Rename this object (SIM specific) if no connection to the device lock code will come up.
     */
    class AccessCodeHandler : public QObject
    {
        Q_OBJECT

    public:
        /*!
          * Constructs an AcessCodeHandler object with the given parent.
          * @param parent the parent object
          * @sa AccessCodeHandler(const CodeType &type, QObject *parent = 0)
          */
        AccessCodeHandler(QObject *parent = 0);

        /*!
          * Destructor, releases allocated resources.
          */
        virtual ~AccessCodeHandler();

        /*!
          *
          *
          * @retval true uppon success. Error handling not supported yet.
          */
        bool isValid();

        /*!
          *
          *
          * @retval true uppon success. Error handling not supported yet.
          */
        void querySim();

        /*!
         * @retval true, if an access code is available for using, false otherwise.
         */
        bool codeAvailable();

        /*!
            @returns the currently available access code.
        */
        QByteArray currentCode() const;

    Q_SIGNALS:
        /*!
            Is emitted when a the SIM data (icc-id) is available.
            @param simData, the SIM's icc-id.
        */
        void simAvailable(const QByteArray &simData);

        /*!
            Is emitted when the SIM is changed.
            @param simData, the new SIM's icc-id.
        */
        void simChanged(const QByteArray &simData);

    private Q_SLOTS:
        void simIccidComplete(QString iccid, SIMError error);
        void simStatusChanged(SIMStatus::Status status);

    private:
        QByteArray m_code;
        SIMStatus::Status m_lastSimStatus;
        SIMIdentity *m_simIdentity;
        SIMStatus *m_simStatus;
    };

} // namespace SignonDaemonNS

#endif // ACCESSCODEHANDLER_H
