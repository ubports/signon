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

extern "C" {
    #include <sys/socket.h>
}

#include <QtDebug>
#include <QDir>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QPluginLoader>
#include <QProcessEnvironment>
#include <QSocketNotifier>

#include <SignOn/AbstractKeyManager>
#include <SignOn/ExtensionInterface>

#include <sim-dlc.h>

#include "signondaemon.h"
#include "signond-common.h"
#include "signondaemonadaptor.h"
#include "signonidentity.h"
#include "signonauthsession.h"
#include "backupifadaptor.h"

#define SIGNON_RETURN_IF_CAM_UNAVAILABLE(_ret_arg_) do {                   \
        if (m_pCAMManager && !m_pCAMManager->credentialsSystemOpened()) {  \
            QDBusMessage errReply = message().createErrorReply(            \
                    internalServerErrName,                                 \
                    internalServerErrStr + QLatin1String("Could not access Signon Database.")); \
            SIGNOND_BUS.send(errReply); \
            return _ret_arg_;           \
        }                               \
    } while(0)

using namespace SignOn;

namespace SignonDaemonNS {

QScopedPointer<RequestCounter> requestCounter;

/* ---------------------- RequestCounter ---------------------- */
RequestCounter *RequestCounter::instance()
{
    if (requestCounter.isNull())
    {
        QScopedPointer<RequestCounter> tmp(new RequestCounter());
        requestCounter.swap(tmp);
    }

    return requestCounter.data();
}

void RequestCounter::addServiceRequest()
{
    ++m_serviceRequests;
}

void RequestCounter::addIdentityRequest()
{
    ++m_identityRequests;
}

int RequestCounter::serviceRequests() const
{
    return m_serviceRequests;
}

int RequestCounter::identityRequests() const
{
    return m_identityRequests;
}

/* ---------------------- SignonDaemonConfiguration ---------------------- */

SignonDaemonConfiguration::SignonDaemonConfiguration()
    : m_loadedFromFile(false),
      m_useSecureStorage(signonDefaultUseEncryption),
      m_storageSize(signonMinumumDbSize),
      m_storageFileSystemType(QLatin1String(signonDefaultFileSystemType)),
      m_storagePath(QLatin1String(signonDefaultStoragePath)),
      m_storageFileSystemName(QLatin1String(signonDefaultFileSystemName)),
      m_identityTimeout(300),//secs
      m_authSessionTimeout(300)//secs
{}

SignonDaemonConfiguration::~SignonDaemonConfiguration()
{
    TRACE();
}

/*
    --- Configuration file template ---

    [General]
    UseSecureStorage=yes
    StoragePath=~/.signon/

    [SecureStorage]
    FileSystemName=signonfs
    Size=8
    FileSystemType=ext2

    [ObjectTimeouts]
    IdentityTimeout=300
    AuthSessionTimeout=300
 */

void SignonDaemonConfiguration::load()
{
    //Daemon configuration file

    if (QFile::exists(QLatin1String("/etc/signond.conf"))) {
        m_loadedFromFile = true;

        QSettings settings(QLatin1String("/etc/signond.conf"),
                           QSettings::NativeFormat);
        m_storagePath =
            QDir(settings.value(QLatin1String("StoragePath")).toString()).path();

        if (m_storagePath.startsWith(QLatin1Char('~')))
            m_storagePath.replace(0, 1, QDir::homePath());

        //Secure storage
        QString useSecureStorage =
            settings.value(QLatin1String("UseSecureStorage")).toString();

        if (!useSecureStorage.isEmpty())
            m_useSecureStorage =
                (useSecureStorage == QLatin1String("yes")
                || useSecureStorage == QLatin1String("true"));

        if (m_useSecureStorage) {
            settings.beginGroup(QLatin1String("SecureStorage"));

            bool isOk = false;
            m_storageSize = settings.value(QLatin1String("Size")).toUInt(&isOk);
            if (!isOk || m_storageSize < signonMinumumDbSize) {
                m_storageSize = signonMinumumDbSize;
                TRACE() << "Less than minimum possible storage size configured."
                        << "Setting to the minimum of:" << signonMinumumDbSize << "Mb";
            }

            m_storageFileSystemType = settings.value(
                QLatin1String("FileSystemType")).toString();

            m_storageFileSystemName = settings.value(QLatin1String("FileSystemName")).toString();

            settings.endGroup();
        }

        //Timeouts
        settings.beginGroup(QLatin1String("ObjectTimeouts"));

        bool isOk = false;
        uint aux = settings.value(QLatin1String("Identity")).toUInt(&isOk);
        if (isOk)
            m_identityTimeout = aux;

        aux = settings.value(QLatin1String("AuthSession")).toUInt(&isOk);
        if (isOk)
            m_authSessionTimeout = aux;

        settings.endGroup();
    } else {
        TRACE() << "/etc/signond.conf not found. Using default daemon configuration.";
    }

    //Environment variables

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    int value = 0;
    bool isOk = false;
    if (environment.contains(QLatin1String("SSO_IDENTITY_TIMEOUT"))) {
        value = environment.value(
            QLatin1String("SSO_IDENTITY_TIMEOUT")).toInt(&isOk);

        m_identityTimeout = (value > 0) && isOk ? value : m_identityTimeout;
    }

    if (environment.contains(QLatin1String("SSO_AUTHSESSION_TIMEOUT"))) {
        value = environment.value(
            QLatin1String("SSO_AUTHSESSION_TIMEOUT")).toInt(&isOk);
        m_authSessionTimeout = (value > 0) && isOk ? value : m_authSessionTimeout;
    }
}

/* ---------------------- SignonDaemon ---------------------- */

const QString internalServerErrName = SIGNOND_INTERNAL_SERVER_ERR_NAME;
const QString internalServerErrStr = SIGNOND_INTERNAL_SERVER_ERR_STR;

static int sigFd[2];

SignonDaemon *SignonDaemon::m_instance = NULL;

SignonDaemon::SignonDaemon(QObject *parent) : QObject(parent)
                                            , m_configuration(NULL)
{}

SignonDaemon::~SignonDaemon()
{
    TRACE() << "started";
    ::close(sigFd[0]);
    ::close(sigFd[1]);

    if (m_backup) {
        TRACE() << "completed";
        exit(0);
    }

    SignonAuthSession::stopAllAuthSessions();
    m_storedIdentities.clear();
    m_unstoredIdentities.clear();

    if (m_pCAMManager) {
        m_pCAMManager->closeCredentialsSystem();
        delete m_pCAMManager;
    }

    QDBusConnection sessionConnection = QDBusConnection::sessionBus();

    sessionConnection.unregisterObject(SIGNOND_DAEMON_OBJECTPATH
                                       + QLatin1String("/Backup"));
    sessionConnection.unregisterService(SIGNOND_SERVICE
                                        + QLatin1String(".Backup"));
    if (m_backup == false)
    {
        sessionConnection.unregisterObject(SIGNOND_DAEMON_OBJECTPATH);
        sessionConnection.unregisterService(SIGNOND_SERVICE);
    }

    delete m_configuration;
    TRACE() << "completed";
}

void SignonDaemon::setupSignalHandlers()
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigFd) != 0)
        BLAME() << "Couldn't create HUP socketpair";

    m_sigSn = new QSocketNotifier(sigFd[1], QSocketNotifier::Read, this);
    connect(m_sigSn, SIGNAL(activated(int)),
            this, SLOT(handleUnixSignal()));
}

