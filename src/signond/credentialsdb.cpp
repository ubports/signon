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


#include "credentialsdb.h"
#include "signond-common.h"

namespace SignonDaemonNS {

    static const QString driver = QLatin1String("QSQLITE");
    static const QString connectionName = QLatin1String("SSO-Connection");

    SqlDatabase::SqlDatabase(const QString &databaseName)
            : m_lastError(QSqlError()),
              m_database(QSqlDatabase::addDatabase(driver, connectionName))

    {
        TRACE() << "Supported Drivers:" << this->supportedDrivers();
        TRACE() << "DATABASE NAME [" << databaseName << "]";

        m_database.setDatabaseName(databaseName);
    }

    SqlDatabase::~SqlDatabase()
    {
        //TODO - sync with driver commit
        m_database.close();
    }

    bool SqlDatabase::connect()
    {
        if (!m_database.open()) {
            TRACE() << "Could not open database connection.\n";
            return false;
        }
        return true;
    }

    void SqlDatabase::disconnect()
    {
        m_database.close();
    }

    QSqlQuery SqlDatabase::exec(const QString &queryStr)
    {
        QSqlQuery query(QString(), m_database);

        if (!query.prepare(queryStr))
            TRACE() << "Query prepare warning: " << query.lastQuery();

        if (!query.exec()) {
            TRACE() << "Query exec error: " << query.lastQuery();
            m_lastError = query.lastError();
            TRACE() << errorInfo(m_lastError);
        } else
            m_lastError.setType(QSqlError::NoError);

        return query;
    }

    bool SqlDatabase::transactionalExec(const QStringList &queryList)
    {
        if (!m_database.transaction()) {
            TRACE() << "Could not start transaction";
            return false;
        }

        bool allOk = true;
        foreach (QString queryStr, queryList) {
            TRACE() << QString::fromLatin1("TRANSACT Query [%1]").arg(queryStr);
            QSqlQuery query = exec(queryStr);

            if (lastError().type() != QSqlError::NoError) {
                TRACE() << "Error occurred while executing query in transaction." << queryStr;
                allOk = false;
                break;
            }
        }

        if (allOk && m_database.commit()) {
            TRACE() << "Commit SUCCEEDED.";
            return true;
        } else if (!m_database.rollback())
            TRACE() << "Rollback failed";

        TRACE() << "Transactional exec FAILED!";
        return false;
    }

    QSqlError SqlDatabase::lastError(bool queryExecuted, bool clearError)
    {
        if (queryExecuted) {
            QSqlError error = m_lastError;
            if (clearError)
                m_lastError.setType(QSqlError::NoError);
            return error;
        } else
            return m_database.lastError();
    }

    QMap<QString, QString> SqlDatabase::configuration()
    {
        QMap<QString, QString> map;

        map.insert(QLatin1String("Database Name"), m_database.databaseName());
        map.insert(QLatin1String("Host Name"), m_database.hostName());
        map.insert(QLatin1String("Username"), m_database.databaseName());
        map.insert(QLatin1String("Password"), m_database.password());
        map.insert(QLatin1String("Tables"), m_database.tables().join(QLatin1String(" ")));
        return map;
    }

    QString SqlDatabase::errorInfo(const QSqlError &error)
    {
        if (!error.isValid())
            return QLatin1String("SQL Error invalid.");

        QString text;
        QTextStream stream(&text);
        stream << "SQL error description:";
        stream << "\n\tType: ";

        const char *errType;
        switch (error.type()) {
            case QSqlError::NoError: errType = "NoError"; break;
            case QSqlError::ConnectionError: errType = "ConnectionError"; break;
            case QSqlError::StatementError: errType = "StatementError"; break;
            case QSqlError::TransactionError: errType = "TransactionError"; break;
            case QSqlError::UnknownError:
                /* fall trough */
            default: errType = "UnknownError";
        }
        stream << errType;
        stream << "\n\tDatabase text: " << error.databaseText();
        stream << "\n\tDriver text: " << error.driverText();
        stream << "\n\tNumber: " << error.number();

        return text;
    }

    void SqlDatabase::removeDatabase()
    {
         QSqlDatabase::removeDatabase(connectionName);
    }


    /*    -------   CredentialsDB  implementation   -------    */


