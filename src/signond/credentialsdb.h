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

/*!
  @file credentialsdb.h
  Definition of the CredentialsDB object.
  @ingroup Accounts_and_SSO_Framework
 */

#ifndef CREDENTIALS_DB_H
#define CREDENTIALS_DB_H

#include <QObject>
#include <QtSql>

#include "signonidentityinfo.h"

#define SSO_MAX_TOKEN_STORAGE (4*1024) // 4 kB for token store/identity/method

class TestDatabase;

namespace SignonDaemonNS {

    /*!
     * @enum IdentityFlags
     * Flags to be stored into database
     */
    enum IdentityFlags {
        Validated = 0x0001,
        RememberPassword = 0x0002
    };

    /*!
        @class SqlDatabase
        Will be used manage the SQL database interaction.
        @ingroup Accounts_and_SSO_Framework
     */
    class SqlDatabase
    {
    public:
        /*!
            Constructs a SqlDatabase object using the given hostname.
            @param hostname
        */
        SqlDatabase(const QString &hostname);

        /*!
            Destroys the SqlDatabase object, closing the database connection.
        */
        ~SqlDatabase();

        /*!
            Creates the database connection.
            @returns true if successful, false otherwise.
        */
        bool connect();
        /*!
            Destroys the database connection.
        */
        void disconnect();
        /*!
            @returns true if database connection is opened, false otherwise.
        */
        bool connected() { return m_database.isOpen(); }

        /*!
            Sets the database name.
            @param databseName
        */
        void setDatabaseName(const QString &databaseName) { m_database.setDatabaseName(databaseName); }

        /*!
            Sets the username for the database connection.
            @param username
        */
        void setUsername(const QString &username) { m_database.setUserName(username); }

        /*!
            Sets the password for the database connection.
            @param password
        */
        void setPassword(const QString &password) { m_database.setPassword(password); }

        /*!
            @returns the database name.
        */
        QString databaseName() const { return m_database.databaseName(); }

        /*!
            @returns the username for the database connection.
        */
        QString username() const { return m_database.userName(); }

        /*!
            @returns the password for the database connection.
        */
        QString password() const { return m_database.password(); }

        /*!
            Executes a specific database query.
            If an error occurres the lastError() method can be used for handling decissions.
            @param query, the query string.
            @returns the resulting sql query, which can be process in the case of a 'SELECT' statement.
        */
        QSqlQuery exec(const QString &query);

        /*!
            Executes a specific database query.
            If an error occurres the lastError() method can be used for handling decissions.
            @param query, the query.
            @returns the resulting sql query, which can be process in the case of a 'SELECT' statement.
        */
        QSqlQuery exec(QSqlQuery &query);

         /*!
            Executes a specific database set of queryes (INSERTs, UPDATEs, DELETEs) in a transaction
            context (No nested transactions supported - sqlite reasons).
            If an error occurres the lastError() method can be used for handling decissions.
            @param queryList, the query list to be executed.
            @returns true if the transaction commits successfully, false otherwise.
        */
        bool transactionalExec(const QStringList &queryList);

        /*!
            @returns true, if the database has any tables created, false otherwise.
        */
        bool hasTables() const { return m_database.tables().count() > 0 ? true : false; }

        /*!
            @returns a list of the supported drivers on the specific OS.
        */
        static QStringList supportedDrivers() { return QSqlDatabase::drivers(); }

        /*!
            @returns the current object configuration as a property, value strings map.
        */
        QMap<QString, QString> configuration();

        /*!
            @param queryExecuted, by default true, if otherwise will return specific database errors - non-query, use to check connection errors.
            @param clearError, clears the error cache, future calls of this methods without any addition querying will indicate no error occurred.
            @returns the last occurred error if any. If not error occurred on the last performed operation the QSqlError object is invalid.
        */
        QSqlError lastError(bool queryExecuted = true, bool clearError = true);

        /*!
            Serializes a SQL error into a string.
            @param error, method will fail if an error object is passed.
            @returns the error information as string.
        */
        static QString errorInfo(const QSqlError &error);

        static void removeDatabase();

    private:
        QSqlError m_lastError;
        QSqlDatabase m_database;

        friend class CredentialsDB;
    };

    /*!
        @class CredentialsDB
        Manages the credentials I/O.
        @ingroup Accounts_and_SSO_Framework
     */

    typedef QSqlError CredentialsDBError;

    class CredentialsDB : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY(CredentialsDB)

        friend class CredentialsAccessManager;
        friend class ::TestDatabase;

        CredentialsDB(const QString &dbName);
        ~CredentialsDB();

    private:
        QSqlQuery exec(const QString &query);
        QSqlQuery exec(QSqlQuery &query);
        bool transactionalExec(const QStringList &queryList);
        bool startTransaction();
        bool commit();
        void rollback();
        bool connect();
        void disconnect();
        QMap<QString, QString> sqlDBConfiguration() const;
        bool hasTableStructure() const;
        bool createTableStructure();
//helpers
        QStringList queryList(const QString &query_str);
        bool insertMethods(QMap<QString, QStringList> methods);
        bool cleanUpTables();

    public:
        CredentialsDBError error(bool queryError = true, bool clearError = true) const;
        bool errorOccurred(bool queryError = true) { return error(queryError, false).type() != QSqlError::NoError; }

        QStringList methods(const quint32 id, const QString &securityToken = QString());
        bool checkPassword(const quint32 id, const QString &username, const QString &password);
        SignonIdentityInfo credentials(const quint32 id, bool queryPassword = true);
        QList<SignonIdentityInfo> credentials(const QMap<QString, QString> &filter);

        quint32 insertCredentials(const SignonIdentityInfo &info, bool storeSecret = true);
        quint32 updateCredentials(const SignonIdentityInfo &info, bool storeSecret = true);
        bool removeCredentials(const quint32 id);

        bool clear();

        QStringList accessControlList(const quint32 identityId);
        QString credentialsOwnerSecurityToken(const quint32 identityId);

        QVariantMap loadData(const quint32 id, const QString &method);
        bool storeData(const quint32 id, const QString &method, const QVariantMap &data);

    private:
        SqlDatabase *m_pSqlDatabase;
    };

} // namespace SignonDaemonNS

#endif // CREDENTIALSDB_H
