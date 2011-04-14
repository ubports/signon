/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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


#include "cryptomanager.h"
#include "cryptohandlers.h"
#include "signond-common.h"

#include <QFile>
#include <QDir>
#include <QMetaEnum>
#include <QSettings>

#define DEVICE_MAPPER_DIR "/dev/mapper/"
#define EXT2 "ext2"
#define EXT3 "ext3"
#define EXT4 "ext4"

namespace SignonDaemonNS {

    const QLatin1String keysStorageFileName = QLatin1String("keys");

    CryptoManager::CryptoManager(QObject *parent)
            : QObject(parent),
              m_accessCode(QByteArray()),
              m_fileSystemPath(QString()),
              m_fileSystemMapPath(QString()),
              m_fileSystemName(QString()),
              m_fileSystemMountPath(QString()),
              m_loopDeviceName(QString()),
              m_fileSystemType(Ext2),
              m_fileSystemSize(4)
    {
        updateMountState(Unmounted);
        if (!CryptsetupHandler::loadDmMod())
            BLAME() << "Could not load `dm_mod`!";
    }

    CryptoManager::CryptoManager(const QByteArray &encryptionKey,
                                 const QString &fileSystemPath,
                                 QObject *parent)
            : QObject(parent),
              m_accessCode(encryptionKey),
              m_loopDeviceName(QString()),
              m_fileSystemType(Ext2),
              m_fileSystemSize(4)
    {
        setFileSystemPath(fileSystemPath);
        updateMountState(Unmounted);

        if (!CryptsetupHandler::loadDmMod())
            BLAME() << "Could not load `dm_mod`!";
    }

    CryptoManager::~CryptoManager()
    {
        unmountFileSystem();
    }

    void CryptoManager::setFileSystemPath(const QString &path)
    {
        m_fileSystemPath = path;

        QFileInfo fsFileInfo(path);

        m_fileSystemName = fsFileInfo.fileName();
        m_fileSystemMapPath = QLatin1String(DEVICE_MAPPER_DIR) + m_fileSystemName;
        m_fileSystemMountPath = path + QLatin1String("-mnt");
    }

    bool CryptoManager::setFileSystemSize(const quint32 size)
    {
        if (size < MINUMUM_ENCRYPTED_FILE_SYSTEM_SIZE) {
            TRACE() << "Minumum encrypted file size is 4 Mb.";
            return false;
        }
        m_fileSystemSize = size;
        return true;
    }

    bool CryptoManager::setFileSystemType(const QString &type)
    {
        QString cmpBase = type.toLower();
        if (cmpBase == QLatin1String(EXT2)) {
            m_fileSystemType = Ext2;
            return true;
        } else if (cmpBase == QLatin1String(EXT3)) {
            m_fileSystemType = Ext3;
            return true;
        } else if (cmpBase == QLatin1String(EXT4)) {
            m_fileSystemType = Ext4;
            return true;
        }
        return false;
    }

    bool CryptoManager::setupFileSystem()
    {
        if (m_mountState == Mounted) {
            TRACE() << "Ecrypyted file system already mounted.";
            return false;
        }

        if (m_accessCode.isEmpty()) {
            TRACE() << "No access code set. Stopping mount process.";
            return false;
        }

        if (!CryptsetupHandler::loadDmMod()) {
            BLAME() << "Could not load `dm_mod`!";
            return false;
        }

        clearFileSystemResources();

        m_loopDeviceName = LosetupHandler::findAvailableDevice();
        if (m_loopDeviceName.isNull()) {
            BLAME() << "No free loop device available!";
            return false;
        }

        if (!PartitionHandler::createPartitionFile(m_fileSystemPath, m_fileSystemSize)) {
            BLAME() << "Could not create partition file.";
            unmountFileSystem();
            return false;
        }

        if (!LosetupHandler::setupDevice(m_loopDeviceName,
                                         m_fileSystemPath)) {
            BLAME() << "Failed to setup loop device:" << m_loopDeviceName;
            unmountFileSystem();
            return false;
        }
        updateMountState(LoopSet);

        if (!CryptsetupHandler::formatFile(m_accessCode, m_loopDeviceName)) {
            BLAME() << "Failed to LUKS format.";
            unmountFileSystem();
            return false;
        }
        updateMountState(LoopLuksFormatted);

        //attempt luks close, in case of a leftover.
        if (QFile::exists(QLatin1String(DEVICE_MAPPER_DIR) + m_fileSystemName))
            CryptsetupHandler::closeFile(m_fileSystemName);

        if (!CryptsetupHandler::openFile(m_accessCode,
                                         m_loopDeviceName,
                                         m_fileSystemName)) {
            BLAME() << "Failed to LUKS open";
            unmountFileSystem();
            return false;
        }
        updateMountState(LoopLuksOpened);

        if (!PartitionHandler::formatPartitionFile(m_fileSystemMapPath,
                                                   m_fileSystemType)) {
            BLAME() << "Could not format mapped partition.";
            unmountFileSystem();
            return false;
        }

        if (!mountMappedDevice()) {
            BLAME() << "Failed to mount ecrypted file system.";
            unmountFileSystem();
            return false;
        }

        updateMountState(Mounted);
        return true;
    }

