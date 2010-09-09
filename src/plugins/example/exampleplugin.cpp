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

#include "exampleplugin.h"
#include "exampledata.h"
#include "SignOn/signonplugincommon.h"

using namespace SignOn;

namespace ExamplePluginNS {

    ExamplePlugin::ExamplePlugin(QObject *parent)
    : AuthPluginInterface(parent)
    {
        TRACE();

    }

    ExamplePlugin::~ExamplePlugin()
    {
        TRACE();
   }

    QString ExamplePlugin::type() const
    {
        return QLatin1String("example");
    }

    QStringList ExamplePlugin::mechanisms() const
    {
        QStringList res = QStringList(QLatin1String("default"));
        res << QLatin1String("example");

        return res;
    }

    void ExamplePlugin::cancel()
    {
    }
    /*
     * example plugin is used for testing purposes only
     * */
    void ExamplePlugin::process(const SignOn::SessionData &inData,
                                const QString &mechanism )
    {
        ExampleData response;
        ExampleData input = inData.data<ExampleData>();

        if (!mechanisms().contains(mechanism) ) {
            TRACE() << "invalid mechanism: " << mechanism;
            // next is commented to allow invalid machanism to be used
            /*
            emit error(PLUGIN_ERROR_MECHANISM_NOT_SUPPORTED);
            return;
            */
        }
        TRACE() << "User: " << inData.UserName() ;
        TRACE() << "Example" << input.Example();

        if ( inData.UserName() == QLatin1String("Example" ) ) {
            qDebug() << inData.UserName();
            response.setExample(QLatin1String("authenticated"));
            emit result(response);
            return;
        }

        if ( inData.UserName() == QLatin1String("error" ) ) {
            emit error(Error::NotAuthorized);
        }

        if ( inData.UserName() == QLatin1String("store" ) ) {
            ExampleData storeData;
            storeData.setExample(QLatin1String("store:")+input.Example());
            emit store(storeData);
        }

        if ( inData.UserName() == QLatin1String("url" ) ) {
            SignOn::UiSessionData data;
            data.setOpenUrl(input.Example());
            data.setNetworkProxy(inData.NetworkProxy());
            emit userActionRequired(data);

            return;
        }
        response.setExample(QLatin1String("authenticated"));
        TRACE() << "Emitting results";

        emit store(response);
        emit result(response);
        return;
    }


    void ExamplePlugin::userActionFinished(const SignOn::UiSessionData &data)
    {
        Q_UNUSED(data);
        ExampleData response;
        TRACE();
        response.setExample(QLatin1String("url shown"));
        emit result(response);

    }
/*
    void ExamplePlugin::refresh(const SignOn::UiSessionData &data)
    {
    }
*/
    SIGNON_DECL_AUTH_PLUGIN(ExamplePlugin)

} //namespace ExamplePluginNS
