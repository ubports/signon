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

#include <QtDebug>
#include <QDBusConnection>

#include "signondaemon.h"
#include "signond-common.h"
#include "signondaemonadaptor.h"
#include "signonidentity.h"
#include "signonauthsession.h"
#include "backupifadaptor.h"

#define IDENTITY_MAX_IDLE_TIME (60 * 5) // five minutes
#define AUTHSESSION_MAX_IDLE_TIME (60 * 5) // five minutes

#define SIGNON_RETURN_IF_CAM_UNAVAILABLE(_ret_arg_) do {                   \
        if (m_pCAMManager && !m_pCAMManager->credentialsSystemOpened()) {  \
            QDBusMessage errReply = message().createErrorReply(            \
                    internalServerErrName,                                 \
                    internalServerErrStr + QLatin1String("Could not access Signon Database.")); \
            SIGNOND_BUS.send(errReply); \
            return _ret_arg_;           \
        }                               \
    } while(0)

using namespace SignOn;

namespace SignonDaemonNS {

    RequestCounter *RequestCounter::m_pInstance = NULL;

    const QString internalServerErrName = SIGNOND_INTERNAL_SERVER_ERR_NAME;
    const QString internalServerErrStr = SIGNOND_INTERNAL_SERVER_ERR_STR;

    SignonDaemon::SignonDaemon(QObject *parent) : QObject(parent)
    {
        m_backup = false;
        m_pCAMManager = CredentialsAccessManager::instance();
    }

    SignonDaemon::~SignonDaemon()
    {
        if (m_backup) exit(0);

        SignonAuthSession::stopAllAuthSessions();
        m_storedIdentities.clear();
        m_unstoredIdentities.clear();

        if (m_pCAMManager) {
            m_pCAMManager->closeCredentialsSystem();
            m_pCAMManager->deleteLater();
        }

        QDBusConnection sessionConnection = QDBusConnection::sessionBus();

        sessionConnection.unregisterObject(SIGNOND_DAEMON_OBJECTPATH
                                           + QLatin1String("/Backup"));
        sessionConnection.unregisterService(SIGNOND_SERVICE
                                            + QLatin1String(".Backup"));

        if (m_backup == false)
        {
            sessionConnection.unregisterObject(SIGNOND_DAEMON_OBJECTPATH);
            sessionConnection.unregisterService(SIGNOND_SERVICE);
        }

        delete RequestCounter::instance();
    }

    bool SignonDaemon::init(bool backup)
    {
        m_backup = backup;

        /* backup dbus interface */
        QDBusConnection sessionConnection = QDBusConnection::sessionBus();

        if (!sessionConnection.isConnected()) {
            QDBusError err = sessionConnection.lastError();
            TRACE() << "Session connection cannot be established:" << err.errorString(err.type());
            TRACE() << err.message();
            return false;
        }

        QDBusConnection::RegisterOptions registerSessionOptions = QDBusConnection::ExportAdaptors;

        (void)new BackupIfAdaptor(this);

        if (!sessionConnection.registerObject(SIGNOND_DAEMON_OBJECTPATH
                                              + QLatin1String("/Backup"), this, registerSessionOptions)) {
            TRACE() << "Object cannot be registered";
            return false;
        }

        if (!sessionConnection.registerService(SIGNOND_SERVICE+QLatin1String(".Backup"))) {
            QDBusError err = sessionConnection.lastError();
            TRACE() << "Service cannot be registered: " << err.errorString(err.type());
            return false;
        }

        if (m_backup) {
            TRACE() << "Signond initialized in backup mode.";
            //skit rest of initialization in backup mode
            return true;
        }

        /* DBus Service init */
        QDBusConnection connection = SIGNOND_BUS;

        if (!connection.isConnected()) {
            QDBusError err = connection.lastError();
            TRACE() << "Connection cannot be established:" << err.errorString(err.type());
            TRACE() << err.message();
            return false;
        }

        int envIdentityTimeout = qgetenv("SSO_IDENTITY_TIMEOUT").toInt();
        m_identityTimeout = envIdentityTimeout > 0 ?
            envIdentityTimeout : IDENTITY_MAX_IDLE_TIME;

        int envAuthSessionTimeout = qgetenv("SSO_AUTHSESSION_TIMEOUT").toInt();
        m_authSessionTimeout = envAuthSessionTimeout > 0 ?
            envAuthSessionTimeout : AUTHSESSION_MAX_IDLE_TIME;

        QDBusConnection::RegisterOptions registerOptions = QDBusConnection::ExportAllContents;

        (void)new SignonDaemonAdaptor(this);
        registerOptions = QDBusConnection::ExportAdaptors;

        if (!connection.registerObject(SIGNOND_DAEMON_OBJECTPATH, this, registerOptions)) {
            TRACE() << "Object cannot be registered";
            return false;
        }

        if (!connection.registerService(SIGNOND_SERVICE)) {
            QDBusError err = connection.lastError();
            TRACE() << "Service cannot be registered: " << err.errorString(err.type());
            return false;
        }

        /*
            Secure storage init
            TODO - make this callable only by external entity
            (e.g. boot script or whichever process has the passphrase)
        */
        if (!initSecureStorage(QByteArray()))
            qFatal("Signond: Cannot initialize credentials secure storage.");

        TRACE() << "Signond SUCCESSFULLY initialized.";

        // TODO - remove this
        QTimer *requestCounterTimer = new QTimer(this);
        requestCounterTimer->setInterval(500000);
        connect(requestCounterTimer,
                SIGNAL(timeout()),
                this,
                SLOT(displayRequestsCount()));
        requestCounterTimer->start();
        //TODO - end

        return true;
    }

