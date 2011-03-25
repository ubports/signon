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
#ifndef SASLDATA_H
#define SASLDATA_H

#include <SignOn/SessionData>

namespace SaslPluginNS {

/*!
 * @class SaslData
 * Data container to hold values for authentication session.
 */
class SaslData : public SignOn::SessionData
{
public:
    /*!
     * @enum SaslData::State
     * State of the authentication process.
     */
    enum State {
        DONE,           /*!< Authentication is finished */
        CONTINUE,       /*!< Authentication in progress */
    };

    /*!
     * The challenge received from the remote server.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QByteArray, Challenge);

    /*!
     * The response computed by the SASL plugin.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QByteArray, Response);

    /*!
     * SASL authentication name.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Authname);

    /*!
     * SASL realm.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Realm);

    /*!
     * SASL service.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Service);

    /*!
     * SASL FQDN (Fully Qualified Domain Name).
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Fqdn);

    /*!
     * SASL local IP address.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, IpLocal);

    /*!
     * SASL remote IP address.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, IpRemote);

    /*!
     * Mechanism chosen after the SASL mechanism negotiation.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, ChosenMechanism);

    /*!
     * State of the authentication.
     * @sa SaslPluginNS::SaslData::State.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(qint32, state);
};

}  // namespace SaslPluginNS

#endif // SASLDATA_H
