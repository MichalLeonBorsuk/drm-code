/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy, Julian Cable
 *
 * Description:
 *	DRM-receiver
 * The hand over of data is done via an intermediate-buffer. The calling
 * convention is always "input-buffer, output-buffer". Additionally, the
 * DRM-parameters are fed to the function.
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- Additions to include AMSS demodulation
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

#include "DrmReceiver.h"
#include "util/Settings.h"
#include "util/Hamlib.h"
#include "datadecoding/Journaline.h"
#include "datadecoding/DABMOT.h"
#include "datadecoding/MOTSlideShow.h"
#include "datadecoding/EPGDecoder.h"

#include "sound.h"
#include "sound/soundnull.h"
#ifdef __linux__
# include "sound/shmsoundin.h"
#endif
#include "sound/soundfile.h"
#include <fstream>

// TODO - does this need to be a qthread anymore ?
// TODO - lock/unlock the Parameter object more consistantly

CAMSSReceiver::CAMSSReceiver()
:PhaseDemod(),ExtractBits(),Decode(), InputResample(),
PhaseBuf(),	ResPhaseBuf(), BitsBuf()
{
}

void CAMSSReceiver::Init()
{
	InputResample.SetInitFlag();
	PhaseDemod.SetInitFlag();
	ExtractBits.SetInitFlag();
	Decode.SetInitFlag();

	PhaseBuf.Clear();
	ResPhaseBuf.Clear();
	BitsBuf.Clear();
}

bool
CAMSSReceiver::Demodulate(
	CParameter& Parameters,
	CSingleBuffer<_REAL>& DataBuf,
	CSingleBuffer<_BINARY>& SDCBuf
)
{
    bool bEnoughData = false;
	/* AMSS phase demodulation */
	if (PhaseDemod.ProcessData(Parameters, DataBuf, PhaseBuf))
	{
		bEnoughData = true;
	}

	/* AMSS resampling */
	if (InputResample.ProcessData(Parameters, PhaseBuf, ResPhaseBuf))
	{
		bEnoughData = true;
	}

	/* AMSS bit extraction */
	if (ExtractBits.ProcessData(Parameters, ResPhaseBuf, BitsBuf))
	{
		bEnoughData = true;
	}

	/* AMSS data decoding */
	if (Decode.ProcessData(Parameters, BitsBuf, SDCBuf))
	{
		bEnoughData = true;
	}
	return bEnoughData;
}

const int
	CDRMReceiver::MAX_UNLOCKED_COUNT = 2;

/* Implementation *************************************************************/
CDRMReceiver::CDRMReceiver(CSettings& s):
pSoundOutInterface(new CSoundOut),
ReceiveData(), WriteData(pSoundOutInterface),
FreqSyncAcq(),
ChannelEstimation(),
FACMLCDecoder(CT_FAC), UtilizeFACData(),
SDCMLCDecoder(CT_SDC), UtilizeSDCData(),
MSCMLCDecoder(CT_MSC), MSCDemultiplexer(),
AudioSourceDecoder(),DataDecoder(),
upstreamRSCI(), DecodeRSIMDI(), downstreamRSCI(),
Parameters(),RSIPacketBuf(),
MSCDecBuf(MAX_NUM_STREAMS), MSCUseBuf(MAX_NUM_STREAMS),
MSCSendBuf(MAX_NUM_STREAMS), iAcquRestartCnt(0),
iAcquDetecCnt(0), iGoodSignCnt(0),
iAudioStreamID(STREAM_ID_NOT_USED),iDataStreamID(STREAM_ID_NOT_USED),
bDoInitRun(false), bRunning(false),
rInitResampleOffset((_REAL) 0.0), iFreqkHz(0),time_keeper(0),
pRig(NULL), pSoundInInterface(new CSoundIn()), settings(s),
onBoardDemod(false), inChanSel(CS_MIX_CHAN)
{
    downstreamRSCI.SetReceiver(this);
    DataDecoder.setApplication(CDataParam::AD_DAB_SPEC_APP, AT_MOTSLISHOW, new CMOTDABDecFactory());
    DataDecoder.setApplication(CDataParam::AD_DAB_SPEC_APP, AT_MOTBROADCASTWEBSITE, new CMOTDABDecFactory());
    DataDecoder.setApplication(CDataParam::AD_DAB_SPEC_APP, AT_MOTEPG, new EPGDecoderFactory());
    DataDecoder.setApplication(CDataParam::AD_DAB_SPEC_APP, AT_JOURNALINE, new JournalineFactory());
    LoadSettings();
}

CDRMReceiver::~CDRMReceiver()
{
	delete pSoundInInterface;
	delete pSoundOutInterface;
}

void
CDRMReceiver::SetEnableSMeter(bool bNew)
{
    if(pRig)
	pRig->SetEnableSMeter(bNew);
}

bool
CDRMReceiver::GetEnableSMeter()
{
    if(pRig)
	return pRig->GetEnableSMeter();
    return false;
}

int
CDRMReceiver::GetAnalogFilterBWHz()
{
	return AMDemodulation.GetFilterBWHz();
}

void
CDRMReceiver::SetAnalogFilterBWHz(int iNew)
{
	AMDemodulation.SetFilterBWHz(iNew);
}

void
CDRMReceiver::SetAnalogDemodAcq(_REAL rNewNorCen)
{
	/* Set the frequency where the AM demodulation should
     * look for the aquisition.
     */
    AMDemodulation.SetAcqFreq(rNewNorCen);
    AMSSReceiver.SetAcqFreq(rNewNorCen);
}

void
CDRMReceiver::EnableAnalogAutoFreqAcq(const bool bNewEn)
{
	AMDemodulation.EnableAutoFreqAcq(bNewEn);
}

bool
CDRMReceiver::AnalogAutoFreqAcqEnabled()
{
	return AMDemodulation.AutoFreqAcqEnabled();
}


void
CDRMReceiver::EnableAnalogPLL(const bool bNewEn)
{
	AMDemodulation.EnablePLL(bNewEn);
}

bool
CDRMReceiver::AnalogPLLEnabled()
{
	return AMDemodulation.PLLEnabled();
}

bool
CDRMReceiver::GetAnalogPLLPhase(CReal& rPhaseOut)
{
	return AMDemodulation.GetPLLPhase(rPhaseOut);
}

void
CDRMReceiver::SetAnalogAGCType(const EType eNewType)
{
	AMDemodulation.SetAGCType(eNewType);
}

EType
CDRMReceiver::GetAnalogAGCType()
{
	return AMDemodulation.GetAGCType();
}

