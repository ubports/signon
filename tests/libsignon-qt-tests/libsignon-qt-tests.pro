include( libsignon-qt-tests.pri )

check.commands = "SSO_PLUGINS_DIR=$${TOP_BUILD_DIR}/src/plugins/ssotest $$RUN_WITH_SIGNOND ./libsignon-qt-tests"
