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

#define MOUNT_DIR "/home/user/"
#define DEVICE_MAPPER_DIR "/device/mapper/"
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
    }

    CryptoManager::~CryptoManager()
    {
        unmountFileSystem();
    }

    void CryptoManager::setFileSystemPath(const QString &path)
    {
        m_fileSystemPath = path;
        QString fileSystemNameBase;

        //todo - improve this using QDir/QFile specific methods.
        if (m_fileSystemPath.contains(QDir::separator()))
            m_fileSystemName = m_fileSystemPath.section(QDir::separator(), -1);
        else
            m_fileSystemName = m_fileSystemPath;

        m_fileSystemMapPath = QLatin1String(DEVICE_MAPPER_DIR) + m_fileSystemName;
        m_fileSystemName = m_fileSystemName;
        m_fileSystemMountPath = QLatin1String(MOUNT_DIR)
                                + m_fileSystemName
                                + QLatin1String("-mnt");
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

        if (!CryptsetupHandler::loadDmMod()) {
            BLAME() << "Could not load `dm_mod`!";
            return false;
        }

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
        storeEncryptionKey(m_accessCode);

        return true;
    }

    //TODO - add checking for LUKS header in case of preavious app run formatting failure
    bool CryptoManager::mountFileSystem()
    {
        if (m_mountState == Mounted) {
            TRACE() << "Ecrypyted file system already mounted.";
            return false;
        }

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

    bool CryptoManager::unmountFileSystem()
    {
        if (m_mountState == Unmounted) {
            TRACE() << "Ecrypyted file system not mounted.";
            return true;
        }

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

    //TODO - debug this method. Current test scenarios do no cover this one
    bool CryptoManager::fileSystemContainsFile(const QString &filePath)
    {
        if (!fileSystemMounted()) {
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
        m_mountState = state;
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
                                         const QByteArray &existingKey,
                                         const QString &keyTag)
    {
        /*
         * TODO -- limit number of stored keys to the total available slots - 1.
         */
        if (m_mountState >= LoopLuksOpened) {
            if (!CryptsetupHandler::addKeySlot(
                    m_loopDeviceName, key, existingKey)) {
                TRACE() << "FAILED to occupy key slot on the encrypted file system header.";
                return false;
            }

            storeEncryptionKey(key, keyTag);
        }
        return true;
    }

    bool CryptoManager::removeEncryptionKey(const QByteArray &key,
                                            const QByteArray &remainingKey)
    {
        if (m_mountState >= LoopLuksOpened) {
            if (!CryptsetupHandler::removeKeySlot(
                    m_loopDeviceName, key, remainingKey)) {
                TRACE() << "FAILED to release key slot from the encrypted file system header.";
                return false;
            }
            removeEncryptionKey(key);
        }
        return true;
    }


    void CryptoManager::storeEncryptionKey(const QByteArray &key, const QString &keyTag)
    {
        /* If it's worth it, optimize this to have only the actual keys which occupy key slots
           - this is for the case when the SIMs in use outnumber the LUKS key slots
         */
        TRACE() << fileSystemMountPath() + QDir::separator() + keysStorageFileName;
        QSettings usedEncryptionKeysFile(
                fileSystemMountPath() + QDir::separator() + keysStorageFileName,
                QSettings::IniFormat);

        if (keyTag.isEmpty()) {
            int keysCount = usedEncryptionKeysFile.childKeys().count();
            usedEncryptionKeysFile.setValue(
                QString(QLatin1String("key%1")).arg(keysCount + 1), key);
        } else {
            usedEncryptionKeysFile.setValue(keyTag, key);
        }
    }

    bool CryptoManager::encryptionKeyInUse(const QByteArray &key)
    {
        if (!fileSystemMounted()) {
            TRACE() << "Encrypted FS not mounted.";
            return false;
        }

        if (m_accessCode == key)
            return true;

        QSettings usedEncryptionKeysFile(
                fileSystemMountPath() + QDir::separator() + keysStorageFileName,
                QSettings::IniFormat);

        foreach (QString childKey, usedEncryptionKeysFile.childKeys())
            if (usedEncryptionKeysFile.value(childKey).toByteArray() == key)
                return true;

        return false;
    }

    void CryptoManager::removeEncryptionKey(const QByteArray &key)
    {
        QSettings usedEndryptionKeysFile(
                fileSystemMountPath() + QDir::separator() + keysStorageFileName,
                QSettings::IniFormat);

        foreach (QString keyStr, usedEndryptionKeysFile.childKeys())
            if (usedEndryptionKeysFile.value(keyStr).toByteArray() == key) {
                usedEndryptionKeysFile.remove(keyStr);
                break;
            }
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
