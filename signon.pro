include( common-vars.pri )

TEMPLATE  = subdirs
SUBDIRS   = lib src server tests
src.depends = lib
tests.depends = lib src

include( common-installs-config.pri )

include( doc/doc.pri )

# End of File
