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

#ifndef SIGNONDISPOSABLE_H_
#define SIGNONDISPOSABLE_H_

#include "signond-common.h"

#include <QtCore>
#include <time.h>

namespace SignonDaemonNS {

/*!
 * @class SignonDisposable
 *
 * Base class for server objects that can be automatically destroyed after
 * a certain period of inactivity.
 */
class SignonDisposable: public QObject
{
    Q_OBJECT

public:
    /*!
     * Construct an object that can be automatically destroyed after
     * having being unused for @maxInactivity seconds.
     * 
     * @param maxInactivity the number of seconds of inactivity.
     * @param parent the parent object.
     */
    SignonDisposable(int maxInactivity, QObject *parent);
    virtual ~SignonDisposable();

    /*!
     * Mark the object as used. Calling this method causes the inactivity
     * timer to be reset.
     */
    void keepInUse() const;

    /*!
     * Deletes all disposable object for which the inactivity time has
     * elapsed.
     */
    static void destroyUnused();

private:
    int maxInactivity;
    mutable time_t lastActivity;
}; //class SignonDaemon

} //namespace SignonDaemonNS

#endif /* SIGNONDISPOSABLE_H_ */