    bool SignonDaemon::initSecureStorage(const QByteArray &lockCode)
    {
        m_pCAMManager = CredentialsAccessManager::instance();

        if (!m_pCAMManager->credentialsSystemOpened()) {
            m_pCAMManager->finalize();

            CAMConfiguration config;

            //Leaving encryption disabled for the moment.
            config.m_useEncryption = false;
            config.m_encryptionPassphrase = lockCode;

            if (!m_pCAMManager->init(config)) {
                qCritical("Signond: Cannot set proper configuration of CAM");
                delete m_pCAMManager;
                m_pCAMManager = NULL;
                return false;
            }

            //If not encryption in use just init the storage here - unsecure
            if (config.m_useEncryption == false) {
                if (!m_pCAMManager->openCredentialsSystem()) {
                    qCritical("Signond: Cannot open CAM credentials system...");
                    delete m_pCAMManager;
                    m_pCAMManager = NULL;
                    return false;
                }
            }
        } else {
            TRACE() << "Secure storage already initialized...";
            return false;
        }

        return true;
    }

    void SignonDaemon::displayRequestsCount()
    {
        TRACE() << "\n\n\nUnstored identities:" << m_unstoredIdentities.count()
                << "\nStored identities:" << m_storedIdentities.count()
                << "\nService requests:" << RequestCounter::instance()->serviceRequests()
                << "\nIdentity requests:" << RequestCounter::instance()->identityRequests()
                << "\n\n";
    }

    void SignonDaemon::unregisterIdentity(SignonIdentity *identity)
    {
        if (m_storedIdentities.contains(identity->id()))
            m_storedIdentities.remove(identity->id());
        else
            m_unstoredIdentities.remove(identity->objectName());

        identity->deleteLater();
    }

    void SignonDaemon::identityStored(SignonIdentity *identity)
    {
        if (m_unstoredIdentities.contains(identity->objectName())) {
            m_unstoredIdentities.remove(identity->objectName());
            m_storedIdentities.insert(identity->id(), identity);
        }
    }

    void SignonDaemon::registerNewIdentity(QDBusObjectPath &objectPath)
    {
        RequestCounter::instance()->addServiceResquest();

        TRACE() << "Registering new identity:";

        SignonIdentity *identity = SignonIdentity::createIdentity(SIGNOND_NEW_IDENTITY, this);

        if (identity == NULL) {
            QDBusMessage errReply = message().createErrorReply(
                    internalServerErrName,
                    internalServerErrStr + QLatin1String("Could not create remote Identity object."));
            SIGNOND_BUS.send(errReply);
            return;
        }

        m_unstoredIdentities.insert(identity->objectName(), identity);

        objectPath = QDBusObjectPath(identity->objectName());
    }

    void SignonDaemon::registerStoredIdentity(const quint32 id, QDBusObjectPath &objectPath, QList<QVariant> &identityData)
    {
        RequestCounter::instance()->addServiceResquest();

        SIGNON_RETURN_IF_CAM_UNAVAILABLE();

        TRACE() << "Registering identity:" << id;

        //1st check if the existing identity is in cache
        SignonIdentity *identity = m_storedIdentities.value(id, NULL);

        //if not create it
        if (identity == NULL)
            identity = SignonIdentity::createIdentity(id, this);

        if (identity == NULL)
        {
            QDBusMessage errReply = message().createErrorReply(
                    internalServerErrName,
                    internalServerErrStr + QLatin1String("Could not create remote Identity object."));
            SIGNOND_BUS.send(errReply);
            return;
        }

        bool ok;
        SignonIdentityInfo info = identity->queryInfo(ok);

        if (info.m_id == 0)
        {
            QDBusMessage errReply = message().createErrorReply(
                                                            SIGNOND_IDENTITY_NOT_FOUND_ERR_NAME,
                                                            SIGNOND_IDENTITY_NOT_FOUND_ERR_STR);
            SIGNOND_BUS.send(errReply);
            objectPath = QDBusObjectPath();
            return;
        }

        //cache the identity as stored
        m_storedIdentities.insert(identity->id(), identity);
        identity->keepInUse();

        identityData = info.toVariantList();

        TRACE() << "DONE REGISTERING IDENTITY";
        objectPath = QDBusObjectPath(identity->objectName());
    }

