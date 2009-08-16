/******************************************************************************\
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	c++ Mathematic Library (Matlib)
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

#ifndef _MATLIB_H_
#define _MATLIB_H_

#include <math.h>
#include <complex>
using namespace std;
#include "../GlobalDefinitions.h"


/* Definitions ****************************************************************/
/* Two different types: constant and temporary buffer */
enum EVecTy {VTY_CONST, VTY_TEMP};


/* These definitions save a lot of redundant code */
#define _VECOP(TYPE, LENGTH, FCT)	const int iL = LENGTH; \
									CMatlibVector<TYPE> vecRet(iL, VTY_TEMP); \
									for (int i = 0; i < iL; i++) \
										vecRet[i] = FCT; \
									return vecRet

#define _VECOPCL(FCT)				for (int i = 0; i < iVectorLength; i++) \
										operator[](i) FCT; \
									return *this

#define _MATOP(TYPE, LENGTHROW, LENGTHCOL, FCT) \
									const int iRL = LENGTHROW; \
									CMatlibMatrix<TYPE> matRet(iRL, LENGTHCOL, VTY_TEMP); \
									for (int i = 0; i < iRL; i++) \
										matRet[i] = FCT; \
									return matRet

#define _MATOPCL(FCT)				for (int i = 0; i < iRowSize; i++) \
										operator[](i) FCT; \
									return *this


/* In debug mode, test input parameters */

#ifdef _DEBUG_
template<typename T>
inline void _TESTRNG(T POS, T limit)
{
    if ((POS >= limit) || (POS < 0))
        DebugError(__FUNCTION__, "POS", POS, "Len", limit);
}

template<typename T>
inline void _TESTSIZE(T INP, T limit)
{
    if (INP != limit)
        DebugError("SizeCheck", "INP", INP, "Len", limit);
}
#else
template<typename T> inline void _TESTRNG(T, T) { }
template<typename T> inline void _TESTSIZE(T, T) { }
#endif

/* Classes ********************************************************************/
/* Prototypes */
template<class T> class			CMatlibVector;
template<class T> class			CMatlibMatrix;

/* Here we can choose the precision of the Matlib calculations */
typedef _REAL					CReal;
typedef complex<CReal>			CComplex;
typedef CMatlibVector<CReal>	CRealVector;
typedef CMatlibVector<CComplex>	CComplexVector;
typedef CMatlibMatrix<CReal>	CRealMatrix;
typedef CMatlibMatrix<CComplex>	CComplexMatrix;


/******************************************************************************/
/* CMatlibVector class ********************************************************/
/******************************************************************************/
template<class T>
class CMatlibVector
{
public:
	/* Construction, Destruction -------------------------------------------- */
	CMatlibVector() : eVType(VTY_CONST), iVectorLength(0), pData(NULL) {}
	CMatlibVector(const int iNLen, const EVecTy eNTy = VTY_CONST) :
		eVType(eNTy), iVectorLength(0), pData(NULL) {Init(iNLen);}
	CMatlibVector(const int iNLen, const T tIniVal) :
		eVType(VTY_CONST), iVectorLength(0), pData(NULL) {Init(iNLen, tIniVal);}
	CMatlibVector(CMatlibVector<T>& vecI);
	CMatlibVector(const CMatlibVector<T>& vecI);
	virtual ~CMatlibVector() {if (pData != NULL) delete[] pData;}

	CMatlibVector(const CMatlibVector<CReal>& fvReal, const CMatlibVector<CReal>& fvImag) :
		eVType(VTY_CONST/*VTY_TEMP*/), iVectorLength(fvReal.Size()), pData(NULL)
	{
		/* Allocate data block for vector */
		pData = new CComplex[iVectorLength];

		/* Copy data from real-vectors in complex vector */
		for (int i = 0; i < iVectorLength; i++)
			pData[i] = CComplex(fvReal[i], fvImag[i]);
	}

