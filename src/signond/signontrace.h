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
#ifndef SIGNONTRACE_H
#define SIGNONTRACE_H

#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QDateTime>

#include "signond-common.h"

namespace SignOn {

    template <typename T = void>
    class SignonTrace
    {
        SignonTrace(const QString &filePath, const quint32 maxFileSize)
            : m_outputFile(filePath),
            m_writeStream(&m_outputFile),
            m_maxFileSize(maxFileSize)
        {}

    public:
        ~SignonTrace()
        {
            delete m_pInstance;
            m_pInstance = NULL;
        }

        static bool initialize(const QString &filePath, const quint32 maxFileSize)
        {
            if (m_pInstance)
                return true;

            m_pInstance = new SignonTrace<T>(filePath, maxFileSize);

            if (!m_pInstance->m_outputFile.open(QIODevice::Append)) {
                TRACE() << "Signon: Failed to initialize file tracing.";
                delete m_pInstance;
                m_pInstance = NULL;
                return false;
            }

            qInstallMsgHandler(output);
            return true;
        }

        static void output(QtMsgType type, const char *msg)
        {
            //todo - handle max file size !!!

            const char *msgType;
            switch (type) {
                case QtWarningMsg: msgType = "Warning"; break;
                case QtCriticalMsg: msgType = "Critical"; break;
                case QtFatalMsg: msgType = "Fatal"; break;
                case QtDebugMsg:
                    /* fall through */
                default: msgType = "Debug"; break;
            }

            if (m_pInstance->m_outputFile.size() >= m_pInstance->m_maxFileSize) {
                m_pInstance->m_outputFile.close();
                m_pInstance->m_outputFile.remove();
            }

            if (!m_pInstance->m_outputFile.isOpen())
                m_pInstance->m_outputFile.open(QIODevice::Append);

            m_pInstance->m_writeStream << QString(QLatin1String("%1: %2\n"))
                . arg(QLatin1String(msgType))
                . arg(QLatin1String(msg));
            m_pInstance->m_outputFile.close();
         }

    private:
        static SignonTrace<T> *m_pInstance;
        QFile m_outputFile;
        QTextStream m_writeStream;
        quint32 m_maxFileSize;
    };

    static void initializeTrace(const QString &fileName, const quint32 maxFileSize) {
        SignonTrace<>::initialize(QString(QLatin1String("/var/log/%1")).arg(fileName), maxFileSize);
    }

template <typename T>
    SignonTrace<T> *SignonTrace<T>::m_pInstance = 0;

} //namespace SignOn

#endif // SIGNONTRACE_H
