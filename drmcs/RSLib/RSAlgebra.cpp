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

#include "RSAlgebra.h"

RSAlgebra::RSAlgebra(	const unsigned int n, const unsigned int k, 
						const GaloisField *const pField, 
						const unsigned int indexFirstRoot, 
						const unsigned int indexStepRoot)
						: mNumCodeBits(n)
						, mNumDataBits(k)
						, mIndexFirstRoot(indexFirstRoot)
						, mIndexStepRoot(indexStepRoot)
						, mpField(pField)
						, mGenerator(pField, pField->GetElement(1), 0)
{

	// Make the Generator polynomial
	for (unsigned int index = indexFirstRoot, i=0; i<n-k; i++, index += indexStepRoot)
		mGenerator.AddRoot(pField->AlphaToPower(index));
}

RSAlgebra::~RSAlgebra(void)
{
}

void RSAlgebra::Encode(FiniteFieldPolynomial &data)
{
	// make polynomial x^(n-k) to achieve shift
	FiniteFieldPolynomial xToNMinusK(mpField, mpField->GetElement(1), mGenerator.Order());
	// shift data word left by (n-k) to make space for the check bytes
	data *= xToNMinusK;

	// make polynomial to hold the remainder
	FiniteFieldPolynomial remainder = data;

	// Do the long division to get the remainder

	remainder %= mGenerator;

	// Add the remainder (check bytes) to the data polynomial
	data += remainder;
}

int RSAlgebra::Decode(FiniteFieldPolynomial &data, list<unsigned int> &erasures)
{
	// 2t = n-k
	unsigned int t2 = mNumCodeBits-mNumDataBits;

	unsigned int numErasures = (unsigned int) erasures.size();

	// Calculate syndrome polynomial
	FiniteFieldPolynomial S(mpField);
	for (unsigned int index = mIndexFirstRoot, i=0; i<t2; index += mIndexStepRoot, i++)
	{
		S.SetCoefficient(i, data(mpField->AlphaToPower(index)));
	}

	// Berlekamp's algorithm
	unsigned int l = numErasures;
	// lambda(x) = 1
	FiniteFieldPolynomial lambda(mpField, mpField->GetElement(1), 0);

	// Add the erasures as roots
	for (list<unsigned int>::const_iterator i = erasures.begin(); i != erasures.end(); i++)
	{
		// add root alpha^-pos
		FiniteFieldElement XiInv = mpField->AlphaToPower(-int(*i));
		lambda.AddRoot(XiInv);
		// Algorithm needs factor of form (1+X_i . x). Correct by multiplying by Xj
		lambda /= XiInv;
	}

	//cout<<"lambda="<<endl<<lambda<<endl;
	//c(x) = 1
	//FiniteFieldPolynomial c(mpField, mpField->GetElement(1), 0);
	// initial c(x) = lambda(x) for erasures decoding
	FiniteFieldPolynomial c = lambda;

	// x^1 for use in the loop
	FiniteFieldPolynomial xTo1(mpField, mpField->GetElement(1), 1);

	for(unsigned int K = numErasures+1; K<=t2; K++)
	{
		FiniteFieldElement e = S.GetCoefficient(K-1); // appears to be mistake in WHP031
		for (unsigned int i = 1; i<=l; i++)
		{
			e = e + lambda.GetCoefficient(i) * S.GetCoefficient(K-1-i);
		}

		//cout<<"K="<<K<<" L="<<l<<" e="<<e<<endl;

		FiniteFieldPolynomial newLambda = lambda;
		c *= xTo1;
		if (e != 0)
		{
			FiniteFieldPolynomial cTimesE = c;
			cTimesE *= e;
			newLambda += cTimesE;
			if (2*l < K + numErasures)
			{
				l = K + numErasures - l;
				c = lambda;
				c /= e;
			}
		}

		//c *= xTo1; // as per white paper
		//cout<<"lambda="<<endl<<lambda<<endl;
		lambda = newLambda;
	}

	// lambda is now the error locator polynomial

	// The Key equation

	FiniteFieldPolynomial Omega = S;
	Omega *= lambda;
	FiniteFieldPolynomial xTo2t(mpField, mpField->GetElement(1), t2);
	Omega %= xTo2t;

	// Calculate lambda' for use in the Forney equation
	FiniteFieldPolynomial lambdaDash = lambda;
	lambdaDash.Differentiate();

	// Chien search for error positions

	// error polynomial - all zeros initially
	FiniteFieldPolynomial e(mpField, mpField->GetElement(0), data.Order());

	unsigned int errorCount = 0;
	for (int errorPos=0; (unsigned int)errorPos<mNumCodeBits; errorPos++)
	{
		FiniteFieldElement XjInv = mpField->AlphaToPower(-errorPos);
		FiniteFieldElement l = lambda(XjInv);
		if (l==0)
		{
			errorCount++;
			// root found at position errorPos
			// Calculate error value Y (Forney algorithm)
			FiniteFieldElement Y = XjInv.ToPower(mIndexFirstRoot - 1) *
									Omega(XjInv) / lambdaDash(XjInv);
			// set the coefficient in the error polynomial
			e.SetCoefficient(errorPos, Y);
			//data.SetCoefficient(errorPos, data.GetCoefficient(errorPos) + Y);
		}
	}

	// remove any high-order zero coefficients
	lambda.Trim();
	if (errorCount != lambda.Order())
	{
		// uncorrectable errors
		return -1;
	}
	else
	{
		// correct the errors
		data += e;
		return errorCount;
	}


}
