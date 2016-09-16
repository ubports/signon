include(../../common-project-config.pri)

TEMPLATE = app
TARGET = session_tool

QT += core dbus
QT -= gui

INCLUDEPATH += \
    $${TOP_SRC_DIR}/lib

SOURCES = \
    session_tool.cpp
