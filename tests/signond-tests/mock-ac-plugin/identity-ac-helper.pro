include(../../../common-project-config.pri)

TEMPLATE = app
TARGET = identity-ac-helper

QT += core
QT -= gui

LIBS += -lsignon-qt5

QMAKE_LIBDIR += \
    $${TOP_BUILD_DIR}/lib/signond/SignOn \
    $${TOP_BUILD_DIR}/lib/SignOn
QMAKE_RPATHDIR = $${QMAKE_LIBDIR}

INCLUDEPATH += \
    $${TOP_SRC_DIR}/lib

SOURCES = \
    identity-ac-helper.cpp
