#! /bin/sh

# If there's already an instance of signond running, kill it
pkill signond

set -e

trap "pkill -9 signond" EXIT

# start a local signond

export SSO_LOGGING_LEVEL=2
export SSO_STORAGE_PATH="/tmp"
${SRCDIR}/tests/signond.sh &
sleep 2

${CLIENT_WRAPPER} $@

