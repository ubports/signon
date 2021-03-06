/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2010 Nokia Corporation.
 * Copyright (C) 2012-2016 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

#ifndef SIGNONPLUGIN_H
#define SIGNONPLUGIN_H

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qplugin.h>

#include <QVariantMap>
#include "SignOn/sessiondata.h"
#include "SignOn/uisessiondata.h"

#ifdef SIGNON_PLUGIN_TRACE
    #include <QDebug>

    #ifdef DEBUG_ENABLED
        #ifdef TRACE
            #undef TRACE
        #endif

        #ifdef BLAME
            #undef BLAME
        #endif

        #define TRACE() qDebug() << __FILE__ << __LINE__ << __func__
        #define BLAME() qCritical() << __FILE__ << __LINE__ << __func__
    #else
        #define TRACE() while (0) qDebug()
        #define BLAME() while (0) qCritical()
    #endif
#endif

#endif // SIGNONPLUGIN_H
