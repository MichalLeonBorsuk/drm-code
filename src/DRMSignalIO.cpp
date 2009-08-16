/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Cesco (HB9TLK)
 *
 * Description:
 *	Transmit and receive data
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

#include "DRMSignalIO.h"
#include "sound.h"
#include "sound/soundfile.h"
#include <iostream>
#include <limits>

const _SAMPLE max_sample = numeric_limits<_SAMPLE>::max();

/* Implementation *************************************************************/

/******************************************************************************\
* Transmitter                                                                  *
\******************************************************************************/
void CTransmitData::SetOutputs(const vector<string>& o)
{
	vecOutputs = o;
}

void CTransmitData::GetOutputs(vector<string>& o) const
{
	o = vecOutputs;
}

void CTransmitData::ProcessDataInternal(CParameter&)
{
	int i;

	/* Apply bandpass filter */
	//BPFilter.Process(*pvecInputData);
	BPFilter.Process(*pvecInputData);

	/* Convert vector type. Fill vector with symbols (collect them) */
	const int iNs2 = iInputBlockSize * 2;
	for (i = 0; i < iNs2; i += 2)
	{
		const int iCurIndex = iBlockCnt * iNs2 + i;

		/* Imaginary, real */
		const _SAMPLE sCurOutReal =
			(_SAMPLE) ((*pvecInputData)[i / 2].real() * rNormFactor);
		const _SAMPLE sCurOutImag =
			(_SAMPLE) ((*pvecInputData)[i / 2].imag() * rNormFactor);

		/* Envelope, phase */
		const _SAMPLE sCurOutEnv =
			(_SAMPLE) (Abs((*pvecInputData)[i / 2]) * (_REAL) 256.0);
		const _SAMPLE sCurOutPhase = /* 2^15 / pi / 2 -> approx. 5000 */
			(_SAMPLE) (Angle((*pvecInputData)[i / 2]) * (_REAL) 5000.0);

		switch (eOutputFormat)
		{
		case OF_REAL_VAL:
			/* Use real valued signal as output for both sound card channels */
			vecsDataOut[iCurIndex] = vecsDataOut[iCurIndex + 1] = sCurOutReal;
			break;

		case OF_IQ_POS:
			/* Send inphase and quadrature (I / Q) signal to stereo sound card
			   output. I: left channel, Q: right channel */
			vecsDataOut[iCurIndex] = sCurOutReal;
			vecsDataOut[iCurIndex + 1] = sCurOutImag;
			break;

		case OF_IQ_NEG:
			/* Send inphase and quadrature (I / Q) signal to stereo sound card
			   output. I: right channel, Q: left channel */
			vecsDataOut[iCurIndex] = sCurOutImag;
			vecsDataOut[iCurIndex + 1] = sCurOutReal;
			break;

		case OF_EP:
			/* Send envelope and phase signal to stereo sound card
			   output. Envelope: left channel, Phase: right channel */
			vecsDataOut[iCurIndex] = sCurOutEnv;
			vecsDataOut[iCurIndex + 1] = sCurOutPhase;
			break;
		}
	}

	iBlockCnt++;
	if (iBlockCnt == iNumBlocks)
	{
		iBlockCnt = 0;
		for(size_t i=0; i<vecpSound.size(); i++)
		{
			/* Write data to sound card or file. */
			vecpSound[i]->Write(vecsDataOut);
		}
	}
}

