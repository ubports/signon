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

#define INIT_ERROR() ErrorMonitor errorMonitor(this)

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
    m_database.commit();
    m_database.close();
}

bool SqlDatabase::init()
{
    if (!connect())
        return false;

    TRACE() <<  "Database connection succeeded.";

    if (!hasTables()) {
        TRACE() << "Creating SQL table structure...";
        if (!createTables())
            return false;
    } else {
        TRACE() << "SQL table structure already created...";
    }

    return true;
}

bool SqlDatabase::connect()
{
    if (!m_database.open()) {
        TRACE() << "Could not open database connection.\n";
        m_lastError = m_database.lastError();
        return false;
    }
    return true;
}

void SqlDatabase::disconnect()
{
    m_database.close();
}

bool SqlDatabase::startTransaction()
{
    return m_database.transaction();
}

bool SqlDatabase::commit()
{
    return m_database.commit();
}

void SqlDatabase::rollback()
{
    if (!m_database.rollback())
        TRACE() << "Rollback failed, db data integrity could be compromised.";
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

QSqlQuery SqlDatabase::exec(QSqlQuery &query)
{

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
    if (!startTransaction()) {
        m_lastError = m_database.lastError();
        TRACE() << "Could not start transaction";
        return false;
    }

    bool allOk = true;
    foreach (QString queryStr, queryList) {
        TRACE() << QString::fromLatin1("TRANSACT Query [%1]").arg(queryStr);
        QSqlQuery query = exec(queryStr);

        if (errorOccurred()) {
            allOk = false;
            break;
        }
    }

    if (allOk && commit()) {
        TRACE() << "Commit SUCCEEDED.";
        return true;
    } else {
        rollback();
    }

    TRACE() << "Transactional exec FAILED!";
    return false;
}

QSqlError SqlDatabase::lastError() const
{
    return m_lastError;
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

QStringList SqlDatabase::queryList(const QString &query_str)
{
    TRACE();
    QStringList list;
    QSqlQuery query = exec(query_str);
    if (errorOccurred()) return list;
    while (query.next()) {
        list.append(query.value(0).toString());
    }
    query.clear();
    return list;
}

bool MetaDataDB::createTables()
{
    /* !!! Foreign keys support seems to be disabled, for the moment... */
    QStringList createTableQuery = QStringList()
        <<  QString::fromLatin1(
            "CREATE TABLE CREDENTIALS"
            "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "caption TEXT,"
            "username TEXT,"
            "password TEXT,"
            "flags INTEGER,"
            "type INTEGER)")
        <<  QString::fromLatin1(
            "CREATE TABLE METHODS"
            "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "method TEXT UNIQUE)")
        <<  QString::fromLatin1(
            "CREATE TABLE MECHANISMS"
            "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "mechanism TEXT UNIQUE)")
        <<  QString::fromLatin1(
            "CREATE TABLE TOKENS"
            "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "token TEXT UNIQUE)")
        <<  QString::fromLatin1(
            "CREATE TABLE REALMS"
            "(identity_id INTEGER CONSTRAINT fk_identity_id REFERENCES CREDENTIALS(id) ON DELETE CASCADE,"
            "realm TEXT,"
            "hostname TEXT,"
            "PRIMARY KEY (identity_id, realm, hostname))")
        <<  QString::fromLatin1(
            "CREATE TABLE ACL"
            "(rowid INTEGER PRIMARY KEY AUTOINCREMENT,"
            "identity_id INTEGER CONSTRAINT fk_identity_id REFERENCES CREDENTIALS(id) ON DELETE CASCADE,"
            "method_id INTEGER CONSTRAINT fk_method_id REFERENCES METHODS(id) ON DELETE CASCADE,"
            "mechanism_id INTEGER CONSTRAINT fk_mechanism_id REFERENCES MECHANISMS(id) ON DELETE CASCADE,"
            "token_id INTEGER CONSTRAINT fk_token_id REFERENCES TOKENS(id) ON DELETE CASCADE)")
        <<  QString::fromLatin1(
            "CREATE TABLE REFS"
            "(identity_id INTEGER CONSTRAINT fk_identity_id REFERENCES CREDENTIALS(id) ON DELETE CASCADE,"
            "token_id INTEGER CONSTRAINT fk_token_id REFERENCES TOKENS(id) ON DELETE CASCADE,"
            "ref TEXT,"
            "PRIMARY KEY (identity_id, token_id, ref))")
        <<  QString::fromLatin1(
            "CREATE TABLE STORE"
            "(identity_id INTEGER CONSTRAINT fk_identity_id REFERENCES CREDENTIALS(id) ON DELETE CASCADE,"
            "method_id INTEGER CONSTRAINT fk_method_id REFERENCES METHODS(id) ON DELETE CASCADE,"
            "key TEXT,"
            "value BLOB,"
            "PRIMARY KEY (identity_id, method_id, key))")

/*
* triggers generated with
* http://www.rcs-comp.com/site/index.php/view/Utilities-SQLite_foreign_key_trigger_generator
*/
        //insert triggers to force foreign keys support
        << QString::fromLatin1(
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_REALMS_identity_id_CREDENTIALS_id "
            "BEFORE INSERT ON [REALMS] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table REALMS violates foreign key constraint fki_REALMS_identity_id_CREDENTIALS_id') "
            "  WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign key preventing update
            "CREATE TRIGGER fku_REALMS_identity_id_CREDENTIALS_id "
            "BEFORE UPDATE ON [REALMS] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table REALMS violates foreign key constraint fku_REALMS_identity_id_CREDENTIALS_id') "
            "      WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER fkdc_REALMS_identity_id_CREDENTIALS_id "
            "BEFORE DELETE ON CREDENTIALS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM REALMS WHERE REALMS.identity_id = OLD.id; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_ACL_identity_id_CREDENTIALS_id "
            "BEFORE INSERT ON [ACL] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table ACL violates foreign key constraint fki_ACL_identity_id_CREDENTIALS_id') "
            "  WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END;"
        )
        << QString::fromLatin1(
            // Foreign key preventing update
            "CREATE TRIGGER fku_ACL_identity_id_CREDENTIALS_id "
            "BEFORE UPDATE ON [ACL] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table ACL violates foreign key constraint fku_ACL_identity_id_CREDENTIALS_id') "
            "      WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER fkdc_ACL_identity_id_CREDENTIALS_id "
            "BEFORE DELETE ON CREDENTIALS "
            "FOR EACH ROW BEGIN "
             "   DELETE FROM ACL WHERE ACL.identity_id = OLD.id; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_ACL_method_id_METHODS_id "
            "BEFORE INSERT ON [ACL] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table ACL violates foreign key constraint fki_ACL_method_id_METHODS_id') "
            "  WHERE NEW.method_id IS NOT NULL AND (SELECT id FROM METHODS WHERE id = NEW.method_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign key preventing update
            "CREATE TRIGGER fku_ACL_method_id_METHODS_id "
            "BEFORE UPDATE ON [ACL] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table ACL violates foreign key constraint fku_ACL_method_id_METHODS_id') "
            "      WHERE NEW.method_id IS NOT NULL AND (SELECT id FROM METHODS WHERE id = NEW.method_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER fkdc_ACL_method_id_METHODS_id "
            "BEFORE DELETE ON METHODS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM ACL WHERE ACL.method_id = OLD.id; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_ACL_mechanism_id_MECHANISMS_id "
            "BEFORE INSERT ON [ACL] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table ACL violates foreign key constraint fki_ACL_mechanism_id_MECHANISMS_id') "
            "  WHERE NEW.mechanism_id IS NOT NULL AND (SELECT id FROM MECHANISMS WHERE id = NEW.mechanism_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign key preventing update
            "CREATE TRIGGER fku_ACL_mechanism_id_MECHANISMS_id "
            "BEFORE UPDATE ON [ACL] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table ACL violates foreign key constraint fku_ACL_mechanism_id_MECHANISMS_id') "
            "      WHERE NEW.mechanism_id IS NOT NULL AND (SELECT id FROM MECHANISMS WHERE id = NEW.mechanism_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER fkdc_ACL_mechanism_id_MECHANISMS_id "
            "BEFORE DELETE ON MECHANISMS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM ACL WHERE ACL.mechanism_id = OLD.id; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_ACL_token_id_TOKENS_id "
            "BEFORE INSERT ON [ACL] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table ACL violates foreign key constraint fki_ACL_token_id_TOKENS_id') "
            "  WHERE NEW.token_id IS NOT NULL AND (SELECT id FROM TOKENS WHERE id = NEW.token_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign key preventing update
            "CREATE TRIGGER fku_ACL_token_id_TOKENS_id "
            "BEFORE UPDATE ON [ACL] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table ACL violates foreign key constraint fku_ACL_token_id_TOKENS_id') "
            "      WHERE NEW.token_id IS NOT NULL AND (SELECT id FROM TOKENS WHERE id = NEW.token_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER fkdc_ACL_token_id_TOKENS_id "
            "BEFORE DELETE ON TOKENS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM ACL WHERE ACL.token_id = OLD.id; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_REFS_identity_id_CREDENTIALS_id "
            "BEFORE INSERT ON [REFS] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table REFS violates foreign key constraint fki_REFS_identity_id_CREDENTIALS_id') "
            "  WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign key preventing update
            "CREATE TRIGGER fku_REFS_identity_id_CREDENTIALS_id "
            "BEFORE UPDATE ON [REFS] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table REFS violates foreign key constraint fku_REFS_identity_id_CREDENTIALS_id') "
            "      WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER fkdc_REFS_identity_id_CREDENTIALS_id "
            "BEFORE DELETE ON CREDENTIALS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM REFS WHERE REFS.identity_id = OLD.id; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_REFS_token_id_TOKENS_id "
            "BEFORE INSERT ON [REFS] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table REFS violates foreign key constraint fki_REFS_token_id_TOKENS_id') "
            "  WHERE NEW.token_id IS NOT NULL AND (SELECT id FROM TOKENS WHERE id = NEW.token_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign key preventing update
            "CREATE TRIGGER fku_REFS_token_id_TOKENS_id "
            "BEFORE UPDATE ON [REFS] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table REFS violates foreign key constraint fku_REFS_token_id_TOKENS_id') "
            "      WHERE NEW.token_id IS NOT NULL AND (SELECT id FROM TOKENS WHERE id = NEW.token_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER fkdc_REFS_token_id_TOKENS_id "
            "BEFORE DELETE ON TOKENS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM REFS WHERE REFS.token_id = OLD.id; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_STORE_identity_id_CREDENTIALS_id "
            "BEFORE INSERT ON [STORE] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table STORE violates foreign key constraint fki_STORE_identity_id_CREDENTIALS_id') "
            "  WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign key preventing update
            "CREATE TRIGGER fku_STORE_identity_id_CREDENTIALS_id "
            "BEFORE UPDATE ON [STORE] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table STORE violates foreign key constraint fku_STORE_identity_id_CREDENTIALS_id') "
            "      WHERE NEW.identity_id IS NOT NULL AND (SELECT id FROM CREDENTIALS WHERE id = NEW.identity_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER fkdc_STORE_identity_id_CREDENTIALS_id "
            "BEFORE DELETE ON CREDENTIALS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM STORE WHERE STORE.identity_id = OLD.id; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign Key Preventing insert
            "CREATE TRIGGER fki_STORE_method_id_METHODS_id "
            "BEFORE INSERT ON [STORE] "
            "FOR EACH ROW BEGIN "
            "  SELECT RAISE(ROLLBACK, 'insert on table STORE violates foreign key constraint fki_STORE_method_id_METHODS_id') "
            "  WHERE NEW.method_id IS NOT NULL AND (SELECT id FROM METHODS WHERE id = NEW.method_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Foreign key preventing update
            "CREATE TRIGGER fku_STORE_method_id_METHODS_id "
            "BEFORE UPDATE ON [STORE] "
            "FOR EACH ROW BEGIN "
            "    SELECT RAISE(ROLLBACK, 'update on table STORE violates foreign key constraint fku_STORE_method_id_METHODS_id') "
            "      WHERE NEW.method_id IS NOT NULL AND (SELECT id FROM METHODS WHERE id = NEW.method_id) IS NULL; "
            "END; "
        )
        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER fkdc_STORE_method_id_METHODS_id "
            "BEFORE DELETE ON METHODS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM STORE WHERE STORE.method_id = OLD.id; "
            "END; "
        );
/*
end of generated code
*/
   foreach (QString createTable, createTableQuery) {
        QSqlQuery query = exec(createTable);
        if (lastError().isValid()) {
            TRACE() << "Error occurred while creating the database.";
            return false;
        }
        query.clear();
        commit();
    }
    return true;
}

QStringList MetaDataDB::methods(const quint32 id, const QString &securityToken)
{
    QStringList list;
    if (securityToken.isEmpty()) {
        list = queryList(
                 QString::fromLatin1("SELECT DISTINCT METHODS.method FROM "
                        "( ACL JOIN METHODS ON ACL.method_id = METHODS.id ) "
                        "WHERE ACL.identity_id = '%1'").arg(id)
                 );
        return list;
    }
    list = queryList(
                 QString::fromLatin1("SELECT DISTINCT METHODS.method FROM "
                        "( ACL JOIN METHODS ON ACL.method_id = METHODS.id) "
                        "WHERE ACL.identity_id = '%1 AND ACL.token_id = "
                        "(SELECT id FROM TOKENS where token = '%2')'")
                 .arg(id).arg(securityToken)
                 );

    return list;
}

bool MetaDataDB::checkPassword(const quint32 id,
                               const QString &username,
                               const QString &password)
{
    QSqlQuery query = exec(
            QString::fromLatin1("SELECT id FROM CREDENTIALS "
                    "WHERE id = '%1' AND username = '%2' AND password = '%3'")
                    .arg(id).arg(username).arg(password));

    if (errorOccurred()) {
        TRACE() << "Error occurred while checking password";
        return false;
    }
    bool valid = false;
    valid = query.first();
    query.clear();

    return valid;
}

SignonIdentityInfo MetaDataDB::credentials(const quint32 id, bool queryPassword)
{
    QString query_str;

    query_str = QString::fromLatin1(
            "SELECT caption, username, flags, type, password "
            "FROM credentials WHERE id = %1").arg(id);
    QSqlQuery query = exec(query_str);

    if (!query.first()) {
        TRACE() << "No result or invalid credentials query.";
        return SignonIdentityInfo();
    }

    QString caption = query.value(0).toString();
    QString username = query.value(1).toString();
    int flags = query.value(2).toInt();
    bool savePassword = flags & RememberPassword;
    bool validated =  flags & Validated;
    int type = query.value(3).toInt();
    QString password;
    if (savePassword && queryPassword)
        password = query.value(4).toString();

    query.clear();
    QStringList realms = queryList(
            QString::fromLatin1("SELECT realm FROM REALMS "
                    "WHERE identity_id = %1").arg(id));

    query_str = QString::fromLatin1("SELECT token FROM TOKENS "
            "WHERE id IN "
            "(SELECT token_id FROM ACL WHERE identity_id = '%1' )")
            .arg(id);
    query = exec(query_str);
    QStringList security_tokens;
    while (query.next()) {
        security_tokens.append(query.value(0).toString());
    }
    query.clear();
    QMap<QString, QVariant> methods;
    query_str = QString::fromLatin1(
            "SELECT DISTINCT ACL.method_id, METHODS.method FROM "
            "( ACL JOIN METHODS ON ACL.method_id = METHODS.id ) "
            "WHERE ACL.identity_id = '%1'").arg(id);
    query = exec(query_str);
    while (query.next()) {
        TRACE() << query.value(0);
        QStringList mechanisms = queryList(
                QString::fromLatin1("SELECT DISTINCT MECHANISMS.mechanism FROM "
                        "( MECHANISMS JOIN ACL "
                        "ON ACL.mechanism_id = MECHANISMS.id ) "
                        "WHERE ACL.method_id = '%1' AND ACL.identity_id = '%2' ")
                        .arg(query.value(0).toInt()).arg(id));
            TRACE() << mechanisms; //TODO HERE
            methods.insert(query.value(1).toString(), mechanisms);
    }
    query.clear();

    int refCount = 0;
    //TODO query for refcount

    return SignonIdentityInfo(id, username, password, savePassword, methods,
                              caption, realms, security_tokens, type, refCount, validated);
}

QList<SignonIdentityInfo> MetaDataDB::credentials(const QMap<QString, QString> &filter)
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

    query.clear();
    return result;
}

