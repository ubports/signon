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

#ifndef SIGNONSESSIONCORE_H_
#define SIGNONSESSIONCORE_H_

#include <QtCore>
#include <QtDBus>

/*
 * TODO: remove invocation of plugin operations into the main signond process
 * */

#include "pluginproxy.h"

#include "signond-common.h"

#include "signondaemon.h"
#include "signondisposable.h"
#include "credentialsaccessmanager.h"
#include "signonui_interface.h"

using namespace SignOn;

namespace SignonDaemonNS {

    /*!
     * @class RequestData
     * Request data.
     * @todo description.
     */
    class RequestData
    {
        public:
            RequestData(const QDBusConnection &conn,
                        const QDBusMessage &msg,
                        const QVariantMap &params,
                        const QString &mechanism,
                        const QString &cancelKey):
                        m_conn(conn),
                        m_msg(msg),
                        m_params(params),
                        m_mechanism(mechanism),
                        m_cancelKey(cancelKey)
            {}

            RequestData(const RequestData &other):
                        m_conn(other.m_conn),
                        m_msg(other.m_msg),
                        m_params(other.m_params),
                        m_mechanism(other.m_mechanism),
                        m_cancelKey(other.m_cancelKey)
            {
            }

            ~RequestData()
            {}

            QDBusConnection m_conn;
            QDBusMessage m_msg;
            QVariantMap m_params;
            QString m_mechanism;
            QString m_cancelKey;
    };

    /*!
     * @class SignonAuthSession
     * Daemon side representation of authentication session.
     * @todo description.
     */
    class SignonSessionCore: public SignonDisposable
    {
        Q_OBJECT

    public:
        static SignonSessionCore *sessionCore(const quint32 id, const QString &method, SignonDaemon *parent);
        virtual ~SignonSessionCore();
        quint32 id() const;
        QString method() const;
        bool setupPlugin();
        /*
         * just for any case
         * */
        static void stopAllAuthSessions();
        static QStringList loadedPluginMethods(const QString &method);

        void destroy();

    public Q_SLOTS:
        QStringList queryAvailableMechanisms(const QStringList &wantedMechanisms);

        void process(const QDBusConnection &connection,
                     const QDBusMessage &message,
                     const QVariantMap &sessionDataVa,
                     const QString &mechanism,
                     const QString &cancelKey);

        void cancel(const QString &cancelKey);
        void setId(quint32 id);

    Q_SIGNALS:
        void stateChanged(const QString &requestId, int state, const QString &message);

    private Q_SLOTS:
        void startNewRequest();

        void processResultReply(const QString &cancelKey, const QVariantMap &data);
        void processUiRequest(const QString &cancelKey, const QVariantMap &data);
        void processRefreshRequest(const QString &cancelKey, const QVariantMap &data);
        void processError(const QString &cancelKey, int err, const QString &message);
        void stateChangedSlot(const QString &cancelKey, int state, const QString &message);

        void queryUiSlot(QDBusPendingCallWatcher *call);

    protected:
        SignonSessionCore(quint32 id, const QString &method, int timeout, SignonDaemon *parent);

        void childEvent(QChildEvent *ce);

    private:
        void startProcess();
        void replyError(const QDBusConnection &conn, const QDBusMessage &msg, int err, const QString &message);

        PluginProxy *m_plugin;
        QQueue<RequestData> m_listOfRequests;
        SignonUiAdaptor *m_signonui;

        QDBusPendingCallWatcher *m_watcher;

        QString m_canceled;

        quint32 m_id;
        QString m_method;

    Q_DISABLE_COPY(SignonSessionCore)
}; //class SignonDaemon

} //namespace SignonDaemonNS

#endif //SIGNONSESSIONQUEUE_H_
