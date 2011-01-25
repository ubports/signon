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

#include <sasl/sasl.h>
#include <sasl/saslutil.h>

#include "saslplugin.h"

#include "SignOn/signonplugincommon.h"

#define N_CALLBACKS (16)
#define SAMPLE_SEC_BUF_SIZE (2048)

namespace SaslPluginNS {

class SaslPlugin::Private
{
public:
    Private()
    {
        TRACE();
        m_conn = NULL;

        /* Init defaults... */
        memset(&m_secprops, 0L, sizeof(m_secprops));
        m_secprops.maxbufsize = SAMPLE_SEC_BUF_SIZE;
        m_secprops.max_ssf = UINT_MAX;
        m_secprops.min_ssf = 0;
        m_secprops.security_flags = 0;
        m_psecret = NULL;
    }

    ~Private() {
        TRACE();

        if (m_conn) {
            sasl_dispose(&m_conn);
            m_conn = NULL;
        }
        if (m_psecret) {
            free(m_psecret);
            m_psecret = NULL;
        }
    }

    static SignOn::Error mapSaslError(int res);

    sasl_callback_t m_callbacks[N_CALLBACKS];
    sasl_conn_t *m_conn;
    sasl_security_properties_t m_secprops;
    sasl_secret_t *m_psecret;

