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

#include "GaloisField.h"
#include "GaloisFieldGenerator.h"

// Construct a Galois Field of size 2^NumberOfElementsLog2
GaloisField::GaloisField(const unsigned int NumberOfElementsLog2, const unsigned int GeneratorPolynomial)
    : mNumberOfElements(1<<NumberOfElementsLog2)
    , mNumberOfElementsLog2(NumberOfElementsLog2)
    , mValueLookupTable(mNumberOfElements, 0)
    , mLogLookupTable(mNumberOfElements-1, 0)
{
    // Deal with the zero element specially
    GaloisFieldElement *pElement = new GaloisFieldElementZero(this);
    mValueLookupTable[0] = pElement;


    // Now fill up the lookup-tables with the nonzero elements
    GaloisFieldGenerator fieldGenerator(mNumberOfElementsLog2, GeneratorPolynomial);
    for (unsigned int index = 0; index < mNumberOfElements - 1; index++)
    {
        unsigned int value = fieldGenerator.GetNextValue();
        pElement = new GaloisFieldElementNonzero(index, value, this);
        mValueLookupTable[value] = pElement;
        mLogLookupTable[index] = pElement;
    }


}

GaloisField::~GaloisField()
{
    // Delete all the members
    for (vector<GaloisFieldElement *>::iterator p = mValueLookupTable.begin();
            p != mValueLookupTable.end();
            p++)
    {
        if (*p)
            delete *p;
    }

}

FiniteFieldElement GaloisField::AlphaToPower(const int power) const
{
    FiniteFieldElement x(AlphaToPowerPointer(power));
    return x;
}

FiniteFieldElement GaloisField::GetElement(unsigned int Value) const
{
    FiniteFieldElement x(GetElementPointer(Value));
    return x;
}

const GaloisFieldElement * GaloisField::Add(const GaloisFieldElement * x, const GaloisFieldElement * y) const
{
    return GetElementPointer(x->Value() ^ y->Value());
}

const GaloisFieldElement * GaloisField::Sub(const GaloisFieldElement * x, const GaloisFieldElement * y) const
{
    return GetElementPointer(x->Value() ^ y->Value());
}

const GaloisFieldElement * GaloisField::Mul(const GaloisFieldElement * x, const GaloisFieldElement * y) const
{
    if (!x->IsZero() && !y->IsZero())
        return AlphaToPowerPointer(x->Log() + y->Log());
    else
        return GetElementPointer(0);
}

const GaloisFieldElement * GaloisField::Div(const GaloisFieldElement * x, const GaloisFieldElement * y) const
{
    if (x->IsZero())
        return GetElementPointer(0);
    else
        return AlphaToPowerPointer(x->Log() - y->Log());
}


const GaloisFieldElement * GaloisField::AlphaToPowerPointer(const int power) const
{
    return mLogLookupTable[ModuloIndex(power)];
}

const GaloisFieldElement * GaloisField::GetElementPointer(const unsigned int Value) const
{
    return mValueLookupTable[Value];
}

unsigned int GaloisField::ModuloIndex(const int Index) const
{
    if (Index<0)
        return (unsigned int) ((int) mNumberOfElements-1 + (Index % ((int)mNumberOfElements-1)));
    else
        return (unsigned int) (Index % (mNumberOfElements-1));
}


