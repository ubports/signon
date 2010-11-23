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

#include "signond-common.h"

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
          keyManagers(),
          m_pCredentialsDB(NULL),
          m_pCryptoFileSystemManager(NULL),
          m_CAMConfiguration(CAMConfiguration())
{
}

CredentialsAccessManager::~CredentialsAccessManager()
{
    closeCredentialsSystem();

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

bool CredentialsAccessManager::openSecretsDB()
{
    //todo remove this variable after LUKS implementation becomes stable.
    QString dbPath;

    if (m_CAMConfiguration.m_useEncryption) {
        dbPath = m_pCryptoFileSystemManager->fileSystemMountPath()
            + QDir::separator()
            + m_CAMConfiguration.m_dbName;

        if (!fileSystemDeployed()) {
            if (deployCredentialsSystem()) {
                if (m_pCredentialsDB->openSecretsDB(dbPath))
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
    } else {
        QFileInfo fInfo(m_CAMConfiguration.m_dbFileSystemPath);
        QDir storageDir = fInfo.dir();
        if (!storageDir.exists()) {
            if (!storageDir.mkpath(storageDir.path()))
                BLAME() << "Could not create storage directory!!!";
        }

        dbPath = storageDir.path()
                 + QDir::separator()
                 + m_CAMConfiguration.m_dbName
                 + QLatin1String(".creds");
    }

    TRACE() << "Database name: [" << dbPath << "]";

    if (m_pCredentialsDB->openSecretsDB(dbPath)) {
        m_systemOpened = true;
        m_error = NoError;
    }

    return m_systemOpened;
}

bool CredentialsAccessManager::closeSecretsDB()
{
    m_pCredentialsDB->closeSecretsDB();

    if (m_CAMConfiguration.m_useEncryption) {
        if (!m_pCryptoFileSystemManager->unmountFileSystem()) {
            m_error = CredentialsDbUnmountFailed;
            return false;
        }
    }

    return true;
}

bool CredentialsAccessManager::openMetaDataDB()
{
    QFileInfo fInfo(m_CAMConfiguration.m_dbFileSystemPath);
    QDir storageDir = fInfo.dir();
    if (!storageDir.exists()) {
        if (!storageDir.mkpath(storageDir.path()))
            BLAME() << "Could not create storage directory!!!";
    }

    QString dbPath = storageDir.path()
             + QDir::separator()
             + m_CAMConfiguration.m_dbName;
    m_pCredentialsDB = new CredentialsDB(dbPath);

    if (!m_pCredentialsDB->init()) {
        m_error = CredentialsDbConnectionError;
        return false;
    }

    return true;
}

void CredentialsAccessManager::closeMetaDataDB()
{
    if (m_pCredentialsDB) {
        delete m_pCredentialsDB;
        m_pCredentialsDB = NULL;
    }
}

bool CredentialsAccessManager::openCredentialsSystem()
{
    RETURN_IF_NOT_INITIALIZED(false);

    if (!openMetaDataDB()) {
        BLAME() << "Couldn't open metadata DB!";
        return false;
    }

    if (!openSecretsDB()) {
        BLAME() << "Failed to open secrets DB.";
        /* Even if the secrets DB couldn't be opened, signond is still usable:
         * that's why we return "true" anyways. */
    }

    return true;
}

bool CredentialsAccessManager::closeCredentialsSystem()
{
    RETURN_IF_NOT_INITIALIZED(false);

    if (!closeSecretsDB())
        return false;
    closeMetaDataDB();

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
                if (openSecretsDB()) {
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

    if (m_pCryptoFileSystemManager->fileSystemMounted()) {
        /* if the secure FS is already mounted, add the new key to it */
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
    } else if (!fileSystemDeployed()) {
        /* if the secure FS does not exist, create it and use this new key to
         * initialize it */
        m_pCryptoFileSystemManager->setEncryptionKey(key);
        m_accessCodeFetched = true;
        if (openSecretsDB()) {
            authorizedKeys << key;
        } else {
            BLAME() << "Couldn't create the secure FS";
        }
    } else {
        BLAME() << "Secure FS already created with another set of keys";
    }
}

