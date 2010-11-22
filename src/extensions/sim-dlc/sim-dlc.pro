include( ../../../common-project-config.pri )
include( $${TOP_SRC_DIR}/common-vars.pri )
include( $${TOP_SRC_DIR}/common-installs-config.pri )

TEMPLATE = lib
TARGET = sim-dlc

CONFIG += \
    qt \
    plugin

QT += \
    core \
    dbus
QT -= gui

PKGCONFIG += CellularQt

INCLUDEPATH += . \
    $${TOP_SRC_DIR}/lib/signond
LIBS += -lsignon-extension


HEADERS = \
    key-manager.h \
    sim-dlc-plugin.h

SOURCES = \
    key-manager.cpp \
    sim-dlc-plugin.cpp

target.path = $${INSTALL_PREFIX}/lib/signon/extensions
INSTALLS = target

