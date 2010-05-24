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
#include "accesscodehandler.h"
#include "cryptomanager.h"

#include <QObject>


/**
 * Manager access to the Credentials DB - synchronized singletone available everywhere in
 * the Authentication Core module.
 */
namespace SignonDaemonNS {

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


        QString m_dbName;           /*!< The database file name. */
        bool m_useEncryption;       /*!< Flag for encryption use, enables/disables all of the bellow. */
        QString m_dbFileSystemPath; /*!< The path of the encrypted file system. */
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
            If encryption is in use, this will start the AccessCodeHandler and create the CryptoManager object,
            preparing everything for the mounting of the encrypted file system
            @param configuration
        */
        bool init(const CAMConfiguration &configuration = CAMConfiguration());

        /*!
            Finalizes the CAM instance, this could include, closing the credentials system
            and resetting the configuration. After this call the CAM needs to be reinitialized.
        */
        void finalize();

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
          Add an encryption key to one of the available keyslots for the encrypted storage.
          @param key The key to be added.
          @param existingKey An already existing key.
          @returns true, if succeeded, false otherwise.
        */
        bool setDeviceLockCodeKey(const QByteArray &newLockCode,
                                  const QByteArray &existingLockCode);

        /*!
          Locks the secure storage.
          @param lockKey The key for locking the secure storage.
          @todo Figure this out.
          @returns true, if succeeded, false otherwise.
        */
        bool lockSecureStorage(const QByteArray &lockKey);

    private Q_SLOTS:
        void accessCodeFetched(const QByteArray &accessCode);

    private:
        // 1st time start - deploys the database.
        bool deployCredentialsSystem();
        bool openCredentialsSystemPriv();
        bool fileSystemLoaded(bool checkForDatabase = false);
        bool fileSystemDeployed();
        bool fetchAccessCode(uint timeout = 30);
        bool openDB(const QString &databaseName);
        bool closeDB();

    private:
        static CredentialsAccessManager *m_pInstance;

        bool m_isInitialized;
        bool m_accessCodeFetched;
        bool m_systemOpened;
        mutable CredentialsAccessError m_error;

        CredentialsDB *m_pCredentialsDB; // make this a QSharedPointer
        CryptoManager *m_pCryptoFileSystemManager;
        AccessCodeHandler *m_pAccessCodeHandler;
        CAMConfiguration m_CAMConfiguration;
    };

} //namespace SignonDaemonNS

#endif // CREDENTIALS_ACCESS_MANAGER_H
