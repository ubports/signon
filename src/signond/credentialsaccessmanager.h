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

/*!
  @file credentialsaccessmanager.h
  Definition of the CredentialsAccessManager object.
  @ingroup Accounts_and_SSO_Framework
 */

#ifndef CREDENTIALS_ACCESS_MANAGER_H
#define CREDENTIALS_ACCESS_MANAGER_H

#include "credentialsdb.h"
#include "cryptomanager.h"
#include "signonui_interface.h"

#include <QObject>
#include <QPointer>
#include <QFlags>

#include "SignOn/AbstractKeyManager"

/*! @def SIGNON_SECURE_STORAGE_NOT_AVAILABLE
    Use this event type to signal the CAM when the secure storage is
    not available. CAM can also reply with a event of this type
    if it doesn't manage to resolve the secure storage access.
    @sa SecureStorageEvent
*/
#define SIGNON_SECURE_STORAGE_NOT_AVAILABLE (QEvent::User + 1001)

/*! @def SIGNON_SECURE_STORAGE_AVAILABLE
    The CAM will reply with an event of this type when
    the secure storage access will be successfully resolved.
    @sa SecureStorageEvent
*/
#define SIGNON_SECURE_STORAGE_AVAILABLE (QEvent::User + 1002)

/**
 * Manager access to the Credentials DB - synchronized singleton available everywhere in
 * the Authentication Core module.
 */
namespace SignonDaemonNS {

/*!
    @typedef EventSender Guarded pointer for event sending objects.
 */
typedef QPointer<QObject> EventSender;

/*!
    @class SecureStorageEvent
    Any object in the signon framework that needs the
    CredentialsAccessManager - CAM -  secure storage in order
    to function properly can signal this event to the CAM.
    The object posting this event should reimplement the
    QObject::customEvent(QEvent *event) method and handle response
    events of the same type coming from the CAM upon success/failure
    in resolving access to the secure storage.
    @sa SIGNON_SECURE_STORAGE_NOT_AVAILABLE
    @sa SIGNON_SECURE_STORAGE_AVAILABLE
 */
class SecureStorageEvent : public QEvent
{
public:

    SecureStorageEvent(QEvent::Type type)
        : QEvent(type),
          m_sender(0)
    {}

    EventSender m_sender;    /*! << The sender object of the event. */
};

/*!
    @class CAMConfiguration
    Configuration object for the CredentialsAccessManager - CAM.
    @ingroup Accounts_and_SSO_Framework
 */
struct CAMConfiguration
{
    /*!
      Constructs a CAMConfiguration object with the default configuration - encryption in use.
    */
    CAMConfiguration();

    /*!
      Serializes the CAMConfiguration object as string to a specific IODevice.
      @param device, must not be null.
    */
    void serialize(QIODevice *device);

    /*!
     * Returns the path to the metadata DB.
     */
    QString metadataDBPath() const;

    /*!
     * Returns the path of the encrypted FS.
     */
    QString encryptedFSPath() const;

    QString m_storagePath;      /*!< The base directory for storage. */
    QString m_dbName;           /*!< The database file name. */
    bool m_useEncryption;       /*!< Flag for encryption use, enables/disables all of the bellow. */
    QString m_fileSystemType;   /*!< The encrypted file system type (ext2, ext3, ext4). */
    quint32 m_fileSystemSize;   /*!< The encrypted file system size. */
    QByteArray m_encryptionPassphrase; /*!< Passphrase used for opening encrypted FS. */
};

/*!
  @enum, error reported by the CAM via the lastError() method.
*/
enum CredentialsAccessError {
    NoError = 0,
    NotInitialized,
    AlreadyInitialized,
    AccessCodeHandlerInitFailed,
    AccessCodeNotReady,
    FailedToFetchAccessCode,
    AccessCodeInvalid,
    EncryptionInUse,
    CredentialsDbSetupFailed,
    CredentialsDbMountFailed,
    CredentialsDbUnmountFailed,
    CredentialsDbDeletionFailed,
    CredentialsDbAlreadyDeployed,
    CredentialsDbAlreadyMounted,
    CredentialsDbNotMounted,
    CredentialsDbConnectionError,
    CredentialsDbSqlError,
    UnknownError
};

/*!
    @class CredentialsAccessManager
    Main singleton and manager object of the credentials database system.
    Offers access to the CredentialsDB and AccessControl objects, using a specific configuration
    (e.g. Access to a SQL database created on an encrypted file system which is mounted by this system).
    Most calls of this object's methods return false or NULL in case of
    failure; the specific error code can be retrieved by calling the
    lastError() method.
    @ingroup Accounts_and_SSO_Framework
    @sa CredentialsDB
    @sa AccessControl
    @sa AccessCodeHandler
    @sa CryptoManager
 */
class CredentialsAccessManager : public QObject
{
    Q_OBJECT

