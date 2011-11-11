include( ../tests.pri )
include( $$TOP_SRC_DIR/common-installs-config.pri )

CONFIG += qtestlib \
          build_all \
          link_pkgconfig
QT += core \
      dbus
QT -= gui

LIBS *= -lsignon-qt
QMAKE_RPATHDIR = $${QMAKE_LIBDIR}

SOURCES += \
    testauthsession.cpp \
    testthread.cpp \
    ssotestclient.cpp \
    testauthserviceresult.cpp \
    testidentityresult.cpp
HEADERS += \
    testauthsession.h \
    testthread.h \
    ssotestclient.h \
    testauthserviceresult.h \
    testidentityresult.h \
    $$TOP_SRC_DIR/src/plugins/ssotest2/ssotest2data.h
INCLUDEPATH += . \
    $$TOP_SRC_DIR/lib/plugins \
    $$TOP_SRC_DIR/src/plugins/ssotest2
DEFINES += SSO_CI_TESTMANAGEMENT
QMAKE_CXXFLAGS += -fno-exceptions \
    -fno-rtti
TARGET = libsignon-qt-tests

testsuite.path = /usr/share/$$TARGET
testsuite.files = tests.xml
INSTALLS += testsuite pkgconfig
