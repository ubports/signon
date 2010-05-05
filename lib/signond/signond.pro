include( ../../common-project-config.pri )
include( ../../common-installs-config.pri )

TEMPLATE = subdirs

# Input
public_headers += \
    signoncommon.h

HEADERS = $$public_headers

INCLUDEPATH += . \
    $$TOP_SRC_DIR/include

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES = \
    com.nokia.singlesignon.SignonAuthSession.xml \
    com.nokia.singlesignon.SignonDaemon.xml \
    com.nokia.singlesignon.SignonIdentity.xml

headers.files = $$public_headers 
headers.path = $${INSTALL_PREFIX}/include/signond

dbus_files.files = $$OTHER_FILES
dbus_files.path =$${INSTALL_PREFIX}/share/dbus-1/interfaces

pkgconfig.files = signond.pc
pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig

INSTALLS += headers dbus_files pkgconfig

