include( ../../common-project-config.pri )
include( ../../common-vars.pri )
include( ../../common-installs-config.pri )

TEMPLATE = lib

QT += core
QT -= gui

CONFIG += \
    plugin \
    build_all \
    warn_on \
    link_pkgconfig \
    thread

INCLUDEPATH += . \
    $$TOP_SRC_DIR/lib/plugins
LIBS += -lsignon-plugins
QMAKE_LIBDIR += \
    $${TOP_BUILD_DIR}/lib/plugins

QMAKE_CXXFLAGS += -fno-exceptions \
    -fno-rtti

headers.path = $${INSTALL_PREFIX}/include/signon-plugins

pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig

target.path = $${INSTALL_PREFIX}/lib/signon
INSTALLS = target
