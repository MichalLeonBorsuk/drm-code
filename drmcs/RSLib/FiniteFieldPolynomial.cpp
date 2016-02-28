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

#include "FiniteFieldPolynomial.h"
#include "FiniteField.h"
#include <string>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

// Default constructor: make a zero polynomial
FiniteFieldPolynomial::FiniteFieldPolynomial(const FiniteField * const field)
    : mCoeffs()
    , mpField(field)
{
}

FiniteFieldPolynomial::FiniteFieldPolynomial(const FiniteField * const field, const FiniteFieldElement value, const unsigned int order)
    : mpField(field)
    , mCoeffs(order+1, field->GetElement(0))
{
    mCoeffs[order] = value;
}


FiniteFieldPolynomial::~FiniteFieldPolynomial()
{
}

void FiniteFieldPolynomial::SetCoefficient(const unsigned int index, const FiniteFieldElement coefficient)
{
    while (mCoeffs.size() < index+1)
        mCoeffs.push_back(mpField->GetElement(0));
    mCoeffs[index] = coefficient;
}

void FiniteFieldPolynomial::AddRoot(const FiniteFieldElement root)
{
    FiniteFieldPolynomial p(mpField, mpField->GetElement(1), 1);
    p.SetCoefficient(0, root);
    *this *= p;
}

void FiniteFieldPolynomial::operator+=(const FiniteFieldPolynomial& rhpoly)
{
    Pad(rhpoly.Order());
    for(unsigned int i=0; i<=Order(); i++)
        SetCoefficient(i, GetCoefficient(i) + rhpoly.GetCoefficient(i));
}

// assignment operator
void FiniteFieldPolynomial::operator=(const FiniteFieldPolynomial& rhpoly)
{
    Pad(rhpoly.Order());
    for(unsigned int i=0; i<=Order(); i++)
        SetCoefficient(i, rhpoly.GetCoefficient(i));
}


// order is highest power of x in the polynomial
unsigned int FiniteFieldPolynomial::Order(void) const
{
    return (unsigned int) mCoeffs.size() - 1;
}

void FiniteFieldPolynomial::operator*=(const FiniteFieldPolynomial& multiplier)
{
    const unsigned int order1 = Order();
    const unsigned int order2 = multiplier.Order();

    // Extend the polynomial by the order of the multiplier
    for (unsigned int i=0; i<order2; i++)
        mCoeffs.push_back(mpField->GetElement(0));

    // In-place multiplication
    //
    for (int index1 = order1; index1 >=0; index1--)
    {
        for (int index2 = order2; index2 > 0; index2--)
        {
            mCoeffs.at((unsigned int) (index1+index2)) = mCoeffs.at((unsigned int) (index1+index2)) +
                    GetCoefficient((unsigned int)index1) * multiplier.GetCoefficient((unsigned int)index2);
        }
        mCoeffs.at((unsigned int)index1) = GetCoefficient((unsigned int)index1) * multiplier.GetCoefficient(0);
    }
}

FiniteFieldElement FiniteFieldPolynomial::GetCoefficient(const unsigned int index) const
{
    try
    {
        return mCoeffs.at(index);
    }
    catch(...)
    {
        return mpField->GetElement(0);
    }
}

ostream& operator<<(ostream& stream, const FiniteFieldPolynomial &poly)
{

    string line1, line2;
    unsigned int index;

    for (index=poly.Order(); index>0; index--)
    {
        // Print the coefficient
        stringstream str1;
        str1 << poly.GetCoefficient(index) << "x";
        for (unsigned int i=0; i< str1.str().length(); i++)
            line1 += " ";
        line2 += str1.str();

        // Print the index

        str1.str("");
        str1 << index;
        for (unsigned int i=0; i< str1.str().length(); i++)
            line2 += " ";
        line1 += str1.str();

        // Print the plus
        line1 += " ";
        line2 += "+";

    }
    // Print the constant
    stringstream str1;
    str1 << poly.GetCoefficient(index);

    for (unsigned int i=0; i< str1.str().length(); i++)
        line1 += " ";
    line2 += str1.str();

    stream << line1 << endl;
    stream << line2 << endl;

    return stream;
}

FiniteFieldElement FiniteFieldPolynomial::operator()(FiniteFieldElement x) const
{
    FiniteFieldElement total = GetCoefficient(0);
    FiniteFieldElement xToPower = x;
    for (unsigned int index = 1; index<=Order(); index++)
    {
        total = total + GetCoefficient(index) * xToPower;
        xToPower = xToPower * x;
    }
    return total;
}

// polynomial times scalar
void FiniteFieldPolynomial::operator*=(const FiniteFieldElement &multiplier)
{
    for (unsigned int index = 0; index<=Order(); index++)
        SetCoefficient(index, GetCoefficient(index) * multiplier);
}

void FiniteFieldPolynomial::operator/=(const FiniteFieldElement &divisor)
{
    for (unsigned int index = 0; index<=Order(); index++)
        SetCoefficient(index, GetCoefficient(index) / divisor);
}

void FiniteFieldPolynomial::Differentiate(void)
{
    // This is possibly only true for a Galoid Field
    for (unsigned int index = 0; index<Order(); index+=2)
    {
        // Even-powered coefficients are just coefficient of next power up
        SetCoefficient(index, GetCoefficient(index+1));
        // Odd-powered coefficients go to zero (presumably because x+x = 0)
        SetCoefficient(index + 1, mpField->GetElement(0));
    }
    SetCoefficient(Order(), mpField->GetElement(0));
}

void FiniteFieldPolynomial::operator%=(const FiniteFieldPolynomial & divisor)
{
    const unsigned int divisorOrder = divisor.Order();
    const unsigned int dividendOrder = Order();
    const FiniteFieldElement divisorCoeffN = divisor.GetCoefficient(divisorOrder);

    // i is the position of the highest power coeff of divisor against the dividend
    // Note that the loop might never execute if divisor is higher order than dividend
    for (unsigned int i=dividendOrder; i >= divisorOrder; i--)
    {
        // How many times does it go?
        FiniteFieldElement multiplier = GetCoefficient(i) /divisorCoeffN;

        // Subtract multiplier times divisor at this position
        for (unsigned int j=0; j<=divisorOrder; j++)
            SetCoefficient(i-j,
                           GetCoefficient(i-j) - multiplier * divisor.GetCoefficient(divisorOrder-j));
    }
}

void FiniteFieldPolynomial::Pad(unsigned int newOrder)
{
    while (mCoeffs.size() < newOrder+1)
        mCoeffs.push_back(mpField->GetElement(0));
}

void FiniteFieldPolynomial::Trim()
{
    unsigned int i = Order();
    while (i>=0 && mCoeffs[i] == 0)
    {
        mCoeffs.pop_back();
        i--;
    }
}