void CTransmitData::InitInternal(CParameter& TransmParam)
{
	const int iSymbolBlockSize = TransmParam.CellMappingTable.iSymbolBlockSize;
	size_t i;

	/* Init vector for storing a complete DRM frame number of OFDM symbols */
	iBlockCnt = 0;
	iNumBlocks = TransmParam.CellMappingTable.iNumSymPerFrame;
	iBigBlockSize = iSymbolBlockSize * 2 /* Stereo */ * iNumBlocks;
	rDefCarOffset = TransmParam.rCarOffset;

	vecsDataOut.resize(iBigBlockSize);

	vecpSound.clear();
	for(i=0; i<vecOutputs.size(); i++)
	{

        CSoundOutInterface* pSound;
		const string& s = vecOutputs[i];
		string ext;
		size_t p = s.rfind('.');
		if (p != string::npos)
			ext = s.substr(p + 1);
		if(ext=="wav")
		{
			pSound = new CSoundFileOut;
		}
		else if(ext=="txt")
		{
			pSound = new CSoundFileOut;
		}
		else if(ext=="raw")
		{
			pSound = new CSoundFileOut;
		}
		else
		{
			/* Init sound interface */
			pSound = new CSoundOut;
		}
		pSound->SetDev(s);
		pSound->Init(iBigBlockSize);
		vecpSound.push_back(pSound);
	}

	/* Init bandpass filter object */
	BPFilter.Init(iSymbolBlockSize, rDefCarOffset,
                    TransmParam.GetNominalBandwidth(), 300.0 /* Hz */);

	/* All robustness modes and spectrum occupancies should have the same output
	   power. Calculate the normalisation factor based on the average power of
	   symbol (the number 3000 was obtained through output tests) */
	rNormFactor = (CReal) 3000.0 / Sqrt(TransmParam.CellMappingTable.rAvPowPerSymbol);

	/* Define block-size for input */
	iInputBlockSize = iSymbolBlockSize;
}

void CTransmitData::Stop()
{
	size_t i;
	for(i=0; i<vecpSound.size(); i++)
	{
		if(vecpSound[i])
		{
			vecpSound[i]->Close();
			delete vecpSound[i];
		}
	}
	vecpSound.clear();
}

CTransmitData::~CTransmitData()
{
	Stop();
}

/******************************************************************************\
* Receive data from the sound card                                             *
\******************************************************************************/
void CReceiveData::ProcessDataInternal(CParameter& Parameter)
{
	int i;

	Parameter.Lock();
	int iNumSymPerFrame = Parameter.CellMappingTable.iNumSymPerFrame;
	bool bMeasurePSD = Parameter.Measurements.PSD.wanted();
	bool bMeasureInputSpectrum = Parameter.Measurements.InputSpectrum.wanted();
	Parameter.Unlock();

	/* OPH: update free-running symbol counter */
	iFreeSymbolCounter++;
	if (iFreeSymbolCounter >= iNumSymPerFrame)
	{
		iFreeSymbolCounter = 0;

		/* calculate the PSD once per frame for the RSI output and/or charts */
		if(bMeasurePSD)
			PutPSD(Parameter);

		/* calculate the InputSpectrum once per frame for the charts */
        if(bMeasureInputSpectrum)
            PutInputSpec(Parameter);
	}

	if(pSound == NULL)
		return;

		/* Get data from sound interface. The read function must be a
		   blocking function! */
	bool bBad = pSound->Read(vecsSoundBuffer);

	Parameter.Lock();
	if (bBad == false)
		Parameter.ReceiveStatus.Interface.SetStatus(RX_OK);
	else
		Parameter.ReceiveStatus.Interface.SetStatus(CRC_ERROR);
	Parameter.Unlock();

		/* Write data to output buffer. Do not set the switch command inside
		   the for-loop for efficiency reasons */
		switch (eInChanSelection)
		{
		case CS_LEFT_CHAN:
			for (i = 0; i < iOutputBlockSize; i++)
				(*pvecOutputData)[i] = (_REAL) vecsSoundBuffer[2 * i];
			break;

		case CS_RIGHT_CHAN:
			for (i = 0; i < iOutputBlockSize; i++)
				(*pvecOutputData)[i] = (_REAL) vecsSoundBuffer[2 * i + 1];
			break;

		case CS_MIX_CHAN:
			for (i = 0; i < iOutputBlockSize; i++)
			{
				/* Mix left and right channel together */
				const _REAL rLeftChan = vecsSoundBuffer[2 * i];
				const _REAL rRightChan = vecsSoundBuffer[2 * i + 1];

				(*pvecOutputData)[i] = _REAL(rLeftChan + rRightChan) / 2.0;
			}
			break;

		/* I / Q input */
		case CS_IQ_POS:
			for (i = 0; i < iOutputBlockSize; i++)
			{
				(*pvecOutputData)[i] =
					HilbertFilt((_REAL) vecsSoundBuffer[2 * i],
					(_REAL) vecsSoundBuffer[2 * i + 1]);
			}
			break;

		case CS_IQ_NEG:
			for (i = 0; i < iOutputBlockSize; i++)
			{
				(*pvecOutputData)[i] =
					HilbertFilt((_REAL) vecsSoundBuffer[2 * i + 1],
					(_REAL) vecsSoundBuffer[2 * i]);
			}
			break;

		case CS_IQ_POS_ZERO:
			for (i = 0; i < iOutputBlockSize; i++)
			{
				/* Shift signal to vitual intermediate frequency before applying
				   the Hilbert filtering */
				_COMPLEX cCurSig = _COMPLEX((_REAL) vecsSoundBuffer[2 * i],
					(_REAL) vecsSoundBuffer[2 * i + 1]);

				cCurSig *= cCurExp;

				/* Rotate exp-pointer on step further by complex multiplication
				   with precalculated rotation vector cExpStep */
				cCurExp *= cExpStep;

				(*pvecOutputData)[i] =
					HilbertFilt(cCurSig.real(), cCurSig.imag());
			}
			break;

		case CS_IQ_NEG_ZERO:
			for (i = 0; i < iOutputBlockSize; i++)
			{
				/* Shift signal to vitual intermediate frequency before applying
				   the Hilbert filtering */
				_COMPLEX cCurSig = _COMPLEX((_REAL) vecsSoundBuffer[2 * i + 1],
					(_REAL) vecsSoundBuffer[2 * i]);

				cCurSig *= cCurExp;

				/* Rotate exp-pointer on step further by complex multiplication
				   with precalculated rotation vector cExpStep */
				cCurExp *= cExpStep;

				(*pvecOutputData)[i] =
					HilbertFilt(cCurSig.real(), cCurSig.imag());
			}
			break;
		}

	/* Flip spectrum if necessary ------------------------------------------- */
	if (bFippedSpectrum == true)
	{
		static bool bFlagInv = false;

		for (i = 0; i < iOutputBlockSize; i++)
		{
			/* We flip the spectrum by using the mirror spectrum at the negative
			   frequencys. If we shift by half of the sample frequency, we can
			   do the shift without the need of a Hilbert transformation */
			if (bFlagInv == false)
			{
				(*pvecOutputData)[i] = -(*pvecOutputData)[i];
				bFlagInv = true;
			}
			else
				bFlagInv = false;
		}
	}


	/* Copy data in buffer for spectrum calculation */
	vecrInpData.AddEnd((*pvecOutputData), iOutputBlockSize);

	/* Update level meter */
	SignalLevelMeter.Update((*pvecOutputData));
	Parameter.Lock();
	Parameter.SetIFSignalLevel(SignalLevelMeter.Level());
	Parameter.Unlock();

}

