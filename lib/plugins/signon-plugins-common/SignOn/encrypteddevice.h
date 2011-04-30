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

#ifndef SIGNON_ENCRYPTEDDEVICE_H
#define SIGNON_ENCRYPTEDDEVICE_H

#include <QIODevice>
#include <QByteArray>
#include <openssl/aes.h>

namespace SignOn {

/**
  * EncryptedDevice allows encrypting (and decrypting) all data that
  * passes through a normal, unencrypted device. The encryption is
  * done using AES in OFB mode.
  * EncryptedDevice always works in sequential mode even if the
  * underlying device supported random access.
  */
class EncryptedDevice : public QIODevice
{
public:
    /**
      * Constructor
      * @param actualDevice The device that acts as the real data source / sink
      * @param encryptionKey The encryption key
      * @param keySize Size of encryption key in bytes. Must be either 16, 24
      *        or 32
      * @param ivIn Initialization vector for data read from the device. The
      *        size of the initialization vector must be the same as AES block
      *        size, i.e. 16 bytes
      * @param ivOut Initialization vector for data written to the device. The
      *        size of the initialization vector must be the same as AES block
      *        size, i.e. 16 bytes
      */
    EncryptedDevice(QIODevice *actualDevice,
                    const unsigned char *encryptionKey, unsigned int keySize,
                    const unsigned char *ivOn, const unsigned char *ivOut);

    virtual bool isSequential () const { return true; }

    virtual bool open(OpenMode mode);
    virtual void close();

    virtual qint64 bytesAvailable() const;
    virtual qint64 bytesToWrite() const;

    /**
      * Temporarily switches all input to use the given QByteArray
      * instead of the QIODevice given when this device was constructed
      */
    void setTemporaryDataSource(QByteArray *tmp) { m_tempByteArray = tmp; m_tempByteArrayPos = 0; }
    void clearTemporaryDataSource() { m_tempByteArray = NULL; }

protected:
    virtual qint64 readData(char *data, qint64 maxLen);
    virtual qint64 writeData(const char *data, qint64 len);

private:
    Q_DISABLE_COPY(EncryptedDevice);

    QIODevice *m_actualDevice;
    unsigned char m_keyStreamOut[AES_BLOCK_SIZE];
    unsigned int m_currentPosOut;
    unsigned char m_keyStreamIn[AES_BLOCK_SIZE];
    unsigned int m_currentPosIn;
    AES_KEY m_encryptionKey;
    QByteArray *m_tempByteArray;
    int m_tempByteArrayPos;
    bool m_valid;
};

class TemporaryEncryptedDataSourceSetter
{
public:
    TemporaryEncryptedDataSourceSetter(EncryptedDevice *dev, QByteArray *arr) : m_dev(dev) {
        m_dev->setTemporaryDataSource(arr);
    }
    ~TemporaryEncryptedDataSourceSetter() {
        m_dev->clearTemporaryDataSource();
    }
private:
    EncryptedDevice *m_dev;
};

}

#endif // SIGNON_ENCRYPTEDDEVICE_H