void
CDRMReceiver::SetAnalogNoiseReductionType(const ENoiRedType eNewType)
{
	AMDemodulation.SetNoiRedType(eNewType);
}

ENoiRedType
CDRMReceiver::GetAnalogNoiseReductionType()
{
	return AMDemodulation.GetNoiRedType();
}

void
CDRMReceiver::Run()
{
    bool bEnoughData = true;
    bool bFrameToSend = false;
    size_t i;
    /* Check for parameter changes from:
     * RSCI
     * GUI
     * Channel Reconfiguration in FAC/SDC
     */
    /* The parameter changes are done through flags, the actual initialization
     * is done in this (the working) thread to avoid problems with shared data */
    Parameters.Lock();
    int iNumSDCBitsPerSuperFrame = Parameters.iNumSDCBitsPerSuperFrame;
    EAcqStat eAcquiState = Parameters.eAcquiState;
    EModulationType eModulation = Parameters.eModulation;
    ERxEvent RxEvent = Parameters.RxEvent;
    /*
    if(eModulation == NONE)
    {
	Parameters.eModulation = eModulation = DRM; // right place for default ??? TODO !!!
    }
    */
    Parameters.RxEvent = None;
    Parameters.Unlock();

    bool initNeeded = false;

    switch(RxEvent)
    {
    case ChannelReconfiguration:
	// initialise channel from NextConfig
	// - might have been signalled or detected
	Parameters.Lock();
	Parameters.Channel = Parameters.NextConfig.Channel;
	Parameters.Unlock();
	LoadSettings();
	initNeeded = true;
	break;
    case ServiceReconfiguration:
	/* Reinitialise MSC Demultiplexer */
	Parameters.Lock();
	Parameters.MSCParameters = Parameters.NextConfig.MSCParameters;
	Parameters.AudioParam = Parameters.NextConfig.AudioParam;
	Parameters.DataParam = Parameters.NextConfig.DataParam;
	Parameters.Unlock();
	MSCMLCDecoder.SetInitFlag(); // protection levels
	MSCDemultiplexer.SetInitFlag();
	/* Reinitialise component decoders */
	for (size_t i = 0; i < MSCDecBuf.size(); i++)
	{
	    SplitMSC[i].SetStream(i);
	    SplitMSC[i].SetInitFlag();
	    MSCDecBuf[i].Clear();
	    MSCUseBuf[i].Clear();
	    MSCSendBuf[i].Clear();
	}
	InitsForAudParam();
	InitsForDataParam();
	break;
    case Tune: // TODO - is this right ? Is this actually used ?
	initNeeded = true;
	break;
    case Reinitialise: // TODO - Test this
	/* Define with which parameters the receiver should try to decode the
	   signal. If we are correct with our assumptions, the receiver does not
	   need to reinitialize */
	Parameters.Lock();
	Parameters.Channel.eRobustness = RM_ROBUSTNESS_MODE_A;
	Parameters.Channel.eSpectrumOccupancy = SO_3;

	/* Set initial MLC parameters */
	Parameters.Channel.eInterleaverDepth = SI_LONG;
	Parameters.Channel.eMSCmode = CS_3_SM;
	Parameters.Channel.eSDCmode = CS_2_SM;

	/* Number of audio and data services */
	Parameters.FACParameters.iNumAudioServices = 1;
	Parameters.FACParameters.iNumDataServices = 0;

	/* Protection levels */
	Parameters.MSCParameters.ProtectionLevel.iPartA = 0;
	Parameters.MSCParameters.ProtectionLevel.iPartB = 1;
	Parameters.MSCParameters.ProtectionLevel.iHierarch = 0;
	Parameters.Unlock();
	initNeeded = true;
	break;
    case SelectAudioComponent:
	InitsForAudParam();
	break;
    case SelectDataComponent:
	InitsForDataParam();
	break;
    case None:
	break;
    }

    if(initNeeded)
    {
	    switch(eModulation)
	    {
	    case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:
		    /* set stream */
		    iAudioStreamID = 0;
		    break;
	    case AM:
		    /* Tell the SDC decoder that it's AMSS to decode (no AFS index) */
		    UtilizeSDCData.SetSDCType(SDC_AMSS);
		    /* set stream */
		    iAudioStreamID = 0;
		    break;
	    case DRM:
		    UtilizeSDCData.SetSDCType(SDC_DRM);
		    break;
	    case NONE:
		    return;
	    }

	    /* Init all modules */
	    SetInStartMode();

	    if (upstreamRSCI.GetOutEnabled() == true)
	    {
		    upstreamRSCI.SetReceiverMode(eModulation);
	    }
    }
    initNeeded = false;

    /* Input - from upstream RSCI or input and demodulation from sound card / file */

    if (upstreamRSCI.GetInEnabled() == true)
    {
	    if (bDoInitRun == false)	/* don't wait for a packet in Init mode */
	    {
		    RSIPacketBuf.Clear();
		    upstreamRSCI.ReadData(Parameters, RSIPacketBuf);
		    if (RSIPacketBuf.GetFillLevel() > 0)
		    {
			    time_keeper = time(NULL);
			    DecodeRSIMDI.ProcessData(Parameters, RSIPacketBuf, FACDecBuf, SDCDecBuf, MSCDecBuf);
			    bFrameToSend = true;
		    }
		    else
		    {
			    time_t now = time(NULL);
			    if ((now - time_keeper) > 2)
			    {
				Parameters.Lock();
				    Parameters.ReceiveStatus.Interface.SetStatus(NOT_PRESENT);
				    Parameters.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
				    Parameters.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
				    Parameters.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
				    Parameters.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
				    Parameters.ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
				    Parameters.ReceiveStatus.MOT.SetStatus(NOT_PRESENT);
				    Parameters.Unlock();
			    }
		    }
	    }
    }
    else
    {
	    if(onBoardDemod)
	    {
		    OnboardDecoder.ReadData(Parameters, AMAudioBuf);
	    }
	    else
	    {
		    ReceiveData.ReadData(Parameters, RecDataBuf);

		    // Split samples, one output to the demodulation, another for IQ recording
		    if (SplitForIQRecord.ProcessData(Parameters, RecDataBuf, DemodDataBuf, IQRecordDataBuf))
		    {
			    bEnoughData = true;
		    }

		    switch(eModulation)
		    {
		    case DRM:
				    bEnoughData |= DemodulateDRM(bFrameToSend);
				    break;
		    case AM:
				    /* The incoming samples are split 2 ways.
				       One set is passed to the AM demodulator.
				       The other set is passed to the AMSS demodulator.
				       The AMSS and AM demodulators work completely independently
				     */
				    if (Split.ProcessData(Parameters, DemodDataBuf, AMDataBuf, AMSSDataBuf))
				    {
					    bEnoughData = true;
				    }

				    /* AMSS Demodulation */
				    if(AMSSReceiver.Demodulate(Parameters, AMSSDataBuf, SDCDecBuf))
				    {
					    bEnoughData = true;
				    }

				    /* AM demodulation ------------------------------------------ */
				    if (AMDemodulation.ProcessData(Parameters, AMDataBuf, AMAudioBuf))
				    {
					    bEnoughData = true;
				    }
				    break;
		    case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:
				    if (AMDemodulation.ProcessData(Parameters, AMDataBuf, AMAudioBuf))
				    {
					    bEnoughData = true;
				    }
				    break;
		    case NONE:
				    break;
		    }
	    }
    }

    /* Split the data for downstream RSCI and local processing. TODO make this conditional */
    switch(eModulation)
    {
    case DRM:
	    SplitFAC.ProcessData(Parameters, FACDecBuf, FACUseBuf, FACSendBuf);

	    /* if we have an SDC block, make a copy and keep it until the next frame is to be sent */
	    if (SDCDecBuf.GetFillLevel() == iNumSDCBitsPerSuperFrame)
	    {
		    SplitSDC.ProcessData(Parameters, SDCDecBuf, SDCUseBuf, SDCSendBuf);
		    bEnoughData = true;
	    }

	    for (i = 0; i < MSCDecBuf.size(); i++)
	    {
		    MSCUseBuf[i].Clear();
		    MSCSendBuf[i].Clear();
		    SplitMSC[i].ProcessData(Parameters, MSCDecBuf[i], MSCUseBuf[i], MSCSendBuf[i]);
	    }
	    break;
    case AM: case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:
	    /* if input is from RSCI we need to decode the audio */
	    if (upstreamRSCI.GetInEnabled() == true)
	    {
		    if (AudioSourceDecoder.ProcessData(Parameters, MSCUseBuf[iAudioStreamID], AMAudioBuf))
		    {
			    bEnoughData = true;
		    }
	    }
	    /* split the received linear audio for playout and RSCI output */
	    SplitAudio.ProcessData(Parameters, AMAudioBuf, AudSoDecBuf, AMSoEncBuf);
	    break;
    case NONE:
	    break;
    }

    /* decoding - loop while something happened */
    while (bEnoughData && bRunning)
    {
	    /* Init flag */
	    bEnoughData = false;

	    // Write output I/Q file
	    if (WriteIQFile.WriteData(Parameters, IQRecordDataBuf))
	    {
		    bEnoughData = true;
	    }

	    switch(eModulation)
	    {
	    case DRM:
		    bEnoughData |= UtilizeDRM();
		    break;
	    case AM:
		    if (UtilizeSDCData.WriteData(Parameters, SDCDecBuf))
		    {
			    bEnoughData = true;
		    }
		    break;
    case USB: case LSB: case  CW: case  NBFM: case  WBFM:
	break; // RDS ?
	    case NONE:
		    break;
	    }
    }

    /* output to downstream RSCI */
    if (downstreamRSCI.GetOutEnabled())
    {
	    switch(eModulation)
	    {
	    case DRM:
		    if (eAcquiState == AS_NO_SIGNAL)
		    {
			    /* we will get one of these between each FAC block, and occasionally we */
			    /* might get two, so don't start generating free-wheeling RSCI until we've. */
			    /* had three in a row */
			    if (FreqSyncAcq.GetUnlockedFrameBoundary())
			    {
				    if (iUnlockedCount < MAX_UNLOCKED_COUNT)
					    iUnlockedCount++;
				    else
					    downstreamRSCI.SendUnlockedFrame(Parameters);
			    }
		    }
		    else if (bFrameToSend)
		    {
			    downstreamRSCI.SendLockedFrame(Parameters, FACSendBuf, SDCSendBuf, MSCSendBuf);
			    iUnlockedCount = 0;
			    bFrameToSend = false;
		    }
		    break;
	    case AM: case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:
		    /* Encode audio for RSI output */
		    if (AudioSourceEncoder.ProcessData(Parameters, AMSoEncBuf, MSCSendBuf[0]))
			    bFrameToSend = true;

		    if (bFrameToSend)
			    downstreamRSCI.SendAMFrame(Parameters, MSCSendBuf[iAudioStreamID]);
		    break;
	    case NONE:
		    break;
	    }
    }
    /* Play and/or save the audio */
    if (iAudioStreamID != STREAM_ID_NOT_USED)
    {
	    if (WriteData.WriteData(Parameters, AudSoDecBuf))
	    {
		    bEnoughData = true;
	    }
    }
}

