include( ../common-project-config.pri )
include( ../common-vars.pri )

include( ../common-installs-config.pri )

TEMPLATE = subdirs

service.path = $${INSTALL_PREFIX}/share/dbus-1/services
service.files = com.nokia.singlesignon.service
service.files += com.nokia.singlesignon.backup.service
INSTALLS += service

backuprestore.path = $${INSTALL_PREFIX}/share/backup-framework/applications
backuprestore.files = singlesignon.conf
INSTALLS += backuprestore

clearuserdata.path = /etc/osso-cud-scripts
clearuserdata.files = signon-cud.sh
INSTALLS += clearuserdata
