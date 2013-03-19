TEMPLATE = lib
TARGET = signon-plugins-common

include( ../../../common-project-config.pri )
include($${TOP_SRC_DIR}/common-installs-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

CONFIG += qt

INCLUDEPATH += ../

DEFINES += SIGNON_PLUGIN_TRACE

SOURCES += \
    SignOn/blobiohandler.cpp
HEADERS += \
    SignOn/blobiohandler.h \
    SignOn/ipc.h

headers.files = \
    SignOn/blobiohandler.h

headers.path = $${INSTALL_PREFIX}/include/signon-plugins/SignOn
INSTALLS += headers

greaterThan(QT_MAJOR_VERSION, 4) {
    LIBQTCORE = Qt5Core
} else {
    LIBQTCORE = QtCore
}

pkgconfig.files = signon-plugins-common.pc
include($${TOP_SRC_DIR}/common-pkgconfig.pri)
INSTALLS += pkgconfig

# configuration feature
feature.files = signon-plugins-common.prf
feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS += feature



