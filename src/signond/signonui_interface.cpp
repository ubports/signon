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

#include "signonui_interface.h"

#include "SignOn/uisessiondata_priv.h"
#include "SignOn/secure-storage-ui.h"

/*
 * Implementation of interface class SignonUiAdaptor
 */

SignonUiAdaptor::SignonUiAdaptor(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}

SignonUiAdaptor::~SignonUiAdaptor()
{
}

/*
 * Open a new dialog
 * */

QDBusPendingCall SignonUiAdaptor::queryDialog(const QVariantMap &parameters)
{
    QList<QVariant> argumentList;
    argumentList << parameters;
    return callWithArgumentListAndBigTimeout(QLatin1String("queryDialog"), argumentList);
}


/*
 * update the existing dialog
 * */
QDBusPendingCall SignonUiAdaptor::refreshDialog(const QVariantMap &parameters)
{
    QList<QVariant> argumentList;
    argumentList << parameters;
    return callWithArgumentListAndBigTimeout(QLatin1String("refreshDialog"), argumentList);
}


/*
 * cancel dialog request
 * */
void SignonUiAdaptor::cancelUiRequest(const QString &requestId)
{
    QList<QVariant> argumentList;
    argumentList << requestId;
    callWithArgumentList(QDBus::NoBlock, QLatin1String("cancelUiRequest"), argumentList);
}

QDBusPendingCall SignonUiAdaptor::callWithArgumentListAndBigTimeout(const QString &method,
                                                         const QList<QVariant> &args)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(service(),
                                                      path(),
                                                      interface(),
                                                      method);
    if (!args.isEmpty())
        msg.setArguments(args);
    return connection().asyncCall(msg, SIGNOND_MAX_TIMEOUT);
}

/* -------------- SignonSecureStorageUiAdaptor -------------- */

SignonSecureStorageUiAdaptor::SignonSecureStorageUiAdaptor(
    const QString &service,
    const QString &path,
    const QDBusConnection &connection,
    QObject *parent)
        : QObject(parent),
          uiAdaptor(new SignonUiAdaptor(service, path, connection, this)),
          isBusy(false)
{
}

SignonSecureStorageUiAdaptor::~SignonSecureStorageUiAdaptor()
{
}


void SignonSecureStorageUiAdaptor::displaySecureStorageUi(const QVariantMap &parameters)
{
    if (isBusy) {
        TRACE() << "Adaptor busy with ongoing call.";
        return;
    }

    TRACE() << "Calling signon-ui - display storage notification";

    QLatin1String method("displaySecureStorageUi");

    QList<QVariant> argumentList;
    argumentList << parameters;

    QDBusPendingCall call = uiAdaptor->callWithArgumentListAndBigTimeout(
        method, argumentList);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(callFinished(QDBusPendingCallWatcher *)));
    isBusy = true;
}

void SignonSecureStorageUiAdaptor::notifyNoKeyPresent()
{
    QVariantMap parameters;
    parameters[SSOUI_SECURE_STORAGE_REQUEST_TYPE] = SignOn::NoKeyPresent;
    parameters[SSOUI_KEY_REQUESTID] = SSOUI_SECURE_STORAGE_REQUEST_ID;
    displaySecureStorageUi(parameters);
}

void SignonSecureStorageUiAdaptor::notifyNoAuthorizedKeyPresent()
{
    QVariantMap parameters;
    parameters[SSOUI_SECURE_STORAGE_REQUEST_TYPE] = SignOn::NoAuthorizedKeyPresent;
    parameters[SSOUI_KEY_REQUESTID] = SSOUI_SECURE_STORAGE_REQUEST_ID;
    displaySecureStorageUi(parameters);
}

void SignonSecureStorageUiAdaptor::notifyKeyAuthorized()
{
    QVariantMap parameters;
    parameters[SSOUI_SECURE_STORAGE_REQUEST_TYPE] = SignOn::KeyAuthorized;
    parameters[SSOUI_KEY_REQUESTID] = SSOUI_SECURE_STORAGE_REQUEST_ID;
    displaySecureStorageUi(parameters);
}

void SignonSecureStorageUiAdaptor::notifyStorageCleared()
{
    QVariantMap parameters;
    parameters[SSOUI_SECURE_STORAGE_REQUEST_TYPE] = SignOn::StorageCleared;
    parameters[SSOUI_KEY_REQUESTID] = SSOUI_SECURE_STORAGE_REQUEST_ID;
    displaySecureStorageUi(parameters);
}

void SignonSecureStorageUiAdaptor::closeUi()
{
    TRACE() << "Closing secure storage UI.";
    uiAdaptor->cancelUiRequest(SSOUI_SECURE_STORAGE_REQUEST_ID);
    isBusy = false;
}

void SignonSecureStorageUiAdaptor::callFinished(QDBusPendingCallWatcher *call)
{
    isBusy = false;
    QDBusPendingReply<QVariantMap> reply = *call;
    delete call;
    call = 0;

    TRACE() << reply.isError() << reply.count();

    if (reply.isError() || (reply.count() == 0)) {
        TRACE();
        emit error();
        return;
    }

    QVariantMap resultParameters = reply.argumentAt<0>();
    if (!resultParameters.contains(SSOUI_SECURE_STORAGE_REPLY_TYPE)) {
        emit error();
        return;
    }

    bool isOk = false;
    SignOn::UiReplyType replyType = static_cast<SignOn::UiReplyType>(
        resultParameters[SSOUI_SECURE_STORAGE_REPLY_TYPE].toInt(&isOk));

    if (!isOk) {
        TRACE() << "Integer cast failed.";
        emit error();
        return;
    }

    switch (replyType) {
        case SignOn::NoKeyPresentAccepted:
            emit noKeyPresentAccepted();
            break;
        case SignOn::ClearPasswordsStorage:
            emit clearPasswordsStorage();
            break;
        case SignOn::UiRejected:
            emit uiClosed();
            break;
        default:
            emit error();
            break;
    }
}
