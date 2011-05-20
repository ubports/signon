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
#define RETURN_IF_NO_SECRETS_DB(retval) \
    if (!isSecretsDBOpen()) { \
        TRACE() << "Secrets DB is not available"; \
        _lastError = noSecretsDB; return retval; \
    }

#define S(s) QLatin1String(s)

namespace SignonDaemonNS {

static const QString driver = QLatin1String("QSQLITE");

SqlDatabase::SqlDatabase(const QString &databaseName,
                         const QString &connectionName,
                         int version):
    m_lastError(QSqlError()),
    m_version(version),
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

        if (!SqlDatabase::updateDB(m_version))
            BLAME() << "Failed to set database version to: " << m_version
                    << ".This could lead to data loss.";
    } else {
        // check the DB version
        QSqlQuery q = exec(S("PRAGMA user_version"));
        int oldVersion = q.first() ? q.value(0).toInt() : 0;
        if (oldVersion < m_version)
            updateDB(oldVersion);
        TRACE() << "SQL table structure already created...";
    }

    return true;
}

bool SqlDatabase::updateDB(int version)
{
    Q_UNUSED(version);
    TRACE() << "Setting DB version:" << m_version;
    exec(QString::fromLatin1("PRAGMA user_version = %1").arg(m_version));
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

QStringList SqlDatabase::queryList(const QString &query_str)
{
    TRACE();
    QSqlQuery query(QString(), m_database);
    if (!query.prepare(query_str))
        TRACE() << "Query prepare warning: " << query.lastQuery();
    return queryList(query);
}

QStringList SqlDatabase::queryList(QSqlQuery &q)
{
    TRACE();
    QStringList list;
    QSqlQuery query = exec(q);
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
    TRACE() << "Creation successful";

    return true;
}

bool MetaDataDB::updateDB(int version)
{
    if (version < 1) {
        TRACE() << "Upgrading from version < 1 not supported. Clearing DB";
        QString fileName = m_database.databaseName();
        QString connectionName = m_database.connectionName();
        m_database.close();
        QFile::remove(fileName);
        m_database = QSqlDatabase(QSqlDatabase::addDatabase(driver,
                                                            connectionName));
        m_database.setDatabaseName(fileName);
        if (!connect())
            return false;

        if (!createTables())
            return false;
    }
    return SqlDatabase::updateDB(version);
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
    QSqlQuery q = newQuery();
    q.prepare(S("SELECT DISTINCT METHODS.method FROM "
                "( ACL JOIN METHODS ON ACL.method_id = METHODS.id) "
                "WHERE ACL.identity_id = :id AND ACL.token_id = "
                "(SELECT id FROM TOKENS where token = :token)"));
    q.bindValue(S(":id"), id);
    q.bindValue(S(":token"), securityToken);
    list = queryList(q);

    return list;
}

quint32 MetaDataDB::methodId(const QString &method)
{
    TRACE() << "method:" << method;

    QSqlQuery q = newQuery();
    q.prepare(S("SELECT id FROM METHODS WHERE method = :method"));
    q.bindValue(S(":method"), method);
    exec(q);
    if (!q.first()) {
        TRACE() << "No result or invalid method query.";
        return 0;
    }

    return q.value(0).toUInt();
}

SignonIdentityInfo MetaDataDB::identity(const quint32 id)
{
    QString query_str;

    query_str = QString::fromLatin1(
            "SELECT caption, username, flags, type "
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
    bool isUserNameSecret = flags & UserNameIsSecret;
    if (isUserNameSecret) username = QString();
    int type = query.value(3).toInt();

    query.clear();
    QStringList realms = queryList(
            QString::fromLatin1("SELECT realm FROM REALMS "
                    "WHERE identity_id = %1").arg(id));

    //TODO change ACL to OWNER
    query_str = QString::fromLatin1("SELECT token FROM TOKENS "
            "WHERE id IN "
            "(SELECT token_id FROM ACL WHERE identity_id = '%1' )")
            .arg(id);
    query = exec(query_str);
    QStringList ownerTokens;
    while (query.next()) {
        ownerTokens.append(query.value(0).toString());
    }
    query.clear();

    query_str = QString::fromLatin1("SELECT token FROM TOKENS "
            "WHERE id IN "
            "(SELECT token_id FROM ACL WHERE identity_id = '%1' )")
            .arg(id);
    query = exec(query_str);
    QStringList securityTokens;
    while (query.next()) {
        securityTokens.append(query.value(0).toString());
    }
    query.clear();
    QMap<QString, QVariant> methods;
    query_str = QString::fromLatin1(
            "SELECT DISTINCT ACL.method_id, METHODS.method FROM "
            "( ACL JOIN METHODS ON ACL.method_id = METHODS.id ) "
            "WHERE ACL.identity_id = '%1'").arg(id);
    query = exec(query_str);
    while (query.next()) {
        QStringList mechanisms = queryList(
                QString::fromLatin1("SELECT DISTINCT MECHANISMS.mechanism FROM "
                        "( MECHANISMS JOIN ACL "
                        "ON ACL.mechanism_id = MECHANISMS.id ) "
                        "WHERE ACL.method_id = '%1' AND ACL.identity_id = '%2' ")
                        .arg(query.value(0).toInt()).arg(id));
            methods.insert(query.value(1).toString(), mechanisms);
    }
    query.clear();

    int refCount = 0;
    //TODO query for refcount

    SignonIdentityInfo info =
        SignonIdentityInfo(id, username, QString(), savePassword,
                           caption, methods, realms, securityTokens,
                           ownerTokens,
                           type, refCount, validated);
    info.setUserNameSecret(isUserNameSecret);
    return info;
}

QList<SignonIdentityInfo> MetaDataDB::identities(const QMap<QString, QString> &filter)
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
        SignonIdentityInfo info = identity(query.value(0).toUInt());
        if (errorOccurred())
            break;
        result << info;
    }

    query.clear();
    return result;
}

