/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: BitRates.c 129 2006-02-10 05:41:33Z julianc $
*
* Copyright (C) British Broadcasting Corporation 2006.
*
* All Rights Reserved.
*
* Contributor(s): Julian Cable, John Elliot, Ollie Haffenden, Andrew Murphy
*
* ***** END LICENSE BLOCK ***** */

#include "BitRates.h"

int CellsPerMuxFrameArray[4][6] =
{
	{1259,	1422,	2632,	2959,	5464,	6118}, // Mode A
	{966,	1110,	2051,	2337,	4249,	4774}, // Mode B
	{-1,	-1,		-1,		1844,	-1,		3867}, // Mode C
	{-1,	-1,		-1,		1226,	-1,		2606} // Mode D
};


int CodeRateNumerators64QAMSM[4][3]=
{
	{1,1,3},
	{1,2,4},
	{1,3,7},
	{2,4,8}
};

int CodeRateDenominators64QAMSM[4][3]=
{
	{4,2,4},
	{3,3,5},
	{2,4,8},
	{3,5,9}
};

int CodeRateNumerators16QAM[4][2]=
{
	{1,2},
	{1,3},
	{-1,-1},
	{-1,-1}
};

int CodeRateDenominators16QAM[4][2]=
{
	{3,3},
	{2,4},
	{-1,-1},
	{-1,-1}
};


int CodeRateNumerators64QAMHMSymSPP[4][2]=
{
	{3,3},
	{4,8},
	{4,7},
	{2,8}
};

int CodeRateDenominators64QAMHMSymSPP[4][2]=
{
	{10,5},
	{11,11},
	{7,8},
	{3,9}
};

int CodeRateNumerators64QAMHMVSPP[4]=
{1,4,3,2};

int CodeRateDenominators64QAMHMVSPP[4]=
{2,7,5,3};

int CodeRateNumerators64QAMHMMixSPP[4][5]=
{
	{1,3,1,3,3},
	{1,4,2,8,4},
	{1,4,3,7,7},
	{2,2,4,8,8}
};

int CodeRateDenominators64QAMHMMixSPP[4][5]=
{
	{4,10,2,5,4},
	{3,11,3,11,5},
	{2,7,4,8,8},
	{3,3,5,9,9}
};

int LCM16QAM[4] = {3,4,-1,-1};
int LCM64QAMSM[4]={4,15,8,45};
int LCM64QAMHMSym[4]={10,11,56,9};
int LCM64QAMHMMix[4]={20,165,56,45};

int CalcFrameLength(int NumLevels, int GrossBitsPerCell, int NumCells, int BytesA, 
					int *pNumeratorsA, int *pDenominatorsA, 
					int *pNumeratorsB, int *pDenominatorsB, int LCM)
{
	int NumCellsA, NumCellsB;
	int NetBitsA, NetBitsB;
	int TotalNetBits;
	int NumLCMA;
	int l;

	int BitsPerLCM = 0;

	if (pNumeratorsA[0]==-1 || pNumeratorsB[0]==-1)
		return -1;

	for (l=0; l<NumLevels; l++)
	{
		BitsPerLCM += LCM/pDenominatorsA[l] * pNumeratorsA[l] * GrossBitsPerCell;
	}

	if (BytesA != 0)
	{
		NumLCMA = 8*BytesA/BitsPerLCM;
		if ((8*BytesA)%BitsPerLCM !=0) // round up
		{
			NumLCMA++;
		}
	}
	else
		NumLCMA = 0;

	NumCellsA = NumLCMA * LCM;

	NumCellsB = NumCells-NumCellsA;

	if (GrossBitsPerCell*NumCellsB-12 <0) return -1; // Negative space for B
	NetBitsA=0; NetBitsB=0;
	for (l=0; l<NumLevels; l++)
	{
		NetBitsA += GrossBitsPerCell*NumCellsA/pDenominatorsA[l]*pNumeratorsA[l]; // Exact
		NetBitsB += (GrossBitsPerCell*NumCellsB-12)/pDenominatorsB[l]*pNumeratorsB[l]; // Rounded down
	}

	TotalNetBits = NetBitsA + NetBitsB;


	if (TotalNetBits<8*BytesA) // Not enough capacity for desired length A
		return -1;
	else 
		return (TotalNetBits/8);
	
}