	/* Operator[] (Regular indices!!!) */
	inline T& operator[](int const iPos) const
		{_TESTRNG(iPos, iVectorLength); return pData[iPos];}
	inline T& operator[](int const iPos)
		{_TESTRNG(iPos, iVectorLength); return pData[iPos];} // For use as l value

	/* Operator() */
	inline T& operator()(int const iPos) const
		{_TESTRNG(iPos - 1, iVectorLength); return pData[iPos - 1];}
	inline T& operator()(int const iPos)
		{_TESTRNG(iPos - 1, iVectorLength); return pData[iPos - 1];} // For use as l value

	CMatlibVector<T> operator()(const int iFrom, const int iTo) const;
	CMatlibVector<T> operator()(const int iFrom, const int iStep, const int iTo) const;

	inline int Size() const {return iVectorLength;}
	void Init(const int iIniLen, const T tIniVal = 0);
	inline CMatlibVector<T>& Reset(const T tResVal = 0) {_VECOPCL(= tResVal);}
	CMatlibVector<T>& PutIn(const int iFrom, const int iTo, CMatlibVector<T>& fvA);
	CMatlibVector<T>& Merge(const CMatlibVector<T>& vecA, T& tB);
	CMatlibVector<T>& Merge(const CMatlibVector<T>& vecA, const CMatlibVector<T>& vecB);
	CMatlibVector<T>& Merge(const CMatlibVector<T>& vecA, const CMatlibVector<T>& vecB,
		const CMatlibVector<T>& vecC);


	/* operator= */
	inline CMatlibVector<T>&		operator=(const CMatlibVector<CReal>& vecI)
	{
		Init(vecI.Size());
		for (int i = 0; i < iVectorLength; i++)
			operator[](i) = vecI[i];

		return *this;
	}

	inline CMatlibVector<CComplex>&	operator=(const CMatlibVector<CComplex>& vecI)
	{
		Init(vecI.Size());
		for (int i = 0; i < iVectorLength; i++)
			operator[](i) = vecI[i];

		return *this;
	}

	/* operator*= */
	inline CMatlibVector<T>&		operator*=(const CReal& rI)
		{_VECOPCL(*= rI);}
	inline CMatlibVector<CComplex>&	operator*=(const CComplex& cI)
		{_VECOPCL(*= cI);}
	inline CMatlibVector<T>&		operator*=(const CMatlibVector<CReal>& vecI)
		{_VECOPCL(*= vecI[i]);}
	inline CMatlibVector<CComplex>&	operator*=(const CMatlibVector<CComplex>& vecI)
		{_VECOPCL(*= vecI[i]);}

	/* operator/= */
	inline CMatlibVector<T>&		operator/=(const CReal& rI)
		{_VECOPCL(/= rI);}
	inline CMatlibVector<CComplex>&	operator/=(const CComplex& cI)
		{_VECOPCL(/= cI);}
	inline CMatlibVector<T>&		operator/=(const CMatlibVector<CReal>& vecI)
		{_VECOPCL(/= vecI[i]);}
	inline CMatlibVector<CComplex>&	operator/=(const CMatlibVector<CComplex>& vecI)
		{_VECOPCL(/= vecI[i]);}

	/* operator+= */
	inline CMatlibVector<T>&		operator+=(const CReal& rI)
		{_VECOPCL(+= rI);}
	inline CMatlibVector<CComplex>&	operator+=(const CComplex& cI)
		{_VECOPCL(+= cI);}
	inline CMatlibVector<T>&		operator+=(const CMatlibVector<CReal>& vecI)
		{_VECOPCL(+= vecI[i]);}
	inline CMatlibVector<CComplex>&	operator+=(const CMatlibVector<CComplex>& vecI)
		{_VECOPCL(+= vecI[i]);}

