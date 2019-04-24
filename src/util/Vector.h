/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *
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

#ifndef _vector_h
#define _vector_h

#include "../GlobalDefinitions.h"
#include <vector>


/******************************************************************************\
* CVector base class                                                           *
\******************************************************************************/
template<class TData> class CVector
{
public:
    CVector() : iBitArrayCounter(0), data(0) {
    }
    CVector(const int iNeSi): iBitArrayCounter(0), data(iNeSi) {
        Init(iNeSi);
    }
    CVector(const int iNeSi, const TData tInVa): iBitArrayCounter(0), data(iNeSi, tInVa) {
        Init(iNeSi, tInVa);
    }
    virtual ~CVector() {}

    /* Copy constructor:
     * The bit access is, by default, reset */
    CVector(const CVector<TData>& vecI) :
        iBitArrayCounter(0), data(vecI.data)
    {
    }

    virtual void Init(const int iNewSize);

    /* Use this init to give all elements a defined value */
    virtual void Init(const int iNewSize, const TData tIniVal);
    void Reset(const TData tResetVal);

    void Enlarge(const int iAddedSize);
    void Add(const TData& tI) {
        data.push_back(tI);
    }

    inline int Size() const {
        return data.size();
    }

    /* This operator allows for a l-value assignment of this object:
       CVector[x] = y is possible */
    inline TData& operator[](const int iPos) {
#ifdef _DEBUG_
        if ((iPos < 0) || (iPos > int(data.size()) - 1))
        {
            DebugError("Writing vector out of bounds", "Vector size",
                       int(data.size()), "New parameter", iPos);
        }
#endif
        return data[iPos];
    }

    inline TData operator[](const int iPos) const {
#ifdef _DEBUG_
        if ((iPos < 0) || (iPos > int(data.size()) - 1))
        {
            DebugError("Reading vector out of bounds", "Vector size",
                       int(data.size()), "New parameter", iPos);
        }
#endif
        return data[iPos];
    }

    inline CVector<TData>& operator=(const CVector<TData>& vecI) {
#ifdef _DEBUG_
        /* Vectors which shall be copied MUST have same size! (If this is
           satisfied, the parameter "int(data.size())" must not be adjusted as
           a side effect) */
        if (vecI.Size() != int(data.size()))
        {
            DebugError("Vector operator=() different size", "Vector size",
                       int(data.size()), "New parameter", vecI.Size());
        }
#endif
        data = vecI.data;
        return *this;
    }

    const vector<TData>& Data() const { return data; }


    /* Bit operation functions */
    void        Enqueue(uint32_t iInformation, const int iNumOfBits);
    uint32_t    Separate(const int iNumOfBits);
    void        ResetBitAccess() {
        iBitArrayCounter = 0;
    }

    friend bool operator==(const CVector<TData>& lhs, const CVector<TData>& rhs) {
        lhs.data == rhs.data;
    }

protected:
    int                                 iBitArrayCounter;
    vector<TData>                       data;
};


/* Implementation *************************************************************/
template<class TData> void CVector<TData>::Init(const int iNewSize)
{
    /* Clear old buffer and reserve memory for new buffer, get iterator
       for pointer operations */
    data.clear();
    data.resize(iNewSize);
}

template<class TData> void CVector<TData>::Init(const int iNewSize,
        const TData tIniVal)
{
    /* Call actual init routine */
    Init(iNewSize);

    /* Set values */
    Reset(tIniVal);
}

template<class TData> void CVector<TData>::Enlarge(const int iAddedSize)
{
    data.resize(data.size()+iAddedSize);
}

template<class TData> void CVector<TData>::Reset(const TData tResetVal)
{
    /* Set all values to reset value */
    for (int i = 0; i < int(data.size()); i++)
        data[i] = tResetVal;
}

template<class TData> void CVector<TData>::Enqueue(uint32_t iInformation,
        const int iNumOfBits)
{
    /* Enqueue bits in bit array */
    for (int i = 0; i < iNumOfBits; i++)
    {
        /* We want to put the bits on the array with the MSB first
         * - Visual Studio 2010 was getting this wrong
         */
        if(iInformation & 1)
            operator[](iBitArrayCounter + iNumOfBits - i - 1) = 1;
        else
            operator[](iBitArrayCounter + iNumOfBits - i - 1) = 0;

        /* Shift one bit to mask next bit at LSB-position */
        iInformation >>= 1;
    }

    iBitArrayCounter += iNumOfBits;
}

template<class TData> uint32_t CVector<TData>::Separate(const int iNumOfBits)
{
    uint32_t iInformation;

    /* Check, if current position plus new bit-size is smaller than the maximum
       length of the bit vector. Error code: return a "0" */
    if (iBitArrayCounter + iNumOfBits > int(data.size()))
        return 0;

    /* Separate out bits from bit-array */
    iInformation = 0;
    for (int i = 0; i < iNumOfBits; i++)
    {
        /* MSB comes first, therefore shift left */
        iInformation <<= 1;

        iInformation |= data[iBitArrayCounter + i] & 1;
    }

    iBitArrayCounter += iNumOfBits;

    return iInformation;
}


