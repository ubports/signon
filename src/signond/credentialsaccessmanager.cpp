/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
 * Contact: Aurel Popirtac <mailto:ext-Aurel.Popirtac@nokia.com>
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


#include "credentialsaccessmanager.h"

#include "simdatahandler.h"
#include "signond-common.h"
#include "devicelockcodehandler.h"

#include <QFile>
#include <QBuffer>


#define RETURN_IF_NOT_INITIALIZED(return_value)                  \
    do {                                                         \
        if (!m_isInitialized) {                                  \
            m_error = NotInitialized;                            \
            TRACE() << "CredentialsAccessManager not initialized."; \
            return return_value;                                \
        }                                                       \
    } while (0)

using namespace SignonDaemonNS;

/* ---------------------- CAMConfiguration ---------------------- */

CAMConfiguration::CAMConfiguration()
        : m_dbName(QLatin1String(signonDefaultDbName)),
          m_useEncryption(signonDefaultUseEncryption),
          m_dbFileSystemPath(
                QLatin1String(signonDefaultStoragePath)
                + QDir::separator()
                + QLatin1String(signonDefaultFileSystemName)),
          m_fileSystemType(QLatin1String(signonDefaultFileSystemType)),
          m_fileSystemSize(signonMinumumDbSize),
          m_encryptionPassphrase(QByteArray())
{}

void CAMConfiguration::serialize(QIODevice *device)
{
    if (device == NULL)
        return;

    if (!device->open(QIODevice::ReadWrite)) {
        return;
    }

    QString buffer;
    QTextStream stream(&buffer);
    stream << "\n\n====== Credentials Access Manager Configuration ======\n\n";
    stream << "File system mount name " << m_dbFileSystemPath << '\n';
    stream << "File system format: " << m_fileSystemType << '\n';
    stream << "File system size:" << m_fileSystemSize << "megabytes\n";

    const char *usingEncryption = m_useEncryption ? "true" : "false";
    stream << "Using encryption: " << usingEncryption << '\n';
    stream << "Credentials database name: " << m_dbName << '\n';
    stream << "======================================================\n\n";
    device->write(buffer.toUtf8());
    device->close();
}

/* ---------------------- CredentialsAccessManager ---------------------- */

CredentialsAccessManager *CredentialsAccessManager::m_pInstance = NULL;

CredentialsAccessManager::CredentialsAccessManager(QObject *parent)
        : QObject(parent),
          m_isInitialized(false),
          m_accessCodeFetched(false),
          m_systemOpened(false),
          m_error(NoError),
          m_currentSimData(QByteArray()),
          keyManagers(),
          m_pCredentialsDB(NULL),
          m_pCryptoFileSystemManager(NULL),
          m_pSimDataHandler(NULL),
          m_pLockCodeHandler(NULL),
          m_CAMConfiguration(CAMConfiguration())
{
}

CredentialsAccessManager::~CredentialsAccessManager()
{
    closeCredentialsSystem();

    if (m_pSimDataHandler)
        delete m_pSimDataHandler;
    if (m_pLockCodeHandler)
        delete m_pLockCodeHandler;

    m_pInstance = NULL;
}

CredentialsAccessManager *CredentialsAccessManager::instance(QObject *parent)
{
    if (!m_pInstance)
        m_pInstance = new CredentialsAccessManager(parent);

    return m_pInstance;
}

void CredentialsAccessManager::finalize()
{
    if (m_systemOpened)
        closeCredentialsSystem();

    if (m_pSimDataHandler) {
        m_pSimDataHandler->blockSignals(true);
        delete m_pSimDataHandler;
        m_pSimDataHandler = NULL;
    }

    if (m_pLockCodeHandler) {
        m_pLockCodeHandler->blockSignals(true);
        delete m_pLockCodeHandler;
        m_pLockCodeHandler = NULL;
    }

    if (m_pCryptoFileSystemManager)
        delete m_pCryptoFileSystemManager;

    m_accessCodeFetched = false;
    m_isInitialized = false;
    m_error = NoError;
}

