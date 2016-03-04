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
#ifndef _RSCODEPFT_H
#define _RSCODEPFT_H

class GaloisField;
class RSAlgebra;

class CRSCodePFT
{
public:
	CRSCodePFT(const unsigned int n=255, const unsigned int k=207, 
				const unsigned int indexFirstRoot = 1, 
				const unsigned int indexStepRoot = 1,
				const unsigned int fieldSizeLog2 = 8,
				const unsigned int fieldGeneratorPolynomial = 0x11D);
	CRSCodePFT(const CRSCodePFT&);
	CRSCodePFT& operator=(const CRSCodePFT&);
	virtual ~CRSCodePFT(void);
	void Encode(unsigned char *pData, unsigned char *pParity);
	int Decode(unsigned char *pData, unsigned int *pErasurePositions = 0, unsigned int numErasures = 0);

private:
	GaloisField * mpField;
	RSAlgebra * mpRSAlgebra;
	const unsigned int mNumCodeBits;
	const unsigned int mNumDataBits;
};

#endif