quint32 MetaDataDB::updateCredentials(const SignonIdentityInfo &info, bool storeSecret)
{
    if (!startTransaction()) {
        TRACE() << "Could not start transaction. Error inserting credentials.";
        return 0;
    }
    quint32 id = 0;
    QSqlQuery insertQuery;
    /* Credentials insert */
    QString password;
    if (storeSecret && info.storePassword())
        password = info.password();

    QString queryStr;
    int flags = 0;
    if (info.validated()) flags |= Validated;
    if (info.storePassword()) flags |= RememberPassword;

    if (!info.isNew()) {
        TRACE() << "UPDATE:" << info.id() ;
         id = info.id() ;
        queryStr = QString::fromLatin1(
            "UPDATE CREDENTIALS SET caption = '%1', username = '%2', "
            "password = '%3', flags = '%4', "
            "type = '%5' WHERE id = '%6'")
            .arg(info.caption()).arg(info.userName()).arg(password)
            .arg(flags).arg(info.type())
            .arg(info.id());

        insertQuery = exec(queryStr);
        insertQuery.clear();
        if (errorOccurred()) {
            rollback();
            TRACE() << "Error occurred while updating crendentials";
            return 0;
        }

     } else {
        TRACE() << "INSERT:" << info.id();
        queryStr = QString::fromLatin1(
            "INSERT INTO CREDENTIALS "
            "(caption, username, password, flags, type) "
            "VALUES('%1', '%2', '%3', '%4', '%5')")
            .arg(info.caption()).arg(info.userName()).arg(password)
            .arg(flags).arg(info.type());

        insertQuery = exec(queryStr);
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
        id = idVariant.toUInt();
    }
    insertQuery.clear();

    /* Methods inserts */
    insertMethods(info.methods());

    if (!info.isNew()) {
        //remove realms list
        queryStr = QString::fromLatin1(
                    "DELETE FROM REALMS WHERE "
                    "identity_id = '%1'")
                    .arg(info.id());
        insertQuery = exec(queryStr);
    }
    insertQuery.clear();

     /* Realms insert */
    foreach (QString realm, info.realms()) {
        queryStr = QString::fromLatin1(
                    "INSERT OR IGNORE INTO REALMS (identity_id, realm) "
                    "VALUES ( '%1', '%2')")
                    .arg(id).arg(realm);
        insertQuery = exec(queryStr);
    }
    insertQuery.clear();

    /* Security tokens insert */
    foreach (QString token, info.accessControlList()) {
        queryStr = QString::fromLatin1(
                    "INSERT OR IGNORE INTO TOKENS (token) "
                    "VALUES ( '%1' )")
                    .arg(token);
        insertQuery = exec(queryStr);
    }
    insertQuery.clear();

    if (!info.isNew()) {
        //remove acl
        queryStr = QString::fromLatin1(
                    "DELETE FROM ACL WHERE "
                    "identity_id = '%1'")
                    .arg(info.id());
        insertQuery = exec(queryStr);
    }
    insertQuery.clear();

    /* ACL insert, this will do basically identity level ACL */
    QMapIterator<QString, QStringList> it(info.methods());
    while (it.hasNext()) {
        it.next();
        if (!info.accessControlList().isEmpty()) {
            foreach (QString token, info.accessControlList()) {
                foreach (QString mech, it.value()) {
                    queryStr = QString::fromLatin1(
                        "INSERT OR REPLACE INTO ACL "
                        "(identity_id, method_id, mechanism_id, token_id) "
                        "VALUES ( '%1', "
                        "( SELECT id FROM METHODS WHERE method = '%2' ),"
                        "( SELECT id FROM MECHANISMS WHERE mechanism= '%3' ), "
                        "( SELECT id FROM TOKENS WHERE token = '%4' ))")
                        .arg(id).arg(it.key()).arg(mech).arg(token);
                    insertQuery = exec(queryStr);
                    insertQuery.clear();
                }
                //insert entires for empty mechs list
                if (it.value().isEmpty()) {
                    queryStr = QString::fromLatin1(
                        "INSERT OR REPLACE INTO ACL (identity_id, method_id, token_id) "
                        "VALUES ( '%1', "
                        "( SELECT id FROM METHODS WHERE method = '%2' ),"
                        "( SELECT id FROM TOKENS WHERE token = '%3' ))")
                        .arg(id).arg(it.key()).arg(token);
                    insertQuery = exec(queryStr);
                    insertQuery.clear();
                }
            }
        } else {
            foreach (QString mech, it.value()) {
                queryStr = QString::fromLatin1(
                    "INSERT OR REPLACE INTO ACL "
                    "(identity_id, method_id, mechanism_id) "
                    "VALUES ( '%1', "
                    "( SELECT id FROM METHODS WHERE method = '%2' ),"
                    "( SELECT id FROM MECHANISMS WHERE mechanism= '%3' )"
                    ")")
                    .arg(id).arg(it.key()).arg(mech);
                insertQuery = exec(queryStr);
                insertQuery.clear();
            }
            //insert entires for empty mechs list
            if (it.value().isEmpty()) {
                queryStr = QString::fromLatin1(
                    "INSERT OR REPLACE INTO ACL (identity_id, method_id) "
                    "VALUES ( '%1', "
                    "( SELECT id FROM METHODS WHERE method = '%2' )"
                    ")")
                    .arg(id).arg(it.key());
                insertQuery = exec(queryStr);
                insertQuery.clear();
            }
        }
    }
    //insert acl in case where methods are missing
    if (info.methods().isEmpty()) {
        foreach (QString token, info.accessControlList()) {
            queryStr = QString::fromLatin1(
                    "INSERT OR REPLACE INTO ACL "
                    "(identity_id, token_id) "
                    "VALUES ( '%1', "
                    "( SELECT id FROM TOKENS WHERE token = '%2' ))")
                    .arg(id).arg(token);
            insertQuery = exec(queryStr);
            insertQuery.clear();
        }
    }

    if (commit()) {
        return id;
    } else {
        rollback();
        TRACE() << "Credentials insertion failed.";
        return 0;
    }
}