void CReceiveData::InitInternal(CParameter& Parameter)
{
	/* Init sound interface. Set it to one symbol. The sound card interface
		   has to taken care about the buffering data of a whole MSC block.
		   Use stereo input (* 2) */

	if(pSound == NULL)
		return;

	Parameter.Lock();
	/* Define output block-size */
	iOutputBlockSize = Parameter.CellMappingTable.iSymbolBlockSize;
	Parameter.Unlock();
	pSound->Init(iOutputBlockSize * 2);

	/* Init buffer size for taking stereo input */
	vecsSoundBuffer.Init(iOutputBlockSize * 2);

	/* Init signal meter */
	SignalLevelMeter.Init(0);

	/* Inits for I / Q input */
	vecrReHist.Init(NUM_TAPS_IQ_INPUT_FILT, (_REAL) 0.0);
	vecrImHist.Init(NUM_TAPS_IQ_INPUT_FILT, (_REAL) 0.0);

	/* Start with phase null (can be arbitrarily chosen) */
	cCurExp = (_REAL) 1.0;

	/* Set rotation vector to mix signal from zero frequency to virtual
	   intermediate frequency */
	const _REAL rNormCurFreqOffsetIQ =
		(_REAL) 2.0 * crPi * ((_REAL) VIRTUAL_INTERMED_FREQ / SOUNDCRD_SAMPLE_RATE);

	cExpStep = _COMPLEX(cos(rNormCurFreqOffsetIQ), sin(rNormCurFreqOffsetIQ));


	/* OPH: init free-running symbol counter */
	iFreeSymbolCounter = 0;

}

