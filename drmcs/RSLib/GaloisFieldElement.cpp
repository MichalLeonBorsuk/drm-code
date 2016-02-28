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

#include "GaloisFieldElement.h"
#include "GaloisField.h"

GaloisFieldElement::GaloisFieldElement(const GaloisField *Field)
    : mcpField(Field)
{
}

GaloisFieldElement::~GaloisFieldElement(void)
{
}

const WorkingFiniteFieldElement * GaloisFieldElement::Add(const WorkingFiniteFieldElement * y)const
{
    const GaloisFieldElement *yCast = dynamic_cast<const GaloisFieldElement *>(y);
    if (yCast)
        return mcpField->Add(this, yCast);
    else
        return 0;

}

const WorkingFiniteFieldElement * GaloisFieldElement::Sub(const WorkingFiniteFieldElement * y)const
{
    const GaloisFieldElement *yCast = dynamic_cast<const GaloisFieldElement *>(y);
    if (yCast)
        return mcpField->Sub(this, yCast);
    else
        return 0;

}

const WorkingFiniteFieldElement * GaloisFieldElement::Mul(const WorkingFiniteFieldElement * y)const
{
    const GaloisFieldElement *yCast = dynamic_cast<const GaloisFieldElement *>(y);
    if (yCast)
        return mcpField->Mul(this, yCast);
    else
        return 0;

}

const WorkingFiniteFieldElement * GaloisFieldElement::Div(const WorkingFiniteFieldElement * y)const
{
    const GaloisFieldElement *yCast = dynamic_cast<const GaloisFieldElement *>(y);
    if (yCast)
        return mcpField->Div(this, yCast);
    else
        return 0;

}