bool MetaDataDB::removeCredentials(const quint32 id)
{
    TRACE();

    QStringList queries = QStringList()
        << QString::fromLatin1(
            "DELETE FROM CREDENTIALS WHERE id = %1").arg(id)
        << QString::fromLatin1(
            "DELETE FROM ACL WHERE identity_id = %1").arg(id)
        << QString::fromLatin1(
            "DELETE FROM REALMS WHERE identity_id = %1").arg(id)
        << QString::fromLatin1(
            "DELETE FROM STORE WHERE identity_id = %1").arg(id);

    return transactionalExec(queries);
}

bool MetaDataDB::clear()
{
    TRACE();

    QStringList clearCommands = QStringList()
        << QLatin1String("DELETE FROM CREDENTIALS")
        << QLatin1String("DELETE FROM METHODS")
        << QLatin1String("DELETE FROM MECHANISMS")
        << QLatin1String("DELETE FROM ACL")
        << QLatin1String("DELETE FROM REALMS")
        << QLatin1String("DELETE FROM TOKENS")
        << QLatin1String("DELETE FROM STORE");

    return transactionalExec(clearCommands);
}

QVariantMap MetaDataDB::loadData(const quint32 id, const QString &method)
{
    TRACE();

    QString query_str = QString::fromLatin1(
            "SELECT key, value "
            "FROM STORE WHERE identity_id = %1 AND "
            "method_id = (SELECT id FROM METHODS WHERE method = '%2')")
            .arg(id).arg(method);

    QSqlQuery query = exec(query_str);
    if (errorOccurred())
        return QVariantMap();

    QVariantMap result;
    while (query.next()) {
        QByteArray array;
        array = query.value(1).toByteArray();
        QDataStream stream(array);
        QVariant data;
        stream >> data;
        result.insert(query.value(0).toString(), data);
        TRACE() << "insert" << query.value(0).toString() << ", " << data ;
    }
    query.clear();
    return result;
}

