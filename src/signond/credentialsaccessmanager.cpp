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
          m_useEncryption(true),
          m_dbFileSystemPath(QLatin1String("/home/user/signonfs")),
          m_fileSystemType(QLatin1String("ext3")),
          m_fileSystemSize(4)
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

bool CredentialsAccessManager::init(const CAMConfiguration &camConfiguration)
{
    if (m_isInitialized) {
        m_error = AlreadyInitialized;
        return false;
    }

    m_CAMConfiguration = camConfiguration;

    QBuffer config;
    m_CAMConfiguration.serialize(&config);
    TRACE() << "\n\nInitualizing CredentialsAccessManager with configuration: " << config.data();

    if (m_CAMConfiguration.m_useEncryption) {

        //TODO initialize the AccessCodeHandler here for the SIM data usage

        m_pCryptoFileSystemManager = new CryptoManager(this);
        m_pCryptoFileSystemManager->setFileSystemPath(m_CAMConfiguration.m_dbFileSystemPath);
        m_pCryptoFileSystemManager->setFileSystemSize(m_CAMConfiguration.m_fileSystemSize);
        m_pCryptoFileSystemManager->setFileSystemType(m_CAMConfiguration.m_fileSystemType);
    }

    m_isInitialized = true;
    m_error = NoError;

    TRACE() << "CredentialsAccessManager successfully initialized...";
    return true;
}

bool CredentialsAccessManager::openCredentialsSystem()
{
    RETURN_IF_NOT_INITIALIZED(false);

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

        if (!fetchAccessCode()) {
            m_error = FailedToFetchAccessCode;
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

CredentialsDB *CredentialsAccessManager::credentialsDB() const
{
    RETURN_IF_NOT_INITIALIZED(NULL);

    return m_pCredentialsDB;
}

bool CredentialsAccessManager::deployCredentialsSystem()
{
    if (m_CAMConfiguration.m_useEncryption) {
        if (!fetchAccessCode()) {
            m_error = FailedToFetchAccessCode;
            return false;
        }

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

bool CredentialsAccessManager::fetchAccessCode(uint timeout)
{
    Q_UNUSED(timeout)
    //TODO use device lock code here and remove hardcoded joke 'passphrase'
    m_pCryptoFileSystemManager->setAccessCode("1234");

    return true;
}

bool CredentialsAccessManager::addEncryptionKey(const QByteArray &key, const QByteArray &existingKey)
{
    return m_pCryptoFileSystemManager->addEncryptionKey(key, existingKey);
}

bool CredentialsAccessManager::removeEncryptionKey(const QByteArray &key, const QByteArray &remainingKey)
{
    return m_pCryptoFileSystemManager->removeEncryptionKey(key, remainingKey);
}
