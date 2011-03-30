#-----------------------------------------------------------------------------
# Common configuration for all projects.
#-----------------------------------------------------------------------------

CONFIG         += link_pkgconfig
#MOC_DIR         = .moc
#OBJECTS_DIR     = .obj
RCC_DIR         = resources
#UI_DIR          = ui
#UI_HEADERS_DIR  = ui/include
#UI_SOURCES_DIR  = ui/src

# we don't like warnings...
QMAKE_CXXFLAGS -= -Werror -Wno-write-strings
# Disable RTTI
QMAKE_CXXFLAGS += -fno-exceptions -fno-rtti

TOP_SRC_DIR     = $$PWD
TOP_BUILD_DIR   = $${TOP_SRC_DIR}/$(BUILD_DIR)
QMAKE_LIBDIR   += $${TOP_BUILD_DIR}/lib/SignOn
INCLUDEPATH    += $${TOP_SRC_DIR}/lib

#DEFINES += QT_NO_DEBUG_OUTPUT
DEFINES += DEBUG_ENABLED
#TODO comment this to restrict plugins to run under signon user
DEFINES += NO_SIGNON_USER

# Default directory for signond extensions
DEFINES += SIGNON_EXTENSIONS_DIR=\\\"/usr/lib/signon/extensions\\\"
# End of File

