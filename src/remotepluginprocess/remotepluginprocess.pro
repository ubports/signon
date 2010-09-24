include( ../../common-project-config.pri )
include( ../../common-vars.pri )
TEMPLATE = app
TARGET = signonpluginprocess
QT += core network
QT -= gui

HEADERS += remotepluginprocess.h

SOURCES += main.cpp \
           remotepluginprocess.cpp

INCLUDEPATH += . \
               $$TOP_SRC_DIR/src \
               $$TOP_SRC_DIR/src/plugins \
               $$TOP_SRC_DIR/src/signond \
               $$TOP_SRC_DIR/lib/plugins

CONFIG += \
    build_all \
    link_pkgconfig

PKGCONFIG += gq-gconf

QMAKE_CXXFLAGS += -fno-exceptions \
                  -fno-rtti

#DEFINES += QT_NO_CAST_TO_ASCII \
#    QT_NO_CAST_FROM_ASCII
DEFINES += SIGNON_PLUGIN_TRACE

include( ../../common-installs-config.pri )
