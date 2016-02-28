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
#ifndef _WORKINGFINITEFIELDELEMENT_H
#define _WORKINGFINITEFIELDELEMENT_H

class WorkingFiniteFieldElement
{
protected:
	WorkingFiniteFieldElement(void);
	virtual ~WorkingFiniteFieldElement(void);
public:
	virtual const WorkingFiniteFieldElement * Add(const WorkingFiniteFieldElement * y) const = 0;
	virtual const WorkingFiniteFieldElement * Sub(const WorkingFiniteFieldElement * y) const = 0;
	virtual const WorkingFiniteFieldElement * Mul(const WorkingFiniteFieldElement * y) const = 0;
	virtual const WorkingFiniteFieldElement * Div(const WorkingFiniteFieldElement * y) const = 0;
	virtual const WorkingFiniteFieldElement * ToPower(const int power) const = 0;
	virtual int Value(void) const = 0;
};
#endif
