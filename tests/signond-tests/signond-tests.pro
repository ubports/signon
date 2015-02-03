TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = \
    tst_access_control_manager_helper.pro \
    tst_timeouts.pro \
    tst_pluginproxy.pro \
    tst_database.pro \
    access-control.pro \

# Disabled until fixed
#SUBDIRS += tst_backup.pro