quint32 MetaDataDB::updateIdentity(const SignonIdentityInfo &info)
{
    if (!startTransaction()) {
        TRACE() << "Could not start transaction. Error inserting credentials.";
        return 0;
    }

    quint32 id = updateCredentials(info);
    if (id == 0) {
        rollback();
        return 0;
    }

    /* Methods inserts */
    insertMethods(info.methods());

    if (!updateRealms(id, info.realms(), info.isNew())) {
        TRACE() << "Error in updating realms";
        rollback();
        return 0;
    }

    /* Security tokens insert */
    foreach (QString token, info.accessControlList()) {
        QSqlQuery tokenInsert = newQuery();
        tokenInsert.prepare(S("INSERT OR IGNORE INTO TOKENS (token) "
                              "VALUES ( :token )"));
        tokenInsert.bindValue(S(":token"), token);
        exec(tokenInsert);
    }

    if (!info.isNew()) {
        //remove acl
        QString queryStr = QString::fromLatin1(
                    "DELETE FROM ACL WHERE "
                    "identity_id = '%1'")
                    .arg(info.id());
        QSqlQuery insertQuery = exec(queryStr);
        insertQuery.clear();
    }

    /* ACL insert, this will do basically identity level ACL */
    QMapIterator<QString, QStringList> it(info.methods());
    while (it.hasNext()) {
        it.next();
        if (!info.accessControlList().isEmpty()) {
            foreach (QString token, info.accessControlList()) {
                foreach (QString mech, it.value()) {
                    QSqlQuery aclInsert = newQuery();
                    aclInsert.prepare(S("INSERT OR REPLACE INTO ACL "
                                        "(identity_id, method_id, mechanism_id, token_id) "
                                        "VALUES ( :id, "
                                        "( SELECT id FROM METHODS WHERE method = :method ),"
                                        "( SELECT id FROM MECHANISMS WHERE mechanism= :mech ), "
                                        "( SELECT id FROM TOKENS WHERE token = :token ))"));
                    aclInsert.bindValue(S(":id"), id);
                    aclInsert.bindValue(S(":method"), it.key());
                    aclInsert.bindValue(S(":mech"), mech);
                    aclInsert.bindValue(S(":token"), token);
                    exec(aclInsert);
                }
                //insert entires for empty mechs list
                if (it.value().isEmpty()) {
                    QSqlQuery aclInsert = newQuery();
                    aclInsert.prepare(S("INSERT OR REPLACE INTO ACL (identity_id, method_id, token_id) "
                                        "VALUES ( :id, "
                                        "( SELECT id FROM METHODS WHERE method = :method ),"
                                        "( SELECT id FROM TOKENS WHERE token = :token ))"));
                    aclInsert.bindValue(S(":id"), id);
                    aclInsert.bindValue(S(":method"), it.key());
                    aclInsert.bindValue(S(":token"), token);
                    exec(aclInsert);
                }
            }
        } else {
            foreach (QString mech, it.value()) {
                QSqlQuery aclInsert = newQuery();
                aclInsert.prepare(S("INSERT OR REPLACE INTO ACL "
                                    "(identity_id, method_id, mechanism_id) "
                                    "VALUES ( :id, "
                                    "( SELECT id FROM METHODS WHERE method = :method ),"
                                    "( SELECT id FROM MECHANISMS WHERE mechanism= :mech )"
                                    ")"));
                aclInsert.bindValue(S(":id"), id);
                aclInsert.bindValue(S(":method"), it.key());
                aclInsert.bindValue(S(":mech"), mech);
                exec(aclInsert);
            }
            //insert entires for empty mechs list
            if (it.value().isEmpty()) {
                QSqlQuery aclInsert = newQuery();
                aclInsert.prepare(S("INSERT OR REPLACE INTO ACL (identity_id, method_id) "
                                    "VALUES ( :id, "
                                    "( SELECT id FROM METHODS WHERE method = :method )"
                                    ")"));
                aclInsert.bindValue(S(":id"), id);
                aclInsert.bindValue(S(":method"), it.key());
                exec(aclInsert);
            }
        }
    }
    //insert acl in case where methods are missing
    if (info.methods().isEmpty()) {
        foreach (QString token, info.accessControlList()) {
            QSqlQuery aclInsert = newQuery();
            aclInsert.prepare(S("INSERT OR REPLACE INTO ACL "
                                "(identity_id, token_id) "
                                "VALUES ( :id, "
                                "( SELECT id FROM TOKENS WHERE token = :token ))"));
            aclInsert.bindValue(S(":id"), id);
            aclInsert.bindValue(S(":token"), token);
            exec(aclInsert);
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

bool MetaDataDB::removeIdentity(const quint32 id)
{
    TRACE();

    QStringList queries = QStringList()
        << QString::fromLatin1(
            "DELETE FROM CREDENTIALS WHERE id = %1").arg(id)
        << QString::fromLatin1(
            "DELETE FROM ACL WHERE identity_id = %1").arg(id)
        << QString::fromLatin1(
            "DELETE FROM REALMS WHERE identity_id = %1").arg(id);

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
        << QLatin1String("DELETE FROM TOKENS");

    return transactionalExec(clearCommands);
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

    /* Security token insert */
    QSqlQuery tokenInsert = newQuery();
    tokenInsert.prepare(S("INSERT OR IGNORE INTO TOKENS (token) "
                          "VALUES ( :token )"));
    tokenInsert.bindValue(S(":token"), token);
    exec(tokenInsert);
    if (errorOccurred()) {
                allOk = false;
    }

    QSqlQuery refsInsert = newQuery();
    refsInsert.prepare(S("INSERT OR REPLACE INTO REFS "
                         "(identity_id, token_id, ref) "
                         "VALUES ( :id, "
                         "( SELECT id FROM TOKENS WHERE token = :token ),"
                         ":reference"
                         ")"));
    refsInsert.bindValue(S(":id"), id);
    refsInsert.bindValue(S(":token"), token);
    refsInsert.bindValue(S(":reference"), reference);
    exec(refsInsert);
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
    QSqlQuery refsDelete = newQuery();

    if (reference.isEmpty()) {
        refsDelete.prepare(S("DELETE FROM REFS "
                             "WHERE identity_id = :id AND "
                             "token_id = ( SELECT id FROM TOKENS WHERE token = :token )"));
        refsDelete.bindValue(S(":id"), id);
        refsDelete.bindValue(S(":token"), token);
    } else {
        refsDelete.prepare(S("DELETE FROM REFS "
                             "WHERE identity_id = :id AND "
                             "token_id = ( SELECT id FROM TOKENS WHERE token = :token ) "
                             "AND ref = :ref"));
        refsDelete.bindValue(S(":id"), id);
        refsDelete.bindValue(S(":token"), token);
        refsDelete.bindValue(S(":ref"), reference);
    }

    exec(refsDelete);
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
    QSqlQuery q = newQuery();
    q.prepare(S("SELECT ref FROM REFS "
                "WHERE identity_id = :id AND "
                "token_id = (SELECT id FROM TOKENS WHERE token = :token )"));
    q.bindValue(S(":id"), id);
    q.bindValue(S(":token"), token);
    return queryList(q);
}

bool MetaDataDB::insertMethods(QMap<QString, QStringList> methods)
{
    bool allOk = true;

    if (methods.isEmpty()) return false;
    //insert (unique) method names
    QMapIterator<QString, QStringList> it(methods);
    while (it.hasNext()) {
        it.next();
        QSqlQuery methodInsert = newQuery();
        methodInsert.prepare(S("INSERT OR IGNORE INTO METHODS (method) "
                               "VALUES( :method )"));
        methodInsert.bindValue(S(":method"), it.key());
        exec(methodInsert);
        if (errorOccurred()) allOk = false;
        //insert (unique) mechanism names
        foreach (QString mech, it.value()) {
            QSqlQuery mechInsert = newQuery();
            mechInsert.prepare(S("INSERT OR IGNORE INTO MECHANISMS (mechanism) "
                                 "VALUES( :mech )"));
            mechInsert.bindValue(S(":mech"), mech);
            exec(mechInsert);
            if (errorOccurred()) allOk = false;
        }
    }
    return allOk;
}

quint32 MetaDataDB::updateCredentials(const SignonIdentityInfo &info)
{
    quint32 id;
    QSqlQuery q = newQuery();

    int flags = 0;
    if (info.validated()) flags |= Validated;
    if (info.storePassword()) flags |= RememberPassword;
    if (info.isUserNameSecret()) flags |= UserNameIsSecret;

    if (!info.isNew()) {
        TRACE() << "UPDATE:" << info.id() ;
        q.prepare(S("UPDATE CREDENTIALS SET caption = :caption, "
                    "username = :username, "
                    "flags = :flags, "
                    "type = :type WHERE id = :id"));
        q.bindValue(S(":id"), info.id());
    } else {
        TRACE() << "INSERT:" << info.id();
        q.prepare(S("INSERT INTO CREDENTIALS "
                    "(caption, username, flags, type) "
                    "VALUES(:caption, :username, :flags, :type)"));
    }
    q.bindValue(S(":username"),
                info.isUserNameSecret() ? QString() : info.userName());
    q.bindValue(S(":caption"), info.caption());
    q.bindValue(S(":flags"), flags);
    q.bindValue(S(":type"), info.type());
    exec(q);
    if (errorOccurred()) {
        TRACE() << "Error occurred while updating crendentials";
        return 0;
    }

    if (info.isNew()) {
        /* Fetch id of the inserted credentials */
        QVariant idVariant = q.lastInsertId();
        if (!idVariant.isValid()) {
            TRACE() << "Error occurred while inserting crendentials";
            return 0;
        }
        id = idVariant.toUInt();
    } else {
        id = info.id() ;
    }

    return id;
}

bool MetaDataDB::updateRealms(quint32 id, const QStringList &realms, bool isNew)
{
    QString queryStr;

    if (!isNew) {
        //remove realms list
        queryStr = QString::fromLatin1(
            "DELETE FROM REALMS WHERE identity_id = '%1'")
            .arg(id);
        exec(queryStr);
    }

    /* Realms insert */
    QSqlQuery q = newQuery();
    q.prepare(S("INSERT OR IGNORE INTO REALMS (identity_id, realm) "
                "VALUES (:id, :realm)"));
    foreach (QString realm, realms) {
        q.bindValue(S(":id"), id);
        q.bindValue(S(":realm"), realm);
        exec(q);
        if (errorOccurred()) return false;
    }
    return true;
}

bool SecretsDB::createTables()
{
    QStringList createTableQuery = QStringList()
        <<  QString::fromLatin1(
            "CREATE TABLE CREDENTIALS"
            "(id INTEGER NOT NULL UNIQUE,"
            "username TEXT,"
            "password TEXT,"
            "PRIMARY KEY (id))")
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

bool SecretsDB::updateCredentials(const quint32 id,
                                  const SignonIdentityInfo &info)
{
    if (!startTransaction()) {
        TRACE() << "Could not start transaction. Error inserting credentials.";
        return false;
    }
    QSqlQuery query = newQuery();
    /* Credentials insert */
    QString password;
    if (info.storePassword())
        password = info.password();


    /* The identity might not be new and have no secret info stored at
     * the same time - e.g. if the secrets db has been deleted */
    bool hasSecretInfoStored = false;
    QString queryStr = QString::fromLatin1(
        "SELECT id FROM credentials WHERE id = %1").arg(info.id());

    QSqlQuery selectQuery = exec(queryStr);
    if (selectQuery.first())
        hasSecretInfoStored = true;

    if (!info.isNew() && hasSecretInfoStored) {
        TRACE() << "UPDATE:" << id;
        query.prepare(S("UPDATE CREDENTIALS SET username = :username, "
                        "password = :password "
                        "WHERE id = :id"));

     } else {
        TRACE() << "INSERT:" << id;
        query.prepare(S("INSERT INTO CREDENTIALS "
                        "(id, username, password) "
                        "VALUES(:id, :username, :password)"));
    }

    query.bindValue(S(":id"), id);
    query.bindValue(S(":username"), info.userName());
    query.bindValue(S(":password"), password);

    exec(query);

    if (errorOccurred()) {
        rollback();
        TRACE() << "Error occurred while storing crendentials";
        return false;
    }
    return commit();
}

bool SecretsDB::removeCredentials(const quint32 id)
{
    TRACE();

    QStringList queries = QStringList()
        << QString::fromLatin1(
            "DELETE FROM CREDENTIALS WHERE id = %1").arg(id)
        << QString::fromLatin1(
            "DELETE FROM STORE WHERE identity_id = %1").arg(id);

    return transactionalExec(queries);
}

bool SecretsDB::loadCredentials(SignonIdentityInfo &info)
{
    TRACE();

    QString queryStr =
        QString::fromLatin1("SELECT username, password FROM credentials "
                            "WHERE id = %1").arg(info.id());
    QSqlQuery query = exec(queryStr);
    if (!query.first()) {
        TRACE() << "No result or invalid credentials query.";
        return false;
    }

    QString username = query.value(0).toString();
    if (info.isUserNameSecret())
        info.setUserName(username);

    QString password = query.value(1).toString();
    info.setPassword(password);

    return true;
}

bool SecretsDB::checkPassword(const quint32 id,
                              const QString &username,
                              const QString &password)
{
    QSqlQuery query = newQuery();
    query.prepare(S("SELECT id FROM CREDENTIALS "
                    "WHERE id = :id AND username = :username AND password = :password"));
    query.bindValue(S(":id"), id);
    query.bindValue(S(":username"), username);
    query.bindValue(S(":password"), password);

    QSqlQuery result = exec(query);

    if (errorOccurred()) {
        TRACE() << "Error occurred while checking password";
        return false;
    }
    bool valid = false;
    valid = result.first();
    result.clear();

    return valid;
}

QVariantMap SecretsDB::loadData(quint32 id, quint32 method)
{
    TRACE();

    QSqlQuery q = newQuery();
    q.prepare(S("SELECT key, value "
                "FROM STORE WHERE identity_id = :id AND method_id = :method"));
    q.bindValue(S(":id"), id);
    q.bindValue(S(":method"), method);
    exec(q);
    if (errorOccurred())
        return QVariantMap();

    QVariantMap result;
    while (q.next()) {
        QByteArray array;
        array = q.value(1).toByteArray();
        QDataStream stream(array);
        QVariant data;
        stream >> data;
        result.insert(q.value(0).toString(), data);
    }
    return result;
}

bool SecretsDB::storeData(quint32 id, quint32 method, const QVariantMap &data)
{
    TRACE();

    if (!startTransaction()) {
        TRACE() << "Could not start transaction. Error inserting data.";
        return false;
    }

    bool allOk = true;
    qint32 dataCounter = 0;
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
            QSqlQuery query = newQuery();
            if (it.value().isValid() && !it.value().isNull()) {
                TRACE() << "insert";
                query.prepare(S(
                    "INSERT OR REPLACE INTO STORE "
                    "(identity_id, method_id, key, value) "
                    "VALUES(:id, :method, :key, :value)"));
                query.bindValue(S(":value"), array);
            } else {
                TRACE() << "remove";
                query.prepare(S(
                    "DELETE FROM STORE WHERE identity_id = :id "
                    "AND method_id = :method "
                    "AND key = :key"));

            }
            query.bindValue(S(":id"), id);
            query.bindValue(S(":method"), method);
            query.bindValue(S(":key"), it.key());
            exec(query);
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

bool SecretsDB::removeData(quint32 id, quint32 method)
{
    TRACE();

    if (!startTransaction()) {
        TRACE() << "Could not start transaction. Error removing data.";
        return false;
    }

    QSqlQuery q = newQuery();
    if (method == 0) {
        q.prepare(S("DELETE FROM STORE WHERE identity_id = :id"));
    } else {
        q.prepare(S("DELETE FROM STORE WHERE identity_id = :id "
                    "AND method_id = :method"));
        q.bindValue(S(":method"), method);
    }
    q.bindValue(S(":id"), id);
    exec(q);
    if (!errorOccurred() && commit()) {
        TRACE() << "Data removal ok.";
        return true;
    } else {
        rollback();
        TRACE() << "Data removal failed.";
        return false;
    }
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
    TRACE();
    if (secretsDB) {
        QString connectionName = secretsDB->connectionName();
        delete secretsDB;
        QSqlDatabase::removeDatabase(connectionName);
    }
    if (metaDataDB) {
        QString connectionName = metaDataDB->connectionName();
        delete metaDataDB;
        QSqlDatabase::removeDatabase(connectionName);
    }
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
    if (secretsDB != 0) {
        QString connectionName = secretsDB->connectionName();
        delete secretsDB;
        QSqlDatabase::removeDatabase(connectionName);
        secretsDB = 0;
    }
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
    RETURN_IF_NO_SECRETS_DB(false);
    return secretsDB->checkPassword(id, username, password);
}

SignonIdentityInfo CredentialsDB::credentials(const quint32 id, bool queryPassword)
{
    TRACE() << "id:" << id << "queryPassword:" << queryPassword;
    INIT_ERROR();
    SignonIdentityInfo info = metaDataDB->identity(id);
    if (queryPassword && !info.isNew() && isSecretsDBOpen()) {
        secretsDB->loadCredentials(info);
    }
    return info;
}

QList<SignonIdentityInfo> CredentialsDB::credentials(const QMap<QString, QString> &filter)
{
    INIT_ERROR();
    return metaDataDB->identities(filter);
}

quint32 CredentialsDB::insertCredentials(const SignonIdentityInfo &info, bool storeSecret)
{
    SignonIdentityInfo newInfo = info;
    if (!info.isNew())
        newInfo.setNew();
    return updateCredentials(newInfo, storeSecret);
}

quint32 CredentialsDB::updateCredentials(const SignonIdentityInfo &info,
                                         bool storeSecret)
{
    INIT_ERROR();
    quint32 id = metaDataDB->updateIdentity(info);
    if (id == 0) return id;

    if (storeSecret && isSecretsDBOpen()) {
        secretsDB->updateCredentials(id, info);
    }

    return id;
}

bool CredentialsDB::removeCredentials(const quint32 id)
{
    INIT_ERROR();

    /* We don't allow removing the credentials if the secrets DB is not
     * available */
    RETURN_IF_NO_SECRETS_DB(false);

    return secretsDB->removeCredentials(id) &&
        metaDataDB->removeIdentity(id);
}

bool CredentialsDB::clear()
{
    TRACE();

    INIT_ERROR();

    /* We don't allow clearing the DB if the secrets DB is not available */
    RETURN_IF_NO_SECRETS_DB(false);

    return secretsDB->clear() && metaDataDB->clear();
}

QVariantMap CredentialsDB::loadData(const quint32 id, const QString &method)
{
    TRACE() << "Loading:" << id << "," << method;

    INIT_ERROR();
    RETURN_IF_NO_SECRETS_DB(QVariantMap());
    if (id == 0) return QVariantMap();

    quint32 methodId = metaDataDB->methodId(method);
    if (methodId == 0) return QVariantMap();

    return secretsDB->loadData(id, methodId);
}

bool CredentialsDB::storeData(const quint32 id, const QString &method,
                              const QVariantMap &data)
{
    TRACE() << "Storing:" << id << "," << method;

    INIT_ERROR();
    RETURN_IF_NO_SECRETS_DB(false);
    if (id == 0) return false;

    quint32 methodId = metaDataDB->methodId(method);
    if (methodId == 0) return false;

    return secretsDB->storeData(id, methodId, data);
}

bool CredentialsDB::removeData(const quint32 id, const QString &method)
{
    TRACE() << "Removing:" << id << "," << method;

    INIT_ERROR();
    RETURN_IF_NO_SECRETS_DB(false);
    if (id == 0) return false;

    quint32 methodId;
    if (!method.isEmpty()) {
        methodId = metaDataDB->methodId(method);
        if (methodId == 0) return false;
    } else {
        methodId = 0;
    }

    return secretsDB->removeData(id, methodId);
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
