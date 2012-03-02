/*
 * This file is part of signon
 *
 * Copyright (C) 2010 Nokia Corporation.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "signondisposable.h"

namespace SignonDaemonNS {

static QList<SignonDisposable *> disposableObjects;

SignonDisposable::SignonDisposable(int maxInactivity, QObject *parent)
    : QObject(parent)
    , maxInactivity(maxInactivity)
    , autoDestruct(true)
{
    disposableObjects.append(this);

    // mark as used
    keepInUse();
}

SignonDisposable::~SignonDisposable()
{
    disposableObjects.removeOne(this);
}

void SignonDisposable::keepInUse() const
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        qWarning("Couldn't get time from monotonic clock");
        return;
    }
    lastActivity = ts.tv_sec;
}

void SignonDisposable::setAutoDestruct(bool value) const
{
    autoDestruct = value;
    keepInUse();
}

void SignonDisposable::destroyUnused()
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        qWarning("Couldn't get time from monotonic clock");
        return;
    }

    foreach (SignonDisposable *object, disposableObjects) {
        if (object->autoDestruct  && (ts.tv_sec - object->lastActivity > object->maxInactivity)) {
            TRACE() << "Object unused, deleting: " << object;
            object->destroy();
            disposableObjects.removeOne(object);
        }
    }
}

} //namespace SignonDaemonNS