    //TODO - add checking for LUKS header in case of preavious app run formatting failure
    bool CryptoManager::mountFileSystem()
    {
        if (m_mountState == Mounted) {
            TRACE() << "Ecrypyted file system already mounted.";
            return false;
        }

        if (m_accessCode.isEmpty()) {
            TRACE() << "No access code set. Stopping mount process.";
            return false;
        }

        clearFileSystemResources();

        if (!CryptsetupHandler::loadDmMod()) {
            BLAME() << "Could not load `dm_mod`!";
            return false;
        }

        m_loopDeviceName = LosetupHandler::findAvailableDevice();
        if (m_loopDeviceName.isNull()) {
            BLAME() << "No free loop device available!";
            return false;
        }

        if (!LosetupHandler::setupDevice(m_loopDeviceName, m_fileSystemPath)) {
            BLAME() << "Failed to setup loop device:" << m_loopDeviceName;
            unmountFileSystem();
            return false;
        }
        updateMountState(LoopSet);

        //attempt luks close, in case of a leftover.
        if (QFile::exists(QLatin1String(DEVICE_MAPPER_DIR) + m_fileSystemName))
            CryptsetupHandler::closeFile(m_fileSystemName);

        if (!CryptsetupHandler::openFile(m_accessCode,
                                         m_loopDeviceName,
                                         m_fileSystemName)) {
            BLAME() << "Failed to LUKS open.";
            unmountFileSystem();
            return false;
        }
        updateMountState(LoopLuksOpened);

        if (!mountMappedDevice()) {
            TRACE() << "Failed to mount ecrypted file system.";
            unmountFileSystem();
            return false;
        }
        updateMountState(Mounted);
        return true;
    }

    void CryptoManager::clearFileSystemResources()
    {
        /*
            This method is a `just in case call` for the situations
            when signond closes whithout handling the unmounting of
            the secure storage.
        */

        TRACE() << "--- START clearing secure storage possibly used resources."
                   " Ignore possible errors. ---";

        if (!unmountMappedDevice())
            TRACE() << "Unmounting mapped device failed.";

        if (QFile::exists(QLatin1String(DEVICE_MAPPER_DIR) + m_fileSystemName)) {
            if (!CryptsetupHandler::closeFile(m_fileSystemName))
                TRACE() << "Failed to LUKS close.";
        }

        /*
         TODO - find a way to check which loop device was previously used by
                signond and close that specific one, until then this will be
                skipped as it might close devices used by different processes.

        if (!LosetupHandler::releaseDevice(m_loopDeviceName)) {
            TRACE() << "Failed to release loop device.";
        */

        TRACE() << "--- DONE clearing secure storage possibly used resources. ---";
    }

    bool CryptoManager::unmountFileSystem()
    {
        if (m_mountState == Unmounted) {
            TRACE() << "Ecrypyted file system not mounted.";
            return true;
        }

        emit fileSystemUnmounting();
        bool isOk = true;

        if ((m_mountState >= Mounted)
            && !unmountMappedDevice()) {
            TRACE() << "Failed to unmount mapped loop device.";
            isOk = false;
        } else {
            TRACE() << "Mapped loop device unmounted.";
        }

        if ((m_mountState >= LoopLuksOpened)
            && (!CryptsetupHandler::closeFile(m_fileSystemName))) {
            TRACE() << "Failed to LUKS close.";
            isOk = false;
        } else {
            TRACE() << "Luks close succeeded.";
        }

        if ((m_mountState >= LoopSet)
            && (!LosetupHandler::releaseDevice(m_loopDeviceName))) {
            TRACE() << "Failed to release loop device.";
            isOk = false;
        } else {
            TRACE() << "Loop device released.";
        }

        updateMountState(Unmounted);
        return isOk;
    }