bool MetaDataDB::storeData(const quint32 id, const QString &method, const QVariantMap &data)
{
    TRACE();

    if (!startTransaction()) {
        TRACE() << "Could not start transaction. Error inserting data.";
        return false;
    }

    TRACE() << "Storing:" << id << ", " << method << ", " << data;
    /* Data insert */
    bool allOk = true;
    qint32 dataCounter = 0;
    QString queryStr;
    QSqlQuery query;
    if (!(data.keys().empty())) {
        QMapIterator<QString, QVariant> it(data);
        while (it.hasNext()) {
            it.next();

            QByteArray array;
            QDataStream stream(&array, QIODevice::WriteOnly);
            stream << it.value();

            dataCounter += it.key().size() +array.size();
            if (dataCounter >= SSO_MAX_TOKEN_STORAGE) {
                BLAME() << "storing data max size exceeded";
                allOk = false;
                break;
            }
            /* Key/value insert/replace/delete */
            if (it.value().isValid() && !it.value().isNull()) {
                TRACE() << "insert";
                query = QSqlQuery(QString(), m_database);
                query.prepare(QString::fromLatin1(
                    "INSERT OR REPLACE INTO STORE "
                    "(identity_id, method_id, key, value) "
                    "VALUES('%1', "
                    "(SELECT id FROM METHODS WHERE method = '%2'), "
                    "'%3', :blob)")
                    .arg(id).arg(method).arg(it.key()));
                query.bindValue(QLatin1String(":blob"), array);
                exec(query);
                if (errorOccurred()) {
                    allOk = false;
                    TRACE() << errorInfo(query.lastError());
                }
            } else {
                TRACE() << "remove";
                queryStr = QString::fromLatin1(
                    "DELETE FROM STORE WHERE identity_id = '%1' AND "
                    "method_id = "
                    "(SELECT id FROM METHODS WHERE method = '%2') "
                    "AND key = '%3'")
                    .arg(id).arg(method).arg(it.key());
                query = exec(queryStr);

            }
            query.clear();
            if (errorOccurred()) {
                allOk = false;
                break;
            }
        }
    }

    if (allOk && commit()) {
        TRACE() << "Data insertion ok.";
        return true;
    }
    rollback();
    TRACE() << "Data insertion failed.";
    return false;
}

