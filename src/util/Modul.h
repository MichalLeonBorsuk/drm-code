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

#if !defined(AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_)
#define AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_

#include "Buffer.h"
#include "../Parameter.h"
#ifdef QT_CORE_LIB
# include <qthread.h>
# include <qmutex.h>
#endif


/* Classes ********************************************************************/
/* CModul ------------------------------------------------------------------- */
template<class TInput, class TOutput>
class CModul
{
public:
	CModul();
	virtual ~CModul() {}

	virtual void Init(CParameter& Parameter);
	virtual void Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer);

protected:
	CVectorEx<TInput>*	pvecInputData;
	CVectorEx<TOutput>*	pvecOutputData;

	/* Max block-size are used to determine the size of the required buffer */
	int					iMaxOutputBlockSize;
	/* Actual read (or written) size of the data */
	int					iInputBlockSize;
	int					iOutputBlockSize;

	void				InitThreadSave(CParameter& Parameter);
	virtual void		InitInternal(CParameter& Parameter) = 0;
	void				ProcessDataThreadSave(CParameter& Parameter);
	virtual void		ProcessDataInternal(CParameter& Parameter) = 0;

#ifdef QT_CORE_LIB
	void				Lock() { mutex.lock(); }
	void				Unlock() { mutex.unlock(); }
private:
	QMutex mutex;
#else
	void				Lock() { }
	void				Unlock() { }
#endif
};

/* Implementation *************************************************************/

template<class TInput, class TOutput>
CModul<TInput, TOutput>::CModul()
{
	/* Initialize everything with zeros */
	iMaxOutputBlockSize = 0;
	iInputBlockSize = 0;
	iOutputBlockSize = 0;
	pvecInputData = NULL;
	pvecOutputData = NULL;
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::ProcessDataThreadSave(CParameter& Parameter)
{
	/* Get a lock for the resources */
	this->Lock();

	/* Call processing routine of derived modul */
	this->ProcessDataInternal(Parameter);

	/* Unlock resources */
	this->Unlock();
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::InitThreadSave(CParameter& Parameter)
{
	/* Get a lock for the resources */
	this->Lock();

	try
	{
		/* Call init of derived modul */
		this->InitInternal(Parameter);

		/* Unlock resources */
		this->Unlock();
	}

	catch (CGenErr)
	{
		/* Unlock resources */
		Unlock();

		/* Throws the same error again which was send by the function */
		throw;
	}
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init some internal variables */
	iInputBlockSize = 0;

	/* Call init of derived modul */
	this->InitThreadSave(Parameter);
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::Init(CParameter& Parameter,
								   CBuffer<TOutput>& OutputBuffer)
{
	/* Init some internal variables */
	iMaxOutputBlockSize = 0;
	iInputBlockSize = 0;
	iOutputBlockSize = 0;

	/* Call init of derived modul */
	this->InitThreadSave(Parameter);

	/* Init output transfer buffer */
	if (iMaxOutputBlockSize != 0)
		OutputBuffer.Init(iMaxOutputBlockSize);
	else
	{
		if (iOutputBlockSize != 0)
			OutputBuffer.Init(iOutputBlockSize);
	}
}

#endif
