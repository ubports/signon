#! /bin/sh

dbus-send --session --type=method_call --dest="com.nokia.singlesignon" --print-reply "/SignonDaemon"  com.nokia.singlesignon.SignonDaemon.clear

