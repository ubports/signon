include(../tests.pri)

TARGET = tst_signond

CONFIG += \
    c++11 \
    debug \
    link_pkgconfig

QT += \
    core \
    dbus \
    testlib

PKGCONFIG += \
    libqtdbusmock-1 \
    libqtdbustest-1

DEFINES += \
    BUILDDIR=\\\"$${TOP_BUILD_DIR}\\\" \
    SIGNONUI_MOCK_TEMPLATE=\\\"$${PWD}/signonui.py\\\" \
    TEST_DBUS_CONFIG_FILE=\\\"../testsession.conf\\\"

INCLUDEPATH += \
    $${TOP_SRC_DIR}/lib

SOURCES += \
    tst_signond.cpp

check.commands = "./$${TARGET}"
check.depends = $${TARGET}