    CredentialsDB::CredentialsDB(const QString &dbName)
        : m_pSqlDatabase(new SqlDatabase(dbName))
    {
    }

    CredentialsDB::~CredentialsDB()
    {
        if (m_pSqlDatabase)
            delete m_pSqlDatabase;

        SqlDatabase::removeDatabase();
    }

    QSqlQuery CredentialsDB::exec(const QString &query)
    {
        if (!m_pSqlDatabase->connected()) {
            if (!m_pSqlDatabase->connect()) {
                TRACE() << "Could not establish database connection.";
                return QSqlQuery();
            }
        }
        return m_pSqlDatabase->exec(query);
    }

    bool CredentialsDB::transactionalExec(const QStringList &queryList)
    {
        if (!m_pSqlDatabase->connected()) {
            if (!m_pSqlDatabase->connect()) {
                TRACE() << "Could not establish database connection.";
                return false;
            }
        }
        return m_pSqlDatabase->transactionalExec(queryList);
    }

    CredentialsDBError CredentialsDB::error(bool queryError, bool clearError) const
    {
        return m_pSqlDatabase->lastError(queryError, clearError);
    }

    QMap<QString, QString> CredentialsDB::sqlDBConfiguration() const
    {
        return m_pSqlDatabase->configuration();
    }

    bool CredentialsDB::hasTableStructure() const
    {
        return m_pSqlDatabase->hasTables();
    }

    bool CredentialsDB::createTableStructure()
    {
        /* !!! Foreign keys support seems to be disabled, for the moment... */
        QStringList createTableQuery = QStringList()
            <<  QString::fromLatin1(
                    "CREATE TABLE CREDENTIALS"
                    "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "username VARCHAR(25),"
                    "password VARCHAR(25),"
                    "caption TEXT,"
                    "realms TEXT,"
                    "security_tokens TEXT,"
                    "type INTEGER)")
            <<  QString::fromLatin1(
                    "CREATE TABLE CORRESPONDENCES"
                    "(identity_id INTEGER,"
                    "method VARCHAR(25),"
                    "mechanisms TEXT)")
            <<  QString::fromLatin1(
                    "CREATE TABLE STORE"
                    "(identity_id INTEGER,"
                    "method TEXT,"
                    "key TEXT,"
                    "value TEXT,"
                    "PRIMARY KEY (identity_id,method,key))");

       foreach (QString createTable, createTableQuery) {
            QSqlQuery query = exec(createTable);
            if (error().type() != QSqlError::NoError) {
                TRACE() << "Error occurred while creating the CREDENTIALS SQL Table structure.";
                return false;
            }
        }
        return true;
    }

    bool CredentialsDB::connect()
    {
        return m_pSqlDatabase->connect();
    }

    void CredentialsDB::disconnect()
    {
        m_pSqlDatabase->disconnect();
    }

    QStringList CredentialsDB::methods(const quint32 id)
    {
        QSqlQuery query = exec(
                QString::fromLatin1("SELECT method FROM correspondences WHERE identity_id = %1").arg(id));

        if (errorOccurred()) {
            TRACE() << "Error occurred while fetching available methods for Identity";
            return QStringList();
        }

        QStringList result;
        while (query.next())
            result << query.value(0).toString();

        return result;
    }

    bool CredentialsDB::checkPassword(const QString &username, const QString &password)
    {
        QSqlQuery query = exec(
                QString::fromLatin1("SELECT id FROM credentials "
                                    "WHERE username = '%1' AND password = '%2'")
                    .arg(username).arg(password));

        if (errorOccurred()) {
            TRACE() << "Error occurred while fetching available methods for Identity";
            return false;
        }

        if (query.first())
            return true;

        return false;
    }

    QList<SignonIdentityInfo> CredentialsDB::credentials(const QMap<QString, QString> &filter)
    {
        TRACE();
        Q_UNUSED(filter)
        QList<SignonIdentityInfo> result;

        QString queryStr(QString::fromLatin1("SELECT id FROM credentials"));

        // TODO - process filtering step here !!!

        queryStr += QString::fromLatin1(" ORDER BY id");

        QSqlQuery query = exec(queryStr);
        if (errorOccurred()) {
            TRACE() << "Error occurred while fetching credentials from database.";
            return result;
        }

        // TODO - avoid making this loop and get all the needed data with maximum 2 queries.
        while (query.next()) {
            SignonIdentityInfo info = credentials(query.value(0).toUInt(), false);
            if (errorOccurred())
                break;
            result << info;
        }

        return result;
    }

