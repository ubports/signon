include( ../../common-project-config.pri )
include( ../../common-vars.pri )
CONFIG += qtestlib
QT += core \
    dbus
QT -= gui
LIBS += -lsignon-qt
SOURCES = testsinglesignon.cpp \
    testidentityresult.cpp \
    ssoclientthread.cpp \
    ssotestclient.cpp \
    testauthserviceresult.cpp
HEADERS += \
    testidentityresult.h \
    ssoclientthread.h \
    ssotestclient.h \
    testauthserviceresult.h \
    testsinglesignon.h
TARGET = testsinglesignon
INCLUDEPATH += . \
    $${TOP_SRC_DIR}/lib/plugins

#OPTIONAL: IF THE AUTHSESSION TESTS WILL APPEAR TO BE PROBLEMATIC
#THEN THIS DEFINE COULD BE DISABLED
DEFINES *= SSOTESTCLIENT_USES_AUTHSESSION

contains(DEFINES, SSOTESTCLIENT_USES_AUTHSESSION) {
     INCLUDEPATH += ../authsessiontest \
                    $$TOP_SRC_DIR/src/plugins/ssotest2

     HEADERS += ../authsessiontest/testauthsession.h \
                $$TOP_SRC_DIR/src/plugins/ssotest2/ssotest2data.h

     SOURCES += ../authsessiontest/testauthsession.cpp
}