    QStringList SignonDaemon::queryMethods()
    {
        RequestCounter::instance()->addServiceResquest();

        QDir pluginsDir(SIGNOND_PLUGINS_DIR);
        //TODO: in the future remove the sym links comment
        QStringList fileNames = pluginsDir.entryList(
                QStringList() << QLatin1String("*.so*"), QDir::Files | QDir::NoDotAndDotDot);

        QStringList ret;
        QString fileName;
        foreach (fileName, fileNames) {
            if (fileName.startsWith(QLatin1String("lib"))) {
                fileName = fileName.mid(3, fileName.indexOf(QLatin1String("plugin")) -3);
                if ((fileName.length() > 0) && !ret.contains(fileName))
                    ret << fileName;
            }
        }

        return ret;
    }

    QStringList SignonDaemon::queryMechanisms(const QString &method)
    {
        RequestCounter::instance()->addServiceResquest();
        TRACE() << "\n\n\n Querying mechanisms\n\n";

        QStringList mechs = SignonSessionCore::loadedPluginMethods(method);

        if (mechs.size())
            return mechs;

        PluginProxy *plugin = PluginProxy::createNewPluginProxy(method);

        if (!plugin) {
            TRACE() << "Could not load plugin of type: " << method;
            QDBusMessage errReply = message().createErrorReply(
                    SIGNOND_METHOD_NOT_KNOWN_ERR_NAME,
                    QString(SIGNOND_METHOD_NOT_KNOWN_ERR_STR
                            + QLatin1String("Method %1 is not known or could not load specific configuration.")).arg(method));
            SIGNOND_BUS.send(errReply);
            return QStringList();
        }

        mechs = plugin->mechanisms();
        delete plugin;

        return mechs;
    }


    QList<QVariant> SignonDaemon::queryIdentities(const QMap<QString, QVariant> &filter)
    {
        RequestCounter::instance()->addServiceResquest();

        SIGNON_RETURN_IF_CAM_UNAVAILABLE(QList<QVariant>());

        TRACE() << "\n\n\n Querying identities\n\n";

        CredentialsDB *db = m_pCAMManager->credentialsDB();
        if (!db) {
            qCritical() << Q_FUNC_INFO << m_pCAMManager->lastError();
            return QList<QVariant>();
        }

        QMap<QString, QString> filterLocal;
        QMapIterator<QString, QVariant> it(filter);
        while (it.hasNext()) {
            it.next();
            filterLocal.insert(it.key(), it.value().toString());
        }

        QList<SignonIdentityInfo> credentials = db->credentials(filterLocal);

        if (db->errorOccurred()) {
            QDBusMessage errReply = message().createErrorReply(
                    internalServerErrName,
                    internalServerErrStr + QLatin1String("Querying database error occurred."));
            SIGNOND_BUS.send(errReply);
            return QList<QVariant>();
        }

        return SignonIdentityInfo::listToVariantList(credentials);
    }

    bool SignonDaemon::clear()
    {
        RequestCounter::instance()->addServiceResquest();

        SIGNON_RETURN_IF_CAM_UNAVAILABLE(false);

        TRACE() << "\n\n\n Clearing DB\n\n";
        CredentialsDB *db = m_pCAMManager->credentialsDB();
        if (!db) {
            qCritical() << Q_FUNC_INFO << m_pCAMManager->lastError();
            return false;
        }

        if (!db->clear()) {
            QDBusMessage errReply = message().createErrorReply(
                                                    SIGNOND_INTERNAL_SERVER_ERR_NAME,
                                                    QString(SIGNOND_INTERNAL_SERVER_ERR_STR
                                                            + QLatin1String("Database error occurred.")));
            SIGNOND_BUS.send(errReply);
            return false;
        }
        return true;
    }

    QString SignonDaemon::getAuthSessionObjectPath(const quint32 id, const QString type)
    {
        return SignonAuthSession::getAuthSessionObjectPath(id, type, this);
    }