bool MetaDataDB::removeData(const quint32 id, const QString &method)
{
    TRACE();

    if (!startTransaction()) {
        TRACE() << "Could not start transaction. Error removing data.";
        return false;
    }

    TRACE() << "Removing:" << id << ", " << method;
    /* Data remove */
    bool allOk = true;
    QString queryStr;
    QSqlQuery query;
    if (method.isEmpty()) {
        queryStr = QString::fromLatin1(
                    "DELETE FROM STORE WHERE identity_id = '%1' ")
                    .arg(id);
    } else {
        queryStr = QString::fromLatin1(
                    "DELETE FROM STORE WHERE identity_id = '%1' AND "
                    "method_id = "
                    "(SELECT id FROM METHODS WHERE method = '%2') ")
                    .arg(id).arg(method);
    }
    query = exec(queryStr);
    query.clear();
    if (errorOccurred()) {
        allOk = false;
    }

    if (allOk && commit()) {
        TRACE() << "Data removal ok.";
        return true;
    }
    rollback();
    TRACE() << "Data removal failed.";
    return false;
}

QStringList MetaDataDB::accessControlList(const quint32 identityId)
{
    return queryList(QString::fromLatin1("SELECT token FROM TOKENS "
            "WHERE id IN "
            "(SELECT token_id FROM ACL WHERE identity_id = '%1' )")
            .arg(identityId));
}

