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


#ifdef __cplusplus
    #include <QLatin1String>
    #include <QDBusConnection>

    #define SIGNOND_STRING(s) QLatin1String(s)
    #define SIGNOND_BUS       QDBusConnection::sessionBus()
#else
    #define SIGNOND_STRING(s) s
    #define SIGNOND_BUS //TODO
#endif


#define SIGNOND_NEW_IDENTITY 0

#define SIGNOND_MAX_TIMEOUT 0x7FFFFFFF

/*
 * todo: the naming convention for interfaces should be clarified
 * */

/*
 * Common DBUS definitions
 * */
#define SIGNOND_SERVICE_PREFIX     "com.nokia.singlesignon"
#define SIGNOND_SERVICE            SIGNOND_STRING(SIGNOND_SERVICE_PREFIX)

#define SIGNOND_DAEMON_OBJECTPATH       SIGNOND_STRING("/com/nokia/SingleSignon")
#define SIGNOND_DAEMON_INTERFACE        SIGNOND_STRING(SIGNOND_SERVICE_PREFIX ".SignonDaemon")
#define SIGNOND_IDENTITY_INTERFACE      SIGNOND_STRING(SIGNOND_SERVICE_PREFIX ".SignonIdentity")
#define SIGNOND_AUTH_SESSION_INTERFACE  SIGNOND_STRING(SIGNOND_SERVICE_PREFIX ".SignonAuthSession")

#define SIGNOND_ERR_PREFIX SIGNOND_SERVICE_PREFIX ".Error."

/*
 * Common server/client sides error names and messages
 * */
#define SIGNOND_UNKNOWN_ERR_STR SIGNOND_STRING("Unknown error.")
#define SIGNOND_UNKNOWN_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "Unknown")

#define SIGNOND_INTERNAL_SERVER_ERR_STR SIGNOND_STRING("Server internal error occurred.")
#define SIGNOND_INTERNAL_SERVER_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "InternalServer")

#define SIGNOND_INTERNAL_COMMUNICATION_ERR_STR SIGNOND_STRING("Communication with the Signon service failed..")
#define SIGNOND_INTERNAL_COMMUNICATION_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "InternalCommunication")

#define SIGNOND_PERMISSION_DENIED_ERR_STR SIGNOND_STRING("Client has insuficient permissions to access the service.")
#define SIGNOND_PERMISSION_DENIED_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "PermissionDenied")

#define SIGNOND_METHOD_NOT_KNOWN_ERR_STR SIGNOND_STRING("Authentication method is not known.")
#define SIGNOND_METHOD_NOT_KNOWN_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "MethodNotKnown")

#define SIGNOND_SERVICE_NOT_AVAILABLE_ERR_STR SIGNOND_STRING("Signon service is currently not available.")
#define SIGNOND_SERVICE_NOT_AVAILABLE_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "ServiceNotAvailable")

#define SIGNOND_INVALID_QUERY_ERR_STR SIGNOND_STRING("Query parameters are invalid.")
#define SIGNOND_INVALID_QUERY_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "InvalidQuery")

#define SIGNOND_METHOD_NOT_AVAILABLE_ERR_STR SIGNOND_STRING("Authentication method is not available.")
#define SIGNOND_METHOD_NOT_AVAILABLE_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "MethodNotAvailable")

#define SIGNOND_IDENTITY_NOT_FOUND_ERR_STR SIGNOND_STRING("The identity was not found on the server.")
#define SIGNOND_IDENTITY_NOT_FOUND_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "IdentityNotFound")

#define SIGNOND_STORE_FAILED_ERR_STR SIGNOND_STRING("Storing of the identity data failed.")
#define SIGNOND_STORE_FAILED_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "StoreFailed")

#define SIGNOND_REMOVE_FAILED_ERR_STR SIGNOND_STRING("Removing identity data failed.")
#define SIGNOND_REMOVE_FAILED_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "RemoveFailed")

#define SIGNOND_SIGNOUT_FAILED_ERR_STR SIGNOND_STRING("Signing out failed.")
#define SIGNOND_SIGNOUT_FAILED_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "SignOutFailed")

#define SIGNOND_IDENTITY_OPERATION_CANCELED_ERR_STR SIGNOND_STRING("Operation canceled by user.")
#define SIGNOND_IDENTITY_OPERATION_CANCELED_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "IdentityOperationCanceled")

#define SIGNOND_CREDENTIALS_NOT_AVAILABLE_ERR_STR SIGNOND_STRING("Query returned no results.")
#define SIGNOND_CREDENTIALS_NOT_AVAILABLE_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "CredentialsNotAvailable")

#define SIGNOND_MECHANISM_NOT_AVAILABLE_ERR_STR SIGNOND_STRING("Requested mechanism is not available.")
#define SIGNOND_MECHANISM_NOT_AVAILABLE_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "MechanismNotAvailable")

