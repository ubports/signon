/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 * Copyright (C) 2013-2016 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QDBusConnection>

#include "SignOn/misc.h"

#include "backup.h"
#include "backupifadaptor.h"
#include "credentialsaccessmanager.h"
#include "signond-common.h"

#define BACKUP_DIR_NAME() \
    (QDir::separator() + QLatin1String("backup"))

using namespace SignOn;

namespace SignonDaemonNS {

Backup::Backup(CredentialsAccessManager *cam, bool backupMode,
               QObject *parent):
    QObject(parent),
    m_cam(cam),
    m_backup(backupMode)
{
    QDBusConnection sessionConnection = QDBusConnection::sessionBus();

    QDBusConnection::RegisterOptions registerSessionOptions =
        QDBusConnection::ExportAdaptors;

    (void)new BackupIfAdaptor(this);

    if (!sessionConnection.registerObject(SIGNOND_DAEMON_OBJECTPATH
                                          + QLatin1String("/Backup"),
                                          this, registerSessionOptions)) {
        TRACE() << "Object cannot be registered";

        qFatal("SignonDaemon requires to register backup object");
    }

    if (!sessionConnection.registerService(SIGNOND_SERVICE +
                                           QLatin1String(".Backup"))) {
        QDBusError err = sessionConnection.lastError();
        TRACE() << "Service cannot be registered: " <<
            err.errorString(err.type());

        qFatal("SignonDaemon requires to register backup service");
    }
}

Backup::~Backup()
{
    QDBusConnection sessionConnection = QDBusConnection::sessionBus();
    sessionConnection.unregisterObject(SIGNOND_DAEMON_OBJECTPATH
                                       + QLatin1String("/Backup"));
    sessionConnection.unregisterService(SIGNOND_SERVICE
                                        + QLatin1String(".Backup"));
}

void Backup::eraseBackupDir() const
{
    const CAMConfiguration &config = m_cam->configuration();
    QString backupRoot = config.m_storagePath + BACKUP_DIR_NAME();

    QDir target(backupRoot);
    if (!target.exists()) return;

    QStringList targetEntries = target.entryList(QDir::Files);
    foreach (QString entry, targetEntries) {
        target.remove(entry);
    }

    target.rmdir(backupRoot);
}

bool Backup::copyToBackupDir(const QStringList &fileNames) const
{
    const CAMConfiguration &config = m_cam->configuration();
    QString backupRoot = config.m_storagePath + BACKUP_DIR_NAME();

    QDir target(backupRoot);
    if (!target.exists() && !target.mkpath(backupRoot)) {
        qCritical() << "Cannot create target directory";
        return false;
    }

    setUserOwnership(backupRoot);

    /* Now copy the files to be backed up */
    bool ok = true;
    foreach (const QString &fileName, fileNames) {
        /* Remove the target file, if it exists */
        if (target.exists(fileName))
            target.remove(fileName);

        /* Copy the source into the target directory */
        QString source = config.m_storagePath + QDir::separator() + fileName;
        if (!QFile::exists(source)) continue;

        QString destination = backupRoot + QDir::separator() + fileName;
        ok = QFile::copy(source, destination);
        if (!ok) {
            BLAME() << "Copying" << source << "to" << destination << "failed";
            break;
        }

        setUserOwnership(destination);
    }

    return ok;
}

bool Backup::copyFromBackupDir(const QStringList &fileNames) const
{
    const CAMConfiguration &config = m_cam->configuration();
    QString backupRoot = config.m_storagePath + BACKUP_DIR_NAME();

    QDir sourceDir(backupRoot);
    if (!sourceDir.exists()) {
        TRACE() << "Backup directory does not exist!";
    }

    if (!sourceDir.exists(config.m_dbName)) {
        TRACE() << "Backup does not contain DB:" << config.m_dbName;
    }

    /* Now restore the files from the backup */
    bool ok = true;
    QDir target(config.m_storagePath);
    QStringList movedFiles, copiedFiles;
    foreach (const QString &fileName, fileNames) {
        /* Remove the target file, if it exists */
        if (target.exists(fileName)) {
            if (target.rename(fileName, fileName + QLatin1String(".bak")))
                movedFiles += fileName;
        }

        /* Copy the source into the target directory */
        QString source = backupRoot + QDir::separator() + fileName;
        if (!QFile::exists(source)) {
            TRACE() << "Ignoring file not present in backup:" << source;
            continue;
        }

        QString destination =
            config.m_storagePath + QDir::separator() + fileName;

        ok = QFile::copy(source, destination);
        if (ok) {
            copiedFiles << fileName;
        } else {
            qWarning() << "Copy failed for:" << source;
            break;
        }
    }

    if (!ok) {
        qWarning() << "Restore failed, recovering previous DB";

        foreach (const QString &fileName, copiedFiles) {
            target.remove(fileName);
        }

        foreach (const QString &fileName, movedFiles) {
            if (!target.rename(fileName + QLatin1String(".bak"), fileName)) {
                qCritical() << "Could not recover:" << fileName;
            }
        }
    } else {
        /* delete ".bak" files */
        foreach (const QString &fileName, movedFiles) {
            target.remove(fileName + QLatin1String(".bak"));
        }

    }
    return ok;
}

bool Backup::createStorageFileTree(const QStringList &backupFiles) const
{
    const CAMConfiguration &config = m_cam->configuration();
    QDir storageDir(config.m_storagePath);

    if (!storageDir.exists()) {
        if (!storageDir.mkpath(config.m_storagePath)) {
            qCritical() << "Could not create storage dir for backup.";
            return false;
        }
    }

    foreach (const QString &fileName, backupFiles) {
        if (storageDir.exists(fileName)) continue;

        QString filePath = storageDir.path() + QDir::separator() + fileName;
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCritical() << "Failed to create empty file for backup:" << filePath;
            return false;
        } else {
            file.close();
        }
    }

