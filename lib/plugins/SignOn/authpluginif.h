/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 *
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
#ifndef AUTHPLUGINIF_H
#define AUTHPLUGINIF_H

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qplugin.h>

#include <QVariantMap>
#include <SignOn/sessiondata.h>
#include <SignOn/uisessiondata.h>
#include <SignOn/signonerror.h>

QT_BEGIN_NAMESPACE
class QString;
class QStringList;
class QByteArray;
class QVariant;
QT_END_NAMESPACE

/*!
 * Codes for errors that may be reported by authentication plugins.
 * @deprecated this enum is deprecated.
 * @see SignOn::Error::ErrorType
 */
enum AuthPluginError {
    PLUGIN_ERROR_NONE = 0,                  /**< No errors. */
    PLUGIN_ERROR_GENERAL = 1,               /**< Generic error during execution. */
    PLUGIN_ERROR_PERMISSION_DENIED = 4,     /**< Permission denied. Client access token was denied.*/
    PLUGIN_ERROR_MECHANISM_NOT_SUPPORTED,   /**< Given mechanism is not supported. */
    PLUGIN_ERROR_MISSING_DATA,              /**< SessionData did not contain required data for authentication. */
    PLUGIN_ERROR_NOT_AUTHORIZED,            /**< Authorization failed. */
    PLUGIN_ERROR_INVALID_STATE,             /**< Multistage plugin got called with wrong state. */
    PLUGIN_ERROR_OPERATION_FAILED,          /**< Temporary failure in authentication. */
    PLUGIN_ERROR_NO_CONNECTION,             /**< Network connection was not available. */
    PLUGIN_ERROR_NETWORK_ERROR,             /**< Network error. */
    PLUGIN_ERROR_SSL_ERROR,                 /**< SSL related error. */
    PLUGIN_ERROR_RUNTIME,                   /**< Casting SessionData into subclass failed. */
    PLUGIN_ERROR_USER_INTERACTION,          /**< Problems during user interaction */
    PLUGIN_ERROR_CANCELED,                  /**< User canceled ui operation */
    PLUGIN_ERROR_QUERY_LOGIN,               /**< @deprecated */
    PLUGIN_ERROR_QUERY_CAPTCHA,             /**< @deprecated */
    PLUGIN_ERROR_OPEN_URL,                  /**< @deprecated */
    PLUGIN_ERROR_LAST
};

/*!
 * Predefined states to be used for progress reporting.
 */
enum AuthPluginState {
    PLUGIN_STATE_NONE = 0,             /**< State unknown. */
    PLUGIN_STATE_RESOLVING,          /**< Resolving remote server host name. */
    PLUGIN_STATE_CONNECTING,      /**< Connecting to remote server. */
    PLUGIN_STATE_SENDING,             /**< Sending data to remote server. */
    PLUGIN_STATE_WAITING,            /**< Waiting for reply from remote server. */
    PLUGIN_STATE_PENDING,             /**< Waiting for response from user. */
    PLUGIN_STATE_REFRESHING,       /**< Refreshing ui request. */
    PLUGIN_STATE_CANCELING,         /**< Canceling current process. */
    PLUGIN_STATE_HOLDING,            /**< Holding long non-expired token. Process should be kept alive. */
    PLUGIN_STATE_DONE                    /**< Process is finished. Process can be terminated. */
};

/*!
 * Macro to create declarations of
 * SSO authentication plugin.
 * */
#define SIGNON_PLUGIN_INSTANCE(pluginclass) \
        { \
            static AuthPluginInterface *_instance = 0; \
            if (!_instance)      \
                _instance = static_cast<AuthPluginInterface *>(new pluginclass()); \
            return _instance; \
        }

#define SIGNON_DECL_AUTH_PLUGIN(pluginclass) \
        Q_EXTERN_C AuthPluginInterface *auth_plugin_instance() \
        SIGNON_PLUGIN_INSTANCE(pluginclass)

/*!
 * @class AuthPluginInterface.
 * Interface definition for authentication plugins.
 */
class AuthPluginInterface : public QObject
{
    Q_OBJECT

public:
    AuthPluginInterface(QObject *parent = 0) : QObject(parent)
        { qRegisterMetaType<SignOn::Error>("SignOn::Error"); }

    /*!
     * Destructor.
     */
    virtual ~AuthPluginInterface() {}