bool
CDRMReceiver::DemodulateDRM(bool& bFrameToSend)
{
    bool bEnoughData = false;

    Parameters.Lock();
    EAcqStat eAcquiState = Parameters.eAcquiState;
    Parameters.Unlock();

	/* Resample input DRM-stream -------------------------------- */
	if (InputResample.ProcessData(Parameters, DemodDataBuf, InpResBuf))
	{
		bEnoughData = true;
	}

	/* Frequency synchronization acquisition -------------------- */
	if (FreqSyncAcq.ProcessData(Parameters, InpResBuf, FreqSyncAcqBuf))
	{
		bEnoughData = true;
	}

	/* Time synchronization ------------------------------------- */
	if (TimeSync.ProcessData(Parameters, FreqSyncAcqBuf, TimeSyncBuf))
	{
		bEnoughData = true;
		/* Use count of OFDM-symbols for detecting
		 * aquisition state for acquisition detection
		 * only if no signal was decoded before */
		if (eAcquiState == AS_NO_SIGNAL)
		{
			/* Increment symbol counter and check if bound is reached */
			iAcquDetecCnt++;

			if (iAcquDetecCnt > NUM_OFDMSYM_U_ACQ_WITHOUT)
				SetInStartMode();
		}
	}

	/* OFDM Demodulation ---------------------------------------- */
	if (OFDMDemodulation.
		ProcessData(Parameters, TimeSyncBuf, OFDMDemodBuf))
	{
		bEnoughData = true;
	}

	/* Synchronization in the frequency domain (using pilots) --- */
	if (SyncUsingPil.
		ProcessData(Parameters, OFDMDemodBuf, SyncUsingPilBuf))
	{
		bEnoughData = true;
	}

	/* Channel estimation and equalisation ---------------------- */
	if (ChannelEstimation.
		ProcessData(Parameters, SyncUsingPilBuf, ChanEstBuf))
	{
		bEnoughData = true;
	}

	/* Demapping of the MSC, FAC, SDC and pilots off the carriers */
	if (OFDMCellDemapping.ProcessData(Parameters, ChanEstBuf,
									  MSCCarDemapBuf,
									  FACCarDemapBuf, SDCCarDemapBuf))
	{
		bEnoughData = true;
	}

	/* FAC ------------------------------------------------------ */
	if (FACMLCDecoder.ProcessData(Parameters, FACCarDemapBuf, FACDecBuf))
	{
		bEnoughData = true;
		bFrameToSend = true;
	}

	/* SDC ------------------------------------------------------ */
	if (SDCMLCDecoder.ProcessData(Parameters, SDCCarDemapBuf, SDCDecBuf))
	{
		bEnoughData = true;
	}

	/* MSC ------------------------------------------------------ */

	/* Symbol de-interleaver */
	if (SymbDeinterleaver.ProcessData(Parameters, MSCCarDemapBuf, DeintlBuf))
	{
		bEnoughData = true;
	}

	/* MLC decoder */
	if (MSCMLCDecoder.ProcessData(Parameters, DeintlBuf, MSCMLCDecBuf))
	{
		bEnoughData = true;
	}

	/* MSC demultiplexer (will leave FAC & SDC alone! */
	if (MSCDemultiplexer.ProcessData(Parameters, MSCMLCDecBuf, MSCDecBuf))
	{
		bEnoughData = true;
	}

	return bEnoughData;
}

