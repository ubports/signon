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

using namespace SignOn;

namespace SignonDaemonNS {

    RequestCounter *RequestCounter::m_pInstance = NULL;

    SignonDaemon::SignonDaemon(QObject *parent) : QObject(parent)
    {
        m_backup = false;
        m_pCAMManager = NULL;
    }

    SignonDaemon::~SignonDaemon()
    {
        if (m_backup) exit(0);

        qDeleteAll(m_storedIdentities);
        qDeleteAll(m_unstoredIdentities);
        SignonAuthSession::stopAllAuthSessions();

        if (m_pCAMManager) {
            m_pCAMManager->closeCredentialsSystem();
            m_pCAMManager->deleteLater();
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
                                              +QLatin1String("/backup"), this, registerSessionOptions)) {
            TRACE() << "Object cannot be registered";
            return false;
        }

        if (!sessionConnection.registerService(SIGNOND_SERVICE+QLatin1String(".backup"))) {
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

        listDBusInterfaces();

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
        Q_UNUSED(lockCode)

        m_pCAMManager = CredentialsAccessManager::instance();
        CAMConfiguration config;

        if (!m_pCAMManager->init(config))  {
            qFatal("Signond: Cannot set proper configuration of CAM");
            delete m_pCAMManager;
            m_pCAMManager = NULL;
            return false;
        }

        if (!m_pCAMManager->openCredentialsSystem()) {
            qFatal("Signond: openCredentialsSystem failed with code %d",
                   m_pCAMManager->lastError());
            delete m_pCAMManager;
            m_pCAMManager = NULL;
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
        m_unstoredIdentities.remove(identity->objectName());
        m_storedIdentities.insert(identity->id(), identity);
    }

    void SignonDaemon::registerNewIdentity(QDBusObjectPath &objectPath)
    {
        RequestCounter::instance()->addServiceResquest();

        TRACE() << "Registering new identity:";

        SignonIdentity *identity = SignonIdentity::createIdentity(SIGNOND_NEW_IDENTITY, this);

        if (identity == NULL)
        {
            QDBusMessage errReply = message().createErrorReply(
                    SIGNOND_INTERNAL_SERVER_ERR_NAME,
                    SIGNOND_INTERNAL_SERVER_ERR_STR + QLatin1String("Could not create remote Identity object."));
            SIGNOND_BUS.send(errReply);
            return;
        }

        m_unstoredIdentities.insert(identity->objectName(), identity);

        objectPath = QDBusObjectPath(identity->objectName());
    }

    void SignonDaemon::registerStoredIdentity(const quint32 id, QDBusObjectPath &objectPath, QList<QVariant> &identityData)
    {
        RequestCounter::instance()->addServiceResquest();

        TRACE() << "Registering identity:" << id;

        //1st check if the existing identity is in cache
        SignonIdentity *identity = m_storedIdentities.value(id, NULL);

        //if not create it
        if (identity == NULL)
            identity = SignonIdentity::createIdentity(id, this);

        if (identity == NULL)
        {
            QDBusMessage errReply = message().createErrorReply(
                    SIGNOND_INTERNAL_SERVER_ERR_NAME,
                    SIGNOND_INTERNAL_SERVER_ERR_STR + QLatin1String("Could not create remote Identity object."));
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
         foreach(fileName, fileNames)
         {
             if (fileName.startsWith(QLatin1String("lib")))
             {
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

        if (!plugin)
        {
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

        TRACE() << "\n\n\n Querying identities\n\n";

        CredentialsDB *db = m_pCAMManager->credentialsDB();
        if (!db) {
            qCritical() << Q_FUNC_INFO << m_pCAMManager->lastError();
            return QList<QVariant>();
        }

        QMap<QString, QString> filterLocal;
        QMapIterator<QString, QVariant> it(filter);
        while(it.hasNext())
        {
            it.next();
            filterLocal.insert(it.key(), it.value().toString());
        }

        QList<SignonIdentityInfo> credentials = db->credentials(filterLocal);

        if (db->errorOccurred())
        {
            QDBusMessage errReply = message().createErrorReply(
                    SIGNOND_INTERNAL_SERVER_ERR_NAME,
                    SIGNOND_INTERNAL_SERVER_ERR_STR + QLatin1String("Querying database error occurred."));
            SIGNOND_BUS.send(errReply);
            return QList<QVariant>();
        }

        return SignonIdentityInfo::listToVariantList(credentials);
    }

    bool SignonDaemon::clear()
    {
        RequestCounter::instance()->addServiceResquest();
        TRACE() << "\n\n\n Clearing DB\n\n";
        CredentialsDB *db = m_pCAMManager->credentialsDB();
        if (!db) {
            qCritical() << Q_FUNC_INFO << m_pCAMManager->lastError();
            return false;
        }

        if (!db->clear())
        {
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

    bool SignonDaemon::setDeviceLockCode(const QByteArray &oldLockCode,
                                         const QByteArray &newLockCode)
    {
        Q_UNUSED(oldLockCode)
        Q_UNUSED(newLockCode)

        // TODO - implement this
        TRACE() << "setDeviceLockCode:   oldLockCode = " << oldLockCode << ", newLockCode = " << newLockCode;
        return false;
    }

    bool SignonDaemon::setSim(const QByteArray& simData,
                              const QByteArray& checkData)
    {
        Q_UNUSED(simData)
        Q_UNUSED(checkData)

        // TODO - implement this
        TRACE() << "setSim:   simData = " << simData << ", checkData = " << checkData;
        return false;
    }

    bool SignonDaemon::remoteLock(const QByteArray &lockCode)
    {
        Q_UNUSED(lockCode)

        // TODO - implement this
        TRACE() << "remoteLock:   lockCode = " << lockCode;
        return false;
    }

    /*
     * backup/restore
     * TODO move fixed strings into config
     */
    uchar SignonDaemon::backup(const QStringList &selectedCategories)
    {
        TRACE() << "backup";
        if (selectedCategories.contains(QLatin1String("settings"), Qt::CaseInsensitive)) {
            //backup requested
            if (!m_backup && m_pCAMManager->credentialsSystemOpenened()) {
                //umount file system
                m_pCAMManager->closeCredentialsSystem();
                 if (m_pCAMManager->credentialsSystemOpenened()) return 2;
            }
            //do backup copy
            CAMConfiguration config;
            QString source;
            if (config.m_useEncryption)
                source = config.m_dbFileSystemPath;
            else
                source = QDir::homePath() + QDir::separator() + config.m_dbName;
            QDir target;
            if (!target.mkpath(QLatin1String("/home/user/.signon/"))) {
                TRACE() << "Cannot create target directory";
                m_pCAMManager->openCredentialsSystem();
                return 2;
            }
            if (!QFile::copy(source, QLatin1String("/home/user/.signon/signondb.bin"))) {
                TRACE() << "Cannot copy database";
                m_pCAMManager->openCredentialsSystem();
                return 2;
            }
            if (!m_backup) {
                //mount file system back
                if (!m_pCAMManager->openCredentialsSystem()) {
                    TRACE() << "Cannot reopen database";
                    return 2;
                }
            }
        }
        return 0;
    }

    bool SignonDaemon::close()
    {
        TRACE() << "close";
        if (m_backup) {
            //close daemon
            TRACE() << "close daemon";
            this->deleteLater();
        }
        return true;
     }

    bool SignonDaemon::prestart()
    {
        TRACE() << "prestart";
        return true;
    }

    uchar SignonDaemon::restore(const QStringList &selectedCategories,
                                const QString &productName,
                                const QStringList &restoredFilePaths)
    {
        Q_UNUSED(productName);
        Q_UNUSED(restoredFilePaths);
        TRACE() << "restore";
        if (selectedCategories.contains(QLatin1String("settings"), Qt::CaseInsensitive)) {
            //restore requested
            if (m_pCAMManager->credentialsSystemOpenened()) {
                //umount file system
                if (!m_pCAMManager->closeCredentialsSystem()) {
                    TRACE() << "database cannot be closed";
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
            if (!QFile::rename(target, target+QLatin1String(".bak"))) {
                TRACE() << "Cannot make backup copy of database";
            }
            if (!QFile::copy(QLatin1String("/home/user/.signon/signondb.bin"), target)) {
                TRACE() << "Cannot copy database";
                if (!QFile::rename(target+QLatin1String(".bak"), target)) {
                    TRACE() << "Cannot restore backup copy of database";
                }
                m_pCAMManager->openCredentialsSystem();
                return 2;
            }
            //TODO check database integrity
            if (!m_backup) {
                //mount file system back
                 if (!m_pCAMManager->openCredentialsSystem()) {
                     return 2;
                 }
            }
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
