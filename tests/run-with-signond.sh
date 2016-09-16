#! /bin/sh

# If there's already an instance of signond running, kill it

set -e

# start a local signond

export HOME="$(mktemp -d --tmpdir signond-tests-XXXXXX)"
export SSO_LOGGING_LEVEL=2
export SSO_STORAGE_PATH="${HOME}"
export SSO_DAEMON_TIMEOUT=5
export SSO_IDENTITY_TIMEOUT=5
export SSO_AUTHSESSION_TIMEOUT=5
export PATH="${BUILDDIR}/src/remotepluginprocess:$PATH"
export LD_LIBRARY_PATH="${BUILDDIR}/lib/plugins":"${BUILDDIR}/lib/plugins/signon-plugins-common":"${BUILDDIR}/lib/signond/SignOn":"$LD_LIBRARY_PATH"
export XDG_RUNTIME_DIR="${HOME}/runtime-dir"
mkdir -p "$XDG_RUNTIME_DIR"

DBUS_CONFIG=${BUILDDIR}/tests/testsession.conf

# If dbus-test-runner exists, use it to run the tests in a separate D-Bus
# session
if command -v dbus-test-runner > /dev/null ; then
    echo "Using dbus-test-runner"
    export SSO_LOGGING_OUTPUT="stdout"
    dbus-test-runner -m 600 --dbus-config=${DBUS_CONFIG} \
        -t "$@" --keep-env
else
    echo "Using dbus-launch"
    eval $(dbus-launch --sh-syntax --config-file=${DBUS_CONFIG})

    cleanUp() {
        echo "Cleaning up."
        kill "$DBUS_SESSION_BUS_PID"
    }

    trap cleanUp EXIT INT TERM

    ${CLIENT_WRAPPER} $@

    trap - EXIT
    cleanUp
fi

rm -rf ${HOME}

