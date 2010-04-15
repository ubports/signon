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
#ifndef SIGNONCOMMON_H_
#define SIGNONCOMMON_H_

#ifdef TRACE
    #undef TRACE
#endif

#include <QDebug>

#define SIGNON_TRACE_FILE QLatin1String("signon_trace_file")
#define SIGNON_TRACE_FILE_MAX_SIZE 256000 // 250 * 1024 bytes

//#ifdef SIGNON_TRACE
//    #undef SIGNON_TRACE
//#endif

#ifndef SIGNON_TRACE
    #define SIGNON_TRACE
#endif


#ifdef SIGNON_TRACE
    #define TRACE() qDebug() << __FILE__ << __LINE__ << __func__ << ":\t"
    #define BLAME() qCritical() << __FILE__ << __LINE__ << __func__ << ":\t"
#else
    #define TRACE() if(1) ; else qDebug()
    #define BLAME() if(1) ; else qDebug()
#endif

#if __GNUC__ >= 4
    #define SIGNON_EXPORT __attribute__ ((visibility("default")))
#endif

#ifndef SIGNON_EXPORT
    #define SIGNON_EXPORT
#endif

#define SSO_KEY_TOKEN            "sso_token"
#define SSO_KEY_OPERATION        "sso_operation_code"
#define SSO_KEY_USERNAME         "username"
#define SSO_KEY_PASSWORD         "password"
#define SSO_KEY_CHALLENGE        "challenge"
#define SSO_KEY_STORECREDENTIALS "storeCredentials"

#define SIGNON_UI_SERVICE           QLatin1String("com.nokia.singlesignonui")
#define SIGNON_UI_DAEMON_OBJECTPATH QLatin1String("/SignonUi")

#define SIGNON_SERVICE            QLatin1String("com.nokia.singlesignon")
#define SIGNON_DAEMON_OBJECTPATH  QLatin1String("/SignonDaemon")
#define SIGNON_DAEMON_INTERFACE   QLatin1String("com.nokia.singlesignon.SignonDaemon")
#define SIGNON_BUS                QDBusConnection::sessionBus()
#define SSO_ERR_PREFIX QString(SIGNON_SERVICE) + QLatin1String(".Error.")

#define SSO_NEW_IDENTITY 0

#define SIGNON_MAX_TIMEOUT 0x7FFFFFFF

/*
 * todo: the naming convention for interfaces should be clarified
 * */

/*
 * SignonDaemon
 * */

#define SSO_DAEMON_INTERNAL_SERVER_ERR_STR   QLatin1String("Internal server error occurred.")
#define SSO_DAEMON_INTERNAL_SERVER_ERR_NAME  QString(SSO_ERR_PREFIX + QLatin1String("InternalServer"))

#define SSO_DAEMON_METHOD_NOT_KNOWN_ERR_STR   QLatin1String("Authentication method is not known.")
#define SSO_DAEMON_METHOD_NOT_KNOWN_ERR_NAME  QString(SSO_ERR_PREFIX + QLatin1String("MethodNotKnown"))

#define SSO_DAEMON_INVALID_QUERY_ERR_STR   QLatin1String("Query parameters are invalid.")
#define SSO_DAEMON_INVALID_QUERY_ERR_NAME  QString(SSO_ERR_PREFIX + QLatin1String("InvalidQuery"))

#define SSO_DAEMON_PERMISSION_DENIED_ERR_STR   QLatin1String("Client has insuficient permissions to access the service.")
#define SSO_DAEMON_PERMISSION_DENIED_ERR_NAME  QString(SSO_ERR_PREFIX + QLatin1String("PermissionDenied"))

#define SSO_DAEMON_UNKNOWN_ERR_STR   QLatin1String("Unknown error.")
#define SSO_DAEMON_UNKNOWN_ERR_NAME  QString(SSO_ERR_PREFIX + QLatin1String("Unknown"))

/*
 * SignonIdentity
 * */

