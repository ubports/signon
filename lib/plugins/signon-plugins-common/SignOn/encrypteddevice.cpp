/*
 * This file is part of signon
 *
 * Copyright (C) 2009-2011 Nokia Corporation.
 *
 * Contact: Rauli Ikonen <rauli.ikonen@nixuopen.org>
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

#include "encrypteddevice.h"

#include <openssl/err.h>
#include <stdlib.h>
#include <stdio.h>

#include "SignOn/signonplugincommon.h"

using namespace SignOn;

EncryptedDevice::EncryptedDevice(QIODevice *actualDevice,
                                 const unsigned char *encryptionKey,
                                 unsigned int keySize,
                                 const unsigned char *ivIn,
                                 const unsigned char *ivOut):
    m_actualDevice(actualDevice),
    m_currentPosOut(0),
    m_currentPosIn(0),
    m_tempByteArray(NULL),
    m_tempByteArrayPos(0),
    m_valid(true)
{
    setOpenMode(actualDevice->openMode());

    if (AES_set_encrypt_key(encryptionKey, keySize * 8,
                            &m_encryptionKey) != 0) {
        BLAME() << "AES_set_encrypt_key failed:" << ERR_get_error();
        m_valid = false;
        return;
    }

    AES_ecb_encrypt(ivOut, m_keyStreamOut, &m_encryptionKey, AES_ENCRYPT);
    AES_ecb_encrypt(ivIn, m_keyStreamIn, &m_encryptionKey, AES_ENCRYPT);
}

bool EncryptedDevice::open(OpenMode mode)
{
    if (!m_valid)
        return false;
    if (!m_actualDevice->open(mode))
        return false;
    return QIODevice::open(mode);
}

void EncryptedDevice::close()
{
    m_actualDevice->close();
    QIODevice::close();
}

qint64 EncryptedDevice::bytesAvailable() const
{
    if (m_tempByteArray != NULL)
        return m_tempByteArray->size() - m_tempByteArrayPos;
    return m_actualDevice->bytesAvailable();
}

qint64 EncryptedDevice::bytesToWrite() const
{
    return m_actualDevice->bytesToWrite();
}

qint64 EncryptedDevice::readData(char *data, qint64 maxLen)
{
    qint64 bytesRead = 0;
    if (m_tempByteArray != NULL) {
        if (m_tempByteArrayPos < m_tempByteArray->size()) {
            int bytesToRead = m_tempByteArray->size() - m_tempByteArrayPos;
            if (bytesToRead > maxLen)
                bytesToRead = (int)maxLen;
            memcpy(data, m_tempByteArray->constData(), bytesToRead);
            bytesRead = bytesToRead;
            m_tempByteArrayPos += bytesToRead;
        }
    } else {
        bytesRead = m_actualDevice->read(data, maxLen);
    }

    for (qint64 i = 0; i < bytesRead; ++i) {
        if (m_currentPosIn == AES_BLOCK_SIZE) {
            AES_ecb_encrypt(m_keyStreamIn, m_keyStreamIn,
                            &m_encryptionKey, AES_ENCRYPT);
            m_currentPosIn = 0;
        }
        data[i] = data[i] ^ m_keyStreamIn[m_currentPosIn];
        ++m_currentPosIn;
    }

    return bytesRead;
}

qint64 EncryptedDevice::writeData(const char *data, qint64 len)
{
    if (len <= 0)
        return 0;

    char *encryptedData = (char *)malloc(len);
    if (encryptedData == NULL)
        return -1;

    for (qint64 i = 0; i < len; ++i) {
        if (m_currentPosOut == AES_BLOCK_SIZE) {
            AES_ecb_encrypt(m_keyStreamOut, m_keyStreamOut,
                            &m_encryptionKey, AES_ENCRYPT);
            m_currentPosOut = 0;
        }
        encryptedData[i] = data[i] ^ m_keyStreamOut[m_currentPosOut];
        ++m_currentPosOut;
    }

    qint64 totalBytesWritten = 0;
    while (totalBytesWritten < len) {
        qint64 bytesWritten =
                m_actualDevice->write(encryptedData + totalBytesWritten,
                                      len - totalBytesWritten);
        if (bytesWritten < 0)
            break;
        totalBytesWritten += bytesWritten;
    }
    free(encryptedData);

    return totalBytesWritten;
}
