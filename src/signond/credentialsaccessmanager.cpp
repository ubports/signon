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
        : m_storagePath(QLatin1String(signonDefaultStoragePath)),
          m_dbName(QLatin1String(signonDefaultDbName)),
          m_useEncryption(signonDefaultUseEncryption),
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
    stream << "File system mount name " << encryptedFSPath() << '\n';
    stream << "File system format: " << m_fileSystemType << '\n';
    stream << "File system size:" << m_fileSystemSize << "megabytes\n";

    const char *usingEncryption = m_useEncryption ? "true" : "false";
    stream << "Using encryption: " << usingEncryption << '\n';
    stream << "Credentials database name: " << m_dbName << '\n';
    stream << "======================================================\n\n";
    device->write(buffer.toUtf8());
    device->close();
}

QString CAMConfiguration::metadataDBPath() const
{
    return m_storagePath + QDir::separator() + m_dbName;
}

QString CAMConfiguration::encryptedFSPath() const
{
    return m_storagePath +
        QDir::separator() +
        QLatin1String(signonDefaultFileSystemName);
}

/* ---------------------- CredentialsAccessManager ---------------------- */

CredentialsAccessManager *CredentialsAccessManager::m_pInstance = NULL;

CredentialsAccessManager::CredentialsAccessManager(QObject *parent)
        : QObject(parent),
          m_isInitialized(false),
          m_systemOpened(false),
          m_systemReady(false),
          m_error(NoError),
          keyManagers(),
          readyKeyManagersCounter(0),
          processingSecureStorageEvent(false),
          keySwapAuthorizingMechanism(Disabled),
          m_pCredentialsDB(NULL),
          m_pCryptoFileSystemManager(NULL),
          m_CAMConfiguration(CAMConfiguration()),
          m_secureStorageUiAdaptor(NULL)
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

    // Disconnect all key managers
    foreach (SignOn::AbstractKeyManager *keyManager, keyManagers)
        keyManager->disconnect();

    m_isInitialized = false;
    m_systemReady = false;
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

    m_systemReady = true;

    if (m_CAMConfiguration.m_useEncryption) {
        //Initialize CryptoManager
        m_pCryptoFileSystemManager = new CryptoManager(this);
        QObject::connect(m_pCryptoFileSystemManager, SIGNAL(fileSystemMounted()),
                         this, SLOT(onEncryptedFSMounted()));
        QObject::connect(m_pCryptoFileSystemManager, SIGNAL(fileSystemUnmounting()),
                         this, SLOT(onEncryptedFSUnmounting()));
        m_pCryptoFileSystemManager->setFileSystemPath(m_CAMConfiguration.encryptedFSPath());
        m_pCryptoFileSystemManager->setFileSystemSize(m_CAMConfiguration.m_fileSystemSize);
        m_pCryptoFileSystemManager->setFileSystemType(m_CAMConfiguration.m_fileSystemType);

        initKeyManagers();
    }

    m_isInitialized = true;
    m_error = NoError;

    TRACE() << "CredentialsAccessManager successfully initialized...";
    return true;
}

void CredentialsAccessManager::initKeyManagers()
{
    if (keyManagers.isEmpty()) {
        BLAME() << "NO Key Manager was subscribed to the CAM.";
        return;
    }
    m_systemReady = false;

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
        if (keyManager->supportsKeyAuthorization()) {
            connect(keyManager,
                    SIGNAL(keyAuthorized(const SignOn::Key, bool)),
                    SLOT(onKeyAuthorized(const SignOn::Key, bool)));
        }
        keyManager->setup();
    }
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

        if (!m_pCryptoFileSystemManager->fileSystemMounted()) {
            /* Do not attempt to mount the FS; we know that it will be mounted
             * automatically, as soon as some encryption keys are provided */
            m_error = CredentialsDbNotMounted;
            return false;
        }
    } else {
        dbPath = m_CAMConfiguration.metadataDBPath() + QLatin1String(".creds");
    }

    TRACE() << "Database name: [" << dbPath << "]";

    if (!m_pCredentialsDB->openSecretsDB(dbPath))
        return false;

    m_error = NoError;
    return true;
}

