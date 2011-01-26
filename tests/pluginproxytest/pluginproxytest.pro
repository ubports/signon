include( ../../common-project-config.pri )
include( ../../common-vars.pri )

CONFIG += qtestlib

QT += core dbus
QT -= gui

SOURCES =  testpluginproxy.cpp \
                  include.cpp

HEADERS += testpluginproxy.h \
           $${TOP_SRC_DIR}/src/signond/pluginproxy.h \
           $${TOP_SRC_DIR}/lib/plugins/signon-plugins-common/SignOn/blobiohandler.h \
           $${TOP_SRC_DIR}/lib/plugins/SignOn/authpluginif.h

TARGET = testpluginproxy

INCLUDEPATH += . \
    $${TOP_SRC_DIR}/lib/plugins \
    $${TOP_SRC_DIR}/lib/plugins/signon-plugins-common \
    $${TOP_SRC_DIR}/lib/plugins/signon-plugins-common/SignOn \
    $${TOP_SRC_DIR}/src/plugins \
    $${TOP_SRC_DIR}/src/signond

