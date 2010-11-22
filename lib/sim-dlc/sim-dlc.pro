include( ../../common-project-config.pri )
include( $$TOP_SRC_DIR/common-installs-config.pri )

TEMPLATE = subdirs

# Input
public_headers += \
    sim-dlc.h

HEADERS = $$public_headers

INCLUDEPATH += . \
    $$TOP_SRC_DIR/include

headers.files = $$public_headers
headers.path = $${INSTALL_PREFIX}/include/signond/extensions/

dbus_files.files = com.nokia.SingleSignOn.DeviceLock.xml
dbus_files.path =$${INSTALL_PREFIX}/share/dbus-1/interfaces

pkgconfig.files = sim-dlc.pc
pkgconfig.path = $${INSTALL_PREFIX}/lib/pkgconfig

INSTALLS += headers dbus_files pkgconfig

