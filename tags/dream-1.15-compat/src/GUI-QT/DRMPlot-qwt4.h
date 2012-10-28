/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#ifndef __DRMPLOT_QWT4_H
#define __DRMPLOT_QWT4_H

protected:
	void SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetData(CVector<_REAL>& vecrData1, CVector<_REAL>& vecrData2,
				 CVector<_REAL>& vecrScale);
	void SetData(CVector<_COMPLEX>& veccData);
	void SetData(CVector<_COMPLEX>& veccMSCConst,
				 CVector<_COMPLEX>& veccSDCConst,
				 CVector<_COMPLEX>& veccFACConst);
	void SetQAM4Grid();
	void SetQAM16Grid();
	void SetQAM64Grid();

	void SetupAvIR();
	void SetupTranFct();
	void SetupAudioSpec();
	void SetupFreqSamOffsHist();
	void SetupDopplerDelayHist();
	void SetupSNRAudHist();
	void SetupPSD();
	void SetupSNRSpectrum();
	void SetupInpSpec();
	void SetupFACConst();
	void SetupSDCConst(const ECodScheme eNewCoSc);
	void SetupMSCConst(const ECodScheme eNewCoSc);
	void SetupAllConst();
	void SetupInpPSD();
	void SetupInpSpecWaterf();

	void AddWhatsThisHelpChar(const ECharType NCharType);
	virtual void showEvent(QShowEvent* pEvent);
	virtual void hideEvent(QHideEvent* pEvent);

	/* Colors */
	QColor			MainPenColorPlot;
	QColor			MainPenColorConst;
	QColor			MainGridColorPlot;
	QColor			SpecLine1ColorPlot;
	QColor			SpecLine2ColorPlot;
	QColor			PassBandColorPlot;
	QColor			BckgrdColorPlot;

	QSize			LastCanvasSize;

	ECharType		CurCharType;
	ECharType		InitCharType;
	long			main1curve, main2curve;
	long			curve1, curve2, curve3, curve4, curve5, curve6;
	QwtSymbol		MarkerSym1, MarkerSym2, MarkerSym3;

	_BOOLEAN		bOnTimerCharMutexFlag;
	QTimer			TimerChart;

	CDRMReceiver*	pDRMRec;
#endif
