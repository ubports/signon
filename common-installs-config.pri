#-----------------------------------------------------------------------------
# Common installation configuration for all projects.
#-----------------------------------------------------------------------------


#-----------------------------------------------------------------------------
# setup the installation prefix
#-----------------------------------------------------------------------------
INSTALL_PREFIX = /usr  # default installation prefix

# default prefix can be overriden by defining PREFIX when running qmake
isEmpty( PREFIX ) {
    message("====")
    message("==== NOTE: To override the installation path run: `qmake PREFIX=/custom/path'")
    message("==== (current installation path is `$${INSTALL_PREFIX}')")
} else {
    INSTALL_PREFIX = $${PREFIX}
    message("====")
    message("==== install prefix set to `$${INSTALL_PREFIX}'")
}

# Setup the library installation directory
exists( meego-release ) {
    ARCH = $$system(tail -n1 meego-release)
} else {
    ARCH = $$system(uname -m)
}

contains( ARCH, x86_64 ) {
    INSTALL_LIBDIR = $${INSTALL_PREFIX}/lib64
} else {
    INSTALL_LIBDIR = $${INSTALL_PREFIX}/lib
}

# default library directory can be overriden by defining LIBDIR when
# running qmake
isEmpty( LIBDIR ) {
    message("====")
    message("==== NOTE: To override the library installation path run: `qmake LIBDIR=/custom/path'")
    message("==== (current installation path is `$${INSTALL_LIBDIR}')")
} else {
    INSTALL_LIBDIR = $${LIBDIR}
    message("====")
    message("==== library install path set to `$${INSTALL_LIBDIR}'")
}


#-----------------------------------------------------------------------------
# default installation target for applications
#-----------------------------------------------------------------------------
contains( TEMPLATE, app ) {
    target.path  = $${INSTALL_PREFIX}/bin
    INSTALLS    += target
    message("====")
    message("==== INSTALLS += target")
}


#-----------------------------------------------------------------------------
# default installation target for libraries
#-----------------------------------------------------------------------------
contains( TEMPLATE, lib ) {
    target.path  = $${INSTALL_LIBDIR}
    INSTALLS    += target
    message("====")
    message("==== INSTALLS += target")
}

#-----------------------------------------------------------------------------
# target for header files
#-----------------------------------------------------------------------------
!isEmpty( headers.files ) {
    headers.path  = $${INSTALL_PREFIX}/include/$${TARGET}
    INSTALLS     += headers
    message("====")
    message("==== INSTALLS += headers")
} else {
    message("====")
    message("==== NOTE: Remember to add your API headers into `headers.files' for installation!")
}


# End of File