_REAL CReceiveData::HilbertFilt(const _REAL rRe, const _REAL rIm)
{
/*
	Hilbert filter for I / Q input data. This code is based on code written
	by Cesco (HB9TLK)
*/
    int i;

	/* Move old data */
    for (i = 0; i < NUM_TAPS_IQ_INPUT_FILT - 1; i++)
	{
		vecrReHist[i] = vecrReHist[i + 1];
		vecrImHist[i] = vecrImHist[i + 1];
	}

    vecrReHist[NUM_TAPS_IQ_INPUT_FILT - 1] = rRe;
    vecrImHist[NUM_TAPS_IQ_INPUT_FILT - 1] = rIm;

	/* Filter */
    _REAL rSum = (_REAL) 0.0;
    for (i = 1; i < NUM_TAPS_IQ_INPUT_FILT; i += 2)
		rSum += fHilFiltIQ[i] * vecrImHist[i];

	return (rSum + vecrReHist[IQ_INP_HIL_FILT_DELAY]) / 2;
}

CReceiveData::~CReceiveData()
{
}

void CReceiveData::CalculatePSD(vector<_REAL>& vecrData,
    int iLenPSDAvEachBlock, int iNumAvBlocksPSD, int iPSDOverlap)
{
        /* Define a plan at the beginning. This should speed up the calls to fftw */
        const CFftPlans FftPlans(iLenPSDAvEachBlock);

        /* Length of spectrum vector including Nyquist frequency */
        const int iLenSpecWithNyFreq = iLenPSDAvEachBlock / 2 + 1;
        /* Init input and output vectors */
        vecrData.resize(iLenSpecWithNyFreq);

        /* Init the constants for scale and normalization */
       //_REAL rFactorScale = _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(iLenSpecWithNyFreq) / 2000.0;

        const _REAL rNormData = (_REAL) max_sample * max_sample *
                iLenPSDAvEachBlock * iLenPSDAvEachBlock *
                iNumAvBlocksPSD * _REAL(PSD_WINDOW_GAIN);

        /* Init intermediate vectors */
        CRealVector vecrAvSqMagSpect(iLenSpecWithNyFreq, (CReal) 0.0);
        CRealVector vecrFFTInput(iLenPSDAvEachBlock);

        /* Init Hamming window */
        CRealVector vecrHammWin(Hamming(iLenPSDAvEachBlock));

        /* Calculate FFT of each small block and average results (estimation
           of PSD of input signal) */

        int i;
        for (i = 0; i < iNumAvBlocksPSD; i++)
        {
                /* Copy data from shift register in Matlib vector */
                for (int j = 0; j < iLenPSDAvEachBlock; j++)
                        vecrFFTInput[j] = vecrInpData[j + i * (iLenPSDAvEachBlock - iPSDOverlap)];

                /* Apply Hamming window */
                vecrFFTInput *= vecrHammWin;

                /* Calculate squared magnitude of spectrum and average results */
                vecrAvSqMagSpect += SqMag(rfft(vecrFFTInput)); //, FftPlans));
        }

        /* Log power spectrum data */
        for (i = 0; i <iLenSpecWithNyFreq; i++)
        {
                const _REAL rNormSqMag = vecrAvSqMagSpect[i] / rNormData;

                if (rNormSqMag > 0)
                        vecrData[i] = (_REAL) 10.0 * log10(rNormSqMag);
                else
                        vecrData[i] = RET_VAL_LOG_0;
        }
}

/* Calculate PSD and put it into the CParameter class.
 * The data will be used by the chart plotter and RSCI output.
 */
