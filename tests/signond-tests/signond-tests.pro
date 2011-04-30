include( ../../common-project-config.pri )
include( $$TOP_SRC_DIR/common-vars.pri )

CONFIG += \
    qtestlib \
    qdbus \
    link_pkgconfig

QT += core \
    sql \
    xml \
    network \
    dbus

QT -= gui

PKGCONFIG += \
    libsignoncrypto-qt

LIBS += -L/usr/lib \
        -lcreds \
        -lcrypto \
        -lsignon-qt

#DEFINES += CAM_UNIT_TESTS_FIXED

DEFINES += TESTS_TRACE
DEFINES += SIGNON_PLUGIN_TRACE

HEADERS += \
    timeouts.h \
    $$TOP_SRC_DIR/src/signond/pluginproxy.h \
    $$TOP_SRC_DIR/tests/pluginproxytest/testpluginproxy.h \
    backuptest.h \
    databasetest.h \
    $$TOP_SRC_DIR/src/signond/credentialsdb.h \
    $${TOP_SRC_DIR}/lib/plugins/signon-plugins-common/SignOn/blobiohandler.h \
    $${TOP_SRC_DIR}/lib/plugins/signon-plugins-common/SignOn/encrypteddevice.h

SOURCES = \
    signond-tests.cpp \
    timeouts.cpp \
    $$TOP_SRC_DIR/tests/pluginproxytest/testpluginproxy.cpp \
    $$TOP_SRC_DIR/tests/pluginproxytest/include.cpp \
    backuptest.cpp \
    databasetest.cpp \
    $${TOP_SRC_DIR}/lib/plugins/signon-plugins-common/SignOn/encrypteddevice.cpp \
           $$TOP_SRC_DIR/src/signond/credentialsdb.cpp

contains(DEFINES, CAM_UNIT_TESTS_FIXED) {
 HEADERS *=$$TOP_SRC_DIR/tests/credentialsaccessmanagertest/cam-test-server/credentialsaccessmanagertest.h \
           $$TOP_SRC_DIR/tests/credentialsaccessmanagertest/cam-test-server/dbuspeer.h \
           $$TOP_SRC_DIR/tests/credentialsaccessmanagertest/defs.h \
           $$TOP_SRC_DIR/src/signond/credentialsaccessmanager.h \
           $$TOP_SRC_DIR/src/signond/accesscodehandler.h \
           $$TOP_SRC_DIR/src/signond/simdbusadaptor.h \
           $$TOP_SRC_DIR/src/signond/cryptomanager.h \
           $$TOP_SRC_DIR/src/signond/credentialsdb.h

 SOURCES *= $$TOP_SRC_DIR/tests/credentialsaccessmanagertest/cam-test-server/credentialsaccessmanagertest.cpp \
            $$TOP_SRC_DIR/tests/credentialsaccessmanagertest/cam-test-server/includes.cpp \
            $$TOP_SRC_DIR/tests/credentialsaccessmanagertest/cam-test-server/dbuspeer.cpp
}

TARGET = signon-tests

INCLUDEPATH += . \
    $$TOP_SRC_DIR/lib/plugins \
    $$TOP_SRC_DIR/tests/pluginproxytest \
    $$TOP_SRC_DIR/src/signond \
    $${TOP_SRC_DIR}/lib/plugins/signon-plugins-common \
    $${TOP_SRC_DIR}/lib/plugins/signon-plugins-common/SignOn \
    $$TOP_SRC_DIR/tests/credentialsaccessmanagertest/cam-test-server

DEFINES += SSO_CI_TESTMANAGEMENT

QMAKE_CXXFLAGS += -fno-exceptions \
    -fno-rtti

target.path = /usr/bin
scripts.path = /usr/bin
scripts.files += signonremoteplugin-test.sh

testsuite.path  = /usr/share/signond-tests
testsuite.files = tests.xml

INSTALLS += target \
            testsuite \
            scripts
