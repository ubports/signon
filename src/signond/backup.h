/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 * Copyright (C) 2012-2016 Canonical Ltd.
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

#ifndef SIGNON_BACKUP_H_
#define SIGNON_BACKUP_H_

#include <QDBusContext>
#include <QObject>
#include <QString>

namespace SignonDaemonNS {

class CredentialsAccessManager;

class Backup: public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    Backup(CredentialsAccessManager *cam, bool backupMode,
           QObject *parent = 0);
    ~Backup();

public Q_SLOTS:
    uchar backupStarts();
    uchar backupFinished();
    uchar restoreStarts();
    uchar restoreFinished();

private:
    void eraseBackupDir() const;
    bool copyToBackupDir(const QStringList &fileNames) const;
    bool copyFromBackupDir(const QStringList &fileNames) const;
    bool createStorageFileTree(const QStringList &fileNames) const;

private:
    CredentialsAccessManager *m_cam;
    bool m_backup; // whether signond was started just to take a backup
};

} // namespace

#endif /* SIGNON_BACKUP_H_ */