    bool CryptoManager::deleteFileSystem()
    {
        if (m_mountState > Unmounted) {
            if (!unmountFileSystem())
                return false;
        }

        //TODO - implement effective deletion in specific handler object
        return false;
    }

    bool CryptoManager::fileSystemIsSetup() const
    {
        return QFile::exists(fileSystemPath());
    }

    //TODO - debug this method. Current test scenarios do no cover this one
    bool CryptoManager::fileSystemContainsFile(const QString &filePath)
    {
        if (!fileSystemIsMounted()) {
            TRACE() << "Ecrypyted file system not mounted.";
            return false;
        }

        QDir mountDir(m_fileSystemMountPath);
        return mountDir.exists(QDir::toNativeSeparators(filePath));
    }

    QString CryptoManager::fileSystemMountPath() const
    {
        return m_fileSystemMountPath;
    }

    void CryptoManager::updateMountState(const FileSystemMountState state)
    {
        TRACE() << "Updating mount state:" << state;
        if (state == m_mountState) return;

        m_mountState = state;
        if (state == Mounted)
            emit fileSystemMounted();
    }

    bool CryptoManager::mountMappedDevice()
    {
        //create mnt dir if not existant
        if (!QFile::exists(m_fileSystemMountPath)) {
            QDir dir;
            dir.mkdir(m_fileSystemMountPath);
        }

        MountHandler::mount(m_fileSystemMapPath, m_fileSystemMountPath);
        return true;
    }

    bool CryptoManager::unmountMappedDevice()
    {
        return MountHandler::umount(m_fileSystemMountPath);
    }

    bool CryptoManager::addEncryptionKey(const QByteArray &key,
                                         const QByteArray &existingKey)
    {
        /*
         * TODO -- limit number of stored keys to the total available slots - 1.
         */
        if (m_mountState >= LoopLuksOpened) {
            if (CryptsetupHandler::addKeySlot(
                m_loopDeviceName, key, existingKey))
                return true;
        }
        TRACE() << "FAILED to occupy key slot on the encrypted file system header.";
        return false;
    }

    bool CryptoManager::removeEncryptionKey(const QByteArray &key,
                                            const QByteArray &remainingKey)
    {
        if (m_mountState >= LoopLuksOpened) {
            if (CryptsetupHandler::removeKeySlot(
                m_loopDeviceName, key, remainingKey))
                return true;
        }
        TRACE() << "FAILED to release key slot from the encrypted file system header.";
        return false;
    }

    bool CryptoManager::encryptionKeyInUse(const QByteArray &key)
    {
        if (fileSystemIsMounted() && (m_accessCode == key))
            return true;

        if(!fileSystemIsMounted()) {
           setEncryptionKey(key);
           return mountFileSystem();
        }

        QByteArray dummyKey("dummy");
        if (addEncryptionKey(dummyKey, key)) {
            if (!removeEncryptionKey(dummyKey, key))
                BLAME() << "Could not remove dummy auxiliary key "
                           "from encrypted file system header.";
            return true;
        }

        return false;
    }

    //TODO - remove this after stable version is achieved.
    void CryptoManager::serializeData()
    {
        TRACE() << "m_accessCode" << m_accessCode;
        TRACE() << "m_fileSystemPath" << m_fileSystemPath;
        TRACE() << "m_fileSystemMapPath" << m_fileSystemMapPath;
        TRACE() << "m_fileSystemName" << m_fileSystemName;
        TRACE() << "m_loopDeviceName" << m_loopDeviceName;
        TRACE() << "m_fileSystemType" << m_fileSystemType;
        TRACE() << "m_fileSystemSize" << m_fileSystemSize;
    }
} //namespace SignonDaemonNS