void SignonDaemon::signalHandler(int signal)
{
    TRACE() << "signal: " << signal;
    ::write(sigFd[0], &signal, sizeof(signal));
}

void SignonDaemon::handleUnixSignal()
{
    m_sigSn->setEnabled(false);

    int signal;
    ::read(sigFd[1], &signal, sizeof(signal));

    TRACE() << "signal received: " << signal;

    switch (signal) {
        case SIGHUP: {
            TRACE() << "\n\n SIGHUP \n\n";
            //todo restart daemon
            deleteLater();

            // reset the m_instance
            m_instance = NULL;
            QMetaObject::invokeMethod(instance(),
                                      "init",
                                      Qt::QueuedConnection);
            break;
        }
        case SIGTERM: {
            TRACE() << "\n\n SIGTERM \n\n";
            //gently stop daemon
            deleteLater();
            QMetaObject::invokeMethod(QCoreApplication::instance(),
                                      "quit",
                                      Qt::QueuedConnection);
            break;
        }
        case SIGINT:  {
            TRACE() << "\n\n SIGINT \n\n";
            //gently stop daemon
            deleteLater();
            QMetaObject::invokeMethod(QCoreApplication::instance(),
                                      "quit",
                                      Qt::QueuedConnection);
            break;
        }
        default: break;
    }

    m_sigSn->setEnabled(true);
}