	/* operator-= */
	inline CMatlibVector<T>&		operator-=(const CReal& rI)
		{_VECOPCL(-= rI);}
	inline CMatlibVector<CComplex>&	operator-=(const CComplex& cI)
		{_VECOPCL(-= cI);}
	inline CMatlibVector<T>&		operator-=(const CMatlibVector<CReal>& vecI)
		{_VECOPCL(-= vecI[i]);}
	inline CMatlibVector<CComplex>&	operator-=(const CMatlibVector<CComplex>& vecI)
		{_VECOPCL(-= vecI[i]);}

protected:
	EVecTy	eVType;
	int		iVectorLength;
	T*		pData;
};

/* operator* ---------------------------------------------------------------- */
inline CMatlibVector<CComplex> // cv, cv
	operator*(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, cvA.Size(), cvA[i] * cvB[i]);}
inline CMatlibVector<CReal> // rv, rv
	operator*(const CMatlibVector<CReal>& rvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CReal, rvA.Size(), rvA[i] * rvB[i]);}

inline CMatlibVector<CComplex> // cv, rv
	operator*(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CComplex, cvA.Size(), cvA[i] * rvB[i]);}
inline CMatlibVector<CComplex> // rv, cv
	operator*(const CMatlibVector<CReal>& rvB, const CMatlibVector<CComplex>& cvA)
	{_VECOP(CComplex, cvA.Size(), cvA[i] * rvB[i]);}

template<class T> inline
CMatlibVector<T> // Tv, r
	operator*(const CMatlibVector<T>& vecA, const CReal& rB)
	{_VECOP(T, vecA.Size(), vecA[i] * rB);}
template<class T> inline
CMatlibVector<T> // r, Tv
	operator*(const CReal& rA, const CMatlibVector<T>& vecB)
	{_VECOP(T, vecB.Size(), rA * vecB[i]);}

template<class T> inline
CMatlibVector<CComplex> // Tv, c
	operator*(const CMatlibVector<T>& vecA, const CComplex& cB)
	{_VECOP(CComplex, vecA.Size(), vecA[i] * cB);}
template<class T> inline
CMatlibVector<CComplex> // c, Tv
	operator*(const CComplex& cA, const CMatlibVector<T>& vecB)
	{_VECOP(CComplex, vecB.Size(), cA * vecB[i]);}


/* operator/ ---------------------------------------------------------------- */
inline CMatlibVector<CComplex> // cv, cv
	operator/(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, cvA.Size(), cvA[i] / cvB[i]);}
inline CMatlibVector<CReal> // rv, rv
	operator/(const CMatlibVector<CReal>& rvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CReal, rvA.Size(), rvA[i] / rvB[i]);}

inline CMatlibVector<CComplex> // cv, rv
	operator/(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CComplex, cvA.Size(), cvA[i] / rvB[i]);}
inline CMatlibVector<CComplex> // rv, cv
	operator/(const CMatlibVector<CReal>& rvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, rvA.Size(), rvA[i] / cvB[i]);}

template<class T> inline
CMatlibVector<T> // Tv, r
	operator/(const CMatlibVector<T>& vecA, const CReal& rB)
	{_VECOP(T, vecA.Size(), vecA[i] / rB);}
template<class T> inline
CMatlibVector<T> // r, Tv
	operator/(const CReal& rA, const CMatlibVector<T>& vecB)
	{_VECOP(T, vecB.Size(), rA / vecB[i]);}

template<class T> inline
CMatlibVector<CComplex> // Tv, c
	operator/(const CMatlibVector<T>& vecA, const CComplex& cB)
	{_VECOP(CComplex, vecA.Size(), vecA[i] / cB);}
template<class T> inline
CMatlibVector<CComplex> // c, Tv
	operator/(const CComplex& cA, const CMatlibVector<T>& vecB)
	{_VECOP(CComplex, vecB.Size(), cA / vecB[i]);}


/* operator+ ---------------------------------------------------------------- */
inline CMatlibVector<CComplex> // cv, cv
	operator+(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, cvA.Size(), cvA[i] + cvB[i]);}
