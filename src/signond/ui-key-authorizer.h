/*
 * This file is part of signon
 *
 * Copyright (C) 2011 Nokia Corporation.
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
/*!
 * @copyright Copyright (C) 2011 Nokia Corporation.
 * @license LGPL
 */

#ifndef SIGNON_UI_KEY_AUTHORIZER_H
#define SIGNON_UI_KEY_AUTHORIZER_H

#include "abstract-key-authorizer.h"
#include "signonui_interface.h"

#include <QObject>

namespace SignonDaemonNS {

/*!
 * @class UiKeyAuthorizer
 *
 * Implements the SignOn UI-based key authorization methods.
 * This class provides two different mechanisms for authorising keys:
 *
 * @li When signond calls queryKeyAuthorization() with no key specified and
 * with the StorageNeeded reason and there is at least one unauthorized key
 * inserted, we will ask the user to remove an inserted key and insert an
 * authorized key; we will then authorize the key that the user removed.  This
 * process involves the SwapWithAuthorized and WaitingAuthorized states.
 *
 * @li When the last authorized key is removed, we ask the user to insert a
 * new unauthorized key, and we'll authorize that. This is handled by the
 * AuthorizeNext and WaitingQuery states.
 */
class UiKeyAuthorizer: public AbstractKeyAuthorizer
{
    Q_OBJECT

public:
    /*!
     * @enum State
     * Internal state of the authorizer.
     * Normally, the authorizer is in Idle state.
     * See the description of the UiKeyAuthorizer class to have a overall
     * understanding of the other states.
     */
    enum State {
        Idle = 0,           /*!< not authorizing any key */

        /*!
         * Will authorize the next inserted key: the key will be cached in
         * m_queriedKey, and when queryKeyAuthorization() is called for that
         * key, we will approve the authorization request.
         */
        AuthorizeNext,
        /*!
         * An unauthorized key has been inserted; we cache it in m_queriedKey,
         * and we are waiting for the queryKeyAuthorization() to happen.
         */
        WaitingQuery,

        /*!
         * We expect the user to remove the currently unauthorized key, and
         * then to insert an authorized one.
         */
        SwapWithAuthorized,
        /*!
         * Waiting for the insertion of an authorized key.
         */
        WaitingAuthorized,
    };

    explicit UiKeyAuthorizer(KeyHandler *keyHandler, QObject *parent = 0);
    ~UiKeyAuthorizer();

    /* reimplemented from KeyAuthorizer */
    void queryKeyAuthorization(const SignOn::Key &key, Reason reason);

private Q_SLOTS:
    void onKeyInserted(const SignOn::Key key);
    void onKeyDisabled(const SignOn::Key key);
    void onLastAuthorizedKeyRemoved(const SignOn::Key key);
    void onSecureStorageUiRejected();
    void onNoKeyPresentAccepted();
    void onClearPasswordsStorage();

private:
    void setState(State state);
    void closeUi();

private:
    SignonSecureStorageUiAdaptor *m_uiAdaptor;
    State m_state;
    SignOn::Key m_queriedKey;
};

} // namespace

#endif // SIGNON_UI_KEY_AUTHORIZER_H
