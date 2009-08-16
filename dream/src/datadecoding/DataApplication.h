/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Julian Cable
 *
 * Description: base class for DAB/DRM Data Applications
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef _DATAAPPLICATION_H
#define _DATAAPPLICATION_H

#include "../util/Vector.h"
#ifdef QT_GUI_LIB
# include <QMutex>
#endif
class CParameter;

class DataApplication
{
public:

	DataApplication(CParameter&):serviceid(0)
#ifdef QT_GUI_LIB
    , mutex()
#endif
    {}
	virtual ~DataApplication() {}

	virtual void AddDataUnit(CVector<_BINARY>&) = 0;

    uint32_t serviceid;
protected:
#ifdef QT_GUI_LIB
    QMutex mutex;
    void Lock() { mutex.lock();}
    void Unlock() { mutex.unlock(); }
#else
    void Lock() {}
    void Unlock() {}
#endif
};

class DataApplicationFactory
{
public:

	DataApplicationFactory() {}
	virtual ~DataApplicationFactory() {}

	virtual DataApplication* create(CParameter&) = 0;

};

#endif