bool CredentialsAccessManager::init(const CAMConfiguration &camConfiguration)
{
    if (m_isInitialized) {
        TRACE() << "CAM already initialized.";
        m_error = AlreadyInitialized;
        return false;
    }

    m_CAMConfiguration = camConfiguration;

    QBuffer config;
    m_CAMConfiguration.serialize(&config);
    TRACE() << "\n\nInitualizing CredentialsAccessManager with configuration: " << config.data();

    if (m_CAMConfiguration.m_useEncryption) {
        //Initialize CryptoManager
        m_pCryptoFileSystemManager = new CryptoManager(this);
        m_pCryptoFileSystemManager->setFileSystemPath(m_CAMConfiguration.m_dbFileSystemPath);
        m_pCryptoFileSystemManager->setFileSystemSize(m_CAMConfiguration.m_fileSystemSize);
        m_pCryptoFileSystemManager->setFileSystemType(m_CAMConfiguration.m_fileSystemType);

        //Initialize SIM handler
        m_pSimDataHandler = new SimDataHandler(this);
        connect(m_pSimDataHandler,
                SIGNAL(simAvailable(QByteArray)),
                SLOT(simDataFetched(QByteArray)));

        connect(m_pSimDataHandler,
                SIGNAL(simRemoved()),
                SLOT(simRemoved()));

        connect(m_pSimDataHandler,
                SIGNAL(error()),
                SLOT(simError()));

        if (!m_pSimDataHandler->isValid())
            BLAME() << "AccessCodeHandler invalid, SIM data might not be available.";

        //If passphrase exists (device lock code) open creds system - sync
        if (!m_CAMConfiguration.m_encryptionPassphrase.isEmpty()) {

            m_pCryptoFileSystemManager->setEncryptionKey(
                    m_CAMConfiguration.m_encryptionPassphrase);
            m_accessCodeFetched = true;

            if (!openCredentialsSystemPriv(true)) {
                BLAME() << "Failed to open credentials system. Fallback to alternative methods.";
            }
        }

        /* Query SIM.
           1. In case of a SIM query error or if the queried SIM auth data
           is not stored as a LUKS key, this will later on trigger the DLC query - async.
           2. In case of the current encryption passphrase being secure (buffered DLC),
           - signond was started by a setDeviceLockCode() call -
           then store the SIM reply data right away using the buffered DLC as master key.
        */
        m_pSimDataHandler->querySim();


        // Initialize all key managers
        foreach (SignOn::AbstractKeyManager *keyManager, keyManagers) {
            connect(keyManager,
                    SIGNAL(keyInserted(const SignOn::Key)),
                    SLOT(onKeyInserted(const SignOn::Key)));
            connect(keyManager,
                    SIGNAL(keyDisabled(const SignOn::Key)),
                    SLOT(onKeyDisabled(const SignOn::Key)));
            connect(keyManager,
                    SIGNAL(keyRemoved(const SignOn::Key)),
                    SLOT(onKeyRemoved(const SignOn::Key)));
            connect(keyManager,
                    SIGNAL(keyAuthorized(const SignOn::Key, bool)),
                    SLOT(onKeyAuthorized(const SignOn::Key, bool)));
            keyManager->setup();
        }
    }

    m_isInitialized = true;
    m_error = NoError;

    TRACE() << "CredentialsAccessManager successfully initialized...";
    return true;
}

void CredentialsAccessManager::addKeyManager(
    SignOn::AbstractKeyManager *keyManager)
{
    keyManagers.append(keyManager);
}

bool CredentialsAccessManager::openCredentialsSystemPriv(bool mountFileSystem)
{
    //todo remove this variable after LUKS implementation becomes stable.
    QString dbPath;

    if (m_CAMConfiguration.m_useEncryption) {
        dbPath = m_pCryptoFileSystemManager->fileSystemMountPath()
            + QDir::separator()
            + m_CAMConfiguration.m_dbName;

        if (mountFileSystem) {
            if (!fileSystemDeployed()) {
                if (deployCredentialsSystem()) {
                    if (openDB(dbPath))
                        m_systemOpened = true;
                }
                return m_systemOpened;
            }

            if (fileSystemLoaded()) {
                m_error = CredentialsDbAlreadyDeployed;
                return false;
            }

            if (!m_pCryptoFileSystemManager->mountFileSystem()) {
                m_error = CredentialsDbMountFailed;
                return false;
            }
        }
    } else {
        QFileInfo fInfo(m_CAMConfiguration.m_dbFileSystemPath);
        QDir storageDir = fInfo.dir();
        if (!storageDir.exists()) {
            if (!storageDir.mkpath(storageDir.path()))
                BLAME() << "Could not create storage directory!!!";
        }

        dbPath = storageDir.path()
                 + QDir::separator()
                 + m_CAMConfiguration.m_dbName;
    }

    TRACE() << "Database name: [" << dbPath << "]";

    if (openDB(dbPath)) {
        m_systemOpened = true;
        m_error = NoError;
    }

    if (m_pLockCodeHandler != 0) {
        delete m_pLockCodeHandler;
        m_pLockCodeHandler = 0;
    }

    return m_systemOpened;
}