SignonDaemon *SignonDaemon::instance()
{
    TRACE();
    if (m_instance != NULL)
        return m_instance;

    QCoreApplication *app = QCoreApplication::instance();

    if (!app)
        qFatal("SignonDaemon requires a QCoreApplication instance to be constructed first");

    TRACE() << "New instance must be created";
    m_instance = new SignonDaemon(app);
    return m_instance;
}

void SignonDaemon::init()
{
    TRACE();
    QCoreApplication *app = QCoreApplication::instance();
    TRACE();
    if (!app)
        qFatal("SignonDaemon requires a QCoreApplication instance to be constructed first");

    setupSignalHandlers();
    m_backup = app->arguments().contains(QLatin1String("-backup"));
    m_pCAMManager = CredentialsAccessManager::instance();

    TRACE();
    if (!(m_configuration = new SignonDaemonConfiguration))
        qWarning("SignonDaemon could not create the configuration object.") ;

    m_configuration->load();

    /* backup dbus interface */
    QDBusConnection sessionConnection = QDBusConnection::sessionBus();

    if (!sessionConnection.isConnected()) {
        QDBusError err = sessionConnection.lastError();
        TRACE() << "Session connection cannot be established:" << err.errorString(err.type());
        TRACE() << err.message();

        qFatal("SignonDaemon requires session bus to start working");
    }

    QDBusConnection::RegisterOptions registerSessionOptions = QDBusConnection::ExportAdaptors;

    (void)new BackupIfAdaptor(this);

    if (!sessionConnection.registerObject(SIGNOND_DAEMON_OBJECTPATH
                                          + QLatin1String("/Backup"), this, registerSessionOptions)) {
        TRACE() << "Object cannot be registered";

        qFatal("SignonDaemon requires to register backup object");
    }

    if (!sessionConnection.registerService(SIGNOND_SERVICE+QLatin1String(".Backup"))) {
        QDBusError err = sessionConnection.lastError();
        TRACE() << "Service cannot be registered: " << err.errorString(err.type());

        qFatal("SignonDaemon requires to register backup service");
    }

    if (m_backup) {
        TRACE() << "Signond initialized in backup mode.";
        //skit rest of initialization in backup mode
        return;
    }

    /* DBus Service init */
    QDBusConnection connection = SIGNOND_BUS;

    if (!connection.isConnected()) {
        QDBusError err = connection.lastError();
        TRACE() << "Connection cannot be established:" << err.errorString(err.type());
        TRACE() << err.message();

        qFatal("SignonDaemon requires DBus to start working");
    }

    QDBusConnection::RegisterOptions registerOptions = QDBusConnection::ExportAllContents;

    (void)new SignonDaemonAdaptor(this);
    registerOptions = QDBusConnection::ExportAdaptors;

    if (!connection.registerObject(SIGNOND_DAEMON_OBJECTPATH, this, registerOptions)) {
        TRACE() << "Object cannot be registered";

        qFatal("SignonDaemon requires to register daemon's object");
    }

    if (!connection.registerService(SIGNOND_SERVICE)) {
        QDBusError err = connection.lastError();
        TRACE() << "Service cannot be registered: " << err.errorString(err.type());

        qFatal("SignonDaemon requires to register daemon's service");
    }

    initExtensions();

    if (!initSecureStorage(QByteArray()))
        qFatal("Signond: Cannot initialize credentials secure storage.");

    // TODO - remove this
    QTimer *requestCounterTimer = new QTimer(this);
    requestCounterTimer->setInterval(500000);
    connect(requestCounterTimer,
            SIGNAL(timeout()),
            this,
            SLOT(displayRequestsCount()));
    requestCounterTimer->start();

    TRACE() << "Signond SUCCESSFULLY initialized.";
    //TODO - end
}

