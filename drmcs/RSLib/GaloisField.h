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
#ifndef _GALOISFIELD_H
#define _GALOISFIELD_H

#include "FiniteField.h"

#include "FiniteFieldElement.h"
#include "GaloisFieldElement.h"
#include "GaloisFieldElementZero.h"
#include "GaloisFieldElementNonzero.h"

#include <vector>
using namespace std;

class GaloisFieldElement;
class GaloisField :
	public FiniteField
{
public:
	GaloisField(const unsigned int NumberOfElementsLog2, const unsigned int GeneratorPolynomial = 0);
	virtual ~GaloisField(void);
	virtual FiniteFieldElement GetElement(unsigned int Value) const;
	FiniteFieldElement AlphaToPower(const int power) const;

	virtual const GaloisFieldElement * Add(const GaloisFieldElement * const x, const GaloisFieldElement * const y) const;
	virtual const GaloisFieldElement * Sub(const GaloisFieldElement * const x, const GaloisFieldElement * const y) const;
	virtual const GaloisFieldElement * Mul(const GaloisFieldElement * const x, const GaloisFieldElement * const y) const;
	virtual const GaloisFieldElement * Div(const GaloisFieldElement * const x, const GaloisFieldElement * const y) const;
	const GaloisFieldElement * AlphaToPowerPointer(const int power) const;
	unsigned int ModuloIndex(const int Index) const;
	const GaloisFieldElement * GetElementPointer(const unsigned int Value) const;

private:
	const unsigned int mNumberOfElements;
	vector<GaloisFieldElement *> mValueLookupTable;
	vector<GaloisFieldElement *> mLogLookupTable;
	const unsigned int mNumberOfElementsLog2;
};
#endif