inline CMatlibVector<CReal> // rv, rv
	operator+(const CMatlibVector<CReal>& rvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CReal, rvA.Size(), rvA[i] + rvB[i]);}

inline CMatlibVector<CComplex> // cv, rv
	operator+(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CComplex, cvA.Size(), cvA[i] + rvB[i]);}
inline CMatlibVector<CComplex> // rv, cv
	operator+(const CMatlibVector<CReal>& rvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, rvA.Size(), rvA[i] + cvB[i]);}

template<class T> inline
CMatlibVector<T> // Tv, r
	operator+(const CMatlibVector<T>& vecA, const CReal& rB)
	{_VECOP(T, vecA.Size(), vecA[i] + rB);}
template<class T> inline
CMatlibVector<T> // r, Tv
	operator+(const CReal& rA, const CMatlibVector<T>& vecB)
	{_VECOP(T, vecB.Size(), rA + vecB[i]);}

template<class T> inline
CMatlibVector<CComplex> // Tv, c
	operator+(const CMatlibVector<T>& vecA, const CComplex& cB)
	{_VECOP(CComplex, vecA.Size(), vecA[i] + cB);}
template<class T> inline
CMatlibVector<CComplex> // c, Tv
	operator+(const CComplex& cA, const CMatlibVector<T>& vecB)
	{_VECOP(CComplex, vecB.Size(), cA + vecB[i]);}


/* operator- ---------------------------------------------------------------- */
inline CMatlibVector<CComplex> // cv, cv
	operator-(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, cvA.Size(), cvA[i] - cvB[i]);}
inline CMatlibVector<CReal> // rv, rv
	operator-(const CMatlibVector<CReal>& rvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CReal, rvA.Size(), rvA[i] - rvB[i]);}

inline CMatlibVector<CComplex> // cv, rv
	operator-(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CComplex, cvA.Size(), cvA[i] - rvB[i]);}
inline CMatlibVector<CComplex> // rv, cv
	operator-(const CMatlibVector<CReal>& rvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, rvA.Size(), rvA[i] - cvB[i]);}

template<class T> inline
CMatlibVector<T> // Tv, r
	operator-(const CMatlibVector<T>& vecA, const CReal& rB)
	{_VECOP(T, vecA.Size(), vecA[i] - rB);}
template<class T> inline
CMatlibVector<T> // r, Tv
	operator-(const CReal& rA, const CMatlibVector<T>& vecB)
	{_VECOP(T, vecB.Size(), rA - vecB[i]);}

template<class T> inline
CMatlibVector<CComplex> // Tv, c
	operator-(const CMatlibVector<T>& vecA, const CComplex& cB)
	{_VECOP(CComplex, vecA.Size(), vecA[i] - cB);}
template<class T> inline
CMatlibVector<CComplex> // c, Tv
	operator-(const CComplex& cA, const CMatlibVector<T>& vecB)
	{_VECOP(CComplex, vecB.Size(), cA - vecB[i]);}


/* Implementation **************************************************************
   (the implementation of template classes must be in the header file!) */
template<class T>
CMatlibVector<T>::CMatlibVector(CMatlibVector<T>& vecI) :
	 eVType(VTY_CONST/*VTY_TEMP*/), iVectorLength(vecI.Size()), pData(NULL)
{
	/* The copy constructor for the constant vector is a real copying
	   task. But in the case of a temporary buffer only the pointer
	   of the temporary buffer is used. The buffer of the temporary
	   vector is then destroyed!!! Therefore the usage of "VTY_TEMP"
	   should be done if the vector IS NOT USED IN A FUNCTION CALL,
	   otherwise this vector will be destroyed afterwards (if the
	   function argument is not declared with "&") */
	if (iVectorLength > 0)
	{
		if (vecI.eVType == VTY_CONST)
		{
			/* Allocate data block for vector */
			pData = new T[iVectorLength];

			/* Copy vector */
			for (int i = 0; i < iVectorLength; i++)
				pData[i] = vecI[i];
		}
		else
		{
			/* We can define the copy constructor as a destroying operator of
			   the input vector for performance reasons. This
			   saves us from always copy the entire vector */
			/* Take data pointer from input vector (steal it) */
			pData = vecI.pData;

			/* Destroy other vector (temporary vectors only) */
			vecI.pData = NULL;
		}
	}
}