    /*!
        Constructs a CredentialsAccessManager object with the given parent.
        @param parent
    */
    CredentialsAccessManager(QObject *parent = 0);

    /*!
       @enum KeySwapAuthorizingMech
       Core key authorization is performed through key swapping mechanisms.
       This feature becomes available when the number of inserted authorized keys
       reaches `0`.
       - If the mechanism is `Disabled`, signon core will attemp authorizing keys
       only through available AbstractKeyManager implementations that support
       key authorization.
       - If the mechanism is `AuthorizedKeyRemovedFirst`, signon core will attempt
       authorization of a newly inserted key using its internal already authorized
       keys collection.
       - If the mechanism is `UnauthorizedKeyRemovedFirst`, signon core will attempt
       authorization of a previously disabled unauthorized key, if the last physically
       inserted key is already authorized. In this case the disabled unauthorized
       key was cached when disabled, prior to its authorization.
     */
    enum KeySwapAuthorizingMech {
        Disabled = 0,               /**< Signon core does not authorize keys. */
        AuthorizedKeyRemovedFirst,  /**< The key swap order is as suggested. */
        UnauthorizedKeyRemovedFirst /**< The key swap order is as suggested. */
    };

    /*!
      @enum StorageUiCleanupFlag
      Specific options for cleaning up resources upon secure storage UI closure.
      @sa KeySwapAuthorizingMech
     */
    enum StorageUiCleanupFlag {
        NoFlags = 0,                 /**< No flags. */
        DisableCoreKeyAuthorization  /**< Disable any core key authorization mechanism
                                          and clears any unauthorized cached key that is
                                          not physically inserted into the device. */
    };
    Q_DECLARE_FLAGS(StorageUiCleanupFlags, StorageUiCleanupFlag)

public:

    /*!
        Destroys a CredentialsAccessManager.
        Closes the credentials access system
         - closes the database connection
         - unmounts the dedicated encrypted filesystem, if in use.
    */
    ~CredentialsAccessManager();

    /*!
        Creates the CAM instance with the given parent.
        @param parent
    */
    static CredentialsAccessManager *instance(QObject *parent = 0);

    /*!
        Initializes the CAM instance with the given configuration.
        If encryption is in use, this will start the key managers and
        create the CryptoManager object, preparing everything for the
        mounting of the encrypted file system.
        @param configuration
    */
    bool init(const CAMConfiguration &configuration = CAMConfiguration());

    /*!
        Finalizes the CAM instance, this could include, closing the credentials system
        and resetting the configuration. After this call the CAM needs to be reinitialized.
    */
    void finalize();

    /*!
     * Adds a key manager. This method must be called before init().
     * @param keyManager The key manager to add.
     */
    void addKeyManager(SignOn::AbstractKeyManager *keyManager);

    /*!
        Opens the credentials system, creates the CreadentialsDB object;
        if encryption is configured this will also mount the encrypted file system, based on
        the AccessControlHandler obtained keys.
        First call of this method on a specific platform also does the
        formatting prior to the effective opening.

        @returns true on success, false otherwise. Call lastError() to get
        the error code.
    */
    bool openCredentialsSystem();

    /*!
        Closes the credentials system
            - closes the database connection
            - if encryption is in use, unmounts the encrypted file system
        This is also called by the destructor.

        @returns true on success, false otherwise. Call lastError() to get
        the error code.
    */
    bool closeCredentialsSystem();

    /*!
        Deletes the credentials system.
            - deletes the credentials database
            - if encryption is in use the encrypted file system will be deleted
        @warning use this carefully. Upon successful completion this call deletes all the stored credentials.

        @returns true on success, false otherwise. Call lastError() to get
        the error code.
    */
    bool deleteCredentialsSystem();

    /*!
      For convenience method.
      @returns true if the credentials system is opened, false otherwise.
    */
    bool credentialsSystemOpened() const { return m_systemOpened; }

    /*!
      The creadentials system is ready when all of the subscribed key managers
      have successfully reported all of the inserted keys. The credentials
      system can be ready while at the same time the secure storage is not opened.
      @returns true if the credentials system is ready, false otherwise.
    */
    bool isCredentialsSystemReady() const { return m_systemReady; }

    /*!
      @returns the credentials database object.
    */
    CredentialsDB *credentialsDB() const;

