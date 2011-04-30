TEMPLATE = lib
TARGET = signon-plugins-common

include( ../../../common-project-config.pri )
include( ../../../common-installs-config.pri )

CONFIG += qt

INCLUDEPATH += ../

DEFINES += SIGNON_PLUGIN_TRACE

SOURCES += \
    SignOn/blobiohandler.cpp \
    SignOn/encrypteddevice.cpp
HEADERS += \
    SignOn/blobiohandler.h \
    SignOn/encrypteddevice.h \
    SignOn/ipc.h

headers.files = \
    SignOn/blobiohandler.h

headers.path = $${INSTALL_PREFIX}/include/signon-plugins/SignOn
INSTALLS += headers

pkgconfig.files = signon-plugins-common.pc
pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig
INSTALLS += pkgconfig

# configuration feature
feature.files = signon-plugins-common.prf
feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS += feature



