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
#ifndef UISESSIONDATA_PRIV_H
#define UISESSIONDATA_PRIV_H

#define SSOUI_KEY_CAPTION         QLatin1String("Caption")
#define SSOUI_KEY_MESSAGEID       QLatin1String("QueryMessageId")
#define SSOUI_KEY_MESSAGE         QLatin1String("QueryMessage")
#define SSOUI_KEY_QUERYUSERNAME   QLatin1String("QueryUserName")
#define SSOUI_KEY_USERNAME        QLatin1String("UserName")
#define SSOUI_KEY_QUERYPASSWORD   QLatin1String("QueryPassword")
#define SSOUI_KEY_PASSWORD        QLatin1String("Secret")
#define SSOUI_KEY_SHOWREALM       QLatin1String("ShowRealm")
#define SSOUI_KEY_REALM           QLatin1String("Realm")
#define SSOUI_KEY_NETWORKPROXY    QLatin1String("NetworkProxy")
#define SSOUI_KEY_UIPOLICY        QLatin1String("UiPolicy")
#define SSOUI_KEY_OPENURL         QLatin1String("OpenUrl")
#define SSOUI_KEY_URLRESPONSE     QLatin1String("UrlResponse")
#define SSOUI_KEY_CAPTCHAURL      QLatin1String("CaptchaUrl")
#define SSOUI_KEY_CAPTCHAIMG      QLatin1String("CaptchaImage") //QByteArray !!!
#define SSOUI_KEY_CAPTCHARESP     QLatin1String("CaptchaResponse")
#define SSOUI_KEY_ERROR           QLatin1String("QueryErrorCode")
#define SSOUI_KEY_REMEMBER        QLatin1String("Remember")
#define SSOUI_KEY_REQUESTID       QLatin1String("requestId") //id of request, used for cancellation
#define SSOUI_KEY_REFRESH         QLatin1String("refreshRequired") //id of request, used for cancellation
#define SSOUI_KEY_WATCHDOG        QLatin1String("watchdog")         // automatic behavior of dialog
#define SSOUI_KEY_STORED_IDENTITY QLatin1String("StoredIdentity") /* flag whether
                                                                     the credentials are stored or not */

#define SSOUI_KEY_SLOT_ACCEPT  "accept"
#define SSOUI_KEY_SLOT_REJECT  "reject"
#define SSOUI_KEY_SLOT_REFRESH "refresh"


#endif /* UISESSIONDATA_PRIV_H */