int CalcFrameLengthSPP(int Mode, int SpecOcc, int Constellation, int RateA, int RateB, int BytesA)
{
	int NumCells;
	int GrossBitsPerCell;
	int NumLevels;
	int LCM;
	int *pNumeratorsA, *pNumeratorsB, *pDenominatorsA, *pDenominatorsB;

	if (RateA<0 || RateA>3) return -1;
	if (RateB<0 || RateB>3) return -1;
	NumCells = CellsPerMuxFrameArray[Mode][SpecOcc];

	if (NumCells == -1) // Illegal Mode/Spec Occ combination
		return -1;

	switch (Constellation)
	{
	case 0: // 64QAM SM
		pNumeratorsA = &CodeRateNumerators64QAMSM[RateA][0];
		pDenominatorsA = &CodeRateDenominators64QAMSM[RateA][0];
		pNumeratorsB = &CodeRateNumerators64QAMSM[RateB][0];
		pDenominatorsB = &CodeRateDenominators64QAMSM[RateB][0];
		LCM = LCM64QAMSM[RateA];
		GrossBitsPerCell = 2;
		NumLevels = 3;
		break;
	case 1: // HMMix
		pNumeratorsA = &CodeRateNumerators64QAMHMMixSPP[RateA][0];
		pDenominatorsA = &CodeRateDenominators64QAMHMMixSPP[RateA][0];
		pNumeratorsB = &CodeRateNumerators64QAMHMMixSPP[RateB][0];
		pDenominatorsB = &CodeRateDenominators64QAMHMMixSPP[RateB][0];
		LCM = LCM64QAMHMMix[RateA];
		GrossBitsPerCell = 1;
		NumLevels = 5;
		break;
	case 2: // HMSym
		pNumeratorsA = &CodeRateNumerators64QAMHMSymSPP[RateA][0];
		pDenominatorsA = &CodeRateDenominators64QAMHMSymSPP[RateA][0];
		pNumeratorsB = &CodeRateNumerators64QAMHMSymSPP[RateB][0];
		pDenominatorsB = &CodeRateDenominators64QAMHMSymSPP[RateB][0];
		LCM = LCM64QAMHMSym[RateA];
		GrossBitsPerCell = 2;
		NumLevels = 2;
		break;
	case 3: // 16QAM
		pNumeratorsA = &CodeRateNumerators16QAM[RateA][0];
		pDenominatorsA = &CodeRateDenominators16QAM[RateA][0];
		pNumeratorsB = &CodeRateNumerators16QAM[RateB][0];
		pDenominatorsB = &CodeRateDenominators16QAM[RateB][0];
		LCM = LCM16QAM[RateA];
		NumLevels = 2;
		GrossBitsPerCell = 2;
		break;
	default:
		return -1;
	}

	return CalcFrameLength(NumLevels, GrossBitsPerCell, NumCells, BytesA, pNumeratorsA, pDenominatorsA, 
		pNumeratorsB, pDenominatorsB, LCM);

}

int CalcFrameLengthVSPP(int Mode, int SpecOcc, int Constellation, int RateHier)
{
	int NumCells;
	int GrossBitsPerCell;
	int NumLevels;
	int LCM;
	int *pNumeratorsA, *pNumeratorsB, *pDenominatorsA, *pDenominatorsB;

	if (RateHier<0 || RateHier>3) return -1;

	NumCells = CellsPerMuxFrameArray[Mode][SpecOcc];

	if (NumCells == -1) // Illegal Mode/Spec Occ combination
		return -1;

	switch (Constellation)
	{
	case 0: // 64QAM
		return 0;
		break;
	case 1: // HMMix
		pNumeratorsA = &CodeRateNumerators64QAMHMVSPP[RateHier];
		pDenominatorsA = &CodeRateDenominators64QAMHMVSPP[RateHier];
		pNumeratorsB = &CodeRateNumerators64QAMHMVSPP[RateHier];
		pDenominatorsB = &CodeRateDenominators64QAMHMVSPP[RateHier];
		LCM = 1;
		GrossBitsPerCell = 1;
		NumLevels = 1;
		break;
	case 2: // HMSym
		pNumeratorsA = &CodeRateNumerators64QAMHMVSPP[RateHier];
		pDenominatorsA = &CodeRateDenominators64QAMHMVSPP[RateHier];
		pNumeratorsB = &CodeRateNumerators64QAMHMVSPP[RateHier];
		pDenominatorsB = &CodeRateDenominators64QAMHMVSPP[RateHier];
		LCM = 1;
		GrossBitsPerCell = 2;
		NumLevels = 1;
		break;

	case 3: // 16QAM SM
		return 0;
		break;
	default:
		return -1;
	}

	return CalcFrameLength(NumLevels, GrossBitsPerCell, NumCells, 0, pNumeratorsA, pDenominatorsA, 
		pNumeratorsB, pDenominatorsB, LCM);

}
