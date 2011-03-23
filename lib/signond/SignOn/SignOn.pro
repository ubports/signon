include( ../../../common-project-config.pri )
include( $${TOP_SRC_DIR}/common-vars.pri )

TEMPLATE = lib
TARGET = signon-extension

HEADERS = \
    abstract-key-manager.h \
    extension-interface.h \
    secure-storage-ui.h

INCLUDEPATH += \
    ..

SOURCES += \
    abstract-key-manager.cpp

QT += core
QT -= gui

QMAKE_CXXFLAGS += \
    -fno-exceptions \
    -fno-rtti

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
include( $${TOP_SRC_DIR}/common-installs-config.pri )

headers.files = \
    AbstractKeyManager \
    abstract-key-manager.h \
    ExtensionInterface \
    extension-interface.h \
    SecureStorageUI \
    secure-storage-ui.h
headers.path = $${INSTALL_PREFIX}/include/$${TARGET}/SignOn
INSTALLS += headers

pkgconfig.files = SignOnExtension.pc
pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig
INSTALLS += pkgconfig