void CReceiveData::PutPSD(CParameter &ReceiverParam)
{
	int i, j;

	vector<_REAL> vecrData;

	int iStartBin, iEndBin, iVecSize, iStartIndex;

	if(ReceiverParam.Measurements.bETSIPSD)
	{
        CalculatePSD(vecrData, LEN_PSD_AV_EACH_BLOCK_RSI, NUM_AV_BLOCKS_PSD_RSI, PSD_OVERLAP_RSI);

        /* Data required for rpsd tag */
        /* extract the values from -8kHz to +8kHz/18kHz relative to 12kHz, i.e. 4kHz to 20kHz */
        /*const int startBin = 4000.0 * LEN_PSD_AV_EACH_BLOCK_RSI /SOUNDCRD_SAMPLE_RATE;
        const int endBin = 20000.0 * LEN_PSD_AV_EACH_BLOCK_RSI /SOUNDCRD_SAMPLE_RATE;*/
        /* The above calculation doesn't round in the way FhG expect. Probably better to specify directly */

        /* For 20k mode, we need -8/+18, which is more than the Nyquist rate of 24kHz. */
        /* Assume nominal freq = 7kHz (i.e. 2k to 22k) and pad with zeroes (roughly 1kHz each side) */

        //_REAL rIFCentreFrequency = ReceiverParam.FrontEndParameters.rIFCentreFreq;

        ESpecOcc eSpecOcc = ReceiverParam.Channel.eSpectrumOccupancy;
        if (eSpecOcc == SO_4 || eSpecOcc == SO_5)
        {
            iStartBin = 0;
            iEndBin = 127;
            iVecSize = 139;
        }
        else
        {
            iStartBin = 22;
            iEndBin = 106;
            iVecSize = iEndBin - iStartBin + 1; //85
        }
        /* Line up the the middle of the vector with the quarter-Nyquist bin of FFT */
        iStartIndex = iStartBin - (LEN_PSD_AV_EACH_BLOCK_RSI/4) + (iVecSize-1)/2;
	}
	else
	{
	    // traditional dream values for plot in System Evaluation Dialog
        CalculatePSD(vecrData, LEN_PSD_AV_EACH_BLOCK, NUM_AV_BLOCKS_PSD, 0);
        iVecSize = vecrData.size();
        iStartIndex = iStartBin = 0;
        iEndBin = iVecSize-1;
	}

	vector<_REAL> psd(iVecSize);
	for (i=iStartIndex, j=iStartBin; j<=iEndBin; i++,j++)
		psd[i] = vecrData[j];

    ReceiverParam.Lock();
    ReceiverParam.Measurements.PSD.set(psd);
	CalculateSigStrengthCorrection(ReceiverParam, vecrData);
	CalculatePSDInterferenceTag(ReceiverParam, vecrData);
    ReceiverParam.Unlock();

}

/* Calculate input spectum (no averaging) and put it into the CParameter class.
 */
void CReceiveData::PutInputSpec(CParameter &ReceiverParam)
{
	/* Length of spectrum vector including Nyquist frequency */
	const int iLenSpecWithNyFreq = NUM_SMPLS_4_INPUT_SPECTRUM / 2 + 1;

	const _REAL rNormData = (_REAL) max_sample * max_sample *
		NUM_SMPLS_4_INPUT_SPECTRUM * NUM_SMPLS_4_INPUT_SPECTRUM;

	/* Copy data from shift register in Matlib vector */
	CRealVector vecrFFTInput(NUM_SMPLS_4_INPUT_SPECTRUM);
	for (int i = 0; i < NUM_SMPLS_4_INPUT_SPECTRUM; i++)
		vecrFFTInput[i] = vecrInpData[i];

	/* Get squared magnitude of spectrum */
	CRealVector vecrSqMagSpect(iLenSpecWithNyFreq);
	vecrSqMagSpect =
		SqMag(rfft(vecrFFTInput * Hann(NUM_SMPLS_4_INPUT_SPECTRUM)));

	/* Log power spectrum data */
    vector<_REAL> spec(iLenSpecWithNyFreq);
	for (int i = 0; i < iLenSpecWithNyFreq; i++)
	{
		const _REAL rNormSqMag = vecrSqMagSpect[i] / rNormData;
		if (rNormSqMag > 0)
			spec[i] = (_REAL) 10.0 * log10(rNormSqMag);
		else
			spec[i] = RET_VAL_LOG_0;
	}

    ReceiverParam.Lock();
	ReceiverParam.Measurements.InputSpectrum.set(spec);
    ReceiverParam.Unlock();
}


/*
 * This function is called in a context where the ReceiverParam structure is Locked.
 */
