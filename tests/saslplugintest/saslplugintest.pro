include( ../../common-project-config.pri )
include( ../../common-vars.pri )

TARGET = signon-saslplugin-tests

QT += core
CONFIG += qtestlib \
    link_pkgconfig

DEFINES += SIGNON_PLUGIN_TRACE

SOURCES += saslplugintest.cpp

HEADERS += saslplugintest.h \
           $${TOP_SRC_DIR}/src/plugins/sasl/saslplugin.h \
           $${TOP_SRC_DIR}/lib/plugins/SignOn/authpluginif.h

INCLUDEPATH += \
    $${TOP_SRC_DIR}/src/plugins/sasl \
    $${TOP_SRC_DIR}/lib/plugins

LIBS += -lsasl2

QMAKE_CXXFLAGS += -fno-exceptions \
                  -fno-rtti
                  
target.path = /usr/bin

testsuite.path  = /usr/share/$$TARGET
testsuite.files = tests.xml

INSTALLS += target \
            testsuite