bool MetaDataDB::addReference(const quint32 id, const QString &token, const QString &reference)
{

    if (!startTransaction()) {
        TRACE() << "Could not start transaction. Error inserting data.";
        return false;
    }

    TRACE() << "Storing:" << id << ", " << token << ", " << reference;
    /* Data insert */
    bool allOk = true;
    QString queryStr;
    QSqlQuery query;

    /* Security token insert */
    queryStr = QString::fromLatin1(
                    "INSERT OR IGNORE INTO TOKENS (token) "
                    "VALUES ( '%1' )")
                    .arg(token);
    query = exec(queryStr);
    query.clear();
    if (errorOccurred()) {
                allOk = false;
    }

    queryStr = QString::fromLatin1(
                    "INSERT OR REPLACE INTO REFS "
                    "(identity_id, token_id, ref) "
                    "VALUES ( '%1', "
                    "( SELECT id FROM TOKENS WHERE token = '%2' ),"
                    "'%3'"
                    ")")
                    .arg(id).arg(token).arg(reference);
    query = exec(queryStr);
    query.clear();
    if (errorOccurred()) {
                allOk = false;
    }

    if (allOk && commit()) {
        TRACE() << "Data insertion ok.";
        return true;
    }
    rollback();
    TRACE() << "Data insertion failed.";
    return false;
}

