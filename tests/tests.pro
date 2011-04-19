include( ../common-project-config.pri )
include( ../common-vars.pri )

check.commands = ./authpluginstest/authpluginstest
check.commands += ./libsignon-qt-tests/libsignon-qt-tests
check.commands += ./libsignon-qt-tests/libsignon-qt-untrusted-tests
check.commands += ./signond-tests/signond-tests
QMAKE_EXTRA_TARGETS += check

TEMPLATE = subdirs

SUBDIRS += authpluginstest/authpluginstest.pro
#SUBDIRS += authpluginstest/testplugintemplate/testplugin.pro
#SUBDIRS += singlesignontest/singlesignontest.pro
SUBDIRS += saslplugintest/saslplugintest.pro
SUBDIRS += passwordplugintest/passwordplugintest.pro
SUBDIRS += pluginproxytest/pluginproxytest.pro
SUBDIRS += libsignon-qt-tests/libsignon-qt-tests.pro
SUBDIRS += libsignon-qt-tests/libsignon-qt-untrusted-tests.pro
SUBDIRS += signond-tests/signond-tests.pro
