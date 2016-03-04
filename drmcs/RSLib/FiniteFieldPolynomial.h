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
#ifndef _FINITE_FIELD_POLYNOMIAL_H
#define _FINITE_FIELD_POLYNOMIAL_H

#include <vector>
#include "FiniteFieldElement.h"
class FiniteField;

class FiniteFieldPolynomial
{
public:
	// Polynomial with no coefficients
	FiniteFieldPolynomial(const FiniteField * const field);
	// Constructor for making a simple polynomial with order coeffs equal to value
	FiniteFieldPolynomial(const FiniteField *const field, const FiniteFieldElement value, const unsigned int order = 1);
	FiniteFieldPolynomial(const FiniteFieldPolynomial&);
	virtual ~FiniteFieldPolynomial(void);
	void SetCoefficient(const unsigned int index, const FiniteFieldElement coefficient);
	void AddRoot(FiniteFieldElement root);
	unsigned int Order(void) const;

	void operator+=(const FiniteFieldPolynomial& rhpoly);
	void operator*=(const FiniteFieldPolynomial& multiplier);
	void operator%=(const FiniteFieldPolynomial & divisor);

	void operator*=(const FiniteFieldElement &multiplier);
	void operator/=(const FiniteFieldElement &divisor);
	
	FiniteFieldPolynomial& operator=(const FiniteFieldPolynomial& rhpoly);

	void Trim(void);

	void Differentiate(void);

	FiniteFieldElement operator()(FiniteFieldElement x) const;
	FiniteFieldElement GetCoefficient(const unsigned int index) const;
private:
	vector<FiniteFieldElement> mCoeffs;
	const FiniteField * const mpField;
	void Pad(unsigned int newOrder);
};

	ostream& operator<<(ostream& stream, const FiniteFieldPolynomial &poly);
#endif