    return true;
}

uchar Backup::backupStarts()
{
    TRACE() << "backup";
    if (!m_backup && m_cam->credentialsSystemOpened())
    {
        m_cam->closeCredentialsSystem();
        if (m_cam->credentialsSystemOpened())
        {
            qCritical() << "Cannot close credentials database";
            return 2;
        }
    }

    const CAMConfiguration &config = m_cam->configuration();

    /* do backup copy: prepare the list of files to be backed up */
    QStringList backupFiles;
    backupFiles << config.m_dbName;
    backupFiles << m_cam->backupFiles();

    /* make sure that all the backup files and storage directory exist:
       create storage dir and empty files if not so, as backup/restore
       operations must be consistent */
    if (!createStorageFileTree(backupFiles)) {
        qCritical() << "Cannot create backup file tree.";
        return 2;
    }

    /* perform the copy */
    eraseBackupDir();
    if (!copyToBackupDir(backupFiles)) {
        qCritical() << "Cannot copy database";
        if (!m_backup)
            m_cam->openCredentialsSystem();
        return 2;
    }

    if (!m_backup)
    {
        //mount file system back
        if (!m_cam->openCredentialsSystem()) {
            qCritical() << "Cannot reopen database";
        }
    }
    return 0;
}

uchar Backup::backupFinished()
{
    TRACE() << "close";

    eraseBackupDir();

    if (m_backup)
    {
        //close daemon
        TRACE() << "close daemon";
        QCoreApplication::quit();
    }

    return 0;
 }

/*
 * Does nothing but start-on-demand
 * */
uchar Backup::restoreStarts()
{
    TRACE();
    return 0;
}

uchar Backup::restoreFinished()
{
    TRACE() << "restore";
    //restore requested
    if (m_cam->credentialsSystemOpened())
    {
        //umount file system
        if (!m_cam->closeCredentialsSystem())
        {
            qCritical() << "database cannot be closed";
            return 2;
        }
    }

    const CAMConfiguration &config = m_cam->configuration();

    QStringList backupFiles;
    backupFiles << config.m_dbName;
    backupFiles << m_cam->backupFiles();

    /* perform the copy */
    if (!copyFromBackupDir(backupFiles)) {
        qCritical() << "Cannot copy database";
        m_cam->openCredentialsSystem();
        return 2;
    }

    eraseBackupDir();

    //TODO check database integrity
    if (!m_backup)
    {
        //mount file system back
         if (!m_cam->openCredentialsSystem())
             return 2;
    }

    return 0;
}

} //namespace SignonDaemonNS
