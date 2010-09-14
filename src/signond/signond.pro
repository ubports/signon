include( ../../common-project-config.pri )
include( ../../common-vars.pri )
TEMPLATE = app
TARGET = signond
QT += core \
    sql \
    xml \
    network \
    dbus
QT -= gui

#generate adaptor for backup
system(qdbusxml2cpp -c BackupIfAdaptor -a backupifadaptor.h:backupifadaptor.cpp \
    ../../lib/signond/com.nokia.SingleSignOn.Backup.xml)

HEADERS += accesscodehandler.h \
    accesscontrolmanager.h \
    credentialsaccessmanager.h \
    credentialsdb.h \
    cryptomanager.h \
    cryptohandlers.h \
    signonsessioncore.h \
    signonauthsessionadaptor.h \
    signonauthsession.h \
    signonidentity.h \
    signond-common.h \
    signondaemonadaptor.h \
    signondaemon.h \
    signondisposable.h \
    signontrace.h \
    simdbusadaptor.h \
    pluginproxy.h \
    signonidentityinfo.h \
    signonui_interface.h \
    signonidentityadaptor.h \
    backupifadaptor.h
SOURCES += accesscodehandler.cpp \
    accesscontrolmanager.cpp \
    credentialsaccessmanager.cpp \
    credentialsdb.cpp \
    cryptomanager.cpp \
    cryptohandlers.cpp \
    simdbusadaptor.cpp \
    signonsessioncore.cpp \
    signonauthsessionadaptor.cpp \
    signonauthsession.cpp \
    signonidentity.cpp \
    signondaemonadaptor.cpp \
    signondisposable.cpp \
    signonui_interface.cpp \
    pluginproxy.cpp \
    main.cpp \
    signondaemon.cpp \
    signonidentityinfo.cpp \
    signonidentityadaptor.cpp \
    backupifadaptor.cpp
INCLUDEPATH += . \
    $${TOP_SRC_DIR}/lib/plugins
CONFIG += build_all \
    link_pkgconfig
QMAKE_CXXFLAGS += -fno-exceptions \
    -fno-rtti
DEFINES += QT_NO_CAST_TO_ASCII \
    QT_NO_CAST_FROM_ASCII
DEFINES += SIGNOND_TRACE \
    SIGNON_PLUGIN_TRACE
LIBS += -lcreds \
    -lcryptsetup

#TODO: add some sort of pkgconfig
#directory autodetection
PKG_CONFIG_PATH = /usr/lib/pkgconfig

exists ($(PKG_CONFIG_PATH)/CellularQt.pc) {
    PKGCONFIG += CellularQt
    DEFINES += SIGNON_USES_CELLULAR_QT
}

QMAKE_CLEAN += backupifadaptor.cpp \
               backupifadaptor.h

headers.files = $$HEADERS
include( ../../common-installs-config.pri )

# Disabling access control if platform is not arm
BUILD_ARCH = $$QMAKE_HOST.arch
contains(BUILD_ARCH, i686):DEFINES += SIGNON_DISABLE_ACCESS_CONTROL
else:# TODO get rid of the 'else' branch after security fixes.
DEFINES += SIGNON_DISABLE_ACCESS_CONTROL
