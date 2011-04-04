/*
 * This file is part of signon
 *
 * Copyright (C) 2011 Nokia Corporation.
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
 * @copyright Copyright (C) 2009-2011 Nokia Corporation.
 * @license LGPL
 */

#ifndef SIGNON_SECURE_STORAGE_UI_H
#define SIGNON_SECURE_STORAGE_UI_H

/*! @def Property key for Ui requests' types. */
#define SSOUI_SECURE_STORAGE_REQUEST_TYPE QLatin1String("secureStorageUiRequestType")
/*! @def Property key for Ui replies' types. */
#define SSOUI_SECURE_STORAGE_REPLY_TYPE   QLatin1String("secureStorageUiReplyType")
/*! @def UI session key for secure storage related requests. */
#define SSOUI_SECURE_STORAGE_REQUEST_ID   QLatin1String("secureStorageUiId")

namespace SignOn {

/*!
  @enum UiInteractionType
  Describes the type of Ui interaction that is to be displayed
  by SignonUi in order to handle specific secure storage use cases.
*/
enum UiInteractionType
{
    NoKeyPresent = 0,        /**< No storage key is present. */
    NoAuthorizedKeyPresent,  /**< Storage keys are present but
                                  none is authorized. */
    KeyAuthorized,           /**< New key was authorized. */
    StorageCleared           /**< The storage was reformatted. */
};

/*!
  @enum UiReplyType
  Describes the type of Ui reply, indicating the user's decisions
  for specific secure storage use cases.
*/
enum UiReplyType
{
    ClearPasswordsStorage = 0,  /**< User decides to clear the passwords'
                                     storage. */
    NoKeyPresentAccepted,       /**< User has accepted the notification about
                                     unavailability of storage keys. */
    UiRejected,                 /**< Ui closed by user. */
};

} //namespace SignOn

#endif // SIGNON_SECURE_STORAGE_UI_H
