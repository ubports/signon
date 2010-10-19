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

extern "C" {
    #include <signal.h>
    #include <unistd.h>
    #include <errno.h>
    #include <stdio.h>
    #include <sys/types.h>
}

#include "signond-common.h"
#include "signondaemon.h"
#include "signontrace.h"

#include <QtCore>

using namespace SignonDaemonNS;
using namespace SignOn;

static bool unix_signals_installed = false;
static SignonDaemon *g_signonDaemon = NULL;
static QCoreApplication *g_qApp = NULL;


void signal_handler(int signal)
{
    //todo - update this so that no memory is allocated/deleted in the signals' handler scope
    if (unix_signals_installed) {
        if (signal == SIGHUP) {
            if (g_signonDaemon) {
                delete g_signonDaemon;
                g_signonDaemon = NULL;
            }
            sleep(1);
            g_signonDaemon = new SignonDaemon(g_qApp);
            if (!g_signonDaemon->init(false)) {
                fprintf(stderr, "Signon daemon could not restart on SIGHUP.\n");
            }
            return;
        }

        exit(0);
    }
}

void installSigHandlers()
{
    unix_signals_installed = true;

    struct sigaction handler;

    memset(&handler, 0, sizeof(handler));

    handler.sa_handler = signal_handler;

    sigaction(SIGTERM, &handler, NULL);
    sigaction(SIGINT, &handler, NULL);
    sigaction(SIGKILL, &handler, NULL);
    sigaction(SIGSTOP, &handler, NULL);
    sigaction(SIGHUP, &handler, NULL);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    g_qApp = QCoreApplication::instance();

    installSigHandlers();

    SIGNOND_INITIALIZE_TRACE(SIGNOND_TRACE_FILE, SIGNOND_TRACE_FILE_MAX_SIZE)

    if (setuid(0) != 0) {
        BLAME() << "Failed to SUID root. Secure storage will not be available.";
    }

    g_signonDaemon = new SignonDaemon(&app);

    bool startedForBackup = app.arguments().contains(QLatin1String("-backup"));
    if (!g_signonDaemon->init(startedForBackup)) {
        qFatal("Signon daemon could not start.");
        return 0;
    }
    else
        return app.exec();
}