bool MetaDataDB::removeReference(const quint32 id, const QString &token, const QString &reference)
{
    TRACE() << "Removing:" << id << ", " << token << ", " << reference;
    //check that there is references
    QStringList refs = references(id, token);
    if (refs.isEmpty())
        return false;
    if (!reference.isNull() && !refs.contains(reference))
        return false;

    if (!startTransaction()) {
        TRACE() << "Could not start transaction. Error removing data.";
        return false;
    }

    bool allOk = true;
    QString queryStr;
    QSqlQuery query;

    if (reference.isEmpty())
        queryStr = QString::fromLatin1(
                    "DELETE FROM REFS "
                    "WHERE identity_id = '%1' AND "
                    "token_id = ( SELECT id FROM TOKENS WHERE token = '%2' )")
                    .arg(id).arg(token);
    else
        queryStr = QString::fromLatin1(
                    "DELETE FROM REFS "
                    "WHERE identity_id = '%1' AND "
                    "token_id = ( SELECT id FROM TOKENS WHERE token = '%2' ) "
                    "AND ref ='%3'")
                    .arg(id).arg(token).arg(reference);

    query = exec(queryStr);
    query.clear();
    if (errorOccurred()) {
                allOk = false;
    }

    if (allOk && commit()) {
        TRACE() << "Data delete ok.";
        return true;
    }
    rollback();
    TRACE() << "Data delete failed.";
    return false;
}

QStringList MetaDataDB::references(const quint32 id, const QString &token)
{
    if (token.isEmpty())
        return queryList(QString::fromLatin1("SELECT ref FROM REFS "
            "WHERE identity_id = '%1'")
            .arg(id));
    return queryList(QString::fromLatin1("SELECT ref FROM REFS "
            "WHERE identity_id = '%1' AND "
            "token_id = (SELECT id FROM TOKENS WHERE token = '%2' )")
            .arg(id).arg(token));
}

bool MetaDataDB::insertMethods(QMap<QString, QStringList> methods)
{
    QString queryStr;
    QSqlQuery insertQuery;
    bool allOk = true;

    if (methods.isEmpty()) return false;
    //insert (unique) method names
    QMapIterator<QString, QStringList> it(methods);
    while (it.hasNext()) {
        it.next();
        queryStr = QString::fromLatin1(
                    "INSERT OR IGNORE INTO METHODS (method) "
                    "VALUES( '%1' )")
                    .arg(it.key());
        insertQuery = exec(queryStr);
        insertQuery.clear();
        if (errorOccurred()) allOk = false;
        //insert (unique) mechanism names
        foreach (QString mech, it.value()) {
            queryStr = QString::fromLatin1(
                        "INSERT OR IGNORE INTO MECHANISMS (mechanism) "
                        "VALUES( '%1' )")
                        .arg(mech);
            insertQuery = exec(queryStr);
            if (errorOccurred()) allOk = false;
            insertQuery.clear();
        }
    }
    return allOk;
}

bool SecretsDB::createTables()
{
    QStringList createTableQuery = QStringList()
        <<  QString::fromLatin1(
            "CREATE TABLE CREDENTIALS"
            "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT,"
            "password TEXT)")
        <<  QString::fromLatin1(
            "CREATE TABLE STORE"
            "(identity_id INTEGER,"
            "method_id INTEGER,"
            "key TEXT,"
            "value BLOB,"
            "PRIMARY KEY (identity_id, method_id, key))")

        << QString::fromLatin1(
            // Cascading Delete
            "CREATE TRIGGER tg_delete_credentials "
            "BEFORE DELETE ON CREDENTIALS "
            "FOR EACH ROW BEGIN "
            "    DELETE FROM STORE WHERE STORE.identity_id = OLD.id; "
            "END; "
        );

   foreach (QString createTable, createTableQuery) {
        QSqlQuery query = exec(createTable);
        if (lastError().isValid()) {
            TRACE() << "Error occurred while creating the database.";
            return false;
        }
        query.clear();
        commit();
    }
    return true;
}

bool SecretsDB::clear()
{
    TRACE();

    QStringList clearCommands = QStringList()
        << QLatin1String("DELETE FROM CREDENTIALS")
        << QLatin1String("DELETE FROM STORE");

    return transactionalExec(clearCommands);
}

/* Error monitor class */

CredentialsDB::ErrorMonitor::ErrorMonitor(CredentialsDB *db)
{
    db->_lastError.setType(QSqlError::NoError);
    db->metaDataDB->clearError();
    if (db->secretsDB != 0)
        db->secretsDB->clearError();
    _db = db;
}

