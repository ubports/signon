/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of signon
 *
 * Copyright (C) 2010 Nokia Corporation.
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
/*!
 * @copyright Copyright (C) 2009-2011 Nokia Corporation.
 * @license LGPL
 */

#ifndef SIMDLC_H
#define SIMDLC_H

#define SIMDLC_SERVICE "com.nokia.SingleSignOn.DeviceLock"
#define SIMDLC_PATH "/com/nokia/SingleSignOn/DeviceLock"
#define SIMDLC_INTERFACE SIMDLC_SERVICE

#ifdef __cplusplus
    #include <QLatin1String>
    #define SIMDLC_STRING(s) QLatin1String(s)

    #define SIMDLC_SERVICE_S QLatin1String(SIMDLC_SERVICE)
    #define SIMDLC_PATH_S QLatin1String(SIMDLC_PATH)
    #define SIMDLC_INTERFACE_S QLatin1String(SIMDLC_INTERFACE)
#endif

#endif // SIMDLC_H