void SignonDaemon::initExtensions()
{
    /* Scan the directory containing signond extensions and attempt loading
     * all of them.
     */
    QDir dir(QString::fromLatin1(SIGNON_EXTENSIONS_DIR));
    QStringList filters(QLatin1String("lib*.so"));
    QStringList extensionList = dir.entryList(filters, QDir::Files);
    foreach(QString filename, extensionList)
        initExtension(dir.filePath(filename));
}

void SignonDaemon::initExtension(const QString &filePath)
{
    TRACE() << "Loading plugin " << filePath;

    QPluginLoader pluginLoader(filePath);
    QObject *plugin = pluginLoader.instance();
    if (plugin == 0) {
        qWarning() << "Couldn't load plugin:" << pluginLoader.errorString();
        return;
    }

    ExtensionInterface *extension = qobject_cast<ExtensionInterface *>(plugin);
    if (extension == 0) {
        qWarning() << "Plugin instance is not an ExtensionInterface";
        return;
    }

    /* Check whether the extension implements some useful objects; if not,
     * unload it. */
    bool extensionInUse = false;
    AbstractKeyManager *keyManager = extension->keyManager(this);
    if (keyManager) {
        m_pCAMManager->addKeyManager(keyManager);
        extensionInUse = true;
    }

    if (!extensionInUse) {
        pluginLoader.unload();
    }
}

bool SignonDaemon::initSecureStorage(const QByteArray &lockCode)
{
    if (!m_pCAMManager->credentialsSystemOpened()) {
        m_pCAMManager->finalize();

        CAMConfiguration config;

        //Use the Signon configuration file if it exists
        if (m_configuration->loadedFromFile()) {
            config.m_useEncryption = m_configuration->useSecureStorage();
            config.m_fileSystemSize = m_configuration->storageSize();
            config.m_fileSystemType = m_configuration->storageFileSystemType();
            config.m_dbFileSystemPath = m_configuration->storagePath()
                                        + QDir::separator()
                                        + m_configuration->storageFileSystemName();
        }

        config.m_encryptionPassphrase = lockCode;

        if (!m_pCAMManager->init(config)) {
            qCritical("Signond: Cannot set proper configuration of CAM");
            return false;
        }

        // If encryption is in use this will just open the metadata DB
        if (!m_pCAMManager->openCredentialsSystem()) {
            qCritical("Signond: Cannot open CAM credentials system...");
            return false;
        }
    } else {
        TRACE() << "Secure storage already initialized...";
        return false;
    }

    return true;
}

void SignonDaemon::displayRequestsCount()
{
    SignonDisposable::destroyUnused();

    TRACE() << "\n\n\nUnstored identities:" << m_unstoredIdentities.count()
            << "\nStored identities:" << m_storedIdentities.count()
            << "\nService requests:" << RequestCounter::instance()->serviceRequests()
            << "\nIdentity requests:" << RequestCounter::instance()->identityRequests()
            << "\n\n";
}

void SignonDaemon::unregisterIdentity(SignonIdentity *identity)
{
    if (m_storedIdentities.contains(identity->id()))
        m_storedIdentities.remove(identity->id());
    else
        m_unstoredIdentities.remove(identity->objectName());

    identity->deleteLater();
}