CredentialsDB::ErrorMonitor::~ErrorMonitor()
{
    /* If there's an error set on the CredentialsDB, just let it be and return.
     * If not, take the error from the SqlDatabase objects, if any.
     */
    if (_db->_lastError.isValid())
        return;

    if (_db->secretsDB != 0 && _db->secretsDB->errorOccurred()) {
        _db->_lastError = _db->secretsDB->lastError();
        return;
    }

    _db->_lastError = _db->metaDataDB->lastError();
}

/*    -------   CredentialsDB  implementation   -------    */

CredentialsDB::CredentialsDB(const QString &metaDataDbName):
    secretsDB(0),
    metaDataDB(new MetaDataDB(metaDataDbName))
{
    noSecretsDB = QSqlError(QLatin1String("Secrets DB not opened"),
                            QLatin1String("Secrets DB not opened"),
                            QSqlError::ConnectionError);
}

CredentialsDB::~CredentialsDB()
{
    if (secretsDB)
        delete secretsDB;
    if (metaDataDB)
        delete metaDataDB;

    SqlDatabase::removeDatabase();
}

bool CredentialsDB::init()
{
    return metaDataDB->init();
}

bool CredentialsDB::openSecretsDB(const QString &secretsDbName)
{
    secretsDB = new SecretsDB(secretsDbName);

    if (!secretsDB->init()) {
        TRACE() << SqlDatabase::errorInfo(lastError());
        delete secretsDB;
        secretsDB = 0;
        return false;
    }

    TRACE() << secretsDB->configuration();
    return true;
}

bool CredentialsDB::isSecretsDBOpen()
{
    return secretsDB != 0;
}

void CredentialsDB::closeSecretsDB()
{
    delete secretsDB;
}

CredentialsDBError CredentialsDB::lastError() const
{
    return _lastError;
}

QStringList CredentialsDB::methods(const quint32 id, const QString &securityToken)
{
    INIT_ERROR();
    return metaDataDB->methods(id, securityToken);
}

bool CredentialsDB::checkPassword(const quint32 id,
                                  const QString &username,
                                  const QString &password)
{
    INIT_ERROR();
    return metaDataDB->checkPassword(id, username, password);
}

SignonIdentityInfo CredentialsDB::credentials(const quint32 id, bool queryPassword)
{
    INIT_ERROR();
    return metaDataDB->credentials(id, queryPassword);
}

QList<SignonIdentityInfo> CredentialsDB::credentials(const QMap<QString, QString> &filter)
{
    INIT_ERROR();
    return metaDataDB->credentials(filter);
}

quint32 CredentialsDB::insertCredentials(const SignonIdentityInfo &info, bool storeSecret)
{
    SignonIdentityInfo newInfo = info;
    if (!info.isNew())
        newInfo.setNew();
    return updateCredentials(newInfo, storeSecret);
}

quint32 CredentialsDB::updateCredentials(const SignonIdentityInfo &info, bool storeSecret)
{
    INIT_ERROR();
    return metaDataDB->updateCredentials(info, storeSecret);
}

bool CredentialsDB::removeCredentials(const quint32 id)
{
    INIT_ERROR();
    return metaDataDB->removeCredentials(id);
}

bool CredentialsDB::clear()
{
    TRACE();

    INIT_ERROR();

    /* We don't allow clearing the DB if the secrets DB is not available */
    if (!isSecretsDBOpen()) {
        TRACE() << "Secrets DB not opened; aborting CLEAR operation";
        _lastError = noSecretsDB;
        return false;
    }

    return secretsDB->clear() && metaDataDB->clear();
}

QVariantMap CredentialsDB::loadData(const quint32 id, const QString &method)
{
    INIT_ERROR();
    return metaDataDB->loadData(id, method);
}

bool CredentialsDB::storeData(const quint32 id, const QString &method, const QVariantMap &data)
{
    INIT_ERROR();
    return metaDataDB->storeData(id, method, data);
}

bool CredentialsDB::removeData(const quint32 id, const QString &method)
{
    INIT_ERROR();
    return metaDataDB->removeData(id, method);
}

QStringList CredentialsDB::accessControlList(const quint32 identityId)
{
    INIT_ERROR();
    return metaDataDB->accessControlList(identityId);
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

bool CredentialsDB::addReference(const quint32 id, const QString &token, const QString &reference)
{
    INIT_ERROR();
    return metaDataDB->addReference(id, token, reference);
}

bool CredentialsDB::removeReference(const quint32 id, const QString &token, const QString &reference)
{
    INIT_ERROR();
    return metaDataDB->removeReference(id, token, reference);
}

QStringList CredentialsDB::references(const quint32 id, const QString &token)
{
    INIT_ERROR();
    return metaDataDB->references(id, token);
}

} //namespace SignonDaemonNS
