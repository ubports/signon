include( ../../../common-project-config.pri )
include( $${TOP_SRC_DIR}/common-vars.pri )

TEMPLATE = lib
TARGET = signon-extension

HEADERS = \
    abstract-key-authorizer.h \
    abstract-key-manager.h \
    debug.h \
    crypto-handlers.h \
    crypto-manager.h \
    export.h \
    extension-interface.h \
    key-handler.h \
    misc.h

INCLUDEPATH += \
    ..

SOURCES += \
    abstract-key-authorizer.cpp \
    abstract-key-manager.cpp \
    crypto-handlers.cpp \
    crypto-manager.cpp \
    debug.cpp \
    key-handler.cpp \
    misc.cpp

QT += core
QT -= gui

QMAKE_CXXFLAGS += \
    -fno-exceptions \
    -fno-rtti \
    -fvisibility=hidden

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
DEFINES += \
    BUILDING_SIGNON \
    SIGNON_ENABLE_UNSTABLE_APIS \
    SIGNON_TRACE

include( $${TOP_SRC_DIR}/common-installs-config.pri )

headers.files = \
    AbstractKeyAuthorizer \
    abstract-key-authorizer.h \
    AbstractKeyManager \
    abstract-key-manager.h \
    Debug \
    debug.h \
    crypto-manager.h \
    export.h \
    ExtensionInterface \
    extension-interface.h \
    KeyHandler \
    key-handler.h
headers.path = $${INSTALL_PREFIX}/include/$${TARGET}/SignOn
INSTALLS += headers

pkgconfig.files = SignOnExtension.pc
pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig
INSTALLS += pkgconfig