bool
CDRMReceiver::UtilizeDRM()
{
    bool bEnoughData = false;
	if (UtilizeFACData.WriteData(Parameters, FACUseBuf))
	{
		bEnoughData = true;

		/* Use information of FAC CRC for detecting the acquisition
		   requirement */
		DetectAcquiFAC();
#if 0
		saveSDCtoFile();
#endif
	}

	if (UtilizeSDCData.WriteData(Parameters, SDCUseBuf))
	{
		bEnoughData = true;
	}

	/* Data decoding */
	if (iDataStreamID != STREAM_ID_NOT_USED)
	{
		if (DataDecoder.WriteData(Parameters, MSCUseBuf[iDataStreamID]))
			bEnoughData = true;
	}

	/* Audio decoding */
	if (iAudioStreamID != STREAM_ID_NOT_USED)
	{
		if (AudioSourceDecoder.ProcessData(Parameters,
										   MSCUseBuf[iAudioStreamID],
										   AudSoDecBuf))
		{
			bEnoughData = true;
		}
	}
	return bEnoughData;
}


void
CDRMReceiver::DetectAcquiFAC()
{
    Parameters.Lock();
    EAcqStat eAcquiState = Parameters.eAcquiState;
    Parameters.Unlock();

	/* If upstreamRSCI in is enabled, do not check for acquisition state because we want
	   to stay in tracking mode all the time */
	if (upstreamRSCI.GetInEnabled() == true)
		return;

	/* Acquisition switch */
	if (UtilizeFACData.GetCRCOk())
	{
		iGoodSignCnt++;

		/* Set the receiver state to "with signal" not until two successive FAC
		   frames are "ok", because there is only a 8-bit CRC which is not good
		   for many bit errors. But it is very unlikely that we have two
		   successive FAC blocks "ok" if no good signal is received */
		if (iGoodSignCnt >= 2)
		{
	    Parameters.Lock();
	    Parameters.eAcquiState = AS_WITH_SIGNAL;
	    Parameters.Unlock();

			/* Take care of delayed tracking mode switch */
			if (iDelayedTrackModeCnt > 0)
				iDelayedTrackModeCnt--;
			else
				SetInTrackingModeDelayed();
		}
		else
		{
			/* If one CRC was correct, reset acquisition since
			   we assume, that this was a correct detected signal */
			iAcquRestartCnt = 0;
			iAcquDetecCnt = 0;

			/* Set in tracking mode */
			SetInTrackingMode();

		}
	}
	else
	{
		/* Reset "good signal" count */
		iGoodSignCnt = 0;

		iAcquRestartCnt++;

		/* Check situation when receiver must be set back in start mode */
		if ((eAcquiState == AS_WITH_SIGNAL)
		&& (iAcquRestartCnt > NUM_FAC_FRA_U_ACQ_WITH))
		{
			SetInStartMode();
		}
	}
}

#ifdef QT_GUI_LIB
void
CDRMReceiver::run()
{
#ifdef _WIN32
	/* it doesn't matter what the GUI does, we want to be normal! */
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
#endif
	try
	{
		/* Call receiver main routine */
		Start();
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}
	qDebug("Working thread complete");
}
#endif

void
CDRMReceiver::Start()
{
	/* Set run flag so that the thread can work */
	bRunning = true;
	/* initialise */
	Parameters.Lock();
	Parameters.RxEvent = Reinitialise;
	Parameters.Unlock();

	do
	{
		Run();

	}
	while (bRunning);

	pSoundInInterface->Close();
	pSoundOutInterface->Close();
}

void
CDRMReceiver::Stop()
{
	bRunning = false;
}

bool
CDRMReceiver::End()
{
    Stop();
    (void)wait(5000);
    return isFinished();
}