    /*!
     * Get type of plugin.
     *
     * @return plugin type.
     */
    virtual QString type() const = 0;

    /*!
     * Get list of supported mechanisms.
     *
     * @return list of mechanisms.
     */
    virtual QStringList mechanisms() const = 0;

    /*!
     * Request to cancel process.
     * Process is terminated after this call.
     * Reimplement this in order to execute specific instructions before
     * the effective cancel occurres.
     */
    virtual void cancel() {}

    /*!
     * Request to abort process.
     * Process is terminated after this call.
     * Reimplement this in order to execute specific instructions before
     * process is killed.
     */
    virtual void abort() {}

    /*!
     * Process authentication.
     * Authentication can be logging to a server, obtain token(s) from a server,
     * calculate response using given challenge, etc.
     * Given session data is used to do authentication and return response.
     * Signal result() is emitted when authentication is completed,
     * or signal error() if authentication failed.
     * @see result
     * @see error
     *
     * @param inData input data for authentication.
     * @param mechanism mechanism to use to do authentication.
     */
    virtual void process(const SignOn::SessionData &inData,
                         const QString &mechanism = QString()) = 0;

Q_SIGNALS:
    /*!
     * Emitted when authentication process has been completed for given data
     * and there are no errors.
     *
     * @param data resulting SessionData, need to be returned to client.
     */
    void result(const SignOn::SessionData &data);

    /*!
     * Emitted when authentication process has been completed for given data
     * and resulting an error.
     *
     * @param error resulting error code.
     * @param errorMessage resulting error message.
     * @deprecated This is deprecated, use error(const Error &err) instead.
     * @see SignOn::Error
     * @warning Do no use both this deprecated and the error(const Error &err) signal.
     *          After switching to error(const Error &err), remove this deprecated
     *          signal emition.
     */
    void error(const AuthPluginError error, const QString &errorMessage = QString());

    /*!
     * Emitted when authentication process has been completed for given data
     * and resulting an error.
     *
     * @param err The error object.
     * @param errorMessage resulting error message.
     */
    void error(const SignOn::Error &err);

    /*!
     * Emitted when authentication process need to interact with user.
     * Basic use cases are: query password, verify captcha, show url.
     * Can also be used to get username/password for proxy authentication etc.
     * Slot userActionFinished() is called when interaction is completed.
     *
     * @see userActionFinished
     * @see SignOn::UiSessionData
     * @note slot userActionFinished() should be reimplemented to get result.
     *
     * @param data ui session data to be filled within user interaction.
     */
    void userActionRequired(const SignOn::UiSessionData &data);

    /*!
     * Emitted when authentication process has completed refresh request.
     * Plugin must emit signal refreshed() to response to refresh() call.
     * @see refreshed
     *
     * @param data refreshed ui session data.
     */
    void refreshed(const SignOn::UiSessionData &data);

     /*!
     * Emitted to report status of authentication process to signond for
     * informing client application.
     *
     * @param state plugin process state @see AuthPluginState.
     * @param message optional message for client application.
     */
    void statusChanged(const AuthPluginState state,
                       const QString &message = QString());

public Q_SLOTS:
    /*!
     * User interaction completed.
     * Signond uses this slot to notice end of ui session.
     * This is response to userActionRequired() signal.
     * This must be reimplemented to get response from user interaction.
     * @see UiSessionData
     * @see userActionRequired
     *
     * @param data user completed ui session data.
     */
    virtual void userActionFinished(const SignOn::UiSessionData &data) {
        Q_UNUSED(data);
    }

    /*!
     * Refresh given session.
     * Signond uses this slot to refresh data in given ui session.
     * Mostly used to refresh captcha images during user interaction.
     * Signal refreshed() or error() must be emitted when refresh is completed.
     * This must be reimplemented to refresh captcha image.
     * @see UiSessionData
     * @see refreshed
     * @note emitting signal userActionRequired() is not allowed to use before ui session is finished.
     *
     * @param data ui session data to be refreshed.
     */
    virtual void refresh(const SignOn::UiSessionData &data) {
        emit refreshed(data);
    }

};

QT_BEGIN_NAMESPACE
 Q_DECLARE_INTERFACE(AuthPluginInterface,
                     "com.nokia.Signon.PluginInterface/1.2")
QT_END_NAMESPACE
Q_DECLARE_METATYPE(AuthPluginError)
#endif // AUTHPLUGINIF_H
