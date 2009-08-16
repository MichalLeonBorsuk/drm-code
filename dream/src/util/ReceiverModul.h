/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	High level class for all modules. The common functionality for reading
 *	and writing the transfer-buffers are implemented here.
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

#ifndef _RECEIVER_MODUL_H
#define _RECEIVER_MODUL_H

#include "Modul.h"

template<class TInput, class TOutput>
class CReceiverModul : public CModul<TInput, TOutput>
{
public:
	CReceiverModul();
	virtual ~CReceiverModul() {}

	void				SetInitFlag() {bDoInit = true;}
	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TOutput>& OutputBuffer2);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TOutput>& OutputBuffer2,
							 CBuffer<TOutput>& OutputBuffer3);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TOutput>& OutputBuffer2,
							 vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual void		Init(CParameter& Parameter,
							 vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual void		ReadData(CParameter& Parameter,
								 CBuffer<TOutput>& OutputBuffer);
	virtual bool	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer);
	virtual bool	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer,
									CBuffer<TOutput>& OutputBuffer2);
	virtual bool	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer,
									CBuffer<TOutput>& OutputBuffer2,
									CBuffer<TOutput>& OutputBuffer3);
	virtual bool	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer,
									CBuffer<TOutput>& OutputBuffer2,
									vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual bool	ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual bool	WriteData(CParameter& Parameter,
								  CBuffer<TInput>& InputBuffer);

protected:
	void SetBufReset1() {bResetBuf = true;}
	void SetBufReset2() {bResetBuf2 = true;}
	void SetBufReset3() {bResetBuf3 = true;}
	void SetBufResetN() {for(size_t i=0; i<vecbResetBuf.size(); i++)
     vecbResetBuf[i] = true;}

	/* Additional buffers if the derived class has multiple output streams */
	CVectorEx<TOutput>*	pvecOutputData2;
	CVectorEx<TOutput>*	pvecOutputData3;
	vector<CVectorEx<TOutput>*>	vecpvecOutputData;

	/* Max block-size are used to determine the size of the required buffer */
	int					iMaxOutputBlockSize2;
	int					iMaxOutputBlockSize3;
	vector<int>			veciMaxOutputBlockSize;
	/* Actual read (or written) size of the data */
	int					iOutputBlockSize2;
	int					iOutputBlockSize3;
	vector<int>			veciOutputBlockSize;

private:
	/* Init flag */
	bool			bDoInit;

	/* Reset flags for output cyclic-buffers */
	bool			bResetBuf;
	bool			bResetBuf2;
	bool			bResetBuf3;
	vector<bool>	vecbResetBuf;
};

/* Take an input buffer and split it 2 ways */

template<class TInput>
class CSplitModul: public CReceiverModul<TInput, TInput>
{
protected:
	virtual void SetInputBlockSize(CParameter& ReceiverParam) = 0;

	virtual void InitInternal(CParameter& ReceiverParam)
	{
		this->SetInputBlockSize(ReceiverParam);
		this->iOutputBlockSize = this->iInputBlockSize;
		this->iOutputBlockSize2 = this->iInputBlockSize;
	}

	virtual void ProcessDataInternal(CParameter&)
	{
		for (int i = 0; i < this->iInputBlockSize; i++)
		{
			TInput n = (*(this->pvecInputData))[i];
			(*this->pvecOutputData)[i] = n;
			(*this->pvecOutputData2)[i] = n;
		}
	}
};

#endif