void
CDRMReceiver::SetInStartMode()
{
    iUnlockedCount = MAX_UNLOCKED_COUNT;

    Parameters.Lock();

    Parameters.CellMappingTable.MakeTable(
	Parameters.Channel.eRobustness,
	Parameters.Channel.eSpectrumOccupancy
    );
    Parameters.FACParameters.iFrameId = 0; // arbitrary ?

    /* Select the service we want to decode. Always zero, because we do not
       know how many services are transmitted in the signal we want to
       decode */

    /* TODO: if service 0 is not used but another service is the audio service
     * we have a problem. We should check as soon as we have information about
     * services if service 0 is really the audio service
     */

    /* Set the following parameters to zero states (initial states) --------- */
    Parameters.ResetServicesStreams();
    Parameters.ResetCurSelAudDatServ();

    /* Set synchronization parameters */
    Parameters.rResampleOffset = rInitResampleOffset;	/* Initial resample offset */
    Parameters.rFreqOffsetAcqui = (_REAL) 0.0;
    Parameters.rFreqOffsetTrack = (_REAL) 0.0;
    Parameters.iTimingOffsTrack = 0;

    EModulationType eModulation = Parameters.eModulation;

    Parameters.Unlock();

    /* Initialization of the modules */
    InitsForAllModules(eModulation);

    /* Activate acquisition */
    FreqSyncAcq.StartAcquisition();
    TimeSync.StartAcquisition();
    ChannelEstimation.GetTimeSyncTrack()->StopTracking();
    ChannelEstimation.StartSaRaOffAcq();
    ChannelEstimation.GetTimeWiener()->StopTracking();

    SyncUsingPil.StartAcquisition();
    SyncUsingPil.StopTrackPil();

    Parameters.Lock();
    /* Set flag that no signal is currently received */
    Parameters.eAcquiState = AS_NO_SIGNAL;

    /* Set flag for receiver state */
    eReceiverState = RS_ACQUISITION;

    Parameters.Unlock();

    /* Reset counters for acquisition decision, "good signal" and delayed
       tracking mode counter */
    iAcquRestartCnt = 0;
    iAcquDetecCnt = 0;
    iGoodSignCnt = 0;
    iDelayedTrackModeCnt = NUM_FAC_DEL_TRACK_SWITCH;

    /* In case upstreamRSCI is enabled, go directly to tracking mode, do not activate the
       synchronization units */
    if (upstreamRSCI.GetInEnabled() == true)
    {
	/* We want to have as low CPU usage as possible, therefore set the
	   synchronization units in a state where they do only a minimum
	   work */
	FreqSyncAcq.StopAcquisition();
	TimeSync.StopTimingAcqu();
	InputResample.SetSyncInput(true);
	SyncUsingPil.SetSyncInput(true);

	/* This is important so that always the same amount of module input
	   data is queried, otherwise it could be that amount of input data is
	   set to zero and the receiver gets into an infinite loop */
	TimeSync.SetSyncInput(true);

	/* Always tracking mode for upstreamRSCI */
	Parameters.Lock();
	Parameters.eAcquiState = AS_WITH_SIGNAL;
	Parameters.Unlock();

	SetInTrackingMode();
    }
}

void
CDRMReceiver::SetInTrackingMode()
{
	/* We do this with the flag "eReceiverState" to ensure that the following
	   routines are only called once when the tracking is actually started */
	if (eReceiverState == RS_ACQUISITION)
	{
		/* In case the acquisition estimation is still in progress, stop it now
		   to avoid a false estimation which could destroy synchronization */
		TimeSync.StopRMDetAcqu();

		/* Acquisition is done, deactivate it now and start tracking */
		ChannelEstimation.GetTimeWiener()->StartTracking();

		/* Reset acquisition for frame synchronization */
		SyncUsingPil.StopAcquisition();
		SyncUsingPil.StartTrackPil();

		/* Set receiver flag to tracking */
		eReceiverState = RS_TRACKING;
	}
}

void
CDRMReceiver::SetInTrackingModeDelayed()
{
	/* The timing tracking must be enabled delayed because it must wait until
	   the channel estimation has initialized its estimation */
	TimeSync.StopTimingAcqu();
	ChannelEstimation.GetTimeSyncTrack()->StartTracking();
}

void
CDRMReceiver::InitsForAllModules(EModulationType eModulation)
{
	//Parameters.Measurements.SetRSCIDefaults(downstreamRSCI.GetOutEnabled());

/*  TODO - why is this here and when would one unsubscribe ?
	if (Parameters.FrontEndParameters.eSMeterCorrectionType !=
		CFrontEndParameters::S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY)
	Parameters.Measurements.subscribe(CMeasurements::PSD);
*/
	/* Set init flags */
	SplitFAC.SetInitFlag();
	SplitSDC.SetInitFlag();
	for (size_t i = 0; i < MSCDecBuf.size(); i++)
	{
		SplitMSC[i].SetStream(i);
		SplitMSC[i].SetInitFlag();
		MSCDecBuf[i].Clear();
		MSCUseBuf[i].Clear();
		MSCSendBuf[i].Clear();
	}

	if(onBoardDemod)
	{
		OnboardDecoder.SetSoundInterface(pSoundInInterface);
		OnboardDecoder.SetInitFlag();
	}
	else
	{
		AMDemodulation.SetDemodType(eModulation);
		AMDemodulation.SetInitFlag();
		AMSSReceiver.Init();
		ReceiveData.SetInChanSel(inChanSel);
		ReceiveData.SetSoundInterface(pSoundInInterface);
		ReceiveData.SetInitFlag();
	}
	InputResample.SetInitFlag();
	FreqSyncAcq.SetInitFlag();
	TimeSync.SetInitFlag();
	OFDMDemodulation.SetInitFlag();
	SyncUsingPil.SetInitFlag();
	ChannelEstimation.SetInitFlag();
	OFDMCellDemapping.SetInitFlag();
	FACMLCDecoder.SetInitFlag();
	UtilizeFACData.SetInitFlag();
	SDCMLCDecoder.SetInitFlag();
	UtilizeSDCData.SetInitFlag();
	SymbDeinterleaver.SetInitFlag();
	MSCMLCDecoder.SetInitFlag();
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	AudioSourceDecoder.SetInitFlag();
	DataDecoder.SetInitFlag();
	WriteData.SetInitFlag();

	Split.SetInitFlag();
	SplitAudio.SetInitFlag();
	AudioSourceEncoder.SetInitFlag();

	SplitForIQRecord.SetInitFlag();
	WriteIQFile.SetInitFlag();

	upstreamRSCI.SetInitFlag();
	//downstreamRSCI.SetInitFlag();

	/* Clear all buffers (this is especially important for the "AudSoDecBuf"
	   buffer since AM mode and DRM mode use the same buffer. When init is
	   called or modes are switched, the buffer could have some data left which
	   lead to an overrun) */
	RecDataBuf.Clear();
	AMDataBuf.Clear();

	DemodDataBuf.Clear();
	IQRecordDataBuf.Clear();

	AMSSDataBuf.Clear();

	InpResBuf.Clear();
	FreqSyncAcqBuf.Clear();
	TimeSyncBuf.Clear();
	OFDMDemodBuf.Clear();
	SyncUsingPilBuf.Clear();
	ChanEstBuf.Clear();
	MSCCarDemapBuf.Clear();
	FACCarDemapBuf.Clear();
	SDCCarDemapBuf.Clear();
	DeintlBuf.Clear();
	FACDecBuf.Clear();
	SDCDecBuf.Clear();
	MSCMLCDecBuf.Clear();
	RSIPacketBuf.Clear();
	AudSoDecBuf.Clear();
	AMAudioBuf.Clear();
	AMSoEncBuf.Clear();
}