/* Copy constructor for constant Matlib vectors */
template<class T>
CMatlibVector<T>::CMatlibVector(const CMatlibVector<T>& vecI) :
	eVType(VTY_CONST), iVectorLength(vecI.Size()), pData(NULL)
{
	if (iVectorLength > 0)
	{
		/* Allocate data block for vector */
		pData = new T[iVectorLength];

		/* Copy vector */
		for (int i = 0; i < iVectorLength; i++)
			pData[i] = vecI[i];
	}
}

template<class T>
void CMatlibVector<T>::Init(const int iIniLen, const T tIniVal)
{
	iVectorLength = iIniLen;

	/* Allocate data block for vector */
	if (iVectorLength > 0)
	{
		if (pData != NULL)
			delete[] pData;

		pData = new T[iVectorLength];

		/* Init with init value */
		for (int i = 0; i < iVectorLength; i++)
			pData[i] = tIniVal;
	}
}

template<class T> inline
CMatlibVector<T> CMatlibVector<T>::operator()(const int iFrom,
											  const int iTo) const
{
	/* This is also capable of "wrap around" blocks (if the value in "iFrom" is
	   larger then the "iTo" value) */
	int			i;
	const int	iStartVal = iFrom - 1;

	if (iFrom > iTo)
	{
		/* Wrap around case */
		CMatlibVector<T> vecRet(iVectorLength - iStartVal + iTo, VTY_TEMP);

		int iCurPos = 0;

		for (i = iStartVal; i < iVectorLength; i++)
			vecRet[iCurPos++] = operator[](i);

		for (i = 0; i < iTo; i++)
			vecRet[iCurPos++] = operator[](i);

		return vecRet;
	}
	else
	{
		CMatlibVector<T> vecRet(iTo - iStartVal, VTY_TEMP);

		for (i = iStartVal; i < iTo; i++)
			vecRet[i - iStartVal] = operator[](i);

		return vecRet;
	}
}

template<class T> inline
CMatlibVector<T> CMatlibVector<T>::operator()(const int iFrom,
											  const int iStep,
											  const int iTo) const
{
    CMatlibVector<T> vecRet(size_t(abs(float(iTo - iFrom)) / abs(float(iStep))) + 1, VTY_TEMP);
	int iOutPos = 0;
	int i;

	if (iFrom > iTo)
	{
		const int iEnd = iTo - 2;
		for (i = iFrom - 1; i > iEnd; i += iStep)
		{
			vecRet[iOutPos] = operator[](i);
			iOutPos++;
		}
	}
	else
	{
		for (i = iFrom - 1; i < iTo; i += iStep)
		{
			vecRet[iOutPos] = operator[](i);
			iOutPos++;
		}
	}

	return vecRet;
}

template<class T> inline
CMatlibVector<T>& CMatlibVector<T>::PutIn(const int iFrom,
										  const int iTo,
										  CMatlibVector<T>& vecI)
{
	const int iStart = iFrom - 1;
	const int iEnd = iTo - iStart;

	for (int i = 0; i < iEnd; i++)
		operator[](i + iStart) = vecI[i];

	return *this;
}

template<class T> inline
CMatlibVector<T>& CMatlibVector<T>::Merge(const CMatlibVector<T>& vecA, T& tB)
{
	const int iSizeA = vecA.Size();

	for (int i = 0; i < iSizeA; i++)
		operator[](i) = vecA[i];

	operator[](iSizeA) = tB;

	return *this;
}

