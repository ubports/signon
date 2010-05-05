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

/*!
 * @todo move this to a common includes folder.
 * This is part of the plugin development kit, too.
 */

#ifndef SESSIONDATA_H
#define SESSIONDATA_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <SignOn/libsignoncommon.h>

namespace SignOn {

/*!
 * Macro to create declarations for parameter setter and getter.
 * This supports same types as QVariant @see QVariant.
 * For user specified types @see QMetaType.
 *
 * @param type_ type of parameter
 * @param name_ of property
 */
#define SIGNON_SESSION_DECLARE_PROPERTY(type_, name_) \
          void set##name_(const type_ &value ) { m_data.insert(QLatin1String(#name_), value); } \
          type_ name_() const { return m_data.value(QLatin1String(#name_)).value<type_>(); }

/*!
 * Property which holds the access control tokens that the requesting application has.
 * @note to be used by the plugins developers only.
 */
#define SSO_ACCESS_CONTROL_TOKENS QLatin1String("AccessControlTokens")

/*!
 * @enum SignonUiPolicy
 * Policy to define how plugin should interact with user.
 * This is hint for plugin how to handle user interaction.
 * NoUserInteractionPolicy do not allow any ui interaction to happen
 * and plugin will get error reply QUERY_ERROR_FORBIDDEN.
 * @see UiPolicy
 */
enum SignonUiPolicy {
    DefaultPolicy = 0,          /**< Plugin can decide when to show ui. */
    RequestPasswordPolicy,      /**< Force user to enter password. */
    NoUserInteractionPolicy     /**< No ui elements are shown to user. */
};

/*!
 * @class SessionData
 * @headerfile sessiondata.h SignOn/SessionData
 *
 * Data container to hold values for authentication session.
 * Inherit this class if you want to extend the property range.
 *
 *
 * @warning All this class' definitions must be inline.
 */
class SIGNON_EXPORT SessionData
{
public:
    /*!
     * Constructor. Creates a SessionData with data 'data'.
     * @param data The data to be contained by the SessionData.
     * @attention internal use only recommended. As a SSO client application developer
     *            use setters/gettters for specific SessionData properties.
     */
    SessionData(const QVariantMap &data = QVariantMap()) { m_data = data; }

    /*!
     * Copy constructor.
     * @param other SessionData object to be copyed to this instance.
     */
    SessionData(const SessionData &other) { m_data = other.m_data; }

    /*!
     * Assignment operator
     * @param other SessionData object to be assigned to this instance.
     * @returns reference to this object
     */
    SessionData &operator=(const SessionData &other) {
        m_data = other.m_data;
        return *this;
    }

    /*!
     * Access the list of runtime existing properties of the SessionData
     * @returns a string list containing the property names.
     */
    const QStringList propertyNames() const {
        return m_data.keys();
    }

    /*!
     * Access the list of runtime existing properties of the SessionData
     * @param propertyName Name of the property to be accessed
     * @returns a variant containing the property value of propertyName, or an empty variant if
     *          property does not exist at runtime.
     */
    const QVariant getProperty(const QString &propertyName) const {
        return m_data.value(propertyName, QVariant());
    }

    /*!
     * Gets the access control tokens that the requesting application has.
     * @note to be used by the plugins developers only.
     */
    QStringList getAccessControlTokens() const {
        return getProperty(SSO_ACCESS_CONTROL_TOKENS).toStringList();
    }

    /*!
     * Creates an instance of type T, which must be derived from SessionData.
     * The instance will contain the data of this instance.
     * @returns an instance of type T, containing the data of this instance.
     */
    template <class T> T data() const {
        T dataImpl;
        dataImpl.m_data = m_data;
        return dataImpl;
    }

    /*!
     * Declare property Secret setter and getter.
     * setSecret(const QString secret);
     * const QString Secret() const;
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Secret)

    /*!
     * Declare property UserName setter and getter.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, UserName)

    /*!
     * Declare property Realm setter and getter
     * Realm that is used for authentication.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, Realm)

    /*!
     * Declare property NetworkProxy setter and getter
     * Network proxy to be used instead of system default.
     */
    SIGNON_SESSION_DECLARE_PROPERTY(QString, NetworkProxy)

    /*!
     * Declare property UiPolicy setter and getter
     * Use UiPolicy to define how plugin should interact with user
     * @see SignonUiPolicy
     */
    SIGNON_SESSION_DECLARE_PROPERTY(int, UiPolicy)

protected:
    QVariantMap m_data;
};

} //namespace SignOn

Q_DECLARE_METATYPE(SignOn::SessionData)
#endif // SESSIONDATA_H
