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
/*!
 * @copyright Copyright (C) 2009-2011 Nokia Corporation.
 * @license LGPL
 */

#ifndef SASLPLUGIN_H_
#define SASLPLUGIN_H_

#include <QtCore>

#include <sasl/sasl.h>

#include "SignOn/SessionData"
#include "SignOn/AuthPluginInterface"
#include "sasldata.h"

class SaslPluginTest;
namespace SaslPluginNS {

/*!
 * @class SaslPlugin
 * SASL authentication plugin.
 */
class SaslPlugin : public AuthPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(AuthPluginInterface)
    friend class ::SaslPluginTest;
public:
    SaslPlugin(QObject *parent = 0);
    ~SaslPlugin();
public Q_SLOTS:
    QString type() const;
    QStringList mechanisms() const;
    void cancel();
    void process(const SignOn::SessionData &inData, const QString &mechanism = 0);
private:
    class Private;
    static int sasl_callback(void *context, int id,
                             const char **result, unsigned *len);
    static int sasl_get_realm(void *context, int id,
                              const char **availrealms, const char **result);
    static int sasl_get_secret(sasl_conn_t *conn, void *context,
                               int id, sasl_secret_t **psecret);
    static int sasl_log(void *context, int priority, const char *message);
    void set_callbacks();
    bool check_and_fix_parameters(SaslData &input);
    Private *d; // Owned.
};

} //namespace SaslPluginNS

#endif /* SASLPLUGIN_H_ */