#define SSO_IDENTITY_UNKNOWN_ERR_STR  QLatin1String("Unknown error.")
#define SSO_IDENTITY_UNKNOWN_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("Unknown"))

#define SSO_IDENTITY_INTERNAL_SERVER_ERR_STR  QLatin1String("Internal server error.")
#define SSO_IDENTITY_INTERNAL_SERVER_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("InternalServer"))

#define SSO_IDENTITY_NOT_FOUND_ERR_STR QLatin1String("Identity not found.")
#define SSO_IDENTITY_NOT_FOUND_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("NotFound"))

#define SSO_IDENTITY_METHOD_NOT_AVAILABLE_ERR_STR  QLatin1String("Authentication method not available.")
#define SSO_IDENTITY_METHOD_NOT_AVAILABLE_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("MethodNotAvailable"))

#define SSO_IDENTITY_PERMISSION_DENIED_ERR_STR  QLatin1String("Client has insuficient permissions to access this identity.")
#define SSO_IDENTITY_PERMISSION_DENIED_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("PermissionDenied"))

#define SSO_IDENTITY_STORE_FAILED_ERR_STR  QLatin1String("Storing of identity data failed")
#define SSO_IDENTITY_STORE_FAILED_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("StoreFailed"))

#define SSO_IDENTITY_REMOVE_FAILED_ERR_STR  QLatin1String("Removing identity failed.")
#define SSO_IDENTITY_REMOVE_FAILED_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("RemoveFailed"))

#define SSO_IDENTITY_SIGNOUT_FAILED_ERR_STR  QLatin1String("Signing out failed.")
#define SSO_IDENTITY_SIGNOUT_FAILED_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("SignOutFailed"))

#define SSO_IDENTITY_CANCELED_ERR_STR  QLatin1String("Operation canceled by user.")
#define SSO_IDENTITY_CANCELED_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("Canceled"))

#define SSO_IDENTITY_CREDENTIALS_NOT_AVAILABLE_ERR_STR  QLatin1String("Query failed.")
#define SSO_IDENTITY_CREDENTIALS_NOT_AVAILABLE_ERR_NAME QString(SSO_ERR_PREFIX + QLatin1String("CredentialsNotAvailable"))

/*
 * SignonDaemon communication client side detectable errors
 * */

#define SSO_DAEMON_NOT_AVAILABLE_ERR_STR           QLatin1String("The Signon service is not available.")
#define SSO_DAEMON_INTERNAL_COMMUNICATION_ERR_STR  QLatin1String("Communication with the Signon service failed.")


/*
 * !!! Deprecaded - TODO remove
 * */
#define notFoundErrorStr                QLatin1String("The identity matching this Identity object was not found on the service")
#define mechanismNotAvailableErrorStr   QLatin1String("The requested mechanism is not available")
#define wrongStateErrorStr              QLatin1String("An operation method has been called in a wrong state")
#define permissionDeniedErrorStr        QLatin1String("The operation cannot be performed due to insufficient client permissions")
#define operationNotSupportedErrorStr   QLatin1String("The operation is not supported by the mechanism implementation")
#define invalidChallengeErrorStr        QLatin1String("The challenge token is invalid for the mechanism implementation")
#define invalidCredentialsErrorStr      QLatin1String("The supplied credentials are invalid for the mechanism implementation")


/*
 * SignonAuthSession
 * */
#define SSO_SESSION_ERR_PREFIX QString(SIGNON_SERVICE) + QLatin1String(".AuthSessionError.")

#define unknownErrorMsg                 QLatin1String("Catch-all for errors not distinguished by another code")
#define unknownErrorName                QLatin1String("com.nokia.singlesignon.AuthSessionError.UnknownError")

#define mechanismNotAvailableErrorMsg   QLatin1String("The requested mechanism is not available")
#define mechanismNotAvailableErrorName  QLatin1String("com.nokia.singlesignon.AuthSessionError.MechanismNotAvailableError")

