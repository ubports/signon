TEMPLATE = lib
TARGET = signon-plugins

include( ../../common-project-config.pri )
include( ../../common-installs-config.pri )

CONFIG += static

HEADERS = \
    SignOn/authpluginif.h \
    SignOn/uisessiondata.h \
    SignOn/uisessiondata_priv.h \
    SignOn/securestorageui.h

headers.files = \
    SignOn/AuthPluginInterface \
    SignOn/authpluginif.h \
    SignOn/signonplugincommon.h \
    SignOn/UiSessionData \
    SignOn/uisessiondata.h \
    SignOn/uisessiondata_priv.h \
    SignOn/securestorageui.h
headers.path = $${INSTALL_PREFIX}/include/signon-plugins/SignOn
INSTALLS += headers

pkgconfig.files = signon-plugins.pc
pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig
INSTALLS += pkgconfig

# configuration feature
feature.files = signon-plugins.prf
feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS += feature