    SaslData m_input;
    QByteArray m_username;
    QByteArray m_authname;
    QByteArray m_realm;
};

SignOn::Error SaslPlugin::Private::mapSaslError(int res)
{
    using SignOn::Error;

    switch (res) {
    case SASL_OK:
        return 0;
    case SASL_BADPARAM:
        return Error::InvalidQuery;
    case SASL_NOMECH:
        return Error::MechanismNotAvailable;
    default:
        return Error::Unknown;
    }
}

SaslPlugin::SaslPlugin(QObject *parent)
    : AuthPluginInterface(parent)
    , d(new Private)
{
    TRACE();

    set_callbacks();

    int result = sasl_client_init(d->m_callbacks);
    if (result != SASL_OK) {
        TRACE() << "libsasl error";
    }
}

SaslPlugin::~SaslPlugin()
{
    TRACE();

    delete d;
    d = 0;

    sasl_done();
}

QString SaslPlugin::type() const
{
    TRACE();
    return QLatin1String("sasl");
}

QStringList SaslPlugin::mechanisms() const
{
    TRACE();
    QStringList res;
    const char **list;

    list = sasl_global_listmech();
    //covert array of strings to QStringlist
    while (*list) {
        res << QLatin1String(*list);
        list++;
    }
    return res;
}

void SaslPlugin::cancel()
{
    TRACE();
    //nothing to do for cancel
}

void SaslPlugin::process(const SignOn::SessionData &inData,
                         const QString &mechanism)
{
    TRACE();

    int serverlast = 0;
    const char *data = "";
    unsigned len = 0;
    const char *chosenmech = NULL;
    int res = 0;
    QByteArray buf;
    SaslData response;
    //get input parameters
    d->m_input = inData.data<SaslData>();

    TRACE() << "mechanism: " << mechanism;

    using SignOn::Error;

    if (!check_and_fix_parameters(d->m_input)) {
        TRACE() << "missing parameters";
        emit error(Error::MissingData);
        return;
    }

    //check state
    if (d->m_input.state() == SaslData::CONTINUE && !d->m_conn) {
        TRACE() << "init not done for CONTINUE";
        emit error(Error::WrongState);
        return;
    }

    //initial connection
    if (d->m_input.state() != SaslData::CONTINUE) {
        res = sasl_client_new(d->m_input.Service().toUtf8().constData(),
                              d->m_input.Fqdn().toUtf8().constData(),
                              d->m_input.IpLocal().toUtf8().constData(),
                              d->m_input.IpRemote().toUtf8().constData(),
                              NULL, serverlast,
                              &(d->m_conn));

        if (res != SASL_OK) {
            TRACE() << "err Allocating sasl connection state";
            emit error(Private::mapSaslError(res));
            return;
        }

        res = sasl_setprop(d->m_conn,
                           SASL_SEC_PROPS,
                           &(d->m_secprops));

        if (res != SASL_OK) {
            TRACE() << "err Setting security properties";
            emit error(Private::mapSaslError(res));
            return;
        }

        res = sasl_client_start(d->m_conn,
                                mechanism.toUtf8().constData(),
                                NULL,
                                &data,
                                &len,
                                &chosenmech);

        if (res != SASL_OK && res != SASL_CONTINUE) {
            TRACE() << "err Starting SASL negotiation";
            emit error(Private::mapSaslError(res));
            return;
        }

        TRACE() << chosenmech;

        response.setChosenMechanism(QByteArray(chosenmech));

        buf.clear();
        if (res == SASL_CONTINUE) {
            buf = d->m_input.Challenge();
        } else {
            buf.append(chosenmech);
        }

        if (data) {
            buf.append('\0');
            buf.append(data, len);
        }

    } else {
        res = SASL_CONTINUE;
        buf = d->m_input.Challenge();
    }

    TRACE() <<buf;
    //here we have initial response
    if (res == SASL_CONTINUE && buf.count() > 0) {
        res = sasl_client_step(d->m_conn, buf.constData(),
                               buf.count(), NULL,
                               &data, &len);
    }

    if (res != SASL_OK && res != SASL_CONTINUE) {
        TRACE() << "err Performing SASL negotiation";
        emit error(Private::mapSaslError(res));
        return;
    }

    //and here we have response for server
    if (data && len) {
        response.setResponse(QByteArray(data, len));
    }

    //Negotiation complete
    SaslData::State state;
    if (res == SASL_CONTINUE) {
        state = SaslData::CONTINUE;
    } else {
        state = SaslData::DONE;
        sasl_dispose(&d->m_conn);
        d->m_conn = NULL;
    }

    //set state into info
    response.setstate(state);
    emit result(response);
    return;
}

//private functions

int SaslPlugin::sasl_callback(void *context, int id,
                              const char **result, unsigned *len)
{
    TRACE();
    if (context == NULL)
        return SASL_BADPARAM;

    SaslPlugin *self = (SaslPlugin *)context;

    if (!result)
        return SASL_BADPARAM;

    switch (id) {
    case SASL_CB_USER:
        self->d->m_username = self->d->m_input.UserName().toUtf8();
        *result = self->d->m_username.constData();
        if (len)
            *len = self->d->m_username.count();
        break;
    case SASL_CB_AUTHNAME:
        self->d->m_authname = self->d->m_input.Authname().toUtf8();
        *result = self->d->m_authname.constData();
        if (len)
            *len = self->d->m_authname.count();
        break;
    case SASL_CB_LANGUAGE:
        *result = NULL;
        if (len)
            *len = 0;
        break;
    default:
        return SASL_BADPARAM;
    }
    TRACE();
    return SASL_OK;
}

int SaslPlugin::sasl_get_realm(void *context, int id,
                               const char **availrealms, const char **result)
{
    Q_UNUSED(availrealms);
    TRACE();
    if (id != SASL_CB_GETREALM) return SASL_FAIL;
    if (context == NULL) return SASL_BADPARAM;
    SaslPlugin *self = (SaslPlugin *)context;
    if (!result) return SASL_BADPARAM;
    self->d->m_realm = self->d->m_input.Realm().toUtf8();
    *result = self->d->m_realm.constData();
    return SASL_OK;
}

int SaslPlugin::sasl_get_secret(sasl_conn_t *conn,
                                void *context,
                                int id,
                                sasl_secret_t **psecret)
{
    Q_UNUSED(conn);
    TRACE();
    if (context == NULL) return SASL_BADPARAM;

    SaslPlugin *self = (SaslPlugin *)context;
    char *password;
    unsigned len;

    if (!psecret || id != SASL_CB_PASS)
        return SASL_BADPARAM;
    QByteArray secret = self->d->m_input.Secret().toUtf8();
    password = secret.data();
    if (!password)
        return SASL_FAIL;

    len = secret.count();
    if (self->d->m_psecret)
        free(self->d->m_psecret);
    self->d->m_psecret = (sasl_secret_t *) malloc(sizeof(sasl_secret_t) + len);

    *psecret = self->d->m_psecret;

    if (!*psecret) {
        return SASL_NOMEM;
    }
    (*psecret)->len = len;
    memcpy((char *)(*psecret)->data, password, len);

    TRACE();
    return SASL_OK;
}

int SaslPlugin::sasl_log(void *context,
                         int priority,
                         const char *message)
{
    Q_UNUSED(context);
    Q_UNUSED(priority);
    if (!message)
        return SASL_BADPARAM;

    TRACE() << message;
    return SASL_OK;
}

//TODO move to private
void SaslPlugin::set_callbacks()
{
    TRACE();
    sasl_callback_t *callback;
    callback = d->m_callbacks;

    /* log */
    callback->id = SASL_CB_LOG;
    callback->proc = (int(*)())(&SaslPluginNS::SaslPlugin::sasl_log);
    callback->context = this;
    ++callback;

    /* user */
    callback->id = SASL_CB_USER;
    callback->proc = (int(*)())(&SaslPluginNS::SaslPlugin::sasl_callback);
    callback->context = this;
    ++callback;

    /* authname */
    callback->id = SASL_CB_AUTHNAME;
    callback->proc = (int(*)())(&SaslPluginNS::SaslPlugin::sasl_callback);
    callback->context = this;
    ++callback;

    /* password */
    callback->id = SASL_CB_PASS;
    callback->proc = (int(*)())(&SaslPluginNS::SaslPlugin::sasl_get_secret);
    callback->context = this;
    ++callback;

    /* realm */
    callback->id = SASL_CB_GETREALM;
    callback->proc = (int(*)())(&SaslPluginNS::SaslPlugin::sasl_get_realm);
    callback->context = this;
    ++callback;

    /* termination */
    callback->id = SASL_CB_LIST_END;
    callback->proc = NULL;
    callback->context = NULL;
    ++callback;

}

bool SaslPlugin::check_and_fix_parameters(SaslData &input)
{
    TRACE();
    if (input.UserName().isEmpty())
        return false;

    //set default parameters
    if (input.Service().isEmpty()) input.setService(QByteArray("default"));
    if (input.Authname().isEmpty()) input.setAuthname(input.UserName());
    if (input.Fqdn().isEmpty()) {
        if (!input.Realm().isEmpty())
            input.setFqdn(input.Realm());
        else
            input.setFqdn(QByteArray("default"));
    }
    if (input.IpLocal().isEmpty()) input.setIpLocal(QByteArray("127.0.0.1"));
    if (input.IpRemote().isEmpty()) input.setIpRemote(QByteArray("127.0.0.1"));

    return true;
}

SIGNON_DECL_AUTH_PLUGIN(SaslPlugin)

} //namespace SaslPluginNS
