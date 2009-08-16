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

#ifndef _RECEIVER_MODUL_IMPLEMENTATION_H
#define _RECEIVER_MODUL_IMPLEMENTATION_H

#include "ReceiverModul.h"

template<class TInput, class TOutput>
CReceiverModul<TInput, TOutput>::CReceiverModul()
{
	/* Initialize all member variables with zeros */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	pvecOutputData2 = NULL;
	pvecOutputData3 = NULL;
	bResetBuf = false;
	bResetBuf2 = false;
	bResetBuf3 = false;
	bDoInit = false;
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer)
{
	/* Init flag */
	bResetBuf = false;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer,
									  CBuffer<TOutput>& OutputBuffer2)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;
	bResetBuf = false;
	bResetBuf2 = false;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer,
									  CBuffer<TOutput>& OutputBuffer2,
									  CBuffer<TOutput>& OutputBuffer3)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	bResetBuf = false;
	bResetBuf2 = false;
	bResetBuf3 = false;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}

	if (iMaxOutputBlockSize3 != 0)
		OutputBuffer3.Init(iMaxOutputBlockSize3);
	else
	{
		if (iOutputBlockSize3 != 0)
			OutputBuffer3.Init(iOutputBlockSize3);
	}
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer,
									  CBuffer<TOutput>& OutputBuffer2,
									  vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	size_t i;
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	veciMaxOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
		veciMaxOutputBlockSize[i]=0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	veciOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciOutputBlockSize.size(); i++)
		veciOutputBlockSize[i]=0;
	bResetBuf = false;
	bResetBuf2 = false;
	bResetBuf3 = false;
	vecbResetBuf.resize(vecOutputBuffer.size());
    for(i=0; i<vecbResetBuf.size(); i++)
		vecbResetBuf[i]=false;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}

    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
    {
		if (veciMaxOutputBlockSize[i] != 0)
			vecOutputBuffer[i].Init(veciMaxOutputBlockSize[i]);
		else
		{
			if (veciOutputBlockSize[i] != 0)
				vecOutputBuffer[i].Init(veciOutputBlockSize[i]);
		}
    }
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
					vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	size_t i;
	/* Init some internal variables */
	veciMaxOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
		veciMaxOutputBlockSize[i]=0;
	veciOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciOutputBlockSize.size(); i++)
		veciOutputBlockSize[i]=0;
	vecbResetBuf.resize(vecOutputBuffer.size());
    for(i=0; i<vecbResetBuf.size(); i++)
		vecbResetBuf[i]=false;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);

	/* Init output transfer buffers */
    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
    {
		if (veciMaxOutputBlockSize[i] != 0)
			vecOutputBuffer[i].Init(veciMaxOutputBlockSize[i]);
		else
		{
			if (veciOutputBlockSize[i] != 0)
				vecOutputBuffer[i].Init(veciOutputBlockSize[i]);
		}
    }
}

template<class TInput, class TOutput>
bool CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == true)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer);

		/* Reset init flag */
		bDoInit = false;
	}

	/* Special case if input block size is zero */
	if (this->iInputBlockSize == 0)
	{
		InputBuffer.Clear();

		return false;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* This flag shows, if enough data was in the input buffer for processing */
	bool bEnoughData = false;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = true;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from vectors */
		(*(this->pvecOutputData)).
			SetExData((*(this->pvecInputData)).GetExData());

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == true)
		{
			/* Reset flag and clear buffer */
			bResetBuf = false;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
bool CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer,
				CBuffer<TOutput>& OutputBuffer2)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == true)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2);

		/* Reset init flag */
		bDoInit = false;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	bool bEnoughData = false;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = true;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == true)
		{
			/* Reset flag and clear buffer */
			bResetBuf = false;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}
		if (bResetBuf2 == true)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = false;
			OutputBuffer2.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer2.Put(this->iOutputBlockSize2);
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
bool CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer,
				CBuffer<TOutput>& OutputBuffer2,
				CBuffer<TOutput>& OutputBuffer3)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == true)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2, OutputBuffer3);

		/* Reset init flag */
		bDoInit = false;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	bool bEnoughData = false;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = true;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		pvecOutputData3 = OutputBuffer3.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == true)
		{
			/* Reset flag and clear buffer */
			bResetBuf = false;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}

		if (bResetBuf2 == true)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = false;
			OutputBuffer2.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer2.Put(iOutputBlockSize2);
		}

		if (bResetBuf3 == true)
		{
			/* Reset flag and clear buffer */
			bResetBuf3 = false;
			OutputBuffer3.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer3.Put(iOutputBlockSize3);
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
bool CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer,
				CBuffer<TOutput>& OutputBuffer2,
				vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == true)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2, vecOutputBuffer);

		/* Reset init flag */
		bDoInit = false;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	bool bEnoughData = false;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		size_t i;
		bEnoughData = true;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		vecpvecOutputData.resize(vecOutputBuffer.size());
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			vecpvecOutputData[i] = vecOutputBuffer[i].QueryWriteBuffer();
		}

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == true)
		{
			/* Reset flag and clear buffer */
			bResetBuf = false;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}

		if (bResetBuf2 == true)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = false;
			OutputBuffer2.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer2.Put(iOutputBlockSize2);
		}

		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			if (vecbResetBuf[i] == true)
			{
				/* Reset flag and clear buffer */
				vecbResetBuf[i] = false;
				vecOutputBuffer[i].Clear();
			}
			else
			{
				/* Write processed data from internal memory in transfer-buffer */
				vecOutputBuffer[i].Put(veciOutputBlockSize[i]);
			}
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
bool CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == true)
	{
		/* Call init routine */
		Init(Parameter, vecOutputBuffer);

		/* Reset init flag */
		bDoInit = false;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	bool bEnoughData = false;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		size_t i;
		bEnoughData = true;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		vecpvecOutputData.resize(vecOutputBuffer.size());
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			vecpvecOutputData[i] = vecOutputBuffer[i].QueryWriteBuffer();
		}

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);

		/* Reset output-buffers if flag was set by processing routine */
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			if (vecbResetBuf[i] == true)
			{
				/* Reset flag and clear buffer */
				vecbResetBuf[i] = false;
				vecOutputBuffer[i].Clear();
			}
			else
			{
				/* Write processed data from internal memory in transfer-buffer */
				vecOutputBuffer[i].Put(veciOutputBlockSize[i]);
			}
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
void CReceiverModul<TInput, TOutput>::
	ReadData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == true)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer);

		/* Reset init flag */
		bDoInit = false;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Query vector from output transfer-buffer for writing */
	this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

	/* Call the underlying processing-routine */
	this->ProcessDataThreadSave(Parameter);

	/* Reset output-buffers if flag was set by processing routine */
	if (bResetBuf == true)
	{
		/* Reset flag and clear buffer */
		bResetBuf = false;
		OutputBuffer.Clear();
	}
	else
	{
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);
	}
}

template<class TInput, class TOutput>
bool CReceiverModul<TInput, TOutput>::
	WriteData(CParameter& Parameter, CBuffer<TInput>& InputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == true)
	{
		/* Call init routine */
		Init(Parameter);

		/* Reset init flag */
		bDoInit = false;
	}

	/* Special case if input block size is zero and buffer, too */
	if ((InputBuffer.GetFillLevel() == 0) && (this->iInputBlockSize == 0))
	{
		InputBuffer.Clear();
		return false;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* This flag shows, if enough data was in the input buffer for processing */
	bool bEnoughData = false;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = true;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);
	}

	return bEnoughData;
}

#endif
