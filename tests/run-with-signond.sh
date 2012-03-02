#! /bin/sh

# If there's already an instance of signond running, kill it
pkill signond

set -e

trap "pkill -9 signond" EXIT

# start a local signond

export SSO_LOGGING_LEVEL=2
export SSO_STORAGE_PATH="/tmp"
export SSO_DAEMON_TIMEOUT=5
export SSO_IDENTITY_TIMEOUT=5
export SSO_AUTHSESSION_TIMEOUT=5
export PATH="${BUILDDIR}/src/remotepluginprocess:$PATH"
${SRCDIR}/tests/signond.sh &
sleep 2

${CLIENT_WRAPPER} $@