    /*!
      @returns the CAM in use configuration.
    */
    const CAMConfiguration &configuration() const { return m_CAMConfiguration; }

    /*!
      @sa CredentialsAccessError
      @returns the last CAM's internally reported error.
    */
    CredentialsAccessError lastError() const { return m_error; }

    /*!
      The CAM manages the encryption keys collection.
      For convenience method.
      @returns whether the CAM detected any encryption keys or not.
    */
    bool keysAvailable() const;

Q_SIGNALS:
    /*!
      Is emitted when the credentials system becomes ready.
    */
    void credentialsSystemReady();

private Q_SLOTS:
    void onKeyInserted(const SignOn::Key key);
    void onKeyDisabled(const SignOn::Key key);
    void onKeyRemoved(const SignOn::Key key);
    void onKeyAuthorized(const SignOn::Key key, bool authorized);
    void onClearPasswordsStorage();
    void onSecureStorageUiRejected();
    void onNoKeyPresentAccepted();
    void onEncryptedFSMounted();
    void onEncryptedFSUnmounting();

protected:
    void customEvent(QEvent *event);

private:
    // 1st time start - deploys the database.
    void initKeyManagers();
    bool deployCredentialsSystem();
    bool openSecretsDB();
    bool isSecretsDBOpen();
    bool closeSecretsDB();
    bool openMetaDataDB();
    void closeMetaDataDB();
    bool fileSystemDeployed();
    void queryEncryptionKeys();
    void replyToSecureStorageEventNotifiers();
    bool processSecureStorageEvent();

    void onSecureStorageUiClosed(const StorageUiCleanupFlags flags = NoFlags);

    /*!
     * Checks if the key can open the secure storage. If it can, the file system
     * is also mounted and the secrets' storage is opened.
     */
    bool encryptionKeyCanOpenStorage(const QByteArray &key);

    /*!
      Checks if core ecryption key authorizing is enabled using a specific
      key swap authorizing mechanism.
      @param mech The mechanism to be checked.
      @returns true if the specific mechanism is enabled, false if otherwise
               or no mechanism is enabled at all. This method will always return
               false if `mech` is `KeySwapAuthorizingMech::Disabled`.
    */
    bool coreKeyAuthorizingEnabled(const KeySwapAuthorizingMech mech) const;

    /*!
      Sets the core encryption key authorization mechanism.
      @param mech The authorization mechanism.
    */
    void setCoreKeyAuthorizationMech(const KeySwapAuthorizingMech mech);

    /*!
      @returns the set of currently inserted authorized keys.
    */
    QSet<SignOn::Key> authorizedInsertedKeys() const;

private:
    static CredentialsAccessManager *m_pInstance;

    bool m_isInitialized;
    bool m_systemOpened;
    /* Flag indicating whether the system is ready or not.
     * Currently the system is ready when all of the key managers have
     * successfully reported all of the inserted keys. */
    bool m_systemReady;
    mutable CredentialsAccessError m_error;
    QList<SignOn::AbstractKeyManager *> keyManagers;

    /* Counter for the key managers that are ready
     * - have signaled at least one key (can be empty).
     * This helps in computing when the CAM is ready;
     * see also credentialsSystemReady(). */
    int readyKeyManagersCounter;

    /* Collection of authorized keys, gathered through the lifetime of the
     * signond process. */
    QList<SignOn::Key> authorizedKeys;
    /* Collection of physically inserted keys at a specific runtime moment. */
    QList<SignOn::Key> insertedKeys;

    /* Flag indicating if the CAM is in the middle of processing
     * a secure storage event. */
    bool processingSecureStorageEvent;
    /* This member will cache the lastly disabled unauthorized
     * key. The cached key will be cleared if any secure storage UI
     * disaplyed at its removal time is closed by the user.
     * Check the KeySwapAuthorizingMech::UnauthorizedKeyRemovedFirst
     * documentation for additional details. */
    QByteArray cachedUnauthorizedKey;

    /* Check the KeySwapAuthorizingMech definitions' documentation. */
    KeySwapAuthorizingMech keySwapAuthorizingMechanism;

    CredentialsDB *m_pCredentialsDB;
    CryptoManager *m_pCryptoFileSystemManager;
    CAMConfiguration m_CAMConfiguration;

    /* List of all the senders of a SecureStorageEvent. */
    QList<EventSender> m_secureStorageEventNotifiers;

    SignonSecureStorageUiAdaptor *m_secureStorageUiAdaptor;
};

} //namespace SignonDaemonNS

#endif // CREDENTIALS_ACCESS_MANAGER_H
