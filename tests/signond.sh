#! /bin/sh

set -e

BUILDDIR="${BUILDDIR:=..}"
SIGNOND="${BUILDDIR}/src/signond/signond"

echo "Starting signond from $SIGNOND"
export LD_LIBRARY_PATH="${BUILDDIR}/lib/plugins/signon-plugins-common":"${BUILDDIR}/lib/signond/SignOn":"$LD_LIBRARY_PATH"

$WRAPPER $SIGNOND