void
CDRMReceiver::InitsForAudParam()
{
    Parameters.Lock();
	int iShortID = Parameters.GetCurSelAudioService();
	iAudioStreamID = Parameters.Service[iShortID].iAudioStream;
	Parameters.Unlock();

	AudioSourceDecoder.SetStream(iAudioStreamID);
	AudioSourceDecoder.SetInitFlag();
}

void
CDRMReceiver::InitsForDataParam()
{
    Parameters.Lock();
	int iShortID = Parameters.GetCurSelDataService();
	iDataStreamID = Parameters.Service[iShortID].iDataStream;
	Parameters.Unlock();

	DataDecoder.SetStream(iDataStreamID);
	DataDecoder.SetInitFlag();
}

// TODO change rig if requested frequency not supported but another rig can support.
//
// TODO set all relevant modes when changing rigs
bool CDRMReceiver::SetFrequency(int iNewFreqkHz)
{
	if (iFreqkHz == iNewFreqkHz)
		return true;
	iFreqkHz = iNewFreqkHz;
	return doSetFrequency();
}

bool CDRMReceiver::doSetFrequency()
{
    bool result = true;

	Parameters.Lock();
	Parameters.SetFrequency(iFreqkHz);
	/* clear out AMSS data and re-initialise AMSS acquisition */
	if(Parameters.eModulation != DRM && Parameters.eModulation != NONE)
		Parameters.ResetServicesStreams();
	Parameters.Unlock();

	if (upstreamRSCI.GetOutEnabled() == true)
	{
		upstreamRSCI.SetFrequency(iFreqkHz);
	}
	else
	{
		if(pRig)
			result = pRig->SetFrequency(iFreqkHz);
	}

	/* tell the RSCI and IQ file writer that freq has changed in case it needs to start a new file */
	if (downstreamRSCI.GetOutEnabled() == true)
		downstreamRSCI.NewFrequency(Parameters);

	WriteIQFile.NewFrequency(Parameters);
	return result; // TODO why ?
}


/* TEST store information about alternative frequency transmitted in SDC */
/* Current AFS data is stored in the ServiceInformation record for the service with
   shortid 0
*/
void
CDRMReceiver::saveSDCtoFile()
{
	ofstream out("test/altfreq.dat", ios::app);
	Parameters.Lock();
	Parameters.ServiceInformation[Parameters.Service[0].iServiceID].AltFreqSign.dump(out);
	Parameters.Unlock();
	out.close();
}

