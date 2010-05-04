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

#ifndef SIGNONDAEMON_H_
#define SIGNONDAEMON_H_

#include <QtCore>
#include <QtDBus>

#include "credentialsaccessmanager.h"

#ifndef SIGNOND_PLUGINS_DIR
    #define SIGNOND_PLUGINS_DIR QLatin1String("/usr/lib/signon")
#endif

#ifndef SIGNOND_PLUGIN_PREFIX
    #define SIGNOND_PLUGIN_PREFIX QLatin1String("lib")
#endif

#ifndef SIGNOND_PLUGIN_SUFFIX
    #define SIGNOND_PLUGIN_SUFFIX QLatin1String("plugin.so")
#endif

namespace SignonDaemonNS {

    /*!
     * Request counter, small info class.
     */
    class RequestCounter
    {
        RequestCounter() : m_serviceRequests(0), m_identityRequests(0)
        {}

    public:
        static RequestCounter* instance()
        {
            if (m_pInstance == NULL)
                m_pInstance = new RequestCounter();

            return m_pInstance;
        }

        void addServiceResquest() { ++m_serviceRequests; }
        void addIdentityResquest() { ++m_identityRequests; }

        int serviceRequests() const { return m_serviceRequests; }
        int identityRequests() const { return m_identityRequests; }

    private:
        static RequestCounter *m_pInstance;
        int m_serviceRequests;
        int m_identityRequests;
    };

    class SignonIdentity;

    /*!
     * @class SignonDaemon
     * Daemon core.
     * @todo description.
     */
    class SignonDaemon: public QObject, protected QDBusContext
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "com.nokia.singlesignon.SignonDaemon")

        friend class SignonIdentity;
        friend class SignonSessionCore;
        friend class SignonDaemonAdaptor;

    public:
        SignonDaemon(QObject *parent);
        ~SignonDaemon();

        bool init(bool backup);

        /*!
         * Returns the number of seconds of inactivity after which identity
         * objects might be automatically deleted.
         */
        int identityTimeout() const
        {
            return m_identityTimeout;
        }

    public Q_SLOTS:
        bool initSecureStorage(const QByteArray &lockCode);

        void registerNewIdentity(QDBusObjectPath &objectPath);
        void registerStoredIdentity(const quint32 id, QDBusObjectPath &objectPath,
                                    QList<QVariant> &identityData);

        QStringList queryMethods();
        QStringList queryMechanisms(const QString &method);
        QList<QVariant> queryIdentities(const QMap<QString, QVariant> &filter);
        bool clear();

        QString getAuthSessionObjectPath(const quint32 id, const QString type);

        // Interface method to set the device lock code
        bool setDeviceLockCode(const QByteArray &oldLockCode,
                               const QByteArray &newLockCode);

        // Interface method to set the sim
        bool setSim(const QByteArray &simData,
                    const QByteArray &checkData);

        // Interface method to remote lock the database
        bool remoteLock(const QByteArray &lockCode);

    public Q_SLOTS: // backup METHODS
        uchar backup(const QStringList &selectedCategories);
        bool close();
        bool prestart();
        uchar restore(const QStringList &selectedCategories,
                      const QString &productName,
                      const QStringList &restoredFilePaths);

    protected Q_SLOTS:
        void displayRequestsCount();

    private:
        void unregisterIdentity(SignonIdentity *identity);
        void identityStored(SignonIdentity *identity);
        void listDBusInterfaces();

    private:
        /*
         * The list of created SignonIdentities
         * */
        QMap<quint32, SignonIdentity *> m_storedIdentities;
        QMap<QString, SignonIdentity *> m_unstoredIdentities;

        /*
         * The instance of CAM
         * */
        CredentialsAccessManager *m_pCAMManager;

        bool m_backup;

        int m_identityTimeout;
    }; //class SignonDaemon

} //namespace SignonDaemonNS

#endif /* SIGNONDAEMON_H_ */