/******************************************************************************\
* CShiftRegister class                                                         *
\******************************************************************************/
template<class TData> class CShiftRegister : public CVector<TData>
{
public:
    CShiftRegister() : CVector<TData>() {}
    CShiftRegister(const int iNeSi) : CVector<TData>(iNeSi) {}
    CShiftRegister(const int iNeSi, const TData tInVa) :
        CVector<TData>(iNeSi, tInVa) {}

    /* Add one value at the beginning, shift the others to the right */
    void AddBegin(const TData tNewD);

    /* Add one value at the end, shift the others to the left */
    void AddEnd(const TData tNewD);

    /* Add a vector at the end, shift others to the left */
    void AddEnd(const CVector<TData>& vectNewD, const int iLen);
};


/* Implementation *************************************************************/
template<class TData> void CShiftRegister<TData>::AddBegin(const TData tNewD)
{
    move_backward(this->data.begin(), this->data.end()-1, this->data.begin()+1);
    this->data[0] = tNewD;
}

template<class TData> void CShiftRegister<TData>::AddEnd(const TData tNewD)
{
    move(this->data.begin()+1, this->data.end(), this->data.begin());
    *this->data.end() = tNewD;
}

template<class TData> void CShiftRegister<TData>::AddEnd(const CVector<TData>& vectNewD,
        const int iLen)
{
    int i, iBlockEnd, iMovLen;

    iBlockEnd = int(this->data.size()) - iLen;
    iMovLen = iLen;

    /* Shift old values */
    for (i = 0; i < iBlockEnd; i++)
        this->data[i] = this->data[iMovLen++];

    /* Add new block of data */
    for (i = 0; i < iLen; i++)
        this->data[iBlockEnd++] = vectNewD[i];
}


/******************************************************************************\
* CFIFO class (first in, first out)                                            *
\******************************************************************************/
template<class TData> class CFIFO : public CVector<TData>
{
public:
    CFIFO() : CVector<TData>(), iCurIdx(0) {}
    CFIFO(const int iNeSi) : CVector<TData>(iNeSi), iCurIdx(0) {}
    CFIFO(const int iNeSi, const TData tInVa) :
        CVector<TData>(iNeSi, tInVa), iCurIdx(0) {}

    void Add(const TData tNewD);
    inline TData Get() {
        return this->data[iCurIdx];
    }

    virtual void Init(const int iNewSize);
    virtual void Init(const int iNewSize, const TData tIniVal);

protected:
    int iCurIdx;
};

template<class TData> void CFIFO<TData>::Init(const int iNewSize)
{
    iCurIdx = 0;
    CVector<TData>::Init(iNewSize);
}

template<class TData> void CFIFO<TData>::Init(const int iNewSize,
        const TData tIniVal)
{
    iCurIdx = 0;
    CVector<TData>::Init(iNewSize, tIniVal);
}

template<class TData> void CFIFO<TData>::Add(const TData tNewD)
{
    this->data[iCurIdx] = tNewD;

    /* Increment index */
    iCurIdx++;
    if (iCurIdx >= int(this->data.size()))
        iCurIdx = 0;
}


/******************************************************************************\
* CMovingAv class (moving average)                                             *
\******************************************************************************/
template<class TData> class CMovingAv : public CVector<TData>
{
public:
    CMovingAv() : CVector<TData>(), iCurIdx(0) {}
    CMovingAv(const int iNeSi) : CVector<TData>(iNeSi), iCurIdx(0) {}
    CMovingAv(const int iNeSi, const TData tInVa) :
        CVector<TData>(iNeSi, tInVa), iCurIdx(0) {}

    void Add(const TData tNewD);
    inline TData GetAverage() {
        return tCurAvResult;
    }

    virtual void Init(const int iNewSize);
    void InitVec(const int iNewSize, const int iNewVecSize);

protected:
    int     iCurIdx;
    TData   tCurAvResult;
};

template<class TData> void CMovingAv<TData>::InitVec(const int iNewSize,
        const int iNewVecSize)
{
    iCurIdx = 0;
    CVector<TData>::Init(iNewSize);

    /* Init each vector in vector */
    for (int i = 0; i < iNewSize; i++)
        this->data[i].Init(iNewVecSize, 0);

    /* Init current average result */
    tCurAvResult.Init(iNewVecSize, 0);
}

template<class TData> void CMovingAv<TData>::Init(const int iNewSize)
{
    iCurIdx = 0;
    tCurAvResult = TData(0); /* Only for scalars! */
    CVector<TData>::Init(iNewSize);
}

