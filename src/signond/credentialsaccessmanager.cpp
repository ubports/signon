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

#include "accesscodehandler.h"
#include "signond-common.h"

#include <QThreadPool>
#include <QCoreApplication>
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
        : m_dbName(QLatin1String("signon.db")),
          m_useEncryption(false), //TODO this need to be set true when encryption is ready
          m_dbFileSystemPath(QLatin1String("/home/user/signonfs")),
          m_fileSystemType(QLatin1String("ext2")),
          m_fileSystemSize(4),
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
          m_pCredentialsDB(NULL),
          m_pCryptoFileSystemManager(NULL),
          m_pAccessCodeHandler(NULL),
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

    if (m_pAccessCodeHandler) {
        m_pAccessCodeHandler->blockSignals(true);
        delete m_pAccessCodeHandler;
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
        m_pCryptoFileSystemManager = new CryptoManager(this);
        m_pCryptoFileSystemManager->setFileSystemPath(m_CAMConfiguration.m_dbFileSystemPath);
        m_pCryptoFileSystemManager->setFileSystemSize(m_CAMConfiguration.m_fileSystemSize);
        m_pCryptoFileSystemManager->setFileSystemType(m_CAMConfiguration.m_fileSystemType);

        if (!m_CAMConfiguration.m_encryptionPassphrase.isEmpty()) {

            m_pCryptoFileSystemManager->setEncryptionKey(
                    m_CAMConfiguration.m_encryptionPassphrase);
            m_accessCodeFetched = true;

            if (!openCredentialsSystemPriv()) {
                BLAME() << "Failed to open credentials system. Fallback to alternative methods.";
            }
        }

        m_pAccessCodeHandler = new AccessCodeHandler(this);
        connect(m_pAccessCodeHandler,
                SIGNAL(simAvailable(QByteArray)),
                SLOT(accessCodeFetched(QByteArray)));

        connect(m_pAccessCodeHandler,
                SIGNAL(simChanged(QByteArray)),
                SLOT(accessCodeFetched(QByteArray)));

        //todo create handling for the sim change too.
        if (!m_pAccessCodeHandler->isValid())
            BLAME() << "AccessCodeHandler invalid, SIM data might not be available.";

        m_pAccessCodeHandler->querySim();
    }

    m_isInitialized = true;
    m_error = NoError;

    TRACE() << "CredentialsAccessManager successfully initialized...";
    return true;
}


bool CredentialsAccessManager::openCredentialsSystemPriv()
{
    //todo remove this variable after LUKS implementation becomes stable.
    QString dbPath;

    if (m_CAMConfiguration.m_useEncryption) {
        dbPath = m_pCryptoFileSystemManager->fileSystemMountPath()
            + QDir::separator()
            + m_CAMConfiguration.m_dbName;

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
    } else {

        /*
         *   TODO - change this so that openDB takes only the CAM configuration db name
         *          after the LUKS system is enabled; practically remove this 'else' branch
         */

        dbPath = QDir::homePath() + QDir::separator() + m_CAMConfiguration.m_dbName;
    }

    TRACE() << "Database name: [" << dbPath << "]";

    if (openDB(dbPath)) {
        m_systemOpened = true;
        m_error = NoError;
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

    return openCredentialsSystemPriv();
}

bool CredentialsAccessManager::closeCredentialsSystem()
{
    RETURN_IF_NOT_INITIALIZED(false);

    if (!closeDB())
        return false;

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

bool CredentialsAccessManager::setDeviceLockCodeKey(const QByteArray &newLockCode,
                                                    const QByteArray &existingLockCode)
{
    if (!m_CAMConfiguration.m_useEncryption)
        return false;

    if (!m_pCryptoFileSystemManager->encryptionKeyInUse(existingLockCode)) {
        BLAME() << "Existing lock code check failed.";
        return false;
    }

    if (!m_pCryptoFileSystemManager->addEncryptionKey(
            newLockCode, existingLockCode)) {
        BLAME() << "Failed to add new device lock code.";
        return false;
    }

    if (!m_pCryptoFileSystemManager->removeEncryptionKey(existingLockCode, newLockCode)) {
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

bool CredentialsAccessManager::closeDB()
{
    if (m_pCredentialsDB) {
        m_pCredentialsDB->disconnect();
        delete m_pCredentialsDB;
        m_pCredentialsDB = NULL;
    }
    return true;
}

void CredentialsAccessManager::accessCodeFetched(const QByteArray &accessCode)
{
    /* todo - this will be improved when missing components are available
             a. sim random data query
             b. device lock code UI query
     */
    if (accessCode.isEmpty()) {
        BLAME() << "Fetched empty access code.";
        m_error = FailedToFetchAccessCode;
        return;
    }

    m_accessCodeFetched = true;

    //todo - query Device Lock Code here - UI dialog.
    QByteArray lockCode = "1234";

    //add the key to a keyslot
    if (!m_pCryptoFileSystemManager->encryptionKeyInUse(accessCode)) {
        if (!m_pCryptoFileSystemManager->addEncryptionKey(accessCode, lockCode)) //device lock code here
            BLAME() << "Could not store encryption key.";
    }

    m_pCryptoFileSystemManager->setEncryptionKey(accessCode);

    if (!credentialsSystemOpened()) {
        if (openCredentialsSystem()) {
            TRACE() << "Credentials system opened.";
        } else {
            BLAME() << "Failed to open credentials system.";
        }
    }
}
