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

#include "FiniteFieldElement.h"

FiniteFieldElement::FiniteFieldElement(const WorkingFiniteFieldElement *pElement)
    : mpElement(pElement)
{
}

FiniteFieldElement::~FiniteFieldElement(void)
{
}

FiniteFieldElement FiniteFieldElement::operator+(const FiniteFieldElement y) const
{
    FiniteFieldElement x(mpElement->Add(y.mpElement));
    return x;
}

FiniteFieldElement FiniteFieldElement::operator-(const FiniteFieldElement y) const
{
    FiniteFieldElement x(mpElement->Sub(y.mpElement));
    return x;
}

FiniteFieldElement FiniteFieldElement::operator*(const FiniteFieldElement y) const
{
    FiniteFieldElement x(mpElement->Mul(y.mpElement));
    return x;
}

FiniteFieldElement FiniteFieldElement::operator/(const FiniteFieldElement y) const
{
    FiniteFieldElement x(mpElement->Div(y.mpElement));
    return x;
}

FiniteFieldElement FiniteFieldElement::ToPower(const int power) const
{
    FiniteFieldElement x(mpElement->ToPower(power));
    return x;
}

// These operators have replaced by a type conversion operator to int
//bool FiniteFieldElement::operator==(const unsigned int y) const
//{
//	return (mpElement->Value() == y);
//}

//bool FiniteFieldElement::operator!=(const unsigned int y) const
//{
//	return (mpElement->Value() != y);
//}

ostream &operator<<(ostream &stream, const FiniteFieldElement& element)
{
    // Output the integer form
    stream << element.mpElement->Value();
    return stream;
}

FiniteFieldElement::operator int() const
{
    return mpElement->Value();
}