void CReceiveData::CalculateSigStrengthCorrection(CParameter &ReceiverParam,
    const vector<_REAL>& vecrPSD)
{

	_REAL rCorrection = _REAL(0.0);

	/* Calculate signal power in measurement bandwidth */

	_REAL rFreqKmin, rFreqKmax;

	_REAL rIFCentreFrequency = ReceiverParam.FrontEndParameters.rIFCentreFreq;

	if (ReceiverParam.GetAcquiState() == AS_WITH_SIGNAL &&
		ReceiverParam.FrontEndParameters.bAutoMeasurementBandwidth)
	{
		// Receiver is locked, so measure in the current DRM signal bandwidth Kmin to Kmax
		_REAL rDCFrequency = ReceiverParam.GetDCFrequency();
		rFreqKmin = rDCFrequency + _REAL(ReceiverParam.CellMappingTable.iCarrierKmin)/ReceiverParam.CellMappingTable.iFFTSizeN * SOUNDCRD_SAMPLE_RATE;
		rFreqKmax = rDCFrequency + _REAL(ReceiverParam.CellMappingTable.iCarrierKmax)/ReceiverParam.CellMappingTable.iFFTSizeN * SOUNDCRD_SAMPLE_RATE;
	}
	else
	{
		// Receiver unlocked, or measurement is requested in fixed bandwidth
		_REAL rMeasBandwidth = ReceiverParam.FrontEndParameters.rDefaultMeasurementBandwidth;
		rFreqKmin = rIFCentreFrequency - rMeasBandwidth/_REAL(2.0);
		rFreqKmax = rIFCentreFrequency + rMeasBandwidth/_REAL(2.0);
	}

	_REAL rSigPower = CalcTotalPower(vecrPSD, FreqToBin(rFreqKmin), FreqToBin(rFreqKmax));

	if (ReceiverParam.FrontEndParameters.eSMeterCorrectionType == CFrontEndParameters::S_METER_CORRECTION_TYPE_AGC_ONLY)
	{
		/* Write it to the receiver params to help with calculating the signal strength */
		rCorrection += _REAL(10.0) * log10(rSigPower);
	}
	else if (ReceiverParam.FrontEndParameters.eSMeterCorrectionType == CFrontEndParameters::S_METER_CORRECTION_TYPE_AGC_RSSI)
	{
		_REAL rSMeterBandwidth = ReceiverParam.FrontEndParameters.rSMeterBandwidth;

		_REAL rFreqSMeterMin = _REAL(rIFCentreFrequency - rSMeterBandwidth / _REAL(2.0));
		_REAL rFreqSMeterMax = _REAL(rIFCentreFrequency + rSMeterBandwidth / _REAL(2.0));

		_REAL rPowerInSMeterBW = CalcTotalPower(vecrPSD, FreqToBin(rFreqSMeterMin), FreqToBin(rFreqSMeterMax));

		/* Write it to the receiver params to help with calculating the signal strength */

		rCorrection += _REAL(10.0) * log10(rSigPower/rPowerInSMeterBW);
	}

	/* Add on the calibration factor for the current mode */
	switch(ReceiverParam.eModulation)
	{
	case DRM:
		rCorrection += ReceiverParam.FrontEndParameters.rCalFactorDRM;
		break;
	case AM: case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:
		rCorrection += ReceiverParam.FrontEndParameters.rCalFactorAM;
		break;
	case NONE:
		;
	}

	ReceiverParam.rSigStrengthCorrection = rCorrection;

	return;
}

/*
 * This function is called in a context where the ReceiverParam structure is Locked.
 */
