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

#ifndef _SIMULATION_MODUL_H
#define _SIMULATION_MODUL_H

#include "Modul.h"

// FIXME something nicer than using "MAX_NUM_TAPS_DRM_CHAN"
/* For simulation, data from channel simulation */
#define MAX_NUM_TAPS_DRM_CHAN			4

/* Path for simulation output and status files */
#define SIM_OUT_FILES_PATH				"test/"

template<class T> class CChanSimData
{
public:
	T					tIn; /* Channel input data */
	T					tOut; /* Output of the channel (with noise) */
	T					tRef; /* Channel reference signal (without noise) */
	_COMPLEX			veccTap[MAX_NUM_TAPS_DRM_CHAN]; /* Tap gains */
	_COMPLEX			veccTapBackw[MAX_NUM_TAPS_DRM_CHAN];
};

template<class TInput, class TOutput, class TInOut2>
class CSimulationModul : public CModul<TInput, TOutput>
{
public:
	CSimulationModul();
	virtual ~CSimulationModul() {}

	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TInOut2>& OutputBuffer2);
	virtual void		TransferData(CParameter& Parameter,
									 CBuffer<TInput>& InputBuffer,
									 CBuffer<TOutput>& OutputBuffer);


// TEST "ProcessDataIn" "ProcessDataOut"
	virtual bool	ProcessDataIn(CParameter& Parameter,
									  CBuffer<TInput>& InputBuffer,
									  CBuffer<TInOut2>& InputBuffer2,
									  CBuffer<TOutput>& OutputBuffer);
	virtual bool	ProcessDataOut(CParameter& Parameter,
									   CBuffer<TInput>& InputBuffer,
									   CBuffer<TOutput>& OutputBuffer,
									   CBuffer<TInOut2>& OutputBuffer2);


protected:
	/* Additional buffers if the derived class has multiple output streams */
	CVectorEx<TInOut2>*	pvecOutputData2;

	/* Max block-size are used to determine the size of the requiered buffer */
	int					iMaxOutputBlockSize2;
	/* Actual read (or written) size of the data */
	int					iOutputBlockSize2;

	/* Additional buffers if the derived class has multiple input streams */
	CVectorEx<TInOut2>*	pvecInputData2;

	/* Actual read (or written) size of the data */
	int					iInputBlockSize2;
};

#endif