template<class T> inline
CMatlibVector<T>& CMatlibVector<T>::Merge(const CMatlibVector<T>& vecA,
										  const CMatlibVector<T>& vecB)
{
	int i;
	const int iSizeA = vecA.Size();
	const int iSizeB = vecB.Size();

	/* Put first vector */
	for (i = 0; i < iSizeA; i++)
		operator[](i) = vecA[i];

	/* Put second vector behind the first one, both
	   together must have length of *this */
	for (i = 0; i < iSizeB; i++)
		operator[](i + iSizeA) = vecB[i];

	return *this;
}

template<class T> inline
CMatlibVector<T>& CMatlibVector<T>::Merge(const CMatlibVector<T>& vecA,
										  const CMatlibVector<T>& vecB,
										  const CMatlibVector<T>& vecC)
{
	int i;
	const int iSizeA = vecA.Size();
	const int iSizeB = vecB.Size();
	const int iSizeC = vecC.Size();
	const int iSizeAB = iSizeA + iSizeB;

	/* Put first vector */
	for (i = 0; i < iSizeA; i++)
		operator[](i) = vecA[i];

	/* Put second vector behind the first one */
	for (i = 0; i < iSizeB; i++)
		operator[](i + iSizeA) = vecB[i];

	/* Put third vector behind previous put vectors */
	for (i = 0; i < iSizeC; i++)
		operator[](i + iSizeAB) = vecC[i];

	return *this;
}


/******************************************************************************/
/* CMatlibMatrix class ********************************************************/
/******************************************************************************/
/*
	We define: Matrix[row][column]
*/
template<class T>
class CMatlibMatrix
{
public:
	/* Construction, Destruction -------------------------------------------- */
	CMatlibMatrix() : eVType(VTY_CONST), iRowSize(0), ppData(NULL) {}
	CMatlibMatrix(const int iNRowLen, const int iNColLen,
		const EVecTy eNTy = VTY_CONST) :  eVType(eNTy),
		iRowSize(0), ppData(NULL) {Init(iNRowLen, iNColLen);}
	CMatlibMatrix(const int iNRowLen, const int iNColLen, const T tIniVal) :
		eVType(VTY_CONST), iRowSize(0), ppData(NULL)
		{Init(iNRowLen, iNColLen, tIniVal);}
	CMatlibMatrix(const CMatlibMatrix<T>& matI);

	virtual ~CMatlibMatrix() {if (ppData != NULL) delete[] ppData;}

	void Init(const int iNRowLen, const int iNColLen, const T tIniVal = 0);
	inline int GetRowSize() const {return iRowSize;}
	inline int GetColSize() const
		{if (iRowSize > 0) return ppData[0].Size(); else return 0;}

	/* Operator[] (Regular indices!!!) */
	inline CMatlibVector<T>& operator[](int const iPos) const
		{_TESTRNG(iPos, iRowSize); return ppData[iPos];}
	inline CMatlibVector<T>& operator[](int const iPos)
		{_TESTRNG(iPos, iRowSize); return ppData[iPos];} // For use as l value

	/* Operator() */
	inline CMatlibVector<T>& operator()(int const iPos) const
		{_TESTRNG(iPos - 1, iRowSize); return ppData[iPos - 1];}
	inline CMatlibVector<T>& operator()(int const iPos)
		{_TESTRNG(iPos - 1, iRowSize); return ppData[iPos - 1];} // For use as l value

	CMatlibMatrix<T> operator()(const int iRowFrom, const int iRowTo,
		const int iColFrom, const int iColTo) const;

	/* operator= */
	inline CMatlibMatrix<T>& operator=(const CMatlibMatrix<CReal>& matI)
		{_TESTSIZE(matI.GetRowSize(), iRowSize); _MATOPCL(= matI[i]);}
	inline CMatlibMatrix<CComplex>& operator=(const CMatlibMatrix<CComplex>& matI)
		{_TESTSIZE(matI.GetRowSize(), iRowSize); _MATOPCL(= matI[i]);}