void SignonDaemon::identityStored(SignonIdentity *identity)
{
    if (m_unstoredIdentities.contains(identity->objectName())) {
        m_unstoredIdentities.remove(identity->objectName());
        m_storedIdentities.insert(identity->id(), identity);
    }
}

void SignonDaemon::registerNewIdentity(QDBusObjectPath &objectPath)
{
    RequestCounter::instance()->addServiceRequest();

    TRACE() << "Registering new identity:";

    SignonIdentity *identity = SignonIdentity::createIdentity(SIGNOND_NEW_IDENTITY, this);

    if (identity == NULL) {
        QDBusMessage errReply = message().createErrorReply(
                internalServerErrName,
                internalServerErrStr + QLatin1String("Could not create remote Identity object."));
        SIGNOND_BUS.send(errReply);
        return;
    }

    m_unstoredIdentities.insert(identity->objectName(), identity);

    objectPath = QDBusObjectPath(identity->objectName());
}

int SignonDaemon::identityTimeout() const
{
    return (m_configuration == NULL ?
                                     300 :
                                     m_configuration->identityTimeout());
}

int SignonDaemon::authSessionTimeout() const
{
    return (m_configuration == NULL ?
                                     300 :
                                     m_configuration->authSessionTimeout());
}

void SignonDaemon::registerStoredIdentity(const quint32 id, QDBusObjectPath &objectPath, QList<QVariant> &identityData)
{
    RequestCounter::instance()->addServiceRequest();

    SIGNON_RETURN_IF_CAM_UNAVAILABLE();

    TRACE() << "Registering identity:" << id;

    //1st check if the existing identity is in cache
    SignonIdentity *identity = m_storedIdentities.value(id, NULL);

    //if not create it
    if (identity == NULL)
        identity = SignonIdentity::createIdentity(id, this);

    if (identity == NULL)
    {
        QDBusMessage errReply = message().createErrorReply(
                internalServerErrName,
                internalServerErrStr + QLatin1String("Could not create remote Identity object."));
        SIGNOND_BUS.send(errReply);
        return;
    }

    bool ok;
    SignonIdentityInfo info = identity->queryInfo(ok);

    if (info.m_id == 0)
    {
        QDBusMessage errReply = message().createErrorReply(
                                                        SIGNOND_IDENTITY_NOT_FOUND_ERR_NAME,
                                                        SIGNOND_IDENTITY_NOT_FOUND_ERR_STR);
        SIGNOND_BUS.send(errReply);
        objectPath = QDBusObjectPath();
        return;
    }

    //cache the identity as stored
    m_storedIdentities.insert(identity->id(), identity);
    identity->keepInUse();

    identityData = info.toVariantList();

    TRACE() << "DONE REGISTERING IDENTITY";
    objectPath = QDBusObjectPath(identity->objectName());
}

QStringList SignonDaemon::queryMethods()
{
    RequestCounter::instance()->addServiceRequest();

    QDir pluginsDir(SIGNOND_PLUGINS_DIR);
    //TODO: in the future remove the sym links comment
    QStringList fileNames = pluginsDir.entryList(
            QStringList() << QLatin1String("*.so*"), QDir::Files | QDir::NoDotAndDotDot);

    QStringList ret;
    QString fileName;
    foreach (fileName, fileNames) {
        if (fileName.startsWith(QLatin1String("lib"))) {
            fileName = fileName.mid(3, fileName.indexOf(QLatin1String("plugin")) -3);
            if ((fileName.length() > 0) && !ret.contains(fileName))
                ret << fileName;
        }
    }

    return ret;
}

