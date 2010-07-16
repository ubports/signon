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

#ifndef LIBSIGNONCOMMON_H
#define LIBSIGNONCOMMON_H

#ifdef TRACE
    #undef TRACE
#endif

#ifdef BLAME
    #undef BLAME
#endif

#ifndef LIBSIGNON_TRACE
    #define LIBSIGNON_TRACE
#endif

#ifdef LIBSIGNON_TRACE
    #include <QDebug>

    #define TRACE() qDebug() << __TIME__ << __FILE__ << __LINE__ << __func__ << ":\t"
    #define BLAME() qCritical() << __TIME__ << __FILE__ << __LINE__ << __func__ << ":\t"
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

/*
   TODO - Add here a common data container for IdentityInfo,
          dbus register it and use it as qt dbus type
          for the signond <--> libsignon communication
*/

#endif // LIBSIGNONCOMMON_H
