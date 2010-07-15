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

#include <signond/signoncommon.h>

#ifdef TRACE
    #undef TRACE
#endif

#ifdef BLAME
    #undef BLAME
#endif

#include <QDebug>

#ifndef SIGNOND_TRACE
    #define SIGNOND_TRACE
#endif

#ifdef SIGNOND_TRACE
    #define TRACE() qDebug() << __TIME__ << __FILE__ << __LINE__ << __func__ << ":\t"
    #define BLAME() qCritical() << __TIME__ << __FILE__ << __LINE__ << __func__ << ":\t"

    #define SIGNOND_TRACE_FILE QLatin1String("signon_trace_file")
    #define SIGNOND_TRACE_DIR  QLatin1String("/var/log")
    #define SIGNOND_TRACE_FILE_MAX_SIZE 102400 // 100 * 1024 bytes

    #define SIGNOND_INITIALIZE_TRACE(_file_name_, _maxFileSize_) \
        initializeTrace(_file_name_, _maxFileSize_);
#else
    #define TRACE() if(1) ; else qDebug()
    #define BLAME() if(1) ; else qDebug()

    #define SIGNOND_INITIALIZE_TRACE(_file_name_, _maxFileSize_)
#endif //SIGNON_TRACE

/*
 * Idle timeout for remote identities and their plugin processes
 * */
#define SIGNOND_MAX_IDLE_TIME 300

/*
 * Signon UI DBUS defs
 * */
#define SIGNON_UI_SERVICE           QLatin1String("com.nokia.singlesignonui")
#define SIGNON_UI_DAEMON_OBJECTPATH QLatin1String("/SignonUi")



#endif // SIGNOND_COMMON_H_
