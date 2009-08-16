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
# include "source/shmsoundin.h"
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
CDRMReceiver::CDRMReceiver():
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
onBoardDemod(false),pcmInput(SoundCard),
rig(NULL),chanSel(CS_MIX_CHAN),
strPCMFile(),pHamlib(NULL),pRig(),soundIn(new CSoundIn())
{
	downstreamRSCI.SetReceiver(this);
    DataDecoder.setApplication(CDataParam::AD_DAB_SPEC_APP, AT_MOTSLISHOW, new CMOTDABDecFactory());
    DataDecoder.setApplication(CDataParam::AD_DAB_SPEC_APP, AT_MOTBROADCASTWEBSITE, new CMOTDABDecFactory());
    DataDecoder.setApplication(CDataParam::AD_DAB_SPEC_APP, AT_MOTTPEG, new EPGDecoderFactory());
    DataDecoder.setApplication(CDataParam::AD_DAB_SPEC_APP, AT_JOURNALINE, new JournalineFactory());
}

CDRMReceiver::~CDRMReceiver()
{
	delete soundIn.real;
	delete pSoundOutInterface;
}

void
CDRMReceiver::SetSoundInput(EInpTy wanted)
{
	if(soundIn.real)
	{
		delete soundIn.real;
		soundIn.real = NULL;
	}
	switch(wanted)
	{
	case SoundCard:
		soundIn.real = new CSoundIn;
		break;
	case Dummy:
		soundIn.real = new CSoundInNull;
		break;
	case Shm:
		{
# ifdef __linux__
			CShmSoundIn* ShmSoundIn = new CShmSoundIn;
			ShmSoundIn->SetShmPath(strPCMFile);
			ShmSoundIn->SetName("Radio Card");
			ShmSoundIn->SetShmChannels(1);
			ShmSoundIn->SetWantedChannels(2);
			soundIn.real = ShmSoundIn;
# endif
		}
		break;
	case File:
		{
			CSoundFileIn *pf = new CSoundFileIn;
			pf->SetFileName(strPCMFile);
			soundIn.real = pf;
		}
		break;
	case RSCI:
		soundIn.real = new CSoundInNull;
	}
}

// TODO make rig and sound card coupling done in the UI, simplify here.
// Pass rig object into receiver, not model, all pre-configured.
// pass soundcard how ??

void
CDRMReceiver::RigUpdate()
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib==NULL)
		return;
	if(rig.current != rig.wanted)
	{
		if(pRig)
		{
			pRig->close(); // could be done in destructor
			delete pRig;
		}
		pRig = rig.wanted;
		if(pRig == NULL)
			return; // TODO decide if throw is appropriate
		rig.current = rig.wanted;
		pRig->open();
	}
# if 0
	if(rigMode.current != rigMode.wanted)
	{
		CRigSettings s;
		bool hasSettings = pHamlib->GetRigSettings(s, rig.current, rigMode.wanted);
		if(hasSettings)
		{
			if(s.attributes["audiotype"]=="shm")
			{
				pcmInput.wanted = Shm;
				strPCMFile = s.config["if_path"];
			}
			else
			{
				pcmInput.wanted = SoundCard;
				string snddevin = s.attributes["snddevin"];
				if(snddevin != "")
				{
					stringstream s(snddevin);
					s >> soundIn.deviceNo.wanted;
				}
			}
			if(s.attributes["onboarddemod"]=="must")
			{
				onBoardDemod.current = true;
			}
			else if(s.attributes["onboarddemod"]=="can")
			{
				onBoardDemod.current = onBoardDemod.wanted;
			}
			else
			{
				onBoardDemod.current = false;
			}
			pRig->set_for_mode(s);
		}
		else
		{
			pcmInput.wanted = SoundCard;
		}
	}
	else
	{
		pcmInput.wanted = SoundCard;
	}
	stringstream s;
	s << soundIn.deviceNo.wanted;
	pHamlib->set_attribute(rig.current, rigMode.wanted, "snddevin", s.str()); // save for persistence
# endif
#endif
}

void
CDRMReceiver::SetEnableSMeter(bool bNew)
{
#ifdef HAVE_LIBHAMLIB
	if(pRig)
		pRig->SetEnableSMeter(bNew);
#endif
}

