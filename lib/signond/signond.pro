include( ../../common-project-config.pri )
include( $${TOP_SRC_DIR}/common-installs-config.pri )
include( $${TOP_SRC_DIR}/common-vars.pri )

TEMPLATE = subdirs
SUBDIRS = SignOn

# Input
public_headers += \
    signoncommon.h

HEADERS = $$public_headers

INCLUDEPATH += . \
    $$TOP_SRC_DIR/include

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES = \
    com.google.code.AccountsSSO.SingleSignOn.AuthService.xml \
    com.google.code.AccountsSSO.SingleSignOn.AuthSession.xml \
    com.google.code.AccountsSSO.SingleSignOn.Identity.xml

headers.files = $$public_headers
headers.path = $${INSTALL_PREFIX}/include/signond

dbus_files.files = $$OTHER_FILES
dbus_files.path =$${INSTALL_PREFIX}/share/dbus-1/interfaces

PKGCONFIG_VARS = INSTALL_PREFIX INSTALL_LIBDIR PROJECT_VERSION
COMMAND = "cat signond.pc.in "
for(var, PKGCONFIG_VARS) {
   eval(VALUE = \$\${$${var}})
   COMMAND += "| sed s,$${var},$${VALUE},"
}
COMMAND += " > signond.pc"

pkgconfig.CONFIG = no_check_exist
pkgconfig.files = signond.pc
pkgconfig.path  = $${INSTALL_LIBDIR}/pkgconfig
pkgconfig.commands = $${COMMAND}
QMAKE_EXTRA_TARGETS += pkgconfig

INSTALLS += headers dbus_files pkgconfig

