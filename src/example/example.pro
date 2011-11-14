include( ../../common-project-config.pri )
include( ../../common-vars.pri )

TEMPLATE = app
TARGET = signonclient
DEPENDPATH += .
INCLUDEPATH += . \
    $$TOP_SRC_DIR/src/plugins/example

CONFIG += uitools \
    debug \
    link_pkgconfig

CONFIG += qdbus

# Input
FORMS += signonclient.ui
HEADERS += signonclient.h \
    $$TOP_SRC_DIR/src/plugins/example/exampledata.h
SOURCES += main.cpp \
    signonclient.cpp

QMAKE_LIBDIR += $${TOP_SRC_DIR}/lib/SignOn
LIBS += -lsignon-qt

# install
include( ../../common-installs-config.pri )
example.path = $${INSTALL_PREFIX}/share/doc/libsignon-qt-dev/example
example.files = $${FORMS} $${HEADERS} $${SOURCES}
INSTALLS += example