bool
CDRMReceiver::GetEnableSMeter()
{
#ifdef HAVE_LIBHAMLIB
	if(pRig)
		return pRig->GetEnableSMeter();
#endif
	return false;
}

void CDRMReceiver::SetUseAnalogHWDemod(bool bUse)
{
	onBoardDemod.wanted = bUse;
	Parameters.Lock();
	Parameters.RxEvent = ChannelReconfiguration; // trigger an update!
    Parameters.Unlock();
}

bool CDRMReceiver::GetUseAnalogHWDemod()
{
	return onBoardDemod.current;
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

void CDRMReceiver::SetAnalogFilterBWHz(EModulationType eNew, int iNew)
{
	AMDemodulation.SetFilterBWHz(eNew, iNew);
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

	/* check for Rig change */
	if(rig.wanted != rig.current)
	{
		RigUpdate();
	}

	/* check for OFDM source change */
	if(pcmInput.current != pcmInput.wanted)
	{
		SetSoundInput(pcmInput.wanted);
		pcmInput.current = pcmInput.wanted;
		initNeeded = true;
	}

	if(soundIn.deviceNo.current != soundIn.deviceNo.wanted)
	{
		soundIn.real->SetDev(soundIn.deviceNo.wanted);
		soundIn.deviceNo.current = soundIn.deviceNo.wanted;
		initNeeded = true;
	}

	if(initNeeded)
	{
		switch(eModulation)
		{
		case AM: case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:

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
		if(onBoardDemod.current)
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
		//cerr << "TimeSync OK" << endl;
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

bool
CDRMReceiver::GetRigChangeInProgress()
{
	return rig.wanted != rig.current;
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

	soundIn.real->Close();
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

	if(onBoardDemod.current)
	{
		OnboardDecoder.SetSoundInterface(soundIn.real);
		OnboardDecoder.SetInitFlag();
	}
	else
	{
		AMDemodulation.SetDemodType(eModulation);
		AMDemodulation.SetInitFlag();
		AMSSReceiver.Init();
		ReceiveData.SetInChanSel(chanSel.wanted);
		ReceiveData.SetSoundInterface(soundIn.real);
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

	Parameters.Lock();
	Parameters.SetFrequency(iFreqkHz);
	/* clear out AMSS data and re-initialise AMSS acquisition */
	if(Parameters.eModulation != DRM && Parameters.eModulation != NONE)
		Parameters.ResetServicesStreams();
	Parameters.Unlock();

	if (upstreamRSCI.GetOutEnabled() == true)
	{
		upstreamRSCI.SetFrequency(iFreqkHz);
		return true;
	}
	else
	{
		/* tell the RSCI and IQ file writer that freq has changed in case it needs to start a new file */
		if (downstreamRSCI.GetOutEnabled() == true)
			downstreamRSCI.NewFrequency(Parameters);

		WriteIQFile.NewFrequency(Parameters);

#ifdef HAVE_LIBHAMLIB
		if(pRig)
			return pRig->SetFrequency(iFreqkHz);
#endif
		return true;
	}
}

void
CDRMReceiver::SetIQRecording(bool bON)
{
	if(bON)
		WriteIQFile.StartRecording(Parameters);
	else
		WriteIQFile.StopRecording();
}

void
CDRMReceiver::SetRSIRecording(bool bOn, const char cProfile)
{
	downstreamRSCI.SetRSIRecording(Parameters, bOn, cProfile);
}

void
CDRMReceiver::SetReadPCMFromFile(const string strNFN)
{
	strPCMFile = strNFN;
	bool bIsIQ = false;
	string ext;
	size_t p = strNFN.rfind('.');
	if (p != string::npos)
		ext = strNFN.substr(p + 1);
	if (ext.substr(0, 2) == "iq")
		bIsIQ = true;
	if (ext.substr(0, 2) == "IQ")
		bIsIQ = true;

	if (bIsIQ)
		chanSel.wanted = CS_IQ_POS_ZERO;
	else
		chanSel.wanted = CS_MIX_CHAN;
	pcmInput.wanted = File;
	SetHamlib(NULL);
	Parameters.Lock();
	Parameters.RxEvent = ChannelReconfiguration; // trigger an update!
	Parameters.Unlock();
}

void CDRMReceiver::SetHamlib(CHamlib* p)
{
    if(pHamlib)
	delete pHamlib;
    pHamlib = p;
}

void CDRMReceiver::SetRig(CRig* r)
{
	rig.wanted = r;
}

void CDRMReceiver::GetRigList(CRigMap& rigs) const
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		pHamlib->GetRigList(rigs);
#endif
}

void CDRMReceiver::GetRigSettings(CRigSettings& s, int model, EModulationType mode) const
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		pHamlib->GetRigSettings(s, model, mode);
#endif
}

CRig* CDRMReceiver::CreateRig(int model) const
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		return pHamlib->GetRig(const_cast<CParameter&>(Parameters), model);
#endif
	return NULL;
}

const rig_caps*
CDRMReceiver::GetRigCaps(int model) const
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		return pHamlib->GetRigCaps(model);
#endif
	return NULL;
}

CRig* CDRMReceiver::GetCurrentRig() const
{
#ifdef HAVE_LIBHAMLIB
	map<EModulationType,int>::const_iterator i=rigformode.find(Parameters.eModulation);
	if(i==rigformode.end())
		return NULL;
	return rigs.find(i->second)->second;
#else
	return NULL;
#endif
}

CRig* CDRMReceiver::GetRig(int id) const
{
#ifdef HAVE_LIBHAMLIB
	map<int,CRig*>::const_iterator i=rigs.find(id);
	if(i==rigs.end())
		return NULL;
	return i->second;
#else
	return NULL;
#endif
}

void CDRMReceiver::SetRig(int id, CRig* r)
{
#ifdef HAVE_LIBHAMLIB
	rigs[id] = r;
#endif
}

void CDRMReceiver::SetRig(EModulationType e, int id)
{
#ifdef HAVE_LIBHAMLIB
	rigformode[e] = id;
#endif
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
CDRMReceiver::LoadSettings(CSettings& s)
{
	string strMode = s.Get("Receiver", "modulation", string("DRM"));

	/* Serial Number */
	string sSerialNumber = s.Get("Receiver", "serialnumber", string(""));

	Parameters.Lock();

	if(sSerialNumber == "")
	{
		Parameters.GenerateRandomSerialNumber();
	}
	else
	Parameters.sSerialNumber = sSerialNumber;
    /* Receiver ID */
	s.Put("Receiver", "serialnumber", Parameters.sSerialNumber);

	Parameters.GenerateReceiverID();

	/* Data files directory */
	string sDataFilesDirectory = s.Get(
	   "Receiver", "datafilesdirectory", Parameters.sDataFilesDirectory);
	// remove trailing slash if there
	size_t p = sDataFilesDirectory.find_last_not_of("/\\");
	if(p != string::npos)
		sDataFilesDirectory.erase(p+1);
	s.Put("Receiver", "datafilesdirectory", Parameters.sDataFilesDirectory);

	Parameters.sDataFilesDirectory = sDataFilesDirectory;

	Parameters.Unlock();

	/* Sync */
	SetFreqInt(ETypeIntFreq(s.Get("Receiver", "frequencyinterpolation", int(FWIENER))));
	SetTimeInt(ETypeIntTime(s.Get("Receiver", "timeinterpolation", int(TWIENER))));
	SetTiSyncTracType(ETypeTiSyncTrac(s.Get("Receiver", "tracking", 0)));

	/* Receiver ------------------------------------------------------------- */

    vector<string> vs;
    int dev;

	/* Sound In device */
	dev = s.Get("Receiver", "snddevin", 0);
	soundIn.real->Enumerate(vs);
	if(vs.size()>0)
	{
	    if(dev>=int(vs.size()))
	    dev = vs.size()-1;
	soundIn.real->SetDev(dev);
	}

	vs.clear();

	/* Sound Out device */
	dev = s.Get("Receiver", "snddevout", 0);
	pSoundOutInterface->Enumerate(vs);
	if(vs.size()>0)
	{
	    if(dev>=int(vs.size()))
	    dev = vs.size()-1;
	pSoundOutInterface->SetDev(dev);
	}

	string str;

	/* input from file (code for bare rs, pcap files moved to CSettings) */
	str = s.Get("command", "fileio", string(""));
	if(str != "")
		SetReadPCMFromFile(str);

	/* Flip spectrum flag */
	ReceiveData.SetFlippedSpectrum(s.Get("Receiver", "flipspectrum", false));

	int n = s.Get("command", "inchansel", -1);
	switch (n)
	{
	case 0:
		ReceiveData.SetInChanSel(CS_LEFT_CHAN);
		break;

	case 1:
		ReceiveData.SetInChanSel(CS_RIGHT_CHAN);
		break;

	case 2:
		ReceiveData.SetInChanSel(CS_MIX_CHAN);
		break;

	case 3:
		ReceiveData.SetInChanSel(CS_IQ_POS);
		break;

	case 4:
		ReceiveData.SetInChanSel(CS_IQ_NEG);
		break;

	case 5:
		ReceiveData.SetInChanSel(CS_IQ_POS_ZERO);
		break;

	case 6:
		ReceiveData.SetInChanSel(CS_IQ_NEG_ZERO);
		break;
	default:
		break;
	}
	n = s.Get("command", "outchansel", -1);
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

	/* AM Parameters */

	/* AGC */
	AMDemodulation.SetAGCType(EType(s.Get("AM Demodulation", "agc", 0)));

	/* noise reduction */
	AMDemodulation.SetNoiRedType(ENoiRedType(s.Get("AM Demodulation", "noisered", 0)));

	/* pll enabled/disabled */
	AMDemodulation.EnablePLL(s.Get("AM Demodulation", "enablepll", 0));

	/* auto frequency acquisition */
	AMDemodulation.EnableAutoFreqAcq(s.Get("AM Demodulation", "autofreqacq", 0));

	AMDemodulation.SetFilterBWHz(AM, s.Get("AM Demodulation", "filterbwam", 10000));
	AMDemodulation.SetFilterBWHz(LSB, s.Get("AM Demodulation", "filterbwlsb", 5000));
	AMDemodulation.SetFilterBWHz(USB, s.Get("AM Demodulation", "filterbwusb", 5000));
	AMDemodulation.SetFilterBWHz(CW, s.Get("AM Demodulation", "filterbwcw", 150));

	/* FM Parameters */
	AMDemodulation.SetFilterBWHz(NBFM, s.Get("FM Demodulation", "nbfilterbw", 6000));
	AMDemodulation.SetFilterBWHz(WBFM, s.Get("FM Demodulation", "wbfilterbw", 80000));

    switch(Parameters.eModulation)
    {
	case AM:
	case LSB:
	case USB:
	case CW:
	case NBFM:
	case WBFM:
	    AMDemodulation.SetDemodType(Parameters.eModulation);
	    break;
        default:;
    }
	/* upstream RSCI */
    str = s.Get("command", "rsiin");
	if(str != "")
	{
		bool bOK = upstreamRSCI.SetOrigin(str); // its a port
		if(!bOK)
	    throw CGenErr(string("can't open RSCI input ")+str);
		// disable sound input
	pcmInput.current = pcmInput.wanted = RSCI;
	SetHamlib(NULL);
		Parameters.Measurements.bETSIPSD = true;
	}

	str = s.Get("command", "rciout");
	if(str != "")
		upstreamRSCI.SetDestination(str);

	/* downstream RSCI */
	for(int i = 0; i<MAX_NUM_RSI_SUBSCRIBERS; i++)
	{
		stringstream ss;
		ss << "rsiout" << i;
		str = s.Get("command", ss.str());
		if(str != "")
		{
			ss.str("");
			ss << "rsioutprofile" << i;
			string profile = s.Get("command", ss.str(), string("A"));
	    Parameters.Measurements.bETSIPSD = true;

			// Check whether the profile has a subsampling ratio (e.g. --rsioutprofile A20)
			int iSubsamplingFactor = 1;
			if (profile.length() > 1)
			{
				iSubsamplingFactor = atoi(profile.substr(1).c_str());
			}

			ss.str("");
			ss << "rciin" << i;
			string origin = s.Get("command", ss.str());
			downstreamRSCI.AddSubscriber(str, origin, profile[0], iSubsamplingFactor);
		}
	}

	for (int i=1; i<=MAX_NUM_RSI_PRESETS; i++)
	{
		// define presets in same format as --rsioutprofile
		stringstream ss;
		ss << "rsioutpreset" << i;
		str = s.Get("RSCI", ss.str());
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
	/* RSCI File Recording */
	str = s.Get("command", "rsirecordprofile");
	string s2 = s.Get("command", "rsirecordtype");
	if(str != "" || s2 != "")
		downstreamRSCI.SetRSIRecording(Parameters, true, str[0], s2);

	/* IQ File Recording */
	if(s.Get("command", "recordiq", false))
		WriteIQFile.StartRecording(Parameters);

	/* Mute audio flag */
	WriteData.MuteAudio(s.Get("Receiver", "muteaudio", false));

	/* Output to File */
	str = s.Get("command", "writewav");
	if(str != "")
		WriteData.StartWriteWaveFile(str);

	/* Reverberation flag */
	AudioSourceDecoder.SetReverbEffect(s.Get("Receiver", "reverb", true));

	/* Bandpass filter flag */
	FreqSyncAcq.SetRecFilter(s.Get("Receiver", "filter", false));

	/* Set parameters for frequency acquisition search window if needed */
	 _REAL rFreqAcSeWinSize = s.Get("command", "fracwinsize", _REAL(SOUNDCRD_SAMPLE_RATE / 2));
	 _REAL rFreqAcSeWinCenter = s.Get("command", "fracwincent", _REAL(SOUNDCRD_SAMPLE_RATE / 4));
	/* Set new parameters */
	FreqSyncAcq.SetSearchWindow(rFreqAcSeWinCenter, rFreqAcSeWinSize);

	/* Modified metrics flag */
	ChannelEstimation.SetIntCons(s.Get("Receiver", "modmetric", false));

	/* Number of iterations for MLC setting */
	MSCMLCDecoder.SetNumIterations(s.Get("Receiver", "mlciter", 0));

	/* Activate/Deactivate EPG decoding */
	//DataDecoder.SetDecodeEPG(s.Get("EPG", "decodeepg", true));
	// TODO epg option


#ifdef HAVE_LIBHAMLIB
    if(pHamlib)
	pHamlib->LoadSettings(s);
#endif

	/* Front-end - combine into Hamlib? */
	Parameters.Lock();

	CFrontEndParameters& FrontEndParameters = Parameters.FrontEndParameters;

	FrontEndParameters.eSMeterCorrectionType =
		CFrontEndParameters::ESMeterCorrectionType(s.Get("FrontEnd", "smetercorrectiontype", 0));

	FrontEndParameters.rSMeterBandwidth = s.Get("FrontEnd", "smeterbandwidth", 0.0);

	FrontEndParameters.rDefaultMeasurementBandwidth = s.Get("FrontEnd", "defaultmeasurementbandwidth", 0);

	FrontEndParameters.bAutoMeasurementBandwidth = s.Get("FrontEnd", "automeasurementbandwidth", true);

	FrontEndParameters.rCalFactorDRM = s.Get("FrontEnd", "calfactordrm", 0.0);

	FrontEndParameters.rCalFactorAM = s.Get("FrontEnd", "calfactoram", 0.0);

	FrontEndParameters.rIFCentreFreq = s.Get("FrontEnd", "ifcentrefrequency", SOUNDCRD_SAMPLE_RATE / 4);

    Parameters.Unlock();

	/* Wanted RF Frequency */
	iFreqkHz = s.Get("Receiver", "frequency", 0);
	doSetFrequency();

    // Put this right at the end so that eModulation is correct and Rx starts
    Parameters.Lock();
	if (strMode == "DRM")
		Parameters.eModulation = DRM;
    else if (strMode == "AM")
	Parameters.eModulation = AM;
    else if (strMode == "USB")
	Parameters.eModulation = USB;
    else if (strMode == "LSB")
	Parameters.eModulation = LSB;
    else if (strMode == "CW")
	Parameters.eModulation = CW;
    else if (strMode == "NBFM")
	Parameters.eModulation = NBFM;
    else if (strMode == "WBFM")
	Parameters.eModulation = WBFM;
    else
	Parameters.eModulation = NONE;
    Parameters.Unlock();
}

void
CDRMReceiver::SaveSettings(CSettings& s)
{
    s.Put("0", "mode", string("RX"));
    string modn;
    Parameters.Lock();
	switch(Parameters.eModulation)
	{
	case DRM:
	modn = "DRM";
		break;
	case AM:
	modn = "AM";
		break;
	case  USB:
	modn = "USB";
		break;
	case  LSB:
	modn = "LSB";
		break;
	case  CW:
	modn = "CW";
		break;
	case  NBFM:
	modn = "NBFM";
		break;
	case  WBFM:
	modn = "WBFM";
		break;
	case NONE:
		;
	}
	Parameters.Unlock();

    s.Put("Receiver", "modulation", modn);

	/* Receiver ------------------------------------------------------------- */

	/* Flip spectrum flag */
	s.Put("Receiver", "flipspectrum", ReceiveData.GetFlippedSpectrum());

	/* Mute audio flag */
	s.Put("Receiver", "muteaudio", WriteData.GetMuteAudio());

	/* Reverberation */
	s.Put("Receiver", "reverb", AudioSourceDecoder.GetReverbEffect());

	/* Bandpass filter flag */
	s.Put("Receiver", "filter", FreqSyncAcq.GetRecFilter());

	/* Modified metrics flag */
	s.Put("Receiver", "modmetric", ChannelEstimation.GetIntCons());

	/* Sync */
	s.Put("Receiver", "frequencyinterpolation", int(GetFreqInt()));
	s.Put("Receiver", "timeinterpolation", int(GetTimeInt()));
	s.Put("Receiver", "tracking", int(GetTiSyncTracType()));

	/* Sound In device */
	s.Put("Receiver", "snddevin", pSoundOutInterface->GetDev());

	/* Sound Out device */
	s.Put("Receiver", "snddevout", pSoundOutInterface->GetDev());

	/* Number of iterations for MLC setting */
	s.Put("Receiver", "mlciter", MSCMLCDecoder.GetInitNumIterations());

	/* Tuned Frequency */
	s.Put("Receiver", "frequency", iFreqkHz);

	/* Active/Deactivate EPG decoding */
	//s.Put("EPG", "decodeepg", DataDecoder.GetDecodeEPG());


	/* AM Parameters */

	/* AGC */
	s.Put("AM Demodulation", "agc", AMDemodulation.GetAGCType());

	/* noise reduction */
	s.Put("AM Demodulation", "noisered", AMDemodulation.GetNoiRedType());

	/* pll enabled/disabled */
	s.Put("AM Demodulation", "enablepll", AMDemodulation.PLLEnabled());

	/* auto frequency acquisition */
	s.Put("AM Demodulation", "autofreqacq", AMDemodulation.AutoFreqAcqEnabled());

	s.Put("AM Demodulation", "filterbwam", AMDemodulation.GetFilterBWHz(AM));
	s.Put("AM Demodulation", "filterbwlsb", AMDemodulation.GetFilterBWHz(LSB));
	s.Put("AM Demodulation", "filterbwusb", AMDemodulation.GetFilterBWHz(USB));
	s.Put("AM Demodulation", "filterbwcw", AMDemodulation.GetFilterBWHz(CW));

	/* FM Parameters */

	s.Put("FM Demodulation", "nbfilterbw", AMDemodulation.GetFilterBWHz(NBFM));
	s.Put("FM Demodulation", "wbfilterbw", AMDemodulation.GetFilterBWHz(WBFM));

	/* Front-end - combine into Hamlib? */
	Parameters.Lock();
	s.Put("FrontEnd", "smetercorrectiontype", int(Parameters.FrontEndParameters.eSMeterCorrectionType));
	s.Put("FrontEnd", "smeterbandwidth", int(Parameters.FrontEndParameters.rSMeterBandwidth));
	s.Put("FrontEnd", "defaultmeasurementbandwidth", int(Parameters.FrontEndParameters.rDefaultMeasurementBandwidth));
	s.Put("FrontEnd", "automeasurementbandwidth", Parameters.FrontEndParameters.bAutoMeasurementBandwidth);
	s.Put("FrontEnd", "calfactordrm", int(Parameters.FrontEndParameters.rCalFactorDRM));
	s.Put("FrontEnd", "calfactoram", int(Parameters.FrontEndParameters.rCalFactorAM));
	s.Put("FrontEnd", "ifcentrefrequency", int(Parameters.FrontEndParameters.rIFCentreFreq));

	/* Serial Number */
	s.Put("Receiver", "serialnumber", Parameters.sSerialNumber);
	s.Put("Receiver", "datafilesdirectory", Parameters.sDataFilesDirectory);
	Parameters.Unlock();
}