	/* operator+= */
	inline CMatlibMatrix<T>& operator+=(const CMatlibMatrix<CReal>& matI)
		{_MATOPCL(+= matI[i]);}
	inline CMatlibMatrix<CComplex>& operator+=(const CMatlibMatrix<CComplex>& matI)
		{_MATOPCL(+= matI[i]);}

	/* operator-= */
	inline CMatlibMatrix<T>& operator-=(const CMatlibMatrix<CReal>& matI)
		{_MATOPCL(-= matI[i]);}
	inline CMatlibMatrix<CComplex>& operator-=(const CMatlibMatrix<CComplex>& matI)
		{_MATOPCL(-= matI[i]);}

	/* operator*= */
	inline CMatlibMatrix<T>& operator*=(const CReal& rI)
		{_MATOPCL(*= rI);}
	inline CMatlibMatrix<CComplex>& operator*=(const CComplex& cI)
		{_MATOPCL(*= cI);}

	/* operator/= */
	inline CMatlibMatrix<T>& operator/=(const CReal& rI)
		{_MATOPCL(/= rI);}
	inline CMatlibMatrix<CComplex>& operator/=(const CComplex& cI)
		{_MATOPCL(/= cI);}

protected:
	EVecTy				eVType;
	int					iRowSize;
	CMatlibVector<T>*	ppData;
};


/* Help functions *************************************************************/
/* operator+ */
inline CMatlibMatrix<CComplex> // cm, cm
operator+(const CMatlibMatrix<CComplex>& cmA, const CMatlibMatrix<CComplex>& cmB)
{
	const int iRowSizeA = cmA.GetRowSize();
	const int iColSizeA = cmA.GetColSize();
	CMatlibMatrix<CComplex> matRet(iRowSizeA, iColSizeA, VTY_TEMP);

	for (int j = 0; j < iRowSizeA; j++)
	{
		for (int i = 0; i < iColSizeA; i++)
			matRet[j][i] = cmA[j][i] + cmB[j][i];
	}

	return matRet;
}

/* operator- */
inline CMatlibMatrix<CComplex> // cm, cm
operator-(const CMatlibMatrix<CComplex>& cmA, const CMatlibMatrix<CComplex>& cmB)
{
	const int iRowSizeA = cmA.GetRowSize();
	const int iColSizeA = cmA.GetColSize();
	CMatlibMatrix<CComplex> matRet(iRowSizeA, iColSizeA, VTY_TEMP);

	for (int j = 0; j < iRowSizeA; j++)
	{
		for (int i = 0; i < iColSizeA; i++)
			matRet[j][i] = cmA[j][i] - cmB[j][i];
	}

	return matRet;
}

/* operator* */
inline CMatlibVector<CComplex> // cm, cv
operator*(const CMatlibMatrix<CComplex>& cmA, const CMatlibVector<CComplex>& cvB)
{
	const int iRowSizeA = cmA.GetRowSize();
	const int iSizeB = cvB.Size();
	CMatlibVector<CComplex> vecRet(iSizeB, VTY_TEMP);

	for (int j = 0; j < iRowSizeA; j++)
	{
		vecRet[j] = (CReal) 0.0;

		for (int i = 0; i < iSizeB; i++)
			vecRet[j] += cmA[j][i] * cvB[i];
	}

	return vecRet;
}

/* operator* */
inline CMatlibVector<CReal> // rm, rv
operator*(const CMatlibMatrix<CReal>& rmA, const CMatlibVector<CReal>& rvB)
{
	const int iRowSizeA = rmA.GetRowSize();
	const int iSizeB = rvB.Size();
	CMatlibVector<CReal> vecRet(iSizeB, VTY_TEMP);

	for (int j = 0; j < iRowSizeA; j++)
	{
		vecRet[j] = (CReal) 0.0;

		for (int i = 0; i < iSizeB; i++)
			vecRet[j] += rmA[j][i] * rvB[i];
	}

	return vecRet;
}

