/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include "accesscodehandler.h"
#include "signond-common.h"
#include "simdbusadaptor.h"

#include <QThreadPool>

namespace SignonDaemonNS {

    // todo remove hardcoded lockCode and integrate functionality when the lock code lib will be available
    static const QString lockCode = QLatin1String("4321");

    static const int simTaskPriority = 2;
    static const int initializeAttempts = 5;

    AccessCodeHandler::AccessCodeHandler(QObject *parent)
        : QObject(parent),
          QRunnable(),
          m_pSimDBusAdaptor(NULL),
          m_code(QString()),
          m_codeType(UNKNOWN),
          m_codeChanged(false)
    {
    }

    AccessCodeHandler::AccessCodeHandler(const CodeType &type, QObject *parent)
        : QObject(parent),
          QRunnable(),
          m_pSimDBusAdaptor(NULL),
          m_code(QString()),
          m_codeType(type),
          m_codeChanged(false)
    {
    }

    AccessCodeHandler::~AccessCodeHandler()
    {
    }

    bool AccessCodeHandler::initialize(void)
    {
        int attempts = initializeAttempts;
        bool initialized = false;

        m_pSimDBusAdaptor = new SimDBusAdaptor(this);

        //TODO remove while loop later
        while (m_pSimDBusAdaptor && !(initialized = m_pSimDBusAdaptor->initialize()) && (attempts > 0)) {
            TRACE() << "FAILED to initialize SIM DBUS Adaptor. Retrying init step...";
            delete m_pSimDBusAdaptor;

            m_pSimDBusAdaptor = new SimDBusAdaptor(this);
            --attempts;
        }

        if (!initialized)
            return false;

        TRACE() << "SimDBusAdaptor successfully initialized...";

        QObject::connect(
                    m_pSimDBusAdaptor,
                    SIGNAL(simChanged(const QString &)),
                    this,
                    SLOT(simChanged(const QString &)),
                    Qt::DirectConnection);

        setAutoDelete(false);
        QThreadPool::globalInstance()->start(this, simTaskPriority);
        QThreadPool::globalInstance()->setMaxThreadCount(2);

        TRACE() << "AccessCodeHandler successfully initialized...";
        return true;
    }

    void AccessCodeHandler::finalize()
    {
        setAutoDelete(true);
        QThreadPool::globalInstance()->releaseThread();
        QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
    }

    void AccessCodeHandler::run()
    {
        QMutexLocker locker(&m_mutex);
        QThreadPool::globalInstance()->releaseThread();

        sleep(1);
        m_pSimDBusAdaptor->checkSim();

        QThreadPool::globalInstance()->tryStart(this);
        QThreadPool::globalInstance()->reserveThread();
    }

    void AccessCodeHandler::simChanged(const QString &newSimPin)
    {
        TRACE() << "SIM CHANGED to " << newSimPin;

        m_codeChanged = true;
        m_code = newSimPin;

        // do not integrate with the multiple key slots feature - for the moment
        //emit newSimAvailable(m_code);
    }

    bool AccessCodeHandler::codeAvailable()
    {
        QMutexLocker locker(&m_mutex);
        return !m_code.isEmpty();
    }

    bool AccessCodeHandler::codeChanged()
    {
        QMutexLocker locker(&m_mutex);
        return m_codeChanged;
    }

    QString AccessCodeHandler::currentCode()
    {
        QMutexLocker locker(&m_mutex);
        m_codeChanged = false;
        return m_code;
    }

} //namespace SignonDaemonNS