    bool SignonDaemon::setDeviceLockCode(const QByteArray &lockCode,
                                         const QByteArray &oldLockCode)
    {
        TRACE() << "SignonDaemon::setDeviceLockCode()";
        bool ret = true;

        if (oldLockCode.isEmpty()) {
            /* --- Current lock code is being communicated to the Signond ---
               Formatting of LUKS partition or regular SIMless start, if
               partition is already formatted.
            */
            if (!initSecureStorage(lockCode)) {
                TRACE() << "Could not initialize secure storage.";
                ret = false;
            }
        } else {
            /* --- New lock code is communicated to the Signond ---
               Set new master key to the LUKS partition.
            */

            // Attempt to initialize secure storage. If already initialized, this will fail.
            if (!initSecureStorage(oldLockCode))
                TRACE() << "Could not initialize secure storage.";

            // If CAM is valid, attempt to set the lock code.
            if (m_pCAMManager != NULL) {
                if (!m_pCAMManager->setDeviceLockCodeKey(lockCode, oldLockCode)) {
                    ret = false;
                    TRACE() << "Failed to set device lock code.";
                } else {
                    TRACE() << "Device lock code successfully set.";
                }
            }
        }
        return ret;
    }

    bool SignonDaemon::remoteLock(const QByteArray &lockCode)
    {
        Q_UNUSED(lockCode)
        // TODO - implement this, research how to.
        TRACE() << "remoteDrop:   lockCode = " << lockCode;
        return false;
    }

    /*
     * backup/restore
     * TODO move fixed strings into config
     */

    static QString &backupCopyFilename()
    {
        static QString name(QLatin1String("/home/user/.signon/signondb.bin"));
        return name;
    }

    uchar SignonDaemon::backupStarts()
    {
        TRACE() << "backup";
        if (!m_backup && m_pCAMManager->credentialsSystemOpened())
        {
            m_pCAMManager->closeCredentialsSystem();
            if (m_pCAMManager->credentialsSystemOpened())
            {
                qCritical() << "Cannot close credentials database";
                return 2;
            }
        }

        //do backup copy
        CAMConfiguration config;
        QString source;
        if (config.m_useEncryption)
            source = config.m_dbFileSystemPath;
        else
            source = QDir::homePath() + QDir::separator() + config.m_dbName;

        QDir target;
        if (!target.mkpath(QLatin1String("/home/user/.signon/")))
        {
            qCritical() << "Cannot create target directory";
            m_pCAMManager->openCredentialsSystem();
            return 2;
        }

        if (!QFile::copy(source, backupCopyFilename()))
        {
            qCritical() << "Cannot copy database";
            m_pCAMManager->openCredentialsSystem();
            return 2;
        }

        if (!m_backup)
        {
            //mount file system back
            if (!m_pCAMManager->openCredentialsSystem()) {
                qCritical() << "Cannot reopen database";
                return 2;
            }
        }
        return 0;
    }

    uchar SignonDaemon::backupFinished()
    {
        TRACE() << "close";

        QFile copy(backupCopyFilename());

        if (copy.exists())
            QFile::remove(backupCopyFilename());

        if (m_backup)
        {
            //close daemon
            TRACE() << "close daemon";
            this->deleteLater();
        }

        return 0;
     }

    /*
     * Does nothing but start-on-demand
     * */
    uchar SignonDaemon::restoreStarts()
    {
        TRACE();
        return 0;
    }

    uchar SignonDaemon::restoreFinished()
    {
        TRACE() << "restore";
        //restore requested
        if (m_pCAMManager->credentialsSystemOpened())
        {
            //umount file system
            if (!m_pCAMManager->closeCredentialsSystem())
            {
                qCritical() << "database cannot be closed";
                return 2;
            }
        }

        //do restore
        //TODO add checking if encryption status has changed
        CAMConfiguration config;
        QString target;
        if (config.m_useEncryption)
            target = config.m_dbFileSystemPath;
        else
            target = QDir::homePath() + QDir::separator() + config.m_dbName;

        if (!QFile::remove(target+QLatin1String(".bak")))
        {
            qCritical() << "Cannot remove backup copy of database";
        }

        if (!QFile::rename(target, target+QLatin1String(".bak")))
        {
            qCritical() << "Cannot make backup copy of database";
        }

        if (!QFile::rename(backupCopyFilename(), target))
        {
            qCritical() << "Cannot copy database";

            if (!QFile::rename(target+QLatin1String(".bak"), target))
                qCritical() << "Cannot restore backup copy of database";

            m_pCAMManager->openCredentialsSystem();
            return 2;
        }

        //try to remove backup database
        if (!QFile::remove(target+QLatin1String(".bak")))
            qCritical() << "Cannot remove backup copy of database";

        //TODO check database integrity
        if (!m_backup)
        {
            //mount file system back
             if (!m_pCAMManager->openCredentialsSystem())
                 return 2;
        }

        return 0;
    }

    void SignonDaemon::listDBusInterfaces()
    {
        QDBusReply<QStringList> reply = SIGNOND_BUS.interface()->registeredServiceNames();
        QStringList list = reply.value();

        QString servicesList = QLatin1String("DBUS registered services: \n");
        servicesList += list.join(QLatin1String("\n"));

        TRACE() << "\n\n" << servicesList.toAscii().data() << "\n";
    }

} //namespace SignonDaemonNS
