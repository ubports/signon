include( ../common-project-config.pri )
include( ../common-vars.pri )

TEMPLATE = subdirs

CONFIG  += ordered

SUBDIRS += \
    passwordplugintest \
    pluginproxytest \
    libsignon-qt-tests/libsignon-qt-tests.pro \
    libsignon-qt-tests/libsignon-qt-untrusted-tests.pro \
    signond-tests \
    extensions

QMAKE_SUBSTITUTES += com.google.code.AccountsSSO.SingleSignOn.service.in
