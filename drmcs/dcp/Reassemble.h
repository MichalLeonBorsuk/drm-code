/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):  Julian Cable, Ollie Haffenden, Andrew Murphy
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
/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	See Reassemble.cpp
 *
 * All rights reserved
 * Note that a GPL version of this code can be found @ drm.sourceforge.net
 * The BBC asserts its right as the copyright holder to use this code in closed source applications.
\******************************************************************************/

#ifndef REASSEMBLE_H
#define REASSEMBLE_H

#include "platform.h"
#include "bytevector.h"
#include <vector>

class CSegmentTracker
{
public:

	CSegmentTracker():vecbHaveSegment(),segments(0) { }

	void Reset ()
	{
		vecbHaveSegment.clear ();
		segments = 0;
	}

	size_t size ()
	{
		return vecbHaveSegment.size ();
	}

	bool Ready ()
	{
		if (vecbHaveSegment.size () == 0)
			return false;
		for (size_t i = 0; i < vecbHaveSegment.size (); i++)
		{
			if (vecbHaveSegment[i] == false)
			{
				return false;
			}
		}
		return true;
	}

	void AddSegment (int iSegNum)
	{
		if ((iSegNum + 1) > int (vecbHaveSegment.size ()))
			vecbHaveSegment.resize (iSegNum + 1, false);
		if(!vecbHaveSegment[iSegNum])
			segments++;
		vecbHaveSegment[iSegNum] = true;
	}

	bool HaveSegment (int iSegNum)
	{
		if (iSegNum < int (vecbHaveSegment.size ()))
			return vecbHaveSegment[iSegNum];
		return false;
	}

	size_t segment_count() { return segments; }

protected:
	std::vector < bool > vecbHaveSegment;
	size_t segments;
};

/* The base class reassembles chunks of byte vectors into one big vector.
 * It assumes that all chunks except the last chunk are the same size.
 * Usage:
 *
 * CReassembler o;
 * o.AddSegment (veco, iSegSize, 1);
 * o.AddSegment (veco, iSegSize, 3);
 * o.AddSegment (veco, iSegSize, 7, true); // last segment, ie there are 8 segments, 0..7
 * o.AddSegment (veco, iSegSize, 2);
 * o.AddSegment (veco, iSegSize, 4);
 * o.AddSegment (veco, iSegSize, 6);
 * o.AddSegment (veco, iSegSize, 5);
 * o.AddSegment (veco, iSegSize, 0);
 * if(o.Ready())
 *   vecoComplete = o.vecData;
 *
 */

class CReassembler
{
public:

	CReassembler(): vecData(), vecLastSegment(),
		iLastSegmentNum(-1), iLastSegmentSize(-1), iSegmentSize(0),
		Tracker(), bReady(false)
	{
	}

	CReassembler (const CReassembler & r);

	virtual ~CReassembler ()
	{
	}

	CReassembler & operator= (const CReassembler & r);

	void Reset ()
	{
		vecData.resize (0);
		vecLastSegment.resize (0);
		iLastSegmentNum = -1;
		iLastSegmentSize = -1;
		iSegmentSize = 0;
		Tracker.Reset ();
		bReady = false;
	}

	bool Ready ()
	{
		return bReady;
	}

	void AddSegment (const bytev &vecDataIn, int iSegNum, bool bLast);

	bytev vecData;

	size_t segment_count() { return Tracker.segment_count(); }

protected:

	virtual void copyin (const bytev& vecDataIn, size_t iSegNum);
	virtual void cachelast (const bytev& vecDataIn, size_t iSegSize);
	virtual void copylast ();

	bytev vecLastSegment;
	int iLastSegmentNum;
	int iLastSegmentSize;
	size_t iSegmentSize;
	CSegmentTracker Tracker;
	bool bReady;
};

#endif
