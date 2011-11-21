include( ../common-project-config.pri )
include( $$TOP_SRC_DIR/common-vars.pri )

RUN_WITH_SIGNOND = "BUILDDIR=$$TOP_BUILD_DIR SRCDIR=$$TOP_SRC_DIR $$TOP_SRC_DIR/tests/run-with-signond.sh"

QMAKE_EXTRA_TARGETS += check

