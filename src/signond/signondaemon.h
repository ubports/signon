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

extern "C" {
    #include <signal.h>
    #include <unistd.h>
    #include <errno.h>
    #include <stdio.h>
    #include <sys/types.h>
}

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

class QSocketNotifier;

namespace SignonDaemonNS {

/*!
 * @class SignonDaemonConfiguration
 * The daemon's configuration object;
 * loads date from the daemon configuration file.
 */
class SignonDaemonConfiguration
{
public:
    SignonDaemonConfiguration();
    ~SignonDaemonConfiguration();

    void load();
    bool loadedFromFile() const { return m_loadedFromFile; }
    bool useSecureStorage() const { return m_useSecureStorage; }

    uint storageSize() const { return m_storageSize; }
    const QString storageFileSystemType() const
        { return m_storageFileSystemType; }
    const QString storagePath() const { return m_storagePath; }
    const QString storageFileSystemName() const
        { return m_storageFileSystemName; }

    uint identityTimeout() const { return m_identityTimeout; }
    uint authSessionTimeout() const { return m_authSessionTimeout; }

private:
    bool m_loadedFromFile;
    //storage related
    bool m_useSecureStorage;

    //valid only if secure storage is enabled.
    uint m_storageSize;
    QString m_storageFileSystemType;
    QString m_storagePath;
    QString m_storageFileSystemName;

    //object timeouts
    uint m_identityTimeout;
    uint m_authSessionTimeout;
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
    Q_CLASSINFO("D-Bus Interface", "com.nokia.SingleSignOn.AuthService")

    friend class SignonIdentity;
    friend class SignonSessionCore;
    friend class SignonDaemonAdaptor;

public:
    static SignonDaemon *instance();
    virtual ~SignonDaemon();

    Q_INVOKABLE void init();

    /*!
     * Returns the number of seconds of inactivity after which identity
     * objects might be automatically deleted.
     */
    int identityTimeout() const;
    int authSessionTimeout() const;

public Q_SLOTS:
    /* Immediate reply calls */

    void registerNewIdentity(QDBusObjectPath &objectPath);
    void registerStoredIdentity(const quint32 id, QDBusObjectPath &objectPath,
                                QList<QVariant> &identityData);
    QString getAuthSessionObjectPath(const quint32 id, const QString type);

    QStringList queryMethods();
    QStringList queryMechanisms(const QString &method);
    QList<QVariant> queryIdentities(const QMap<QString, QVariant> &filter);
    bool clear();

    /* Delayed reply calls */

    // Interface method to set the device lock code
    bool setDeviceLockCode(const QByteArray &lockCode,
                           const QByteArray &oldLockCode);

    // Interface method to drop the database
    bool remoteLock(const QByteArray &lockCode);

public Q_SLOTS: // backup METHODS
    uchar backupStarts();
    uchar backupFinished();
    uchar restoreStarts();
    uchar restoreFinished();

private:
    SignonDaemon(QObject *parent);
    void initExtensions();
    void initExtension(const QString &filePath);
    bool initSecureStorage(const QByteArray &lockCode);

    void unregisterIdentity(SignonIdentity *identity);
    void identityStored(SignonIdentity *identity);
    void setupSignalHandlers();
    void listDBusInterfaces();

private:
    /*
     * The list of created SignonIdentities
     * */
    QMap<quint32, SignonIdentity *> m_storedIdentities;
    QMap<QString, SignonIdentity *> m_unstoredIdentities;

    SignonDaemonConfiguration *m_configuration;

    /*
     * The instance of CAM
     * */
    CredentialsAccessManager *m_pCAMManager;

    bool m_backup;

    int m_identityTimeout;
    int m_authSessionTimeout;

    /*
     * UNIX signals handling related
     * */
public:
    static void signalHandler(int signal);

public Q_SLOTS:
    void handleUnixSignal();

private:
    QSocketNotifier *m_sigSn;
    static SignonDaemon *m_instance;
}; //class SignonDaemon

} //namespace SignonDaemonNS

#endif /* SIGNONDAEMON_H_ */