    SignonIdentityInfo CredentialsDB::credentials(const quint32 id, bool queryPassword)
    {
        /*
           NOTE - Sqlite does not support RIGHT and FULL OUTER JOINS, and the Query Language
           does not allow the creation of stored procedures (accessing sqlite through Qt API only),
           so this method is going to execute 2 queries for now.
           Bottom line - could not solve the case of querying identities without stored auth. methods
                         in a single join query.
         */

        // TODO - add support for getting the ACL with this query, too.

        QString query_str;

        if (queryPassword)
            query_str = QString::fromLatin1(
                                    "SELECT username, caption, realms, security_tokens, type, password "
                                    "FROM credentials WHERE id = %1").arg(id);
        else
            query_str = QString::fromLatin1(
                                    "SELECT username, caption, realms, security_tokens, type "
                                    "FROM credentials WHERE id = %1").arg(id);
        // Query #1
        QSqlQuery query = exec(query_str);

        if (!query.first()) {
            TRACE() << "No result or invalid credentials query.";
            return SignonIdentityInfo();
        }

        QString username = query.value(0).toString();
        QString caption = query.value(1).toString();
        QStringList realms = query.value(2).toString().split(
                                SSO_DELIMITER, QString::SkipEmptyParts);

        QStringList security_tokens = query.value(3).toString().split(
                                SSO_DELIMITER, QString::SkipEmptyParts);

        int type = query.value(4).toInt();

        QString password;
        if (queryPassword)
            password = query.value(5).toString();

        query_str = QString::fromLatin1("SELECT method, mechanisms FROM correspondences "
                                        "WHERE identity_id = %1").arg(id);
        // Query #2
        query = exec(query_str);
        QMap<QString, QVariant> methods;
        if (!query.first()) {
            TRACE() << "Credentials have no authentication method stored.";
        } else {
            do {
                methods.insert(
                        query.value(0).toString(),
                        QVariant(query.value(1).toString().split(
                            SSO_DELIMITER, QString::SkipEmptyParts)));
            } while (query.next());
        }

        return SignonIdentityInfo(id, username, password, methods, caption, realms, security_tokens, type);
    }

    quint32 CredentialsDB::insertCredentials(const SignonIdentityInfo &info, bool storeSecret)
    {
        if (!startTransaction()) {
            TRACE() << "Could not start transaction. Error inserting credentials.";
            return 0;
        }

        /* Credentials insert */

        QString password;
        if (storeSecret)
            password = info.m_password;

        QString queryStr;
        queryStr = QString::fromLatin1(
            "INSERT INTO CREDENTIALS (username, password, caption, "
            "realms, security_tokens, type) "
            "VALUES('%1', '%2', '%3', '%4', '%5', '%6')")
            .arg(info.m_userName).arg(password).arg(info.m_caption)
            .arg(info.m_realms.join(SSO_DELIMITER))
            .arg(info.m_accessControlList.join(SSO_DELIMITER))
            .arg(info.m_type);

        QSqlQuery insertQuery = exec(queryStr);
        if (errorOccurred()) {
            rollback();
            TRACE() << "Error occurred while inserting crendentials";
            return 0;
        }

        /* Fetch id of the inserted credentials */
        QVariant idVariant = insertQuery.lastInsertId();
        if (!idVariant.isValid()) {
            rollback();
            TRACE() << "Error occurred while inserting crendentials";
            return 0;
        }
        quint32 id = idVariant.toUInt();

        bool allOk = true;
        /* Methods and correspndences inserts */
        QMap<QString, QStringList> methods = info.m_methods;
        if (!(methods.keys().empty())) {
            QMapIterator<QString, QStringList> it(methods);
            while (it.hasNext()) {
                it.next();
                /* Correspondences insert */
                queryStr = QString::fromLatin1("INSERT INTO CORRESPONDENCES(identity_id, method, mechanisms) "
                                   "VALUES(%1, '%2', '%3')").
                                   arg(id).arg(it.key()).arg(it.value().join(SSO_DELIMITER));
                exec(queryStr);
                if (errorOccurred()) {
                    allOk = false;
                    break;
                }
            }
        }

        if (allOk && commit()) {
            return id;
        } else {
            rollback();
            TRACE() << "Credentials insertion failed.";
            return 0;
        }
    }