QStringList SignonDaemon::queryMechanisms(const QString &method)
{
    RequestCounter::instance()->addServiceRequest();
    TRACE() << "\n\n\n Querying mechanisms\n\n";

    QStringList mechs = SignonSessionCore::loadedPluginMethods(method);

    if (mechs.size())
        return mechs;

    PluginProxy *plugin = PluginProxy::createNewPluginProxy(method);

    if (!plugin) {
        TRACE() << "Could not load plugin of type: " << method;
        QDBusMessage errReply = message().createErrorReply(
                SIGNOND_METHOD_NOT_KNOWN_ERR_NAME,
                QString(SIGNOND_METHOD_NOT_KNOWN_ERR_STR
                        + QLatin1String("Method %1 is not known or could not load specific configuration.")).arg(method));
        SIGNOND_BUS.send(errReply);
        return QStringList();
    }

    mechs = plugin->mechanisms();
    delete plugin;

    return mechs;
}


QList<QVariant> SignonDaemon::queryIdentities(const QMap<QString, QVariant> &filter)
{
    RequestCounter::instance()->addServiceRequest();

    SIGNON_RETURN_IF_CAM_UNAVAILABLE(QList<QVariant>());

    TRACE() << "\n\n\n Querying identities\n\n";

    CredentialsDB *db = m_pCAMManager->credentialsDB();
    if (!db) {
        qCritical() << Q_FUNC_INFO << m_pCAMManager->lastError();
        return QList<QVariant>();
    }

    QMap<QString, QString> filterLocal;
    QMapIterator<QString, QVariant> it(filter);
    while (it.hasNext()) {
        it.next();
        filterLocal.insert(it.key(), it.value().toString());
    }

    QList<SignonIdentityInfo> credentials = db->credentials(filterLocal);

    if (db->errorOccurred()) {
        QDBusMessage errReply = message().createErrorReply(
                internalServerErrName,
                internalServerErrStr + QLatin1String("Querying database error occurred."));
        SIGNOND_BUS.send(errReply);
        return QList<QVariant>();
    }

    return SignonIdentityInfo::listToVariantList(credentials);
}

bool SignonDaemon::clear()
{
    RequestCounter::instance()->addServiceRequest();

    SIGNON_RETURN_IF_CAM_UNAVAILABLE(false);

    TRACE() << "\n\n\n Clearing DB\n\n";
    CredentialsDB *db = m_pCAMManager->credentialsDB();
    if (!db) {
        qCritical() << Q_FUNC_INFO << m_pCAMManager->lastError();
        return false;
    }

    if (!db->clear()) {
        QDBusMessage errReply = message().createErrorReply(
                                                SIGNOND_INTERNAL_SERVER_ERR_NAME,
                                                QString(SIGNOND_INTERNAL_SERVER_ERR_STR
                                                        + QLatin1String("Database error occurred.")));
        SIGNOND_BUS.send(errReply);
        return false;
    }
    return true;
}

QString SignonDaemon::getAuthSessionObjectPath(const quint32 id, const QString type)
{
    return SignonAuthSession::getAuthSessionObjectPath(id, type, this);
}

bool SignonDaemon::setDeviceLockCode(const QByteArray &lockCode,
                                     const QByteArray &oldLockCode)
{
    /* TODO: remove this: it is just a temporary solution. The whole method
     * should disappear once the harmattan System-ui starts using the new
     * interface in the sim-dlc library.
     */
    TRACE() << "Forwarding to new interface. New:" << lockCode <<
        "Old: " << oldLockCode;

    QDBusConnection connection = QDBusConnection::sessionBus();

    QDBusMessage msg =
        QDBusMessage::createMethodCall(SIMDLC_SERVICE_S,
                                       SIMDLC_PATH_S,
                                       SIMDLC_INTERFACE_S,
                                       QLatin1String("setDeviceLockCode"));
    QList<QVariant> args;
    args << lockCode;
    args << oldLockCode;
    msg.setArguments(args);
    // Since it's just a hack, we don't care about the result
    connection.asyncCall(msg);
    return true;
}

bool SignonDaemon::remoteLock(const QByteArray &lockCode)
{
    Q_UNUSED(lockCode)
    // TODO - implement this, research how to.
    TRACE() << "remoteDrop:   lockCode = " << lockCode;
    return false;
}