bool CredentialsAccessManager::openCredentialsSystem()
{
    RETURN_IF_NOT_INITIALIZED(false);

    if (m_CAMConfiguration.m_useEncryption && !m_accessCodeFetched) {
        m_error = AccessCodeNotReady;
        return false;
    }

    return openCredentialsSystemPriv(true);
}

bool CredentialsAccessManager::closeCredentialsSystem()
{
    RETURN_IF_NOT_INITIALIZED(false);

    closeDB();

    if (m_CAMConfiguration.m_useEncryption) {
        if (!m_pCryptoFileSystemManager->unmountFileSystem()) {
            m_error = CredentialsDbUnmountFailed;
            return false;
        }
    }

    m_error = NoError;
    m_systemOpened = false;
    return true;
}

bool CredentialsAccessManager::deleteCredentialsSystem()
{
    RETURN_IF_NOT_INITIALIZED(false);

    if (m_systemOpened && !closeCredentialsSystem()) {
        /* The close operation failed: we cannot proceed */
        return false;
    }

    m_error = NoError;

    if (m_CAMConfiguration.m_useEncryption) {
        if (!m_pCryptoFileSystemManager->deleteFileSystem())
            m_error = CredentialsDbDeletionFailed;
    } else {
        QFile dbFile(m_CAMConfiguration.m_dbName);
        if (dbFile.exists()) {
            if (!dbFile.remove())
                m_error = CredentialsDbDeletionFailed;
        }
    }

    return m_error == NoError;
}

bool CredentialsAccessManager::setMasterEncryptionKey(const QByteArray &newKey,
                                                      const QByteArray &existingKey)
{
    if (!m_CAMConfiguration.m_useEncryption)
        return false;

    /* Clear this for security reasons - SIM data must not be stored
       using an deprecated master key
    */
    m_CAMConfiguration.m_encryptionPassphrase.clear();

    if (!encryptionKeyCanMountFS(existingKey)) {
        BLAME() << "Existing lock code check failed.";
        return false;
    }

    if (!m_pCryptoFileSystemManager->addEncryptionKey(
            newKey, existingKey)) {
        BLAME() << "Failed to add new device lock code.";
        return false;
    }

    if (!m_pCryptoFileSystemManager->removeEncryptionKey(existingKey, newKey)) {
        BLAME() << "Failed to remove old device lock code.";
        return false;
    }

    return true;
}

bool CredentialsAccessManager::encryptionKeyPendingAddition() const
{
    return !m_currentSimData.isEmpty();
}

bool CredentialsAccessManager::storeEncryptionKey(const QByteArray &masterKey)
{
    bool ret = false;
    /*Add the key to a keyslot.
      If the SIM was recognized by the system or it was just successfully added to it
      consider it as the current encryption key.
    */
    if (!encryptionKeyCanMountFS(m_currentSimData)) {
        TRACE() << "Encryption key not in use, attemptin store.";

        if (m_pCryptoFileSystemManager->addEncryptionKey(m_currentSimData, masterKey)) {
            m_pCryptoFileSystemManager->setEncryptionKey(m_currentSimData);
            ret = true;
        } else {
            BLAME() << "Could not store encryption key.";
        }
    } else {
        TRACE() << masterKey;
        m_pCryptoFileSystemManager->setEncryptionKey(m_currentSimData);
    }

    m_currentSimData.clear();
    return ret;
}

