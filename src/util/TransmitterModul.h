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

#ifndef _TRANSMITTER_MODUL_H
#define _TRANSMITTER_MODUL_H

#include "Modul.h"

/* CTransmitterModul -------------------------------------------------------- */
template<class TInput, class TOutput>
class CTransmitterModul : public CModul<TInput, TOutput>
{
public:
	CTransmitterModul();
	virtual ~CTransmitterModul() {}

	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer);
	virtual void		Init(CParameter& Parameter,
							CBuffer<TOutput>& OutputBuffer,
							CBuffer<TOutput>& OutputBuffer2,
							vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual void		ReadData(CParameter& Parameter,
								 CBuffer<TOutput>& OutputBuffer);
	virtual void		ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer);
	virtual void		ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TInput>& InputBuffer2,
									CBuffer<TInput>& InputBuffer3,
									CBuffer<TOutput>& OutputBuffer);
	virtual void		ProcessData(CParameter& Parameter,
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer,
									CBuffer<TOutput>& OutputBuffer2,
									vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual bool		WriteData(CParameter& Parameter,
								  CBuffer<TInput>& InputBuffer);
	virtual bool		WriteData(CParameter& Parameter,
								  CBuffer<TInput>& InputBuffer,
								  CBuffer<TInput>& InputBuffer2,
								  vector<CSingleBuffer<TInput> >& vecInputBuffer);

protected:
	/* Additional buffers if the derived class has multiple input streams */
	CVectorEx<TInput>*	pvecInputData2;
	CVectorEx<TInput>*	pvecInputData3;
	vector<CVectorEx<TInput>*>	vecpvecInputData;

	/* Actual read (or written) size of the data */
	int					iInputBlockSize2;
	int					iInputBlockSize3;
	vector<int>			veciInputBlockSize;

	int					iOutputBlockSize2;
	int					iMaxOutputBlockSize2;
	vector<int>			veciOutputBlockSize;
	vector<int>			veciMaxOutputBlockSize;

	/* Additional buffers if the derived class has multiple output streams */
	CVectorEx<TOutput>*	pvecOutputData2;
	vector<CVectorEx<TOutput>*>	vecpvecOutputData;
};

#endif