/*
 * backup/restore
 * TODO move fixed strings into config
 */

static QString &backupCopyFilename()
{
    static QString name(QLatin1String("/home/user/.signon/signondb.bin"));
    return name;
}

uchar SignonDaemon::backupStarts()
{
    TRACE() << "backup";
    if (!m_backup && m_pCAMManager->credentialsSystemOpened())
    {
        m_pCAMManager->closeCredentialsSystem();
        if (m_pCAMManager->credentialsSystemOpened())
        {
            qCritical() << "Cannot close credentials database";
            return 2;
        }
    }

    //do backup copy
    CAMConfiguration config;
    QString source;
    if (config.m_useEncryption)
        source = config.m_dbFileSystemPath;
    else
        source = QDir::homePath() + QDir::separator() + QLatin1String(".signon")
             + QDir::separator() + config.m_dbName;

    QDir target;
    if (!target.mkpath(QLatin1String("/home/user/.signon/")))
    {
        qCritical() << "Cannot create target directory";
        m_pCAMManager->openCredentialsSystem();
        return 2;
    }

    if (!QFile::copy(source, backupCopyFilename()))
    {
        qCritical() << "Cannot copy database";
        m_pCAMManager->openCredentialsSystem();
        return 2;
    }

    if (!m_backup)
    {
        //mount file system back
        if (!m_pCAMManager->openCredentialsSystem()) {
            qCritical() << "Cannot reopen database";
            return 2;
        }
    }
    return 0;
}

uchar SignonDaemon::backupFinished()
{
    TRACE() << "close";

    QFile copy(backupCopyFilename());

    if (copy.exists())
        QFile::remove(backupCopyFilename());

    if (m_backup)
    {
        //close daemon
        TRACE() << "close daemon";
        this->deleteLater();
    }

    return 0;
 }

/*
 * Does nothing but start-on-demand
 * */
uchar SignonDaemon::restoreStarts()
{
    TRACE();
    return 0;
}

uchar SignonDaemon::restoreFinished()
{
    TRACE() << "restore";
    //restore requested
    if (m_pCAMManager->credentialsSystemOpened())
    {
        //umount file system
        if (!m_pCAMManager->closeCredentialsSystem())
        {
            qCritical() << "database cannot be closed";
            return 2;
        }
    }

    //do restore
    //TODO add checking if encryption status has changed
    CAMConfiguration config;
    QString target;
    if (config.m_useEncryption)
        target = config.m_dbFileSystemPath;
    else
        target = QDir::homePath() + QDir::separator()
                 + QLatin1String(".signon")+ QDir::separator() + config.m_dbName;

    if (!QFile::remove(target+QLatin1String(".bak")))
    {
        qCritical() << "Cannot remove backup copy of database";
    }

    if (!QFile::rename(target, target+QLatin1String(".bak")))
    {
        qCritical() << "Cannot make backup copy of database";
    }

    if (!QFile::rename(backupCopyFilename(), target))
    {
        qCritical() << "Cannot copy database";

        if (!QFile::rename(target+QLatin1String(".bak"), target))
            qCritical() << "Cannot restore backup copy of database";

        m_pCAMManager->openCredentialsSystem();
        return 2;
    }

    //try to remove backup database
    if (!QFile::remove(target+QLatin1String(".bak")))
        qCritical() << "Cannot remove backup copy of database";

    //TODO check database integrity
    if (!m_backup)
    {
        //mount file system back
         if (!m_pCAMManager->openCredentialsSystem())
             return 2;
    }

    return 0;
}

void SignonDaemon::listDBusInterfaces()
{
    QDBusReply<QStringList> reply = SIGNOND_BUS.interface()->registeredServiceNames();
    QStringList list = reply.value();

    QString servicesList = QLatin1String("DBUS registered services: \n");
    servicesList += list.join(QLatin1String("\n"));

    TRACE() << "\n\n" << servicesList.toAscii().data() << "\n";
}

} //namespace SignonDaemonNS
