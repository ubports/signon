include( ../../common-project-config.pri )
include( $$TOP_SRC_DIR/common-vars.pri )
include( $$TOP_SRC_DIR/common-installs-config.pri )

CONFIG += qtestlib \
          build_all \
          link_pkgconfig
PKGCONFIG += libsignoncrypto-qt
QT += core \
      dbus
QT -= gui

LIBS *= -lsignon-qt

SOURCES += libsignon-qt-tests.cpp \
    $$TOP_SRC_DIR/tests/authsessiontest/testauthsession.cpp \
    $$TOP_SRC_DIR/tests/sso-mt-test/ssotestclient.cpp \
    $$TOP_SRC_DIR/tests/sso-mt-test/testauthserviceresult.cpp \
    $$TOP_SRC_DIR/tests/sso-mt-test/testidentityresult.cpp
HEADERS += \
    $$TOP_SRC_DIR/tests/authsessiontest/testauthsession.h \
    $$TOP_SRC_DIR/src/plugins/ssotest2/ssotest2data.h \
    $$TOP_SRC_DIR/tests/sso-mt-test/ssotestclient.h \
    $$TOP_SRC_DIR/tests/sso-mt-test/testauthserviceresult.h \
    $$TOP_SRC_DIR/tests/sso-mt-test/testidentityresult.h
INCLUDEPATH += . \
    $$TOP_SRC_DIR/lib/plugins \
    $$TOP_SRC_DIR/tests/authsessiontest \
    $$TOP_SRC_DIR/tests/sso-mt-test \
    $$TOP_SRC_DIR/src/plugins/ssotest2
DEFINES += SSO_CI_TESTMANAGEMENT
QMAKE_CXXFLAGS += -fno-exceptions \
    -fno-rtti
TARGET = libsignon-qt-tests

testsuite.path = /usr/share/$$TARGET
testsuite.files = tests.xml
INSTALLS += testsuite pkgconfig

