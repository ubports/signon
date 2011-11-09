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

DEFINES += SIGNON_PLUGIN_TRACE
INCLUDEPATH += . \
    $$TOP_SRC_DIR/lib/plugins
LIBS += -lsignon-plugins
QMAKE_LIBDIR += \
    $${TOP_BUILD_DIR}/lib/plugins

QMAKE_CXXFLAGS += -fno-exceptions \
    -fno-rtti

headers.path = $${INSTALL_PREFIX}/include/signon-plugins

exists( ../../meego-release ) {
    ARCH = $$system(tail -n1 ../../meego-release)
} else {
    ARCH = $$system(uname -m)
}
contains( ARCH, x86_64 ) {
    pkgconfig.path  = $${INSTALL_PREFIX}/lib64/pkgconfig
} else {
    pkgconfig.path  = $${INSTALL_PREFIX}/lib/pkgconfig
}

target.path = $${INSTALL_PREFIX}/lib/signon
INSTALLS = target
