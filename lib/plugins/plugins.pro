TEMPLATE = lib
TARGET = signon-plugins

include( ../../common-project-config.pri )
include( ../../common-installs-config.pri )

CONFIG += static

HEADERS = \
    SignOn/authpluginif.h

headers.files = \
    SignOn/authpluginif.h \
    SignOn/signonplugin.h \
    SignOn/UiSessionData \
    SignOn/uisessiondata.h \
    SignOn/uisessiondata_priv.h
headers.path = $${INSTALL_PREFIX}/include/signon-plugins/SignOn
INSTALLS += headers

pkgconfig.files = signon-plugins.pc
pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig
INSTALLS += pkgconfig

# configuration feature
feature.files = signon-plugins.prf
feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS += feature


