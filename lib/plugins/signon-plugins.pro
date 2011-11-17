TEMPLATE = lib
TARGET = signon-plugins

include( ../../common-project-config.pri )
include( ../../common-installs-config.pri )

CONFIG += static

HEADERS = \
    SignOn/authpluginif.h \
    SignOn/uisessiondata.h \
    SignOn/uisessiondata_priv.h \
    SignOn/securestorageui.h

headers.files = \
    SignOn/AuthPluginInterface \
    SignOn/authpluginif.h \
    SignOn/signonplugincommon.h \
    SignOn/UiSessionData \
    SignOn/uisessiondata.h \
    SignOn/uisessiondata_priv.h
headers.path = $${INSTALL_PREFIX}/include/signon-plugins/SignOn
INSTALLS += headers

PKGCONFIG_VARS = INSTALL_PREFIX INSTALL_LIBDIR SIGNOND_PLUGINS_DIR
COMMAND = "cat signon-plugins.pc.in "
for(var, PKGCONFIG_VARS) {
   eval(VALUE = \$\${$${var}})
   COMMAND += "| sed s,$${var},$${VALUE},"
}
COMMAND += " > signon-plugins.pc"

pkgconfig.files = signon-plugins.pc
pkgconfig.path  = $${INSTALL_LIBDIR}/pkgconfig
pkgconfig.commands = $${COMMAND}
QMAKE_EXTRA_TARGETS += pkgconfig
INSTALLS += pkgconfig

# configuration feature
feature.files = signon-plugins.prf
feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS += feature