/* operator* */
inline CMatlibMatrix<CComplex> // cm, cm
operator*(const CMatlibMatrix<CComplex>& cmA, const CMatlibMatrix<CComplex>& cmB)
{
	const int iRowSizeA = cmA.GetRowSize();
	const int iRowSizeB = cmB.GetRowSize();
	const int iColSizeB = cmB.GetColSize();
	CMatlibMatrix<CComplex> matRet(iRowSizeA, iColSizeB, VTY_TEMP);

	for (int k = 0; k < iColSizeB; k++)
	{
		for (int j = 0; j < iRowSizeA; j++)
		{
			matRet[j][k] = (CReal) 0.0;

			for (int i = 0; i < iRowSizeB; i++)
				matRet[j][k] += cmA[j][i] * cmB[i][k];
		}
	}

	return matRet;
}

/* operator* */
inline CMatlibMatrix<CComplex> // c, cm
operator*(const CComplex& cA, const CMatlibMatrix<CComplex>& cmB)
{
	const int iRowSizeB = cmB.GetRowSize();
	const int iColSizeB = cmB.GetColSize();
	CMatlibMatrix<CComplex> matRet(iRowSizeB, iColSizeB, VTY_TEMP);

	for (int k = 0; k < iColSizeB; k++)
	{
		for (int j = 0; j < iRowSizeB; j++)
			matRet[j][k] = cA * cmB[j][k];
	}

	return matRet;
}

/* operator* */
inline CMatlibMatrix<CReal> // r, rm
operator*(const CReal& rA, const CMatlibMatrix<CReal>& rmB)
{
	const int iRowSizeB = rmB.GetRowSize();
	const int iColSizeB = rmB.GetColSize();
	CMatlibMatrix<CReal> matRet(iRowSizeB, iColSizeB, VTY_TEMP);

	for (int k = 0; k < iColSizeB; k++)
	{
		for (int j = 0; j < iRowSizeB; j++)
			matRet[j][k] = rA * rmB[j][k];
	}

	return matRet;
}


/* Implementation **************************************************************
   (the implementation of template classes must be in the header file!) */
template<class T>
CMatlibMatrix<T>::CMatlibMatrix(const CMatlibMatrix<T>& matI) :
	eVType(VTY_CONST), iRowSize(matI.GetRowSize()), ppData(NULL)
{
	if (iRowSize > 0)
	{
		/* Allocate data block for vector */
		ppData = new CMatlibVector<T>[iRowSize];

		/* Init column vectors and copy */
		for (int i = 0; i < iRowSize; i++)
		{
			ppData[i].Init(matI.GetColSize());

			/* Copy entire vector */
			ppData[i] = matI[i];
		}
	}
}

template<class T>
void CMatlibMatrix<T>::Init(const int iNRowLen, const int iNColLen, const T tIniVal)
{
	iRowSize = iNRowLen;

	/* Allocate data block for vector */
	if (iRowSize > 0)
	{
		if (ppData != NULL)
			delete[] ppData;

		ppData = new CMatlibVector<T>[iRowSize];

		/* Init column vectors and set to init value */
		for (int i = 0; i < iRowSize; i++)
			ppData[i].Init(iNColLen, tIniVal);
	}
}

template<class T> inline
CMatlibMatrix<T> CMatlibMatrix<T>::operator()(const int iRowFrom, const int iRowTo,
											  const int iColFrom, const int iColTo) const
{
	const int iStartRow = iRowFrom - 1;
	const int iStartCol = iColFrom - 1;
	CMatlibMatrix<T> matRet(iRowTo - iStartRow, iColTo - iStartCol, VTY_TEMP);

	for (int j = iStartRow; j < iRowTo; j++)
	{
		for (int i = iStartCol; i < iColTo; i++)
			matRet[j - iStartRow][i - iStartCol] = operator[](j)[i];
	}

	return matRet;
}


/* Include toolboxes after all type definitions */
#include "MatlibStdToolbox.h"
#include "MatlibSigProToolbox.h"


#endif /* _MATLIB_H_ */
