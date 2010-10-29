include( ../../common-project-config.pri )

include( ../../common-vars.pri )

CONFIG += qtestlib \
    link_pkgconfig
PKGCONFIG += libsignoncrypto-qt

QT += core \
      dbus
      
QT -= gui

LIBS += -lsignon-qt

SOURCES = testauthsession.cpp


HEADERS += testauthsession.h \
           $${TOP_SRC_DIR}/src/plugins/ssotest2/ssotest2data.h

TARGET = testauthsession

INCLUDEPATH += . \
    $${TOP_SRC_DIR}/lib/plugins \
    $${TOP_SRC_DIR}/src/plugins/ssotest2
