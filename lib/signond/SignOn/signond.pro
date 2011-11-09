include( ../../../common-project-config.pri )
include( $${TOP_SRC_DIR}/common-vars.pri )

TEMPLATE = lib
TARGET = signond

HEADERS = \
    secure-storage-ui.h

INCLUDEPATH += \
    ..

QT += core
QT -= gui

QMAKE_CXXFLAGS += \
    -fno-exceptions \
    -fno-rtti

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
include( $${TOP_SRC_DIR}/common-installs-config.pri )

headers.files = \
    SecureStorageUI \
    secure-storage-ui.h
headers.path = $${INSTALL_PREFIX}/include/$${TARGET}/SignOn
INSTALLS += headers

pkgconfig.files = SignOnExtension.pc
exists( ../../../meego-release ) {
    ARCH = $$system(tail -n1 ../../../meego-release)
} else {
    ARCH = $$system(uname -m)
}
contains( ARCH, x86_64 ) {
    pkgconfig.path  = $${INSTALL_PREFIX}/lib64/pkgconfig
} else {
    pkgconfig.path  = $${INSTALL_PREFIX}/lib/pkgconfig
}
INSTALLS += pkgconfig