template<class TData> void CMovingAv<TData>::Add(const TData tNewD)
{
    /*
        Optimized calculation of the moving average. We only add a new value and
        subtract the old value from the result. We only need one addition and a
        history buffer
    */
    /* Subtract oldest value */
    tCurAvResult -= this->data[iCurIdx];

    /* Add new value and write in memory */
    tCurAvResult += tNewD;
    this->data[iCurIdx] = tNewD;

    /* Increase position pointer and test if wrap */
    iCurIdx++;
    if (iCurIdx >= int(this->data.size()))
        iCurIdx = 0;
}


/******************************************************************************\
* CVectorEx class (Extended vector with additional information)                *
\******************************************************************************/
class CExtendedVecData
{
public:
    /* Symbol ID of the current block. This number only identyfies the
       position in a frame, NOT in a super-frame */
    int         iSymbolID;

    /* This flag indicates that the symbol ID has changed */
    bool    bSymbolIDHasChanged;

    /* The channel estimation needs information about timing corrections,
       because it is using information from the symbol memory */
    int         iCurTimeCorr;
};

template<class TData> class CVectorEx : public CVector<TData>
{
public:
    CVectorEx() {}
    virtual ~CVectorEx() {}

    CExtendedVecData&   GetExData() {
        return ExtendedData;
    }
    void                SetExData(CExtendedVecData& NewExData)
    {
        ExtendedData = NewExData;
    }

protected:
    CExtendedVecData ExtendedData;
};


/******************************************************************************\
* CMatrix base class                                                           *
\******************************************************************************/
template<class TData> class CMatrix
{
public:
    CMatrix() : ppData(NULL), iRow(0), iCol(0) {}
    CMatrix(const int iNewR, const int iNewC) {
        Init(iNewR, iNewC);
    }
    CMatrix(const int iNewR, const int iNewC, const TData tInVa)
    {
        Init(iNewR, iNewC, tInVa);
    }
    CMatrix(const CMatrix& m): ppData(NULL) {
        Init(m.iRow,m.iCol);
        for (int i=0; i<m.NumRows(); i++)
            ppData[i] = m[i];
    }
    virtual ~CMatrix();

    void Init(const int iNewRow, const int iNewColumn);

    /* Use this init to give all elements a defined value */
    void Init(const int iNewRow, const int iNewColumn, const TData tIniVal);
    void Reset(const TData tResetVal);

    inline CVector<TData>& operator[](const int iPos) const {
#ifdef _DEBUG_
        if ((iPos < 0) || (iPos > iRow - 1))
        {
            DebugError("Matrix: Writing vector out of bounds", "Row size",
                       iRow, "New parameter", iPos);
        }
#endif
        return ppData[iPos];
    }

    inline CMatrix& operator=(const CMatrix& m) {
        this->Init(m.NumRows(), m.NumColumns());
        for (int i=0; i<m.NumRows(); i++)
            this->ppData[i] = m[i];
        return *this;
    }

#ifdef _DEBUG_
    inline CVector<TData> operator[](const int iPos) const {
        if ((iPos < 0) || (iPos > iRow - 1))
        {
            DebugError("Matrix: Reading vector out of bounds", "Row size",
                       iRow, "New parameter", iPos);
        }
        return ppData[iPos];
    }
#endif
    inline int NumRows(void) const {
        return iRow;
    }
    inline int NumColumns(void) const {
        return iCol;
    }

protected:
    CVector<TData>* ppData;
    int             iRow;
    int             iCol;
};


/* Implementation *************************************************************/
template<class TData> void CMatrix<TData>::Init(const int iNewRow,
        const int iNewColumn)
{
    iRow = iNewRow;
    iCol = iNewColumn;

    if (iRow > 0)
    {
        /* Delete resources from previous init */
        if (ppData != NULL)
            delete[] ppData;

        /* Allocate new memory for history buffer */
        ppData = new CVector<TData>[iRow];
        for (int i = 0; i < iRow; i++)
            ppData[i].Init(iNewColumn);
    }
}

template<class TData> void CMatrix<TData>::Init(const int iNewRow,
        const int iNewColumn,
        const TData tIniVal)
{
    /* Call actual init routine */
    Init(iNewRow, iNewColumn);

    /* Set values */
    Reset(tIniVal);
}

template<class TData> void CMatrix<TData>::Reset(const TData tResetVal)
{
    /* Set all values to reset value */
    for (int i = 0; i < iRow; i++)
        for (int j = 0; j < ppData[i].Size(); j++)
            ppData[i][j] = tResetVal;
}

template<class TData> CMatrix<TData>::~CMatrix()
{
    /* Delete buffer */
    if (ppData != NULL)
        delete[] ppData;
}

#endif // _vector_h
