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
#ifndef _RSALGEBRA_H
#define _RSALGEBRA_H

#include "GaloisField.h"
#include "FiniteFieldPolynomial.h"
#include <list>

class RSAlgebra
{
public:
	RSAlgebra(	const unsigned int n, const unsigned int k, 
				const GaloisField *const pField, 
				const unsigned int indexFirstRoot = 1, 
				const unsigned int indexStepRoot = 1);
	virtual ~RSAlgebra(void);
	void Encode(FiniteFieldPolynomial &data);
	int Decode(FiniteFieldPolynomial &data, list<unsigned int> &erasures);
private:
	const GaloisField * const mpField;
	const unsigned int mNumCodeBits;
	const unsigned int mNumDataBits;
	const unsigned int mIndexFirstRoot;
	const unsigned int mIndexStepRoot;
	FiniteFieldPolynomial mGenerator;
};
#endif
