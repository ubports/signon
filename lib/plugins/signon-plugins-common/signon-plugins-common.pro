TEMPLATE = lib
TARGET = signon-plugins-common

include( ../../../common-project-config.pri )
include( ../../../common-installs-config.pri )

CONFIG += qt

INCLUDEPATH += ../

SOURCES += SignOn/blobiohandler.cpp 
HEADERS += SignOn/blobiohandler.h

headers.files = $$HEADERS

headers.path = $${INSTALL_PREFIX}/include/signon-plugins/SignOn
INSTALLS += headers

pkgconfig.files = signon-plugins-common.pc
pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig
INSTALLS += pkgconfig

# configuration feature
feature.files = signon-plugins-common.prf
feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS += feature



