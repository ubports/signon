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
                    "username TEXT,"
                    "password TEXT,"
                    "caption TEXT,"
                    "type INTEGER)")
            <<  QString::fromLatin1(
                    "CREATE TABLE METHODS"
                    "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "identity_id INTEGER,"
                    "method TEXT)")
            <<  QString::fromLatin1(
                    "CREATE TABLE MECHANISMS"
                    "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "method_id INTEGER,"
                    "mechanism TEXT)")
            <<  QString::fromLatin1(
                    "CREATE TABLE ACL"
                    "(rowid INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "identity_id INTEGER,"
                    "method_id INTEGER,"
                    "mechanism_id INTEGER,"
                    "token TEXT)")
            <<  QString::fromLatin1(
                    "CREATE TABLE REALMS"
                    "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "identity_id INTEGER,"
                    "realm TEXT)");

       foreach (QString createTable, createTableQuery) {
            QSqlQuery query = exec(createTable);
            if (error().type() != QSqlError::NoError) {
                TRACE() << "Error occurred while creating the database.";
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

    QStringList CredentialsDB::methods(const quint32 id, const QString &securityToken)
    {
        Q_UNUSED(securityToken);

        QStringList list = queryList(
                QString::fromLatin1("SELECT method FROM METHODS WHERE identity_id = %1").arg(id));
        list.removeDuplicates();
        return list;
    }

    bool CredentialsDB::checkPassword(const quint32 id, const QString &username, const QString &password)
    {
        QSqlQuery query = exec(
                QString::fromLatin1("SELECT id FROM CREDENTIALS "
                                    "WHERE id = '%1' AND username = '%2' AND password = '%3'")
                    .arg(id).arg(username).arg(password));

        if (errorOccurred()) {
            TRACE() << "Error occurred while checking password";
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

        while (query.next()) {
            SignonIdentityInfo info = credentials(query.value(0).toUInt(), false);
            if (errorOccurred())
                break;
            result << info;
        }

        return result;
    }

    bool CredentialsDB::insertList(const QStringList &list, const QString &query_str, const quint32 id)
    {
        if (list.isEmpty()) return false;
        bool allOk = true;
        QStringListIterator it(list);
        while (it.hasNext()) {
            QString queryStr2 = query_str + QString::fromLatin1("VALUES('%1', '%2')").
                                   arg(id).arg(it.next());
            exec(queryStr2);
            if (errorOccurred()) {
                allOk = false;
                break;
            }
        }
    return allOk;
    }

    bool CredentialsDB::removeList(const QString &query_str)
    {
        TRACE() << query_str;
        bool allOk = true;
        exec(query_str);
        if (errorOccurred()) {
            allOk = false;
        }
    return allOk;
    }

    QStringList CredentialsDB::queryList(const QString &query_str)
    {
        TRACE();
        QStringList list;
        QSqlQuery query = exec(query_str);
        if (errorOccurred())
            return list;
        while (query.next()) {
            list.append(query.value(0).toString());
        }
        return list;
    }

    SignonIdentityInfo CredentialsDB::credentials(const quint32 id, bool queryPassword)
    {
        QString query_str;

        if (queryPassword)
            query_str = QString::fromLatin1(
                                    "SELECT username, caption, type, password "
                                    "FROM credentials WHERE id = %1").arg(id);
        else
            query_str = QString::fromLatin1(
                                    "SELECT username, caption, type "
                                    "FROM credentials WHERE id = %1").arg(id);

        QSqlQuery query = exec(query_str);

        if (!query.first()) {
            TRACE() << "No result or invalid credentials query.";
            return SignonIdentityInfo();
        }

        QString username = query.value(0).toString();
        QString caption = query.value(1).toString();
        int type = query.value(2).toInt();
        QString password;
        if (queryPassword)
            password = query.value(3).toString();

        QStringList realms = queryList(QString::fromLatin1("SELECT realm FROM REALMS "
                                        "WHERE identity_id = %1").arg(id));

        QStringList security_tokens = queryList(QString::fromLatin1("SELECT token FROM ACL "
                                        "WHERE identity_id = %1").arg(id));

        QMap<QString, QVariant> methods;
        query_str = QString::fromLatin1("SELECT id, method FROM METHODS "
                                        "WHERE identity_id = %1").arg(id);

        query = exec(query_str);
        if (!query.first()) {
            TRACE() << "Credentials have no authentication method stored.";
        } else {
            do {
                QStringList mechanisms = queryList(
                            QString::fromLatin1("SELECT mechanism FROM MECHANISMS "
                                        "WHERE method_id = %1").arg(query.value(0).toInt()));
                methods.insert(query.value(1).toString(), mechanisms);
            } while (query.next());
        }

        return SignonIdentityInfo(id, username, password, methods,
                                  caption, realms, security_tokens, type);
    }

    bool CredentialsDB::insertMethods(const quint32 id, QMap<QString, QStringList> methods)
    {
        QString queryStr;
        bool allOk = true;

        /* Methods inserts */
        if (!(methods.keys().empty())) {
            QMapIterator<QString, QStringList> it(methods);
            while (it.hasNext()) {
                it.next();
                /* Correspondences insert */
                queryStr = QString::fromLatin1(
                                    "INSERT INTO METHODS(identity_id, method) "
                                    "VALUES(%1, '%2')").
                                    arg(id).arg(it.key());

                exec(queryStr);
                if (errorOccurred()) {
                    allOk = false;
                    break;
                }
                        /* Fetch id of the inserted method */
                QSqlQuery insertQuery = exec(queryStr);
                QVariant idVariant = insertQuery.lastInsertId();
                if (!idVariant.isValid()) {
                    rollback();
                    TRACE() << "Error occurred while inserting crendentials";
                    return 0;
                }
                quint32 id2 = idVariant.toUInt();

                /* Mechanisms insert */

                QStringList mechs = it.value();
                if (!mechs.isEmpty()) {
                    QStringListIterator it(mechs);
                    while (it.hasNext()) {
                        /* Mechanisms insert */
                        queryStr = QString::fromLatin1(
                                    "INSERT INTO MECHANISMS(method_id, mechanism) "
                                    "VALUES(%1, '%2')").
                                    arg(id2).arg(it.next());
                        exec(queryStr);
                        if (errorOccurred()) {
                            allOk = false;
                            break;
                        }
                        /* Fetch id of the inserted method */
                        QVariant idVariant = insertQuery.lastInsertId();
                        if (!idVariant.isValid()) {
                            rollback();
                            TRACE() << "Error occurred while inserting crendentials";
                            return 0;
                        }

/* TODO uncomment this to set mechanism level acl
                        quint32 id3 = idVariant.toUInt();
                        QStringList tokens = info.m_accessControlList;
                        if (!tokens.isEmpty()) {
                            QStringListIterator it2(tokens);
                            while (it2.hasNext()) {
                                queryStr = QString::fromLatin1(
                                       "INSERT INTO ACL(identity_id, method_id, mechanism_id, token) "
                                       "VALUES('%1', '%2', '%3', '%4')").
                                       arg(id).arg(id2).arg(id3).arg(it2.next());
                                exec(queryStr);
                                if (errorOccurred()) {
                                    allOk = false;
                                    break;
                                }
                            }
                        }
*/
                    }
                }
            }
        }

        return allOk;
    }

    bool CredentialsDB::removeMethods(const quint32 id)
    {
        QString queryStr;
        bool allOk = true;

        /* query methods */
        queryStr = QString::fromLatin1(
                "SELECT id FROM METHODS WHERE identity_id = %1").arg(id);
        QSqlQuery query = exec(queryStr);
        if (errorOccurred()) {
            rollback();
            return false;
        }

        /* remove mechanisms */
        while (query.next()) {
            exec(QString::fromLatin1(
                    "DELETE FROM MECHANISMS WHERE method_id = %1")
                    .arg(query.value(0).toInt()));
        }

        /* remove methods */
        exec(QString::fromLatin1(
                "DELETE FROM METHODS WHERE identity_id = %1").arg(id));
        return allOk;
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
            "INSERT INTO CREDENTIALS (username, password, caption, type) "
            "VALUES('%1', '%2', '%3', '%4')")
            .arg(info.m_userName).arg(password).arg(info.m_caption)
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
        /* Methods inserts */
        insertMethods(id,  info.m_methods);

        /* ACL insert, this will do identity level ACL */
        insertList(info.m_accessControlList, QString::fromLatin1(
                "INSERT INTO ACL(identity_id, token) "), id);

        /* Realms insert */
        insertList(info.m_realms, QString::fromLatin1(
                "INSERT INTO REALMS(identity_id, realm) "), id);

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

        if (!startTransaction()) {
            TRACE() << "Could not start transaction. Error updating credentials.";
            return false;
        }

        /* Credentials update */
        QString password;
        if (storeSecret)
            password = info.m_password;

        QString queryStr;
        queryStr = QString::fromLatin1(
            "UPDATE CREDENTIALS SET username = '%1', password = '%2', "
            "caption = '%3', type = '%4' WHERE id = '%5'")
            .arg(info.m_userName).arg(password).arg(info.m_caption)
            .arg(info.m_type).arg(info.m_id);

        QSqlQuery insertQuery = exec(queryStr);
        if (errorOccurred()) {
            rollback();
            TRACE() << "Error occurred while updating crendentials";
            return 0;
        }

        bool allOk = true;
        /* update other tables by removing old values and inserting new ones */

        /* Methods remove */
        removeMethods(info.m_id);
        /* Methods inserts */
        insertMethods(info.m_id,  info.m_methods);

        /* ACL remove */
        removeList(QString::fromLatin1(
                "DELETE FROM ACL WHERE identity_id = %1")
                   .arg(info.m_id));
        /* ACL insert */
        insertList(info.m_accessControlList, QString::fromLatin1(
                "INSERT INTO ACL(identity_id, token) "), info.m_id);

        /* Realms remove */
        removeList(QString::fromLatin1(
                "DELETE FROM REALMS WHERE identity_id = %1").arg( info.m_id));
        /* Realms insert */
        insertList(info.m_realms, QString::fromLatin1(
                "INSERT INTO REALMS(identity_id, realm) "), info.m_id);

        if (allOk && commit()) {
            return true;
        } else {
            rollback();
            TRACE() << "Credentials update failed.";
            return 0;
        }

        return 0;
    }

    bool CredentialsDB::removeCredentials(const quint32 id)
    {
        if (!startTransaction()) {
            TRACE() << "Could not start database transaction.";
            return false;
        }

        exec(QString::fromLatin1(
                "DELETE FROM CREDENTIALS WHERE id = %1").arg(id));
        if (errorOccurred()) {
            rollback();
            return false;
        }

        exec(QString::fromLatin1(
                "DELETE FROM MECHANISMS WHERE method_id IN "
                "(SELECT id FROM METHODS WHERE identity_id = %1)").arg(id));
        if (errorOccurred()) {
            rollback();
            return false;
        }

        exec(QString::fromLatin1(
                "DELETE FROM METHODS WHERE identity_id = %1").arg(id));
        if (errorOccurred()) {
            rollback();
            return false;
        }

        exec(QString::fromLatin1(
                "DELETE FROM ACL WHERE identity_id = %1").arg(id));
        if (errorOccurred()) {
            rollback();
            return false;
        }

        exec(QString::fromLatin1(
                "DELETE FROM REALMS WHERE identity_id = %1").arg(id));
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
        exec(QLatin1String("DELETE FROM CREDENTIALS"));
        if (errorOccurred())
            return false;

        exec(QLatin1String("DELETE FROM METHODS"));
        if (errorOccurred())
            return false;

        exec(QLatin1String("DELETE FROM MECHANISMS"));
        if (errorOccurred())
            return false;

        exec(QLatin1String("DELETE FROM ACL"));
        if (errorOccurred())
            return false;

        exec(QLatin1String("DELETE FROM REALMS"));
        if (errorOccurred())
            return false;

        return true;
    }

    QStringList CredentialsDB::accessControlList(const quint32 identityId)
    {
        return queryList(QString::fromLatin1(
                "SELECT token FROM ACL WHERE identity_id = %1")
                         .arg(identityId));
    }

    QString CredentialsDB::credentialsOwnerSecurityToken(const quint32 identityId)
    {
        QStringList acl = accessControlList(identityId);
        int index = -1;
        QRegExp aegisIdTokenPrefixRegExp(QLatin1String("^AID::.*"));
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

} //namespace SignonDaemonNS
