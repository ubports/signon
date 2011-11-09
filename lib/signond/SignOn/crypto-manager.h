/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2011 Nokia Corporation.
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

/*!
  @file cryptomanager.h
  Definition of the CryptoManager object.
  @ingroup Accounts_and_SSO_Framework
 */

#ifndef SIGNON_CRYPTO_MANAGER_H
#define SIGNON_CRYPTO_MANAGER_H

#ifdef SIGNON_ENABLE_UNSTABLE_APIS

#include "SignOn/export.h"

#include <QObject>

#define MINUMUM_ENCRYPTED_FILE_SYSTEM_SIZE 4

namespace SignOn {

    /*!
        @class CryptoManager
        Encrypted file system manager. Uses cryptsetup and LUKS.
        @ingroup Accounts_and_SSO_Framework
    */
    class SIGNON_EXPORT CryptoManager : public QObject
    {
        Q_OBJECT

        // DO NOT change the order of the enum values!!!
        enum FileSystemMountState {
            Unmounted = 0,
            LoopSet,
            LoopLuksFormatted,
            LoopLuksOpened,
            Mounted
        };

    public:

        /*!
          @enum FileSystemType
          Supported encrypted partion filesystem type.
        */
        enum FileSystemType {
            Ext2 = 0,
            Ext3,
            Ext4
        };

        /*!
            Constructs a CryptoManager object with the given parent.
            @param parent
        */
        CryptoManager(QObject *parent = 0);

        /*!
            Constructs a CryptoManager object with the given access code, file system name and parent.
            @param accessCode Key for the encrypted file system
            @param fileSystemPath path of the ecnrypted file system's file/device
            @param parent Parent object
        */
        CryptoManager(const QByteArray &encryptionKey, const QString &fileSystemPath, QObject *parent = 0);

        /*!
            Destroys a CryptoManager object.
        */
        ~CryptoManager();

        /*!
            Sets the encryption key.
            @param key, the encrypted file system key.
        */
        void setEncryptionKey(const QByteArray &key);

        /*!
            @return The in use encryption key.
        */
        QByteArray encryptionKey() const { return m_accessCode; }

        /*!
            Sets the file system type.
            @param type The file system's type.
            @see FileSystemType
        */
        void setFileSystemType(const FileSystemType type) { m_fileSystemType = type; }

        /*!
            Overloaded, for convenience method.
            @param type The file system's type as string.
        */
        bool setFileSystemType(const QString &type);

        /*!
            @return the file system's type.
            @see FileSystemType
        */
        FileSystemType fileSystemType() const { return m_fileSystemType; }

        /*!
            @param size The size of the file system (Mb).
            @returns true if the call succeeded, false otherwise (minimum 4 Mb).
        */
        bool setFileSystemSize(const quint32 size);

        /*!
            @return the file system's size (Mb).
        */
        quint32 fileSystemSize() const { return m_fileSystemSize; }

        /*!
            Sets the file system's path.
            @param path The path of the file system's file/source.
        */
        void setFileSystemPath(const QString &path);

        /*!
            @returns the path of the file system.
        */
        QString fileSystemPath() const { return m_fileSystemPath; }

        /*!
            Sets up an encrypted file system. This method is to be called only at the file system creation/foramtting.
            Use mountFileSystem() on subsequent uses. This method handles also the mounting so when using it, a call
            to mountFileSystem() is not necessary.
            @returns true, if successful, false otherwise.
            @warning this method will always format the file system, use carefully.
        */
        bool setupFileSystem();

        /*!
            Mounts the encrypted file system.
            @returns true, if successful, false otherwise.
        */
        bool mountFileSystem();

        /*!
            Unmounts the encrypted file system.
            @returns true, if successful, false otherwise.
        */
        bool unmountFileSystem();

        /*!
            Deletes the encrypted file system.
            @returns true, if successful, false otherwise.
            @warning use this carefully, this will lead to data loss.
            @todo finish implemetation.
        */
        bool deleteFileSystem();

        /*!
            @returns true if the file system is mounted, false otherwise.
        */
        bool fileSystemIsMounted() const { return m_mountState == Mounted; }

        /*!
            @returns true if the file system is setup, false otherwise.
        */
        bool fileSystemIsSetup() const;

        /*!
            @returns true, if the file system contains a specific file.
            @attention The file system must be mounted prior to calling this.
        */
        bool fileSystemContainsFile(const QString &filePath);

        /*!
            @returns the path of the mounted file system.
        */
        QString fileSystemMountPath() const;

        /*!
            @attention if the file system is not mounted and the encryption
            key can access it, this method will cause the file system to be
            mounted.
            @returns whether the key `key` is occupying a keyslot in the encrypted file system.
        */
        bool encryptionKeyInUse(const QByteArray &key);

        /*!
            Adds an encryption key to one of the available keyslots of the LUKS partition's header.
            Use the `keyTag` parameter in order to store and keep track of the key.
            @sa isEncryptionKey(const QByteArray &key)
            @param key The key to be added/set.
            @param existingKey An already existing key.
            @returns true, if succeeded, false otherwise.
        */
        bool addEncryptionKey(const QByteArray &key,
                              const QByteArray &existingKey);

        /*!
            Releases an existing used keyslot in the LUKS partition's header.
            @param key The key to be removed.
            @param remainingKey Another valid key
            @attention The system cannot remain keyless.
            @returns true, if succeeded, false otherwise.
        */
        bool removeEncryptionKey(const QByteArray &key,
                                 const QByteArray &remainingKey);

    Q_SIGNALS:
        void fileSystemMounted();
        void fileSystemUnmounting();

    private:
        void clearFileSystemResources();
        bool mountMappedDevice();
        bool unmountMappedDevice();
        void updateMountState(const FileSystemMountState state);

        static bool createPartitionFile(const QString &filePath);
        static bool formatMapFileSystem(const QString &fileSystemPath);

        const QString keychainFilePath() const;
        void addKeyToKeychain(const QByteArray &key) const;
        void removeKeyFromKeychain(const QByteArray &key) const;
        bool keychainContainsKey(const QByteArray &key) const;

    private:
        //TODO remove this
        void serializeData();

    private:
        QByteArray m_accessCode;
        QString m_fileSystemPath;
        QString m_fileSystemMapPath;
        QString m_fileSystemName;
        QString m_fileSystemMountPath;
        QString m_loopDeviceName;

        FileSystemMountState m_mountState;
        FileSystemType m_fileSystemType;
        quint32 m_fileSystemSize;
    };

} //namespace SignonDaemonNS

#endif  // SIGNON_ENABLE_UNSTABLE_APIS

#endif // SIGNON_CRYPTOMANAGER_H