bool CredentialsAccessManager::isSecretsDBOpen()
{
    return m_pCredentialsDB->isSecretsDBOpen();
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
    QString dbPath = m_CAMConfiguration.metadataDBPath();
    QFileInfo fileInfo(dbPath);
    if (!fileInfo.exists()) {
        QDir storageDir(fileInfo.dir());
        if (!storageDir.mkpath(storageDir.path())) {
            BLAME() << "Could not create storage directory:" <<
                storageDir.path();
            m_error = CredentialsDbSetupFailed;
            return false;
        }
        //Set the right permissions for the storage directory
        QFile storageDirAsFile(storageDir.path());
        QFile::Permissions permissions = storageDirAsFile.permissions();
        QFile::Permissions desiredPermissions = permissions
            | QFile::WriteGroup | QFile::ReadGroup
            | QFile::WriteOther | QFile::ReadOther;

        if (permissions != desiredPermissions)
            storageDirAsFile.setPermissions(desiredPermissions);
    }

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

    m_systemOpened = true;
    return true;
}

bool CredentialsAccessManager::closeCredentialsSystem()
{
    RETURN_IF_NOT_INITIALIZED(false);

    if (!credentialsSystemOpened())
        return true;

    bool allClosed = true;
    if (isSecretsDBOpen() && !closeSecretsDB())
        allClosed = false;

    closeMetaDataDB();

    m_error = NoError;
    m_systemOpened = false;
    return allClosed;
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
        if (!m_pCryptoFileSystemManager->setupFileSystem()) {
            m_error = CredentialsDbSetupFailed;
            return false;
        }
    }
    return true;
}

bool CredentialsAccessManager::fileSystemDeployed()
{
    return QFile::exists(m_pCryptoFileSystemManager->fileSystemPath());
}

bool CredentialsAccessManager::encryptionKeyCanOpenStorage(const QByteArray &key)
{
    if (!fileSystemDeployed()) {
        TRACE() << "Secure FS not deployed, deploying now...";
        m_pCryptoFileSystemManager->setEncryptionKey(key);

        if (!deployCredentialsSystem()) {
            BLAME() << "Could not deploy encrypted file system.";
            return false;
        }
    }

    if (m_pCryptoFileSystemManager->encryptionKeyInUse(key)) {
        TRACE() << "Key already in use.";
        return true;
    }

    return false;
}

void CredentialsAccessManager::onKeyInserted(const SignOn::Key key)
{
    TRACE() << "Key inserted.";

    if (!m_systemReady) {
        /* Check that at least one non empty key was signaled or if not so,
         * that all key managers have signaled at least one key. */
        ++readyKeyManagersCounter;
        if (!key.isEmpty() || (readyKeyManagersCounter == keyManagers.count())) {
            m_systemReady = true;
            emit credentialsSystemReady();
        }
    }

    if (key.isEmpty()) return;

    //Close the secure storage UI 1st
    if (m_secureStorageUiAdaptor)
        m_secureStorageUiAdaptor->closeUi();

    insertedKeys.append(key);

    /* The `key in use` check will attempt to mount using the new key if
       the file system is not already mounted
    */
    if (encryptionKeyCanOpenStorage(key)) {
        TRACE() << "Key already in use.";
        if (!authorizedKeys.contains(key))
            authorizedKeys << key;

        if (coreKeyAuthorizingEnabled(UnauthorizedKeyRemovedFirst))
            onKeyAuthorized(cachedUnauthorizedKey, true);

        return;
    }

    if (coreKeyAuthorizingEnabled(AuthorizedKeyRemovedFirst)) {
        onKeyAuthorized(key, true);
    } else {
        /* We got here because the inserted key is totally new to the CAM
         * and the core key authizing mechanisms were disabled.
         * Let's see if any key manager wants to authorize it: we call
         * authorizeKey() on each of the key mangers that support
         * authorizing keys, and continue processing the key when
         * the keyAuthorized() signal comes.
         */
        foreach (SignOn::AbstractKeyManager *keyManager, keyManagers) {
            if (keyManager->supportsKeyAuthorization())
                keyManager->authorizeKey(key);
        }
    }
}