bool CredentialsAccessManager::lockSecureStorage(const QByteArray &lockData)
{
    // TODO - implement this, research how to.
    Q_UNUSED(lockData)
    return false;
}

CredentialsDB *CredentialsAccessManager::credentialsDB() const
{
    RETURN_IF_NOT_INITIALIZED(NULL);

    return m_pCredentialsDB;
}

bool CredentialsAccessManager::deployCredentialsSystem()
{
    if (m_CAMConfiguration.m_useEncryption) {
        if (!m_pCryptoFileSystemManager->setupFileSystem()) {
            m_error = CredentialsDbSetupFailed;
            return false;
        }
    }
    return true;
}

bool CredentialsAccessManager::fileSystemLoaded(bool checkForDatabase)
{
    if (!m_pCryptoFileSystemManager->fileSystemMounted())
        return false;

    if (checkForDatabase
        && !m_pCryptoFileSystemManager->fileSystemContainsFile(m_CAMConfiguration.m_dbName))
        return false;

    return true;
}

bool CredentialsAccessManager::fileSystemDeployed()
{
    return QFile::exists(m_pCryptoFileSystemManager->fileSystemPath());
}

bool CredentialsAccessManager::openDB(const QString &databaseName)
{
    m_pCredentialsDB = new CredentialsDB(databaseName);

    if (!m_pCredentialsDB->connect()) {
        TRACE() << SqlDatabase::errorInfo(m_pCredentialsDB->error(false));
        m_error = CredentialsDbConnectionError;
        return false;
    }
    TRACE() <<  "Database connection succeeded.";

    if (!m_pCredentialsDB->hasTableStructure()) {
        TRACE() << "Creating SQL table structure...";
        if (!m_pCredentialsDB->createTableStructure()) {
            TRACE() << SqlDatabase::errorInfo(m_pCredentialsDB->error());
            m_error = CredentialsDbSqlError;
            return false;
        }
    } else {
        TRACE() << "SQL table structure already created...";
    }

    TRACE() << m_pCredentialsDB->sqlDBConfiguration();

    return true;
}

bool CredentialsAccessManager::encryptionKeyCanMountFS(const QByteArray &key)
{
    if (!fileSystemDeployed()) {
        TRACE() << "Secure FS not deployed";
        return false;
    }

    if (m_pCryptoFileSystemManager->encryptionKeyInUse(key)) {
        TRACE() << "SIM data already in use.";
        if (m_pCryptoFileSystemManager->fileSystemMounted()) {

            m_pCryptoFileSystemManager->setEncryptionKey(key);

            if (!credentialsSystemOpened()) {
                if (openCredentialsSystemPriv(false)) {
                    TRACE() << "Credentials system opened.";
                } else {
                    BLAME() << "Failed to open credentials system.";
                }
            } else {
                TRACE() << "Credentials system already opened.";
            }
        }
        return true;
    } else {
        return false;
    }
}

void CredentialsAccessManager::closeDB()
{
    if (m_pCredentialsDB) {
        m_pCredentialsDB->disconnect();
        delete m_pCredentialsDB;
        m_pCredentialsDB = NULL;
    }
}

void CredentialsAccessManager::simDataFetched(const QByteArray &simData)
{
    TRACE() << "SIM data fetched.";

    if (simData.isEmpty()) {
        BLAME() << "Fetched empty access code.";
        m_error = FailedToFetchAccessCode;
        return;
    }

    /* The `key in use` check will attempt to mount using the simData if
       the file system is not already mounted
    */
    if (encryptionKeyCanMountFS(simData)) {
        TRACE() << "SIM data already in use.";
        return;
    }

    /* Keep SIM data until the master key (device lock code) is available,
       or if the current encryption passphrase is secure
       (the daemon has been started by a setDeviceLockCode call, thus
       the CAMConfiguration passphrase is the current DLC), then
       store the SIM data right away.
    */
    m_currentSimData = simData;

    if (!m_CAMConfiguration.m_encryptionPassphrase.isEmpty()) {
        if (!storeEncryptionKey(m_CAMConfiguration.m_encryptionPassphrase))
            BLAME() << "Failed to store SIM data as encryption key.";
        m_CAMConfiguration.m_encryptionPassphrase.clear();
        return;
    }

    //Query Device Lock Code here - UI dialog.
    if (m_pLockCodeHandler == NULL)
        m_pLockCodeHandler = new DeviceLockCodeHandler(this);

    m_pLockCodeHandler->queryLockCode();
}

