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

#ifndef _SIMULATIONMODUL_IMPL_H
#define _SIMULATIONMODUL_IMPL_H

#include "SimulationModul.h"

template<class TInput, class TOutput, class TInOut2>
CSimulationModul<TInput, TOutput, TInOut2>::CSimulationModul()
{
	/* Initialize all member variables with zeros */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;
	iInputBlockSize2 = 0;
	pvecOutputData2 = NULL;
	pvecInputData2 = NULL;
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::Init(CParameter& Parameter)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::
	Init(CParameter& Parameter,
		 CBuffer<TOutput>& OutputBuffer)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::
	Init(CParameter& Parameter,
		 CBuffer<TOutput>& OutputBuffer,
		 CBuffer<TInOut2>& OutputBuffer2)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;

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

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::
	TransferData(CParameter& Parameter,
				 CBuffer<TInput>& InputBuffer,
				 CBuffer<TOutput>& OutputBuffer)
{
	/* TransferData needed for simulation */
	/* Check, if enough input data is available */
	if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
	{
		/* Set request flag */
		InputBuffer.SetRequestFlag(true);

		return;
	}

	/* Get vector from transfer-buffer */
	this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

	/* Query vector from output transfer-buffer for writing */
	this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

	/* Call the underlying processing-routine */
	this->ProcessDataInternal(Parameter);

	/* Write processed data from internal memory in transfer-buffer */
	OutputBuffer.Put(this->iOutputBlockSize);
}

template<class TInput, class TOutput, class TInOut2>
bool CSimulationModul<TInput, TOutput, TInOut2>::
	ProcessDataIn(CParameter& Parameter,
				  CBuffer<TInput>& InputBuffer,
				  CBuffer<TInOut2>& InputBuffer2,
				  CBuffer<TOutput>& OutputBuffer)
{
	/* This flag shows, if enough data was in the input buffer for processing */
	bool bEnoughData = false;

	/* Check if enough data is available in the input buffer for processing */
	if ((InputBuffer.GetFillLevel() >= this->iInputBlockSize) &&
		(InputBuffer2.GetFillLevel() >= iInputBlockSize2))
	{
		bEnoughData = true;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
		pvecInputData2 = InputBuffer2.Get(iInputBlockSize2);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from FIRST input vector (definition!) */
		(*(this->pvecOutputData)).
			SetExData((*(this->pvecInputData)).GetExData());

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);
	}

	return bEnoughData;
}

template<class TInput, class TOutput, class TInOut2>
bool CSimulationModul<TInput, TOutput, TInOut2>::
	ProcessDataOut(CParameter& Parameter,
				   CBuffer<TInput>& InputBuffer,
				   CBuffer<TOutput>& OutputBuffer,
				   CBuffer<TInOut2>& OutputBuffer2)
{
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
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* Write processed data from internal memory in transfer-buffers */
		OutputBuffer.Put(this->iOutputBlockSize);
		OutputBuffer2.Put(iOutputBlockSize2);
	}

	return bEnoughData;
}

#endif