void CredentialsAccessManager::onKeyDisabled(const SignOn::Key key)
{
    TRACE() << "Key disabled.";

    insertedKeys.removeAll(key);

    /* If no authorized inserted keys left, enable the suitable core key
     * authorizing mechanism and close the secure storage. */
    if (authorizedInsertedKeys().isEmpty()) {
        if (processingSecureStorageEvent) {
            /* If while processing a secure storage event, cache the disabled
             * key if it was unauthorized and enable the
             * `UnauthorizedKeyRemovedFirst` mechanism. */
            if (!authorizedKeys.contains(key)) {
                setCoreKeyAuthorizationMech(UnauthorizedKeyRemovedFirst);
                cachedUnauthorizedKey = key;
            }
        } else  if (authorizedKeys.contains(key)) {
            /* If the last disabled key was an authorized one enable the
               `AuthorizedKeyRemovedFirst` mechanism and notify the user. */
            TRACE() << "All inserted keys disabled, notifying user.";
            if (m_secureStorageUiAdaptor == 0) {
                m_secureStorageUiAdaptor =
                    new SignonSecureStorageUiAdaptor(
                        SIGNON_UI_SERVICE,
                        SIGNON_UI_DAEMON_OBJECTPATH,
                        SIGNOND_BUS);
            }

            connect(m_secureStorageUiAdaptor,
                    SIGNAL(noKeyPresentAccepted()),
                    SLOT(onNoKeyPresentAccepted()));
            connect(m_secureStorageUiAdaptor,
                    SIGNAL(uiRejected()),
                    SLOT(onSecureStorageUiRejected()));
            connect(m_secureStorageUiAdaptor,
                    SIGNAL(error()),
                    SLOT(onSecureStorageUiRejected()));

            m_secureStorageUiAdaptor->notifyNoKeyPresent();
            setCoreKeyAuthorizationMech(AuthorizedKeyRemovedFirst);
        }

        TRACE() << "All keys disabled. Closing secure storage.";
        if (isSecretsDBOpen() || m_pCryptoFileSystemManager->fileSystemMounted())
            if (!closeSecretsDB())
                BLAME() << "Error occurred while closing secure storage.";

        TRACE() << "Querying for keys.";
        queryEncryptionKeys();
    }
}

