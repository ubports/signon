#!/bin/bash

if [ `whoami` != "root" ]
then
    echo "test need to be run as root"
    exit 1
fi

#run as user and should fail on plugin load
if [ -x /bin/su ] ;
then
su user -c /usr/bin/signonpluginprocess

if [ $? = 1 ] ;
then
    echo "ok"
else
    echo "fail"
    exit 1
fi

fi
#run as root and should not be able to run
/usr/bin/signonpluginprocess

if [ $? = 2 ] ;
then
    echo "ok"
else
    echo "fail"
    exit 1
fi