    bool CredentialsDB::updateCredentials(const SignonIdentityInfo &info, bool storeSecret)
    {
        TRACE() << "UPDATING CREDENTIALS...";

        this->listDBContents();

        if (!startTransaction()) {
            TRACE() << "Could not start transaction. Error updating credentials.";
            return false;
        }

        /* Credentials update */
        QString password;
        if (storeSecret)
            password = info.m_password;

        QString queryStr;
        queryStr = QString::fromLatin1("UPDATE credentials SET username = '%1', password = '%2', caption = '%3', realms = '%4', "
                           "security_tokens = '%5', type = '%6' WHERE id = %7").
                            arg(info.m_userName).arg(password).arg(info.m_caption).
                            arg(info.m_realms.join(SSO_DELIMITER)).
                            arg(info.m_accessControlList.join(SSO_DELIMITER)).
                            arg(info.m_type).
                            arg(info.m_id);
        exec(queryStr);
        if (errorOccurred()) {
            rollback();
            TRACE() << "Error occurred while updating crendentials";
            return false;
        }

        /*
            Methods and correspndences update based on the specified methods list
                - remove outdated records
                - update existing ones
                - insert new ones
        */

        /* Remove outdated ones */
        queryStr = QString::fromLatin1("SELECT co.method FROM correspondences co WHERE co.identity_id = %1").arg(info.m_id);
        QSqlQuery query = exec(queryStr);
        if (errorOccurred()) {
            rollback();
            TRACE() << "Error occurred while updating crendentials";
            return false;
        }

        QStringList currentlySupportedMethods;
        while (query.next()) {
            QString method = query.value(0).toString();
            if (!info.m_methods.contains(method)) {
                queryStr = QString::fromLatin1("DELETE FROM correspondences "
                                   "WHERE (identity_id = %1 AND method = '%2')").arg(info.m_id).arg(method);
                exec(queryStr);
                if (errorOccurred()) {
                    rollback();
                    TRACE() << "Error occurred while updating crendentials";
                    return false;
                }
            } else {
                currentlySupportedMethods << method;
            }
        }
        bool allOk = true;

        /* Update existing ones and insert new ones */
        if (!(info.m_methods.keys().empty())) {
            TRACE() << "METHODS ANALISYS";

            QMapIterator<QString, QStringList> it(info.m_methods);
            while (it.hasNext()) {
                it.next();

                if (currentlySupportedMethods.contains(it.key())) {
                    queryStr = QString::fromLatin1("UPDATE correspondences SET mechanisms = '%1' "
                                       "WHERE identity_id = %2 AND method = '%3'").
                                       arg(it.value().join(SSO_DELIMITER)).arg(info.m_id).arg(it.key());

                } else {
                   queryStr = QString::fromLatin1("INSERT INTO correspondences(identity_id, method, mechanisms) "
                                   "VALUES(%1, '%2', '%3')").
                                   arg(info.m_id).arg(it.key()).arg(it.value().join(SSO_DELIMITER));
                }
                exec(queryStr);
                if (errorOccurred()) {
                    allOk = false;
                    break;
                }
            }
        }

        if (allOk && commit()) {
            TRACE() << "Credentials successfully updated.";
            this->listDBContents();
            return true;
        } else {
            rollback();
            TRACE() << "Credentials update failed.";
            return false;
        }
    }

    bool CredentialsDB::removeCredentials(const quint32 id)
    {
        if (!startTransaction()) {
            TRACE() << "Could not start database transaction.";
            return false;
        }

        exec(QString::fromLatin1("DELETE FROM credentials WHERE id = %1").arg(id));
        if (errorOccurred()) {
            rollback();
            return false;
        }

        exec(QString::fromLatin1("DELETE FROM correspondences WHERE identity_id = %1").arg(id));
        if (errorOccurred()) {
            rollback();
            return false;
        }

        if (!commit())
            return false;

        return true;
    }

    bool CredentialsDB::clear()
    {
        exec(QLatin1String("DELETE FROM credentials"));
        if (errorOccurred())
            return false;

        exec(QLatin1String("DELETE FROM correspondences"));
        if (errorOccurred())
            return false;

        return true;
    }

