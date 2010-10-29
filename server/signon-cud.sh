#! /bin/sh

# ============================================================================
#
#  This file is part of SingleSignOn
#
#  Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
#
#  Contact: Tomi Suviola <tomi.suviola@nokia.com>
#
#  signond is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  version 2.1 as published by the Free Software Foundation.
#
#  signond is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with Alarmd; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
#  02110-1301 USA
#
# ============================================================================

dbus-send --session --type=method_call --dest="com.nokia.SingleSignOn" --print-reply "/com/nokia/SingleSignOn"  com.nokia.SingleSignOn.AuthService.clear