#define wrongStateErrorMsg              QLatin1String("An operation method has been called in a wrong state")
#define wrongStateErrorName             QLatin1String("com.nokia.singlesignon.AuthSessionError.WrongStateError")

#define permissionDeniedErrorMsg        QLatin1String("The operation cannot be performed due to insufficient client permissions")
#define permissionDeniedErrorName       QLatin1String("com.nokia.singlesignon.AuthSessionError.PermissionDeniedError")

#define operationNotSupportedErrorMsg   QLatin1String("The operation is not supported by the mechanism implementation")
#define operationNotSupportedErrorName  QLatin1String("com.nokia.singlesignon.AuthSessionError.OperationNotSupportedError")

#define noConnectionErrorMsg            QLatin1String("Network operation failed, no connection")
#define noConnectionErrorName           QLatin1String("com.nokia.singlesignon.AuthSessionError.noConnectionErrorName")

#define networkErrorMsg  QLatin1String("Network connetion failed")
#define networkErrorName QString(SSO_SESSION_ERR_PREFIX + QLatin1String("NetworkError"))

#define sslErrorMsg  QLatin1String("Ssl connetion failed")
#define sslErrorName QString(SSO_SESSION_ERR_PREFIX + QLatin1String("SslError"))

#define invalidCredentialsErrorMsg      QLatin1String("The supplied credentials are invalid for the mechanism implementation")
#define invalidCredentialsErrorName     QLatin1String("com.nokia.singlesignon.AuthSessionError.InvalidCredentialsError")

#define canceledErrorMsg               QLatin1String("Challenge was canceled")
#define canceledErrorName              QLatin1String("com.nokia.singlesignon.AuthSessionError.CanceledError")

#define userInteractionErrorMsg               QLatin1String("Problems in user interaction dialog")
#define userInteractionErrorName              QLatin1String("com.nokia.singlesignon.AuthSessionError.UserInteractionError")

#define timedOutErrorMsg                QLatin1String("Challenge was timed out")
#define timedOutErrorName               QLatin1String("com.nokia.singlesignon.AuthSessionError.TimedOutError")

#define missingDataErrorMsg             QLatin1String("The SessionData object does not contain necessary information")
#define missingDataErrorName            QLatin1String("com.nokia.singlesignon.AuthSessionError.MissingDataError")

#define runtimeErrorMsg                 QLatin1String("Runtime problems during authentication")
#define runtimeErrorName                QLatin1String("com.nokia.singlesignon.AuthSessionError.RuntimeError")

/*!
  * @namespace Single Sign-On namespace for client side objects.
  * @brief Namespace for client side objects.
  *
  */
namespace SignOn {

    /*
     * Flag values used to inform identity clients about the server side identity state
     * */
    enum IdentityState {
        IdentityDataUpdated = 0,
        IdentityRemoved,
        IdentitySignedOut
    };

    /*!
     * @enum AuthSessionState
     * Codes for the states of the AuthSession.
     * @remarks This is not a part of the public AuthSession and should be kept as an internal enum.
     *          This is not the same as AuthSession::AuthSessionState, it could even go with a different name.
     * @todo The order of the states must be synchronized with AuthPluginState enum
     */
    enum AuthSessionState {
        SessionNotStarted  = 0,         /**< No message. */
        HostResolving,                  /**< Resolving remote server host name. */
        ServerConnecting,               /**< Connecting to remote server. */
        DataSending,                    /**< Sending data to remote server. */
        ReplyWaiting,                   /**< Waiting reply from remote server. */
        UserPending,                    /**< Waiting response from user. */
        UiRefreshing,                   /**< Refreshing ui request. */
        ProcessPending,                 /**< Waiting another process to start. */
        SessionStarted,                 /**< Authentication session is started. */
        ProcessCanceling,               /**< Canceling.current process: */
        ProcessDone,                    /** < Authentication completed. > */
        CustomState,                    /**< Custom message. */
        MaxState
    };

} //namespace SignOn

#endif /* SIGNONCOMMON_H_ */
