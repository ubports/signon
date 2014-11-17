TARGET = tst_access_control_manager_helper

include(signond-tests.pri)

SOURCES = \
    $${SIGNOND_SRC}/accesscontrolmanagerhelper.cpp \
    tst_access_control_manager_helper.cpp

HEADERS = \
    $${SIGNOND_SRC}/accesscontrolmanagerhelper.h \
    $${SIGNOND_SRC}/credentialsdb.h

check.commands = "./$$TARGET"