#define SIGNOND_MISSING_DATA_ERR_STR SIGNOND_STRING("The SessionData object does not contain all necessary information.")
#define SIGNOND_MISSING_DATA_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "MissingData")

#define SIGNOND_INVALID_CREDENTIALS_ERR_STR SIGNOND_STRING("The supplied credentials are invalid for the mechanism implementation.")
#define SIGNOND_INVALID_CREDENTIALS_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "InvalidCredentials")

#define SIGNOND_NOT_AUTHORIZED_ERR_STR SIGNOND_STRING("Not authorized to access service account.")
#define SIGNOND_NOT_AUTHORIZED_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "NotAuthorized")

#define SIGNOND_WRONG_STATE_ERR_STR SIGNOND_STRING("Operation method has been called in a wrong state.")
#define SIGNOND_WRONG_STATE_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "WrongState")

#define SIGNOND_OPERATION_NOT_SUPPORTED_ERR_STR SIGNOND_STRING("The operation is not supported by the mechanism implementation.")
#define SIGNOND_OPERATION_NOT_SUPPORTED_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "OperationNotSupported")

#define SIGNOND_NO_CONNECTION_ERR_STR SIGNOND_STRING("No network connection.")
#define SIGNOND_NO_CONNECTION_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "NoConnection")

#define SIGNOND_NETWORK_ERR_STR SIGNOND_STRING("Network connetion failed.")
#define SIGNOND_NETWORK_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "Network")

#define SIGNOND_SSL_ERR_STR SIGNOND_STRING("Ssl connection failed.")
#define SIGNOND_SSL_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "Ssl")

#define SIGNOND_RUNTIME_ERR_STR SIGNOND_STRING("Casting SessionData into subclass failed.")
#define SIGNOND_RUNTIME_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "Runtime")

#define SIGNOND_SESSION_CANCELED_ERR_STR SIGNOND_STRING("Session processing was canceled.")
#define SIGNOND_SESSION_CANCELED_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "SessionCanceled")

#define SIGNOND_TIMED_OUT_ERR_STR SIGNOND_STRING("Session processing timed out.")
#define SIGNOND_TIMED_OUT_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "TimedOut")

#define SIGNOND_USER_INTERACTION_ERR_STR SIGNOND_STRING("User interaction dialog failed.")
#define SIGNOND_USER_INTERACTION_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "UserInteraction")

#define SIGNOND_OPERATION_FAILED_ERR_STR SIGNOND_STRING("Temporary failure in authentication.")
#define SIGNOND_OPERATION_FAILED_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "OperationFailed")

#define SIGNOND_USER_ERROR_ERR_NAME SIGNOND_STRING(SIGNOND_ERR_PREFIX "User")


#ifdef __cplusplus
    namespace SignOn {
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
            ProcessDone,                    /**< Authentication completed. > */
            CustomState,                    /**< Custom message. */
            MaxState
        };

        /*
         * Flag values used to inform identity clients about the server side identity state
         * TODO - the DBUS signal using this will be replaced by 3 specific signals, thus
         *        this will be removed.
         * */
        enum IdentityState {
            IdentityDataUpdated = 0,
            IdentityRemoved,
            IdentitySignedOut
        };
    }// namespace SignOn
#else
    enum SignonAuthSessionState {
        SIGNON_AUTH_SESSION_STATE_NOT_STARTED = 0,   /**< No message. */
        SIGNON_AUTH_SESSION_STATE_RESOLVING_HOST,    /**< Resolving remote server host name. */
        SIGNON_AUTH_SESSION_STATE_CONNECTING,        /**< Connecting to remote server. */
        SIGNON_AUTH_SESSION_STATE_SENDING_DATA,      /**< Sending data to remote server. */
        SIGNON_AUTH_SESSION_STATE_WAITING_REPLY,     /**< Waiting reply from remote server. */
        SIGNON_AUTH_SESSION_STATE_USER_PENDING,      /**< Waiting response from user. */
        SIGNON_AUTH_SESSION_STATE_UI_REFRESHING,     /**< Refreshing ui request. */
        SIGNON_AUTH_SESSION_STATE_PROCESS_PENDING,   /**< Waiting another process to start. */
        SIGNON_AUTH_SESSION_STATE_STARTED,           /**< Authentication session is started. */
        SIGNON_AUTH_SESSION_STATE_PROCESS_CANCELING, /**< Canceling.current process: */
        SIGNON_AUTH_SESSION_STATE_PROCESS_DONE,      /**< Authentication completed. > */
        SIGNON_AUTH_SESSION_STATE_CUSTOM,            /**< Custom message. */
        SIGNON_AUTH_SESSION_STATE_LAST
    };

#endif //__cplusplus


#endif /* SIGNONCOMMON_H_ */
