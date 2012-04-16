# Include this file after defining the pkgconfig.files variable

!isEmpty(pkgconfig.files) {
    PKGCONFIG_VARS = INSTALL_PREFIX INSTALL_LIBDIR PROJECT_VERSION \
        SIGNOND_PLUGINS_DIR SIGNOND_EXTENSIONS_DIR
    COMMAND = "cat $${pkgconfig.files}.in "
    for(var, PKGCONFIG_VARS) {
        eval(VALUE = \$\${$${var}})
        COMMAND += "| sed s,$${var},$${VALUE},"
    }
    COMMAND += " > $${pkgconfig.files}"

    pkgconfig.CONFIG = no_check_exist
    pkgconfig.path  = $${INSTALL_LIBDIR}/pkgconfig
    pkgconfig.commands = $${COMMAND}
    QMAKE_EXTRA_TARGETS += pkgconfig
}
