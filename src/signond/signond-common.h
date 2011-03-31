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

#ifndef SIGNOND_COMMON_H_
#define SIGNOND_COMMON_H_

#include "signond/signoncommon.h"

#ifdef TRACE
    #undef TRACE
#endif

#ifdef BLAME
    #undef BLAME
#endif

#ifdef SIGNOND_TRACE
    #include <QDebug>

    #ifdef DEBUG_ENABLED
        /* 0 - fatal, 1 - critical(default), 2 - info/debug */
        extern int loggingLevel;

        inline bool debugEnabled()
        {
            return loggingLevel >= 2;
        }

        inline bool criticalsEnabled()
        {
            return loggingLevel >= 1;
        }

        #define TRACE() \
            if (debugEnabled()) qDebug() << __FILE__ << __LINE__ << __func__
        #define BLAME() \
            if (criticalsEnabled()) qCritical() << __FILE__ << __LINE__ << __func__

        #define SIGNOND_INITIALIZE_TRACE() initializeTrace();
    #else
        inline bool debugEnabled() { return false; }
        inline bool criticalsEnabled() { return false; }
        #define TRACE() while (0) qDebug()
        #define BLAME() while (0) qDebug()

        #define SIGNOND_INITIALIZE_TRACE()
    #endif
#endif

#ifdef TESTS_TRACE
    #define TRACE() \
        qDebug() << __FILE__ << __LINE__ << __func__
    #define BLAME() \
        qCritical() << __FILE__ << __LINE__ << __func__
#endif

/*
 * Idle timeout for remote identities and their plugin processes
 * */
#define SIGNOND_MAX_IDLE_TIME 300

/*
 * Signon UI DBUS defs
 * */
#define SIGNON_UI_SERVICE           QLatin1String("com.nokia.singlesignonui")
#define SIGNON_UI_DAEMON_OBJECTPATH QLatin1String("/SignonUi")

/*
 * Signon Daemon default configuration values
 */
const char signonDefaultDbName[] = "signon.db";
const char signonDefaultStoragePath[] = "/home/user/.signon";
const char signonDefaultFileSystemName[] = "signonfs";
const char signonDefaultFileSystemType[] = "ext2";
const bool signonDefaultUseEncryption = true;
const uint signonMinumumDbSize = 8;

#endif // SIGNOND_COMMON_H_