void
CDRMReceiver::LoadSettings()
{
    vector<string> vs;
    int dev;

    string section = "Input-";
    int deffilterbw = 10000;

    EModulationType modn = EModulationType(settings.Get("Receiver", "modulation", int(DRM)));
    switch(modn)
    {
	case DRM:
	    section += "DRM";
	    break;
	case AM:
	case CW:
	case USB:
	case LSB:
	case NBFM:
	    section += "AM";
	    break;
	case WBFM:
	    section += "FM";
	    break;
	case NONE:
	    section += "None"; // shouldn't happen
	    break;
    }

    // Input (Status)
    string str = settings.Get("command", "rsiin", string(""));
    if(str!="")
    {
	size_t p = str.rfind('.');
	if (p == string::npos)
	{
	    settings.Put(section, "rsiin", str);
	    settings.Put(section, "input", string("net"));
	}
	else
	{
	    settings.Put(section, "file", str);
	    settings.Put(section, "input", string("file"));
	}
    }
    str = settings.Get("command", "fileio", string(""));
    if(str!="")
    {
	settings.Put(section, "file", str);
	settings.Put(section, "input", string("file"));
    }

    switch(modn)
    {
	case AM:
	case CW:
	case USB:
	case LSB:
	case NBFM:
	    AMDemodulation.SetDemodType(modn);
	    AMDemodulation.SetAGCType(EType(settings.Get(section, "agc", 0)));
	    AMDemodulation.SetNoiRedType(ENoiRedType(settings.Get(section, "noisered", 0)));
	    AMDemodulation.EnablePLL(settings.Get(section, "enablepll", 0));
	    AMDemodulation.EnableAutoFreqAcq(settings.Get(section, "autofreqacq", 0));
	    AMDemodulation.SetFilterBWHz(settings.Get(section, "filterbw", deffilterbw));
	    break;
	default:
	    break;
    }

    /* Serial Number */
    string sSerialNumber = settings.Get("Receiver", "serialnumber", string(""));

    Parameters.Lock();

    if(sSerialNumber == "")
    {
	    Parameters.GenerateRandomSerialNumber();
	    settings.Put("Receiver", "serialnumber", Parameters.sSerialNumber);
    }
    else
	Parameters.sSerialNumber = sSerialNumber;

    /* Receiver ID */
    Parameters.GenerateReceiverID();

    /* Data files directory */
    string sDataFilesDirectory = settings.Get(
       "Receiver", "datafilesdirectory", Parameters.sDataFilesDirectory);
    // remove trailing slash if there
    size_t p = sDataFilesDirectory.find_last_not_of("/\\");
    if(p != string::npos)
	    sDataFilesDirectory.erase(p+1);
    settings.Put("Receiver", "datafilesdirectory", Parameters.sDataFilesDirectory);

    Parameters.sDataFilesDirectory = sDataFilesDirectory;

    Parameters.Unlock();

    /* Sync */
    SetFreqInt(ETypeIntFreq(settings.Get("Input-DRM", "frequencyinterpolation", int(FWIENER))));
    SetTimeInt(ETypeIntTime(settings.Get("Input-DRM", "timeinterpolation", int(TWIENER))));
    SetTiSyncTracType(ETypeTiSyncTrac(settings.Get("Input-DRM", "tracking", 0)));

    // Input / Status
    int n = settings.Get("command", "inchansel", -1);
    if(n != -1)
	settings.Put(section, "channels", n);
    n = settings.Get("command", "flipspectrum", -1);
    if(n != -1)
	settings.Put(section, "flipspectrum", n);

    // input can be from RSCI or sound card or file
    string inp = settings.Get(section, "input", string("card"));
    if(inp == "RSCI")
    {
	string str = settings.Get(section, "rsiin");
	if(str != "")
	{
	    bool bOK = upstreamRSCI.SetOrigin(str); // its a port
	    if(!bOK)
		throw CGenErr(string("can't open RSCI input ")+str);

	    Parameters.Measurements.bETSIPSD = true;
	}
    }
    else
    {
	ReceiveData.SetFlippedSpectrum(settings.Get(section, "flipspectrum", false));
	ReceiveData.SetInChanSel(EInChanSel(settings.Get(section, "channels", CS_MIX_CHAN)));
    }
    if(inp == "card")
    {
	delete pSoundInInterface;
	pSoundInInterface = new CSoundIn;
	dev = settings.Get(section, "soundcard", 0);
	pSoundInInterface->Enumerate(vs);
	if(vs.size()>0)
	{
	    if(dev>=int(vs.size()))
	    dev = vs.size()-1;
	    pSoundInInterface->SetDev(dev);
	}
	vs.clear();
    }
    if(inp == "file")
    {
	string file = settings.Get(section, "file", string(""));
	if(file != "")
	{
	    CSoundFileIn* sfi = new CSoundFileIn();
	    if(sfi)
	    {
		sfi->SetFileName(file);
		delete pSoundInInterface;
		pSoundInInterface = sfi;
	    }
	}
    }

    /* Receiver ------------------------------------------------------------- */

    /* Sound Out device */
    dev = settings.Get("Receiver", "snddevout", 0);
    pSoundOutInterface->Enumerate(vs);
    if(vs.size()>0)
    {
	if(dev>=int(vs.size()))
	dev = vs.size()-1;
	pSoundOutInterface->SetDev(dev);
    }

    n = settings.Get("Receiver", "outchansel", -1);
    switch (n)
    {
	case 0:
		WriteData.SetOutChanSel(CWriteData::CS_BOTH_BOTH);
		break;

	case 1:
		WriteData.SetOutChanSel(CWriteData::CS_LEFT_LEFT);
		break;

	case 2:
		WriteData.SetOutChanSel(CWriteData::CS_RIGHT_RIGHT);
		break;

	case 3:
		WriteData.SetOutChanSel(CWriteData::CS_LEFT_MIX);
		break;

	case 4:
		WriteData.SetOutChanSel(CWriteData::CS_RIGHT_MIX);
		break;
	default:
		break;
    }

    SetIQRecording(settings.Get("Receiver", "writeiq", false));

    // set RSI recording format. Default is ff according to TS 102 349
    downstreamRSCI.SetRSIRecordType(settings.Get("Receiver", "rsirecordprofile", string("ff")));

    str = settings.Get("Receiver", "rsirecordprofile", string(""));
    if(str == "")
    {
	downstreamRSCI.StopRSIRecording();
    }
    else
    {
	downstreamRSCI.StartRSIRecording(Parameters, str[0]);
    }

    // Control Interface
    str = settings.Get("command", "rciout", string(""));
    if(str == "")
    {
	settings.Put(section, "control", string("Hamlib"));
	// look for rig - override Rig-0 from the command line
	str = settings.Get("command", "hamlib-model", string(""));
	settings.Put("Rig-0", "model", str);
	str = settings.Get("command", "hamlib-config", string(""));
	settings.Put("Rig-0", "config", str);
	settings.Put(section, "rig", 0);
    }
    else
    {
	settings.Put(section, "control", string("RSCI"));
	settings.Put(section, "rciout", str);
    }

    str = settings.Get(section, "control", string("Hamlib"));
    if(str=="Hamlib")
    {
    	string r = settings.Get(section, "rig", string(""));
	if(r!="")
	{
	    r = "Rig-"+r;
	    rig_model_t model = settings.Get(r, "model", -1);
	    if(model != -1)
	    {
		if(pRig)
		    delete pRig;
		pRig = new CRig(model, &Parameters);
		cerr << "set rig model " << model << endl;
		// TODO configure rig
	    }
	}
    }
    else
    {
	upstreamRSCI.SetDestination(str);
	if(pRig)
	    delete pRig;
	pRig = NULL;
    }

    /* downstream RSCI (network or file) */
    for(int i = 0; i<MAX_NUM_RSI_SUBSCRIBERS; i++)
    {
	stringstream ss;
	ss << "rsiout" << i;
	str = settings.Get("Receiver", ss.str());
	if(str != "")
	{
	    ss.str("");
	    ss << "rsioutprofile" << i;
	    string profile = settings.Get("command", ss.str(), string("A"));
	    Parameters.Measurements.bETSIPSD = true;

	    // Check whether the profile has a subsampling ratio (e.g. --rsioutprofile A20)
	    int iSubsamplingFactor = 1;
	    if (profile.length() > 1)
	    {
		iSubsamplingFactor = atoi(profile.substr(1).c_str());
	    }

	    downstreamRSCI.AddSubscriber(str, profile[0], iSubsamplingFactor);
	}
    }

    for (int i=1; i<=MAX_NUM_RSI_PRESETS; i++)
    {
	// define presets in same format as --rsioutprofile
	stringstream ss;
	ss << "rsioutpreset" << i;
	str = settings.Get("RSCI", ss.str());
	if(str != "")
	{
	    // Check whether the preset has a subsampling ratio (e.g. A20)
	    int iSubsamplingFactor = 1;
	    if (str.length() > 1)
	    {
		    iSubsamplingFactor = atoi(str.substr(1).c_str());
	    }
	    downstreamRSCI.DefineRSIPreset(i, str[0], iSubsamplingFactor);
	}
    }

    /* TODO
	    ss.str("");
	    ss << "rciin" << i;
	    string origin = settings.Get("command", ss.str());

    */

    /* IQ File Recording */
    if(settings.Get("command", "recordiq", false))
	    WriteIQFile.StartRecording(Parameters);

    /* Mute audio flag */
    WriteData.MuteAudio(settings.Get("Receiver", "mute", false));

    /* Output to File */
    str = settings.Get("command", "writewav");
    if(str != "")
	    WriteData.StartWriteWaveFile(str);

    /* Reverberation flag */
    AudioSourceDecoder.SetReverbEffect(settings.Get("Receiver", "reverb", true));

    /* Bandpass filter flag */
    FreqSyncAcq.SetRecFilter(settings.Get(section, "filter", false));

    /* Set parameters for frequency acquisition search window if needed */
     _REAL rFreqAcSeWinSize = settings.Get("command", "fracwinsize", _REAL(SOUNDCRD_SAMPLE_RATE / 2));
     _REAL rFreqAcSeWinCenter = settings.Get("command", "fracwincent", _REAL(SOUNDCRD_SAMPLE_RATE / 4));
    /* Set new parameters */
    FreqSyncAcq.SetSearchWindow(rFreqAcSeWinCenter, rFreqAcSeWinSize);

    /* Modified metrics flag */
    ChannelEstimation.SetIntCons(settings.Get("Input-DRM", "modmetric", false));

    /* Number of iterations for MLC setting */
    MSCMLCDecoder.SetNumIterations(settings.Get("Input-DRM", "mlciter", 0));

    /* Activate/Deactivate EPG decoding */
    //DataDecoder.SetDecodeEPG(settings.Get("EPG", "decodeepg", true));
    // TODO epg option

    /* Front-end - combine into Hamlib? */
    Parameters.Lock();

    CFrontEndParameters& FrontEndParameters = Parameters.FrontEndParameters;

    FrontEndParameters.eSMeterCorrectionType =
	    CFrontEndParameters::ESMeterCorrectionType(settings.Get("FrontEnd", "smetercorrectiontype", 0));

    FrontEndParameters.rSMeterBandwidth = settings.Get("FrontEnd", "smeterbandwidth", 0.0);

    FrontEndParameters.rDefaultMeasurementBandwidth = settings.Get("FrontEnd", "defaultmeasurementbandwidth", 0);

    FrontEndParameters.bAutoMeasurementBandwidth = settings.Get("FrontEnd", "automeasurementbandwidth", true);

    FrontEndParameters.rCalFactor = settings.Get(section, "calfactor", 0.0);

    FrontEndParameters.rIFCentreFreq = settings.Get("FrontEnd", "ifcentrefrequency", SOUNDCRD_SAMPLE_RATE / 4);

    Parameters.Unlock();

    /* Wanted RF Frequency */
    iFreqkHz = settings.Get("Receiver", "frequency", 0);
    doSetFrequency();

    // Put this right at the end so that eModulation is correct and Rx starts
    Parameters.Lock();
    Parameters.eModulation = modn;
    Parameters.Unlock();
}

