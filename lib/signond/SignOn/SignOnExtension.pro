include( ../../../common-project-config.pri )
include( $${TOP_SRC_DIR}/common-vars.pri )

TEMPLATE = lib
TARGET = signon-extension

HEADERS = \
    abstract-key-manager.h \
    debug.h \
    extension-interface.h

INCLUDEPATH += \
    ..

SOURCES += \
    abstract-key-manager.cpp \
    debug.cpp

QT += core
QT -= gui

QMAKE_CXXFLAGS += \
    -fno-exceptions \
    -fno-rtti

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
DEFINES += \
    SIGNON_TRACE

include( $${TOP_SRC_DIR}/common-installs-config.pri )

headers.files = \
    AbstractKeyManager \
    abstract-key-manager.h \
    Debug \
    debug.h \
    ExtensionInterface \
    extension-interface.h
headers.path = $${INSTALL_PREFIX}/include/$${TARGET}/SignOn
INSTALLS += headers

pkgconfig.files = SignOnExtension.pc
pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig
INSTALLS += pkgconfig
