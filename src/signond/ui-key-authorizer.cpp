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

#include "cryptomanager.h"
#include "ui-key-authorizer.h"

using namespace SignonDaemonNS;

UiKeyAuthorizer::UiKeyAuthorizer(KeyHandler *keyHandler, QObject *parent):
    AbstractKeyAuthorizer(keyHandler, parent),
    m_uiAdaptor(0),
    m_state(Idle)
{
    QObject::connect(keyHandler, SIGNAL(keyInserted(const SignOn::Key)),
                     this, SLOT(onKeyInserted(const SignOn::Key)));
    QObject::connect(keyHandler, SIGNAL(keyDisabled(const SignOn::Key)),
                     this, SLOT(onKeyDisabled(const SignOn::Key)));
    QObject::connect(keyHandler,
                     SIGNAL(lastAuthorizedKeyRemoved(const SignOn::Key)),
                     this,
                     SLOT(onLastAuthorizedKeyRemoved(const SignOn::Key)));
}

UiKeyAuthorizer::~UiKeyAuthorizer()
{
}

void UiKeyAuthorizer::queryKeyAuthorization(const SignOn::Key &key,
                                            Reason reason)
{
    if (m_state == WaitingQuery) {
        if (key == m_queriedKey) {
            emit keyAuthorizationQueried(key, Approved);
            return;
        }
    } else if (reason & StorageNeeded &&
               key.isEmpty() &&
               !keyHandler()->insertedKeys().isEmpty()) {
        if (m_state == SwapWithAuthorized) {
            TRACE() << "Ignoring second request for StorageNeeded";
            return;
        }
        TRACE() << "Secure Storage not available, notifying user.";
        if (m_uiAdaptor == 0) {
            m_uiAdaptor =
                new SignonSecureStorageUiAdaptor(SIGNON_UI_SERVICE,
                                                 SIGNON_UI_DAEMON_OBJECTPATH,
                                                 SIGNOND_BUS);
        }

        connect(m_uiAdaptor,
                SIGNAL(clearPasswordsStorage()),
                SLOT(onClearPasswordsStorage()));
        connect(m_uiAdaptor,
                SIGNAL(uiRejected()),
                SLOT(onSecureStorageUiRejected()));
        connect(m_uiAdaptor,
                SIGNAL(error()),
                SLOT(onSecureStorageUiRejected()));

        m_uiAdaptor->notifyNoAuthorizedKeyPresent();
        setState(SwapWithAuthorized);
        return;
    } else if (m_state == Idle) {
        if (!keyHandler()->cryptoManager()->fileSystemIsSetup()) {
            TRACE() << "Secure storage not setup; authorizing key";
            emit keyAuthorizationQueried(key, Approved);
            return;
        }
    }
    emit keyAuthorizationQueried(key, Denied);
}

void UiKeyAuthorizer::onKeyInserted(const SignOn::Key key)
{
    //Close the secure storage UI 1st
    if (m_uiAdaptor)
        m_uiAdaptor->closeUi();

    if (keyHandler()->keyIsAuthorized(key)) {
        if (m_state == WaitingAuthorized) {
            emit keyAuthorizationQueried(m_queriedKey, Approved);
            /* Notify the user */
            if (m_uiAdaptor)
                m_uiAdaptor->notifyKeyAuthorized();
            closeUi();
            setState(Idle);
        }
    } else if (m_state == AuthorizeNext) {
        m_queriedKey = key;
        setState(WaitingQuery);
    }
}

void UiKeyAuthorizer::onKeyDisabled(const SignOn::Key key)
{
    TRACE();
    if (m_state == SwapWithAuthorized) {
        m_queriedKey = key;
        /* The non authorized key has been removed; we'll now wait for the
         * re-insertion of an authorized key
         */
        setState(WaitingAuthorized);
    }
}

void UiKeyAuthorizer::onLastAuthorizedKeyRemoved(const SignOn::Key key)
{
    Q_UNUSED(key);
    /* Enable the SwapWithAuthorized mechanism and notify the user */
    TRACE() << "All inserted keys disabled, notifying user.";
    if (m_uiAdaptor == 0) {
        m_uiAdaptor =
            new SignonSecureStorageUiAdaptor(SIGNON_UI_SERVICE,
                                             SIGNON_UI_DAEMON_OBJECTPATH,
                                             SIGNOND_BUS);
    }

    connect(m_uiAdaptor,
            SIGNAL(noKeyPresentAccepted()),
            SLOT(onNoKeyPresentAccepted()));
    connect(m_uiAdaptor,
            SIGNAL(uiRejected()),
            SLOT(onSecureStorageUiRejected()));
    connect(m_uiAdaptor,
            SIGNAL(error()),
            SLOT(onSecureStorageUiRejected()));

    m_uiAdaptor->notifyNoKeyPresent();
    setState(AuthorizeNext);
}

void UiKeyAuthorizer::onNoKeyPresentAccepted()
{
    TRACE();
    closeUi();
}

void UiKeyAuthorizer::setState(State state)
{
    if (m_state == state) return;
    TRACE() << "State:" << state;

    switch (state) {
    case Idle:
        m_queriedKey.clear();
        break;
    default:
        break;
    }
    m_state = state;
}

void UiKeyAuthorizer::closeUi()
{
    if (m_uiAdaptor) {
        delete m_uiAdaptor;
        m_uiAdaptor = 0;
    }
}

void UiKeyAuthorizer::onSecureStorageUiRejected()
{
    TRACE();
    closeUi();
    setState(Idle);
}

void UiKeyAuthorizer::onClearPasswordsStorage()
{
    TRACE() << "Asking to reformat secure storage.";

    emit keyAuthorizationQueried(m_queriedKey, Exclusive);

    if (m_uiAdaptor) {
        m_uiAdaptor->notifyStorageCleared();
    }

    closeUi();
    setState(Idle);
}