void CredentialsAccessManager::simRemoved()
{
    TRACE() << "SIM removed, closing secure storage.";
    if (credentialsSystemOpened())
        if (!closeCredentialsSystem())
            BLAME() << "Error occurred while closing secure storage.";

    if (m_pLockCodeHandler == NULL)
        m_pLockCodeHandler = new DeviceLockCodeHandler;

    TRACE() << "Querying DLC.";
    m_pLockCodeHandler->queryLockCode();
}

void CredentialsAccessManager::simError()
{
    TRACE() << "SIM error occurred.";

    if (!m_pSimDataHandler->isSimPresent() && !credentialsSystemOpened()) {
        if (m_pLockCodeHandler == NULL)
            m_pLockCodeHandler = new DeviceLockCodeHandler;

        TRACE() << "Querying DLC.";
        m_pLockCodeHandler->queryLockCode();
    }
}

void CredentialsAccessManager::onKeyInserted(const SignOn::Key key)
{
    TRACE() << "Key:" << key.toHex();

    if (key.isEmpty()) return;

    /* The `key in use` check will attempt to mount using the new key if
       the file system is not already mounted
    */
    if (encryptionKeyCanMountFS(key)) {
        TRACE() << "SIM data already in use.";
        authorizedKeys << key;
        return;
    }

    /* We got here because the inserted key is totally new to the CAM.
     * Let's see if any key manager wants to authorize it: we call
     * authorizeKey() on each of them, and continue processing the key when
     * the keyAuthorized() signal comes.
     */
    foreach (SignOn::AbstractKeyManager *keyManager, keyManagers) {
        keyManager->authorizeKey(key);
    }
}

void CredentialsAccessManager::onKeyDisabled(const SignOn::Key key)
{
    TRACE() << "Key:" << key.toHex();

    if (authorizedKeys.removeAll(key) == 0) {
        TRACE() << "Key was already disabled";
        return;
    }

    if (authorizedKeys.isEmpty()) {
        TRACE() << "All keys removed, closing secure storage.";
        if (credentialsSystemOpened())
            if (!closeCredentialsSystem())
                BLAME() << "Error occurred while closing secure storage.";

        TRACE() << "Querying for keys.";
        foreach (SignOn::AbstractKeyManager *keyManager, keyManagers) {
            keyManager->queryKeys();
        }
    }
}

void CredentialsAccessManager::onKeyRemoved(const SignOn::Key key)
{
    TRACE() << "Key:" << key.toHex();

    // Make sure the key is disabled:
    onKeyDisabled(key);

    if (!encryptionKeyCanMountFS(key)) {
        TRACE() << "Key is not known to the CryptoManager.";
        return;
    }

    if (authorizedKeys.isEmpty()) {
        BLAME() << "Cannot remove key: no authorized keys";
        return;
    }

    SignOn::Key authorizedKey = authorizedKeys.first();
    if (!m_pCryptoFileSystemManager->removeEncryptionKey(key, authorizedKey)) {
        BLAME() << "Failed to remove key.";
    } else {
        TRACE() << "Key successfully removed.";
    }
}

void CredentialsAccessManager::onKeyAuthorized(const SignOn::Key key,
                                               bool authorized)
{
    TRACE() << "Key:" << key.toHex() << "Authorized:" << authorized;

    if (!authorized) return;

    if (encryptionKeyCanMountFS(key)) {
        TRACE() << "Encryption key already in use.";
        authorizedKeys << key;
        return;
    }

    if (authorizedKeys.isEmpty()) {
        BLAME() << "No authorized keys: cannot add new key";
        return;
    }

    SignOn::Key authorizedKey = authorizedKeys.first();
    if (m_pCryptoFileSystemManager->addEncryptionKey(key, authorizedKey)) {
        TRACE() << "Encryption key successfullyadded into the CryptoManager.";
        m_pCryptoFileSystemManager->setEncryptionKey(key);
        authorizedKeys << key;
    } else {
        BLAME() << "Could not store encryption key.";
    }
}

