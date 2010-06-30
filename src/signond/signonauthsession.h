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

#ifndef SIGNONAUTHSESSION_H_
#define SIGNONAUTHSESSION_H_

#include <QtCore>
#include <QtDBus>

/*
 * TODO: remove invocation of plugin operations into the main signond process
 * */
#include "signond-common.h"
#include "signonsessioncore.h"

using namespace SignOn;

namespace SignonDaemonNS {

    /*!
     * @class SignonAuthSession
     * Daemon side representation of authentication session.
     * @todo description.
     */
    class SignonAuthSession: public QObject, protected QDBusContext
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "com.nokia.singlesignon.SignonAuthSession")

    public:
        inline SignonSessionCore *parent() const
        {
            return static_cast<SignonSessionCore *>(QObject::parent());
        }

        static QString getAuthSessionObjectPath(const quint32 id, const QString &method, SignonDaemon *parent);
        static void stopAllAuthSessions();
        quint32 id() const;

    public Q_SLOTS:
        QStringList queryAvailableMechanisms(const QStringList &wantedMechanisms);
        QVariantMap process(const QVariantMap &sessionDataVa, const QString &mechanism);
        void cancel();
        void setId(quint32 id);
        void objectUnref();

    Q_SIGNALS:
        void stateChanged(int state, const QString &message);
        void remoteObjectDestroyed();

    private Q_SLOTS:
        void stateChangedSlot(const QString &sessionKey, int state, const QString &message);

    protected:
        SignonAuthSession(quint32 id, const QString &method);
        virtual ~SignonAuthSession();

    private:
        quint32 m_id;
        QString m_method;

    Q_DISABLE_COPY(SignonAuthSession)
}; //class SignonDaemon

} //namespace SignonDaemonNS

#endif //SIGNONAUTHSESSION_H_
