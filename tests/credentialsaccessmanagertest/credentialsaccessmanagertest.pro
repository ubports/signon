include( ../../common-project-config.pri )
include( ../../common-vars.pri )

TARGET = signoncamtest

QT += core sql dbus

QT -= gui

CONFIG += qtestlib link_pkgconfig

LIBS += \
    -lcreds \
    -lcryptsetup
SOURCES += \
    credentialsaccessmanagertest.cpp \
    includes.cpp
HEADERS += \
    credentialsaccessmanagertest.h \
    $$TOP_SRC_DIR/src/signond/credentialsaccessmanager.h \
    $$TOP_SRC_DIR/src/signond/accesscodehandler.h \
    $$TOP_SRC_DIR/src/signond/cryptohandlers.h \
    $$TOP_SRC_DIR/src/signond/cryptomanager.h \
    $$TOP_SRC_DIR/src/signond/credentialsdb.h
INCLUDEPATH += \
    $$TOP_SRC_DIR/src/signond \
    $$TOP_SRC_DIR/include

PKGCONFIG += CellularQt

QMAKE_CXXFLAGS += -fno-exceptions -fno-rtti

include( ../../common-installs-config.pri )
