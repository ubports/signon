'''signonui mock template

This creates the expected methods and properties of the
com.nokia.singlesignonui service.
'''

# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 2.1 of the License.  See
# http://www.gnu.org/copyleft/lgpl.html for the full text of the license.

__author__ = 'Alberto Mardegan'
__email__ = 'alberto.mardegan@canonical.com'
__copyright__ = '(c) 2016 Canonical Ltd.'
__license__ = 'LGPL 2.1'

import dbus
import time

from dbusmock import MOCK_IFACE
import dbusmock

BUS_NAME = 'com.nokia.singlesignonui'
MAIN_OBJ = '/SignonUi'
MAIN_SERVICE_IFACE = 'com.nokia.singlesignonui'
MAIN_IFACE = MAIN_SERVICE_IFACE
SYSTEM_BUS = False


def query_dialog(self, params):
    if self.nextReplyError:
        raise dbus.exceptions.DBusException('SignOnUi error',
                                            name=self.nextReplyError)
    return self.nextReplyData


def load(mock, parameters):
    mock.query_dialog = query_dialog
    mock.refresh_dialog = query_dialog
    mock.AddMethods(MAIN_SERVICE_IFACE, [
        ('queryDialog', 'a{sv}', 'a{sv}', 'ret = self.query_dialog(self, args[0])'),
        ('refreshDialog', 'a{sv}', 'a{sv}', 'ret = self.refresh_dialog(self, args[0])'),
        ('cancelUiRequest', 's', '', 'ret = None'),
        ('removeIdentityData', 'u', '', 'ret = None'),
    ])

    mock.nextReplyData = {}
    mock.nextReplyError = ''


@dbus.service.method(MOCK_IFACE, in_signature='a{sv}', out_signature='')
def SetNextReply(self, data):
    self.nextReplyData = data.get('data', {})
    self.nextReplyError = data.get('error', '')

