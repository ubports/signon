include( libsignon-qt-tests.pri )

TARGET = libsignon-qt-untrusted-tests
SOURCES += libsignon-qt-untrusted-tests.cpp
SOURCES -= $$TOP_SRC_DIR/tests/sso-mt-test/ssotestclient.cpp

check.commands = "$$RUN_WITH_SIGNOND ./libsignon-qt-untrusted-tests"
