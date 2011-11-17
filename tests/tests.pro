include( ../common-project-config.pri )
include( ../common-vars.pri )

TEMPLATE = subdirs

CONFIG  += ordered

SUBDIRS += saslplugintest/saslplugintest.pro
SUBDIRS += passwordplugintest/passwordplugintest.pro
SUBDIRS += pluginproxytest/pluginproxytest.pro
SUBDIRS += libsignon-qt-tests/libsignon-qt-tests.pro
SUBDIRS += libsignon-qt-tests/libsignon-qt-untrusted-tests.pro
SUBDIRS += signond-tests/signond-tests.pro
