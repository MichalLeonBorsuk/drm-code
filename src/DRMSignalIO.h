/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DRMSignalIO.cpp
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

#if !defined(DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_)
#define DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_

#include "soundinterface.h"
#include <math.h>
#include "matlib/Matlib.h"
#include "IQInputFilter.h"
#include "util/ReceiverModul.h"
#include "util/TransmitterModul.h"
#include "util/Utilities.h"

/* Definitions ****************************************************************/
/* Number of FFT blocks used for averaging. See next definition
   ("NUM_SMPLS_4_INPUT_SPECTRUM") for how to set the parameters */
#define NUM_AV_BLOCKS_PSD			16
#define LEN_PSD_AV_EACH_BLOCK		512

/* same but for the rpsd tag */
#define NUM_AV_BLOCKS_PSD_RSI	150
#define LEN_PSD_AV_EACH_BLOCK_RSI		256
#define PSD_OVERLAP_RSI	128

/* power gain of the Hamming window */
#define PSD_WINDOW_GAIN 0.39638

/* Length of vector for input spectrum. We use approx. 0.2 sec
   of sampled data for spectrum calculation, this is 2^13 = 8192 to
   make the FFT work more efficient. Make sure that this number is not smaller
   than the symbol lenght in 48 khz domain of longest mode (which is mode A/B:
   1280) */
#define NUM_SMPLS_4_INPUT_SPECTRUM (NUM_AV_BLOCKS_PSD * LEN_PSD_AV_EACH_BLOCK)

/* The RSI output needs 400ms with a 50% overlap, so this needs more space
   I think the RSCI spec is slightly wrong - using 150 windows consumes just over 400ms, 149 would be exact */
#define INPUT_DATA_VECTOR_SIZE (NUM_AV_BLOCKS_PSD_RSI * (LEN_PSD_AV_EACH_BLOCK_RSI-PSD_OVERLAP_RSI)+PSD_OVERLAP_RSI)

#define RNIP_SEARCH_RANGE_NARROW 5100.0
#define RNIP_SEARCH_RANGE_WIDE 15100.0
#define RNIP_EXCLUDE_BINS 2 // either side of the peak

/* Use raw 16 bit data or in text form for file format for DRM data. Defining
   the following macro will enable the raw data option */
#define FILE_DRM_USING_RAW_DATA

enum EFileOutFormat { OFF_RAW, OFF_TXT, OFF_WAV };



/* Classes ********************************************************************/
class CTransmitData : public CTransmitterModul<_COMPLEX, _COMPLEX>
{
public:
	enum EOutFormat {OF_REAL_VAL /* real valued */, OF_IQ_POS,
		OF_IQ_NEG /* I / Q */, OF_EP /* envelope / phase */};

	CTransmitData() : vecOutputs(),
		eOutputFormat(OF_REAL_VAL), rDefCarOffset((_REAL) VIRTUAL_INTERMED_FREQ)
		{}
	virtual ~CTransmitData();

	void SetIQOutput(const EOutFormat eFormat) {eOutputFormat = eFormat;}
	EOutFormat GetIQOutput() {return eOutputFormat;}

	void SetCarOffset(const CReal rNewCarOffset)
		{rDefCarOffset = rNewCarOffset;}

	void	SetOutputs(const vector<string>& o);
	void	GetOutputs(vector<string>& o) const;

	void Stop();

protected:

	vector<string>		vecOutputs;
	vector<CSoundOutInterface*>	vecpSound;
	vector<_SAMPLE>		vecsDataOut;
	int					iBlockCnt;
	int					iNumBlocks;
	EOutFormat			eOutputFormat;
	EFileOutFormat		eOutFileFormat;

	CDRMBandpassFilt	BPFilter;
	CReal				rDefCarOffset;

	CReal				rNormFactor;

	int					iBigBlockSize;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& Parameter);
};

class CReceiveData : public CReceiverModul<_REAL, _REAL>
{
public:
	CReceiveData() : pSound(NULL),
	vecrInpData(INPUT_DATA_VECTOR_SIZE, (_REAL) 0.0),
	bFippedSpectrum(false), eInChanSelection(CS_MIX_CHAN)
		 {}
	virtual ~CReceiveData();

	void SetFlippedSpectrum(const bool bNewF) {bFippedSpectrum = bNewF;}
	bool GetFlippedSpectrum() {return bFippedSpectrum;}

	void SetSoundInterface(CSoundInInterface* pS) { pSound = pS;}
	void SetInChanSel(const EInChanSel eNS) {eInChanSelection = eNS;}
	EInChanSel GetInChanSel() {return eInChanSelection;}

protected:
	CSignalLevelMeter		SignalLevelMeter;

	CSoundInInterface*		pSound;
	CVector<_SAMPLE>		vecsSoundBuffer;

	CShiftRegister<_REAL>	vecrInpData;

	bool				    bFippedSpectrum;

	EInChanSel				eInChanSelection;

	CVector<_REAL>			vecrReHist;
	CVector<_REAL>			vecrImHist;
	_COMPLEX				cCurExp;
	_COMPLEX				cExpStep;

	_REAL HilbertFilt(const _REAL rRe, const _REAL rIm);

	/* OPH: counter to count symbols within a frame in order to generate */
	/* RSCI output */
	int							iFreeSymbolCounter;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);

	void PutInputSpec(CParameter&);
	void PutPSD(CParameter&);
	void CalculatePSD(vector<_REAL>& vecrData,
	   int iLenPSDAvEachBlock = LEN_PSD_AV_EACH_BLOCK,
	   int iNumAvBlocksPSD = NUM_AV_BLOCKS_PSD,
	   int iPSDOverlap = 0);

	void CalculateSigStrengthCorrection(CParameter &ReceiverParam, const vector<_REAL> &vecrPSD);
	void CalculatePSDInterferenceTag(CParameter &ReceiverParam, const vector<_REAL> &vecrPSD);

	int FreqToBin(_REAL rFreq);
	_REAL CalcTotalPower(const vector<_REAL> &vecrData, int iStartBin, int iEndBin);

};

class COnboardDecoder : public CReceiverModul<_SAMPLE,_SAMPLE>
{
public:

	COnboardDecoder() : pSound(NULL) {}
	virtual ~COnboardDecoder() {}

	void SetSoundInterface(CSoundInInterface* pS) { pSound = pS;}

protected:

	CSoundInInterface*		pSound;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};

#endif // !defined(DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_)