void CredentialsAccessManager::onKeyRemoved(const SignOn::Key key)
{
    TRACE() << "Key removed.";

    // Make sure the key is disabled:
    onKeyDisabled(key);

    if (authorizedKeys.isEmpty()) {
        BLAME() << "Cannot remove key: no authorized keys";
        return;
    }

    if (!encryptionKeyCanOpenStorage(key)) {
        TRACE() << "Key is not known to the CryptoManager.";
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
    TRACE() << "Key authorized:" << authorized;

    if (!authorized || key.isEmpty()) return;

    if (encryptionKeyCanOpenStorage(key)) {
        TRACE() << "Encryption key already in use.";
        if (!authorizedKeys.contains(key))
            authorizedKeys << key;

        return;
    }

    /* Make sure that the secure file system is mounted, so that the key `key`
     * can be successfully authorized by the encryption backend.
     */
    if (!m_pCryptoFileSystemManager->fileSystemMounted()) {

        m_pCryptoFileSystemManager->setEncryptionKey(authorizedKeys.first());
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

        /* If the key was authorized through the `UnauthorizedKeyRemovedFirst`
         * mechanism notify the user. */
        if (coreKeyAuthorizingEnabled(UnauthorizedKeyRemovedFirst)) {
            //Notify only if indeed key was authorized
            if (m_secureStorageUiAdaptor && authorizedKeys.contains(key))
                m_secureStorageUiAdaptor->notifyKeyAuthorized();

            //cleanup secure storage ui related data
            onSecureStorageUiClosed(DisableCoreKeyAuthorization);
        }
    } else if (!fileSystemDeployed()) {
        /* if the secure FS does not exist, create it and use this new key to
         * initialize it */
        m_pCryptoFileSystemManager->setEncryptionKey(key);
        if (!authorizedKeys.contains(key)) {
            authorizedKeys << key;
        } else {
            BLAME() << "Couldn't create the secure FS";
        }
    } else  {
        TRACE() << "Secure FS already created with another set of keys.";
    }
}

void CredentialsAccessManager::queryEncryptionKeys()
{
    /* TODO extend the KeyManager interface to specify if querying or authorizing
     *      keys implies the usage of a UI, and implement a Publish/subscribe
     *      serialized/chained pattern to remove the possibility of displaying
     *      multiple UIs at once. */
    foreach (SignOn::AbstractKeyManager *keyManager, keyManagers)
        keyManager->queryKeys();
}

bool CredentialsAccessManager::keysAvailable() const
{
    return insertedKeys.count() > 0;
}

void
CredentialsAccessManager::setCoreKeyAuthorizationMech(
    const KeySwapAuthorizingMech mech)
{
    keySwapAuthorizingMechanism = mech;
}

bool
CredentialsAccessManager::coreKeyAuthorizingEnabled(
    const KeySwapAuthorizingMech mech) const
{
    /* All flags/values checked in this method are subject to
     * change based on secure storage UI user actions, or based
     * on physically inserting/removing keys without any prior
     * secure storage UI user actions. */

    /* Always return false if the internal mechanism is disabled. */
    if (keySwapAuthorizingMechanism == Disabled)
        return false;

    /* If key swapping is:
     *      1) Remove authorized key
     *      2) Insert unauthorized key */
    if (mech == AuthorizedKeyRemovedFirst)
        return keySwapAuthorizingMechanism == AuthorizedKeyRemovedFirst;

    /* If key swapping is:
     *      1) Remove unauthorized key
     *      2) Insert authorized key */
    if (mech == UnauthorizedKeyRemovedFirst)
        return (keySwapAuthorizingMechanism == UnauthorizedKeyRemovedFirst)
               && (!cachedUnauthorizedKey.isEmpty());

    return false;
}

QSet<SignOn::Key> CredentialsAccessManager::authorizedInsertedKeys() const
{
    return insertedKeys.toSet().
        intersect(authorizedKeys.toSet());
}

void CredentialsAccessManager::onNoKeyPresentAccepted()
{
    onSecureStorageUiClosed();
    //enforce the setting of the core key authorization mechanism
    setCoreKeyAuthorizationMech(AuthorizedKeyRemovedFirst);
}

void CredentialsAccessManager::onClearPasswordsStorage()
{
    if (insertedKeys.isEmpty()) {
        TRACE() << "No keys available. The reformatting of the secure storage skipped.";
        onSecureStorageUiClosed(DisableCoreKeyAuthorization);
        return;
    }

    TRACE() << "Reformatting secure storage.";

    /* Here we use the 1st inserted unauthorized key for storage reformatting.
     * This key was present before the emitting of the secure storage event that
     * lead to the UI decision to reformat the passwords storage. */
    m_pCryptoFileSystemManager->setEncryptionKey(insertedKeys.first());
    if (m_pCryptoFileSystemManager->setupFileSystem()) {
        authorizedKeys.clear();
        authorizedKeys << insertedKeys.first();

        if (m_secureStorageUiAdaptor) {
            m_secureStorageUiAdaptor->notifyStorageCleared();
        }

    } else {
        BLAME() << "Failed to reformat secure storage file system.";
    }

    onSecureStorageUiClosed(DisableCoreKeyAuthorization);
}

void
CredentialsAccessManager::onSecureStorageUiClosed(
    const StorageUiCleanupFlags options)
{
    if (m_secureStorageUiAdaptor) {
        delete m_secureStorageUiAdaptor;
        m_secureStorageUiAdaptor = 0;
    }

    if (processingSecureStorageEvent) {
        processingSecureStorageEvent = false;
        replyToSecureStorageEventNotifiers();
    }

    if (options.testFlag(DisableCoreKeyAuthorization)) {
        setCoreKeyAuthorizationMech(Disabled);
        cachedUnauthorizedKey.clear();
    }
}

void CredentialsAccessManager::onSecureStorageUiRejected()
{
    onSecureStorageUiClosed(DisableCoreKeyAuthorization);
}

void CredentialsAccessManager::replyToSecureStorageEventNotifiers()
{
    TRACE();
    //Notify secure storage notifiers if any.
    int eventType = SIGNON_SECURE_STORAGE_NOT_AVAILABLE;
    if (m_pCredentialsDB->isSecretsDBOpen())
        eventType = SIGNON_SECURE_STORAGE_AVAILABLE;

    // Signal objects that posted secure storage not available events
    foreach (EventSender object, m_secureStorageEventNotifiers) {
        if (object.isNull())
            continue;

        SecureStorageEvent *secureStorageEvent =
            new SecureStorageEvent((QEvent::Type)eventType);

        QCoreApplication::postEvent(
            object.data(),
            secureStorageEvent,
            Qt::HighEventPriority);
    }

    m_secureStorageEventNotifiers.clear();
}

bool CredentialsAccessManager::processSecureStorageEvent()
{
    /* If keys have been inserted but none authorized notify the signon UI.
     * If keys are not present at all - the scenario is not handled
     * by this event hander, as Signon UI was already informed about
     * this in the creadentials query dilog display context. */
    if (!insertedKeys.isEmpty()) {
        TRACE() << "Secure Storage not available notifying user.";
        if (m_secureStorageUiAdaptor == 0) {
            m_secureStorageUiAdaptor =
                new SignonSecureStorageUiAdaptor(
                    SIGNON_UI_SERVICE,
                    SIGNON_UI_DAEMON_OBJECTPATH,
                    SIGNOND_BUS);
        }

        connect(m_secureStorageUiAdaptor,
                SIGNAL(clearPasswordsStorage()),
                SLOT(onClearPasswordsStorage()));
        connect(m_secureStorageUiAdaptor,
                SIGNAL(uiRejected()),
                SLOT(onSecureStorageUiRejected()));
        connect(m_secureStorageUiAdaptor,
                SIGNAL(error()),
                SLOT(onSecureStorageUiRejected()));

        m_secureStorageUiAdaptor->notifyNoAuthorizedKeyPresent();
        processingSecureStorageEvent = true;
        return true;
    }
    return false;
}


void CredentialsAccessManager::customEvent(QEvent *event)
{
    TRACE() << "Custom event received.";
    if (event->type() != SIGNON_SECURE_STORAGE_NOT_AVAILABLE) {
        QObject::customEvent(event);
        return;
    }

    SecureStorageEvent *localEvent =
        static_cast<SecureStorageEvent *>(event);

    /* All senders of this event will receive a reply when
     * the secure storage becomes available or an error occurs. */
    m_secureStorageEventNotifiers.append(localEvent->m_sender);

    TRACE() << "Processing secure storage not available event.";
    if ((localEvent == 0) || (m_pCredentialsDB == 0)) {
        replyToSecureStorageEventNotifiers();
        QObject::customEvent(event);
        return;
    }

    //Double check if the secrets DB is indeed unavailable
    if (m_pCredentialsDB->isSecretsDBOpen()) {
        replyToSecureStorageEventNotifiers();
        QObject::customEvent(event);
        return;
    }

    if (!processingSecureStorageEvent) {
        if (!processSecureStorageEvent()) {
            /* If by any chance the event wasn't properly processed
             * reply immediately. */
            replyToSecureStorageEventNotifiers();
        }
    } else {
        TRACE() << "Already processing a secure storage not available event.";
    }

    QObject::customEvent(event);
}

void CredentialsAccessManager::onEncryptedFSMounted()
{
    TRACE();
    if (!isSecretsDBOpen()) {
        if (openSecretsDB()) {
            TRACE() << "Secrets DB opened.";
        } else {
            BLAME() << "Failed to open secrets DB.";
        }
    } else {
        BLAME() << "Secrets DB already opened?";
    }
}

void CredentialsAccessManager::onEncryptedFSUnmounting()
{
    TRACE();
    if (isSecretsDBOpen()) {
        m_pCredentialsDB->closeSecretsDB();
    }
}