void CReceiveData::CalculatePSDInterferenceTag(CParameter &ReceiverParam,
    const vector<_REAL> &vecrPSD)
{

	/* Interference tag (rnip) */

	// Calculate search range: defined as +/-5.1kHz except if locked and in 20k
	_REAL rIFCentreFrequency = ReceiverParam.FrontEndParameters.rIFCentreFreq;

	_REAL rFreqSearchMin = rIFCentreFrequency - _REAL(RNIP_SEARCH_RANGE_NARROW);
	_REAL rFreqSearchMax = rIFCentreFrequency + _REAL(RNIP_SEARCH_RANGE_NARROW);

	ESpecOcc eSpecOcc = ReceiverParam.Channel.eSpectrumOccupancy;

	if (ReceiverParam.GetAcquiState() == AS_WITH_SIGNAL &&
		(eSpecOcc == SO_4 || eSpecOcc == SO_5) )
	{
		rFreqSearchMax = rIFCentreFrequency + _REAL(RNIP_SEARCH_RANGE_WIDE);
	}
	int iSearchStartBin = FreqToBin(rFreqSearchMin);
	int iSearchEndBin = FreqToBin(rFreqSearchMax);

	if (iSearchStartBin < 0) iSearchStartBin = 0;
	if (iSearchEndBin > LEN_PSD_AV_EACH_BLOCK_RSI/2)
		iSearchEndBin = LEN_PSD_AV_EACH_BLOCK_RSI/2;

	_REAL rMaxPSD = _REAL(-1000.0);
	int iMaxPSDBin = 0;

	for (int i=iSearchStartBin; i<=iSearchEndBin; i++)
	{
		_REAL rPSD = _REAL(2.0) * pow(_REAL(10.0), vecrPSD[i]/_REAL(10.0));
		if (rPSD > rMaxPSD)
		{
			rMaxPSD = rPSD;
			iMaxPSDBin = i;
		}
	}

	// For total signal power, exclude the biggest one and e.g. 2 either side
	int iExcludeStartBin = iMaxPSDBin - RNIP_EXCLUDE_BINS;
	int iExcludeEndBin = iMaxPSDBin + RNIP_EXCLUDE_BINS;

	// Calculate power. TotalPower() function will deal with start>end correctly
	_REAL rSigPowerExcludingInterferer = CalcTotalPower(vecrPSD, iSearchStartBin, iExcludeStartBin-1) +
		CalcTotalPower(vecrPSD, iExcludeEndBin+1, iSearchEndBin);

	/* interferer level wrt signal power */
	ReceiverParam.Measurements.rMaxPSDwrtSig = _REAL(10.0) * log10(rMaxPSD / rSigPowerExcludingInterferer);

	/* interferer frequency */
	ReceiverParam.Measurements.rMaxPSDFreq = _REAL(iMaxPSDBin) * _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(LEN_PSD_AV_EACH_BLOCK_RSI) - rIFCentreFrequency;

}


int CReceiveData::FreqToBin(_REAL rFreq)
{
	return int(rFreq/SOUNDCRD_SAMPLE_RATE * LEN_PSD_AV_EACH_BLOCK_RSI);
}

_REAL CReceiveData::CalcTotalPower(const vector<_REAL> &vecrData, int iStartBin, int iEndBin)
{
	if (iStartBin < 0) iStartBin = 0;
	if (iEndBin > LEN_PSD_AV_EACH_BLOCK_RSI/2)
		iEndBin = LEN_PSD_AV_EACH_BLOCK_RSI/2;

	_REAL rSigPower = _REAL(0.0);
	for (int i=iStartBin; i<=iEndBin; i++)
	{
		_REAL rPSD = pow(_REAL(10.0), vecrData[i]/_REAL(10.0));
		// The factor of 2 below is needed because half of the power is in the negative frequencies
		rSigPower += rPSD * _REAL(2.0);
	}

	return rSigPower;
}

/******************************************************************************\
 *	Receive audio data decoded in hardware
 *	currently only supports _SAMPLE stereo in to _REAL stereo out
 *	and _SAMPLE mono in to _REAL mono out
 *	TODO mono in to stereo out
\******************************************************************************/
void COnboardDecoder::ProcessDataInternal(CParameter& Parameter)
{
	if(pSound == NULL)
		return;
	vector<_SAMPLE> s(iOutputBlockSize);
	bool bBad = pSound->Read(s);
	for(int i=0; i<iOutputBlockSize; i++)
		(*pvecOutputData)[i] = s[i];

	Parameter.Lock();
	if (bBad == false)
		Parameter.ReceiveStatus.Interface.SetStatus(RX_OK);
	else
		Parameter.ReceiveStatus.Interface.SetStatus(CRC_ERROR);
	Parameter.Unlock();
}

void COnboardDecoder::InitInternal(CParameter&)
{
	if(pSound == NULL)
		return;

	const int iNumInSamplesMono = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *
										 (_REAL) 0.05 /* 50 ms (arbitrary) */ );
	iOutputBlockSize = 2*iNumInSamplesMono;

	iMaxOutputBlockSize = 2 * (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *
										 (_REAL) 0.4 /* 400 ms (not-so-arbitrary) */ );

	pSound->Init(iOutputBlockSize);
}
