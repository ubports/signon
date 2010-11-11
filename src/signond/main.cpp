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

#include "signond-common.h"
#include "signondaemon.h"
#include "signontrace.h"

#include <QtCore>

#include <errno.h>

using namespace SignonDaemonNS;
using namespace SignOn;

void installSigHandlers()
{
    struct sigaction act;
    act.sa_handler = SignonDaemon::signalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;

    sigaction(SIGHUP, &act, 0);
    sigaction(SIGTERM, &act, 0);
    sigaction(SIGINT, &act, 0);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    installSigHandlers();

    SIGNOND_INITIALIZE_TRACE(SIGNOND_TRACE_FILE, SIGNOND_TRACE_FILE_MAX_SIZE)

    if (getuid() != 0) {
        BLAME() << "Failed to SUID root. Secure storage will not be available.";
    }

    QMetaObject::invokeMethod(SignonDaemon::instance(),
                              "init",
                              Qt::QueuedConnection);
    return app.exec();
}
