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
    enum State {
        DONE,
        CONTINUE,
    };

    /*!
     * Declare property Example setter and getter
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QByteArray, Challenge);
    SIGNON_SESSION_DECLARE_PROPERTY(QByteArray, Response);
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Authname);
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Realm);
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Service);
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Fqdn);
    SIGNON_SESSION_DECLARE_PROPERTY(QString, IpLocal);
    SIGNON_SESSION_DECLARE_PROPERTY(QString, IpRemote);
    SIGNON_SESSION_DECLARE_PROPERTY(qint32, state);
};

}  // namespace SaslPluginNS

#endif // SASLDATA_H
