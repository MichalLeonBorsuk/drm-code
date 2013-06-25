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

#include "RSCodePFT.h"
#include "RSAlgebra.h"
#include "GaloisField.h"
#include "FiniteFieldPolynomial.h"

CRSCodePFT::CRSCodePFT(const unsigned int n, const unsigned int k, 
				const unsigned int indexFirstRoot, 
				const unsigned int indexStepRoot,
				const unsigned int fieldSizeLog2,
				const unsigned int fieldGeneratorPolynomial)
				: mpField(new GaloisField(fieldSizeLog2, fieldGeneratorPolynomial))
				, mpRSAlgebra(new RSAlgebra(n, k, mpField, indexFirstRoot, indexStepRoot))
				, mNumCodeBits(n)
				, mNumDataBits(k)
{

}

CRSCodePFT::~CRSCodePFT(void)
{
	delete mpRSAlgebra;
	delete mpField;
}

void CRSCodePFT::Encode(unsigned char *pData, unsigned char *pParity)
{
	// Map to a polynomial
	FiniteFieldPolynomial p(mpField);

	// DRM order: first data byte maps to highest order coefficient
	for (unsigned int i = 0; i<mNumDataBits; i++)
		p.SetCoefficient(mNumDataBits-1- i, mpField->GetElement(pData[i]));

	// Encode (On return, p is a polynomial of order n-1)
	mpRSAlgebra->Encode(p);

	// Put the parity bits into the output
	for (unsigned int i = 0; i<mNumCodeBits - mNumDataBits; i++)
		pParity[i] = p.GetCoefficient(mNumCodeBits - mNumDataBits - 1 - i);
}

int CRSCodePFT::Decode(unsigned char *pData, 
					   unsigned int *pErasurePositions, 
					   unsigned int numErasures)
{
	// Map to a polynomial
	FiniteFieldPolynomial p(mpField);

	// DRM order: first data byte maps to highest order coefficient
	for (unsigned int i = 0; i<mNumCodeBits; i++)
		p.SetCoefficient(mNumCodeBits-1- i, mpField->GetElement(pData[i]));

	// Make an STL list of the erasure positions
	list<unsigned int> erasures;
	for (unsigned int i = 0; i < numErasures; i++)
		erasures.push_back(mNumCodeBits - 1 - pErasurePositions[i]);

	// Do the error correction
	int corrections = mpRSAlgebra->Decode(p, erasures);

	// Copy back to data array
	for (unsigned int i = 0; i<mNumCodeBits; i++)
		pData[i] = p.GetCoefficient(mNumCodeBits-1-i);

	return corrections;
}