void
CDRMReceiver::SaveSettings()
{
    settings.Put("0", "mode", string("RX"));
    string modn;
    string section = "Input-";
    Parameters.Lock();
	switch(Parameters.eModulation)
	{
	case DRM:
	    modn = "DRM";
	    section+=modn;
	    break;
	case AM:
	    modn = "AM";
	    section+=modn;
	    settings.Put("Input-AM", "filterbw", AMDemodulation.GetFilterBWHz());
	    break;
	case  USB:
	    modn = "USB";
	    section+="AM";
	    settings.Put(section, "filterbwusb", AMDemodulation.GetFilterBWHz());
	    break;
	case  LSB:
	    modn = "LSB";
	    settings.Put(section, "filterbwlsb", AMDemodulation.GetFilterBWHz());
	    section+="AM";
	    break;
	case  CW:
	    modn = "CW";
	    section+="AM";
	    settings.Put(section, "filterbwcw", AMDemodulation.GetFilterBWHz());
	    break;
	case  NBFM:
	    modn = "NBFM";
	    section+="AM";
	    settings.Put(section, "filterbwfm", AMDemodulation.GetFilterBWHz());
	    break;
	case  WBFM:
	    modn = "FM";
	    section+=modn;
	    settings.Put(section, "filterbw", AMDemodulation.GetFilterBWHz());
	    break;
	case NONE:
		;
	}
	settings.Put("Receiver", "modulation", int(Parameters.eModulation));
	Parameters.Unlock();

	/* Receiver ------------------------------------------------------------- */

	/* Flip spectrum flag */
	settings.Put(section, "flipspectrum", ReceiveData.GetFlippedSpectrum());

	/* Bandpass filter flag */
	settings.Put(section, "filter", FreqSyncAcq.GetRecFilter());

	/* Mute audio flag */
	settings.Put("Receiver", "mute", WriteData.GetMuteAudio());

	/* Sound In device */
	settings.Put(section, "soundcard", pSoundOutInterface->GetDev());

	/* Modified metrics flag */
	settings.Put("Input-DRM", "modmetric", ChannelEstimation.GetIntCons());

	/* Sync */
	settings.Put("Input-DRM", "frequencyinterpolation", int(GetFreqInt()));
	settings.Put("Input-DRM", "timeinterpolation", int(GetTimeInt()));
	settings.Put("Input-DRM", "tracking", int(GetTiSyncTracType()));
	/* Number of iterations for MLC setting */
	settings.Put("Input-DRM", "mlciter", MSCMLCDecoder.GetInitNumIterations());

	/* Sound Out device */
	settings.Put("Receiver", "snddevout", pSoundOutInterface->GetDev());

	/* Reverberation */
	settings.Put("Receiver", "reverb", AudioSourceDecoder.GetReverbEffect());

	/* Tuned Frequency */
	settings.Put("Receiver", "frequency", iFreqkHz);

	/* Active/Deactivate EPG decoding */
	//s.Put("EPG", "decodeepg", DataDecoder.GetDecodeEPG());

	/* AM Parameters */

	/* AGC */
	settings.Put("Input-AM", "agc", AMDemodulation.GetAGCType());
	/* noise reduction */
	settings.Put("Input-AM", "noisered", AMDemodulation.GetNoiRedType());
	/* pll enabled/disabled */
	settings.Put("Input-AM", "enablepll", AMDemodulation.PLLEnabled());
	/* auto frequency acquisition */
	settings.Put("Input-AM", "autofreqacq", AMDemodulation.AutoFreqAcqEnabled());

	/* Front-end - combine into Hamlib? */
	Parameters.Lock();
	settings.Put("FrontEnd", "smetercorrectiontype", int(Parameters.FrontEndParameters.eSMeterCorrectionType));
	settings.Put("FrontEnd", "smeterbandwidth", int(Parameters.FrontEndParameters.rSMeterBandwidth));
	settings.Put("FrontEnd", "defaultmeasurementbandwidth", int(Parameters.FrontEndParameters.rDefaultMeasurementBandwidth));
	settings.Put("FrontEnd", "automeasurementbandwidth", Parameters.FrontEndParameters.bAutoMeasurementBandwidth);
	settings.Put("FrontEnd", "ifcentrefrequency", int(Parameters.FrontEndParameters.rIFCentreFreq));

	/* Serial Number */
	settings.Put("Receiver", "serialnumber", Parameters.sSerialNumber);
	settings.Put("Receiver", "datafilesdirectory", Parameters.sDataFilesDirectory);
	Parameters.Unlock();
}

void CDRMReceiver::SetIQRecording(bool bOn)
{
    if(bOn)
	    WriteIQFile.StartRecording(Parameters);
    else
	    WriteIQFile.StopRecording();
}

void CDRMReceiver::SetRSIRecording(bool bOn, char cProfile)
{
    if(bOn)
	downstreamRSCI.StartRSIRecording(Parameters, cProfile);
    else
	downstreamRSCI.StopRSIRecording();
}