    QVariantMap CredentialsDB::loadData(const quint32 id, const QString &method)
    {
        TRACE();

        QString query_str = QString::fromLatin1(
                "SELECT key, value "
                "FROM STORE WHERE identity_id = %1 AND method = %2").arg(id).arg(method);

        QSqlQuery query = exec(query_str);
        if (!query.first()) {
            TRACE() << "No result or invalid query.";
            return QVariantMap();
        }

        QVariantMap data;
        do {
            data.insert(
                    query.value(0).toString(),
                    query.value(1));
        } while (query.next());

        return data;
    }

    bool CredentialsDB::storeData(const quint32 id, const QString &method, const QVariantMap &data)
    {
        TRACE();

        if (!startTransaction()) {
            TRACE() << "Could not start transaction. Error inserting data.";
            return 0;
        }

        /* Data insert */
        bool allOk = true;
        QString queryStr;
        /* Methods and correspndences inserts */
        if (!(data.keys().empty())) {
            QMapIterator<QString, QVariant> it(data);
            while (it.hasNext()) {
                it.next();
                /* Key/value insert/replace/delete */
                if (it.value().isValid()) {
                    queryStr = QString::fromLatin1(
                        "INSERT OR REPLACE INTO STORE (identity_id, method, key, value) "
                        "VALUES('%1', '%2', '%3', '%4')")
                        .arg(id).arg(method).arg(it.key()).arg(it.value().toString());
                } else {
                    queryStr = QString::fromLatin1(
                        "DELETE FROM STORE WHERE identity_id = '%1' AND method = '%2' AND key = '%3'")
                        .arg(id).arg(method).arg(it.key());
                }
                exec(queryStr);
                if (errorOccurred()) {
                    allOk = false;
                    break;
                }
            }
        }

        if (allOk && commit()) {
            return true;
        }
        rollback();
        TRACE() << "Data insertion failed.";
        return false;
    }

    QStringList CredentialsDB::accessControlList(const quint32 identityId)
    {
        QString queryStr = QString::fromLatin1("SELECT security_tokens FROM credentials WHERE id = %1").
                                  arg(identityId);
        QSqlQuery query = exec(queryStr);

        if (errorOccurred()) {
            TRACE() << QString::fromLatin1("Error occurred while querying the access control list for id = %1.").
                              arg(identityId);
            return QStringList();
        }

        if (query.first()) {
            QString tokensStr = query.value(0).toString();
            if (tokensStr.length() > 0)
                return tokensStr.split(SSO_DELIMITER);
        }

        return QStringList();
    }

    QString CredentialsDB::credentialsOwnerSecurityToken(const quint32 identityId)
    {
        QStringList acl = accessControlList(identityId);
        int index = -1;
        QRegExp aegisIdTokenPrefixRegExp(QLatin1String("^AID::*"));
        if ((index = acl.indexOf(aegisIdTokenPrefixRegExp)) != -1)
            return acl.at(index);
        return QString();
    }

    bool CredentialsDB::startTransaction()
    {
        return m_pSqlDatabase->m_database.transaction();
    }

    bool CredentialsDB::commit()
    {
        return m_pSqlDatabase->m_database.commit();
    }

    void CredentialsDB::rollback()
    {
        if (!m_pSqlDatabase->m_database.rollback())
            TRACE() << "Rollback failed, db data integrity could be compromised.";
    }

    void CredentialsDB::listDBContents()
    {
#define DB_TRACE

#ifdef DB_TRACE
        TRACE() << "\n\nCREDENTIALS\n";
        QSqlQuery query = exec(QLatin1String("SELECT * FROM credentials"));
        while (query.next()) {
            TRACE() << query.value(0).toInt()    << " "
                    << query.value(1).toString() << " "
                    << query.value(2).toString() << " "
                    << query.value(3).toString() << " "
                    << query.value(4).toString() << " "
                    << query.value(5).toString() << "."
                    << query.value(6).toInt();
        }

        TRACE() << "\n\nCORRESPONDENCES\n";
        query = exec(QLatin1String("SELECT * FROM correspondences"));
        while (query.next()) {
            TRACE() << query.value(0).toInt()    << " "
                    << query.value(1).toString() << " "
                    << query.value(2).toString();
        }
#endif
    }

} //namespace SignonDaemonNS
