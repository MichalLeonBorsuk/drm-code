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
#include "util/Utilities.h"

#include "sound.h"
#include "sound/soundnull.h"
#include "audiofilein.h"
#include "GUI-QT/Rig.h"

const int
CDRMReceiver::MAX_UNLOCKED_COUNT = 2;

/* Implementation *************************************************************/
CDRMReceiver::CDRMReceiver():
    pSoundInInterface(new CSoundInNull), pSoundOutInterface(new CSoundOut),
    ReceiveData(), WriteData(pSoundOutInterface),
    FreqSyncAcq(),
    ChannelEstimation(),
    UtilizeFACData(), UtilizeSDCData(), MSCDemultiplexer(),
    AudioSourceDecoder(),
    upstreamRSCI(), DecodeRSIMDI(), downstreamRSCI(),
    pReceiverParam(NULL), pDRMParam(NULL), pAMParam(NULL),
    RSIPacketBuf(),
    MSCDecBuf(MAX_NUM_STREAMS), MSCUseBuf(MAX_NUM_STREAMS),
    MSCSendBuf(MAX_NUM_STREAMS), iAcquRestartCnt(0),
    iAcquDetecCnt(0), iGoodSignCnt(0), eReceiverMode(RM_DRM),
    eNewReceiverMode(RM_DRM), iAudioStreamID(STREAM_ID_NOT_USED),
    iDataStreamID(STREAM_ID_NOT_USED), bRestartFlag(TRUE),
    rInitResampleOffset((_REAL) 0.0),
    iBwAM(10000), iBwLSB(5000), iBwUSB(5000), iBwCW(150), iBwFM(6000),
    time_keeper(0),
#ifdef HAVE_LIBHAMLIB
    pRig(NULL),
#endif
    PlotManager(),rsiOrigin("")
{
    pReceiverParam = new CParameter(this);
    downstreamRSCI.SetReceiver(this);
    PlotManager.SetReceiver(this);
}

CDRMReceiver::~CDRMReceiver()
{
    delete pSoundInInterface;
    delete pSoundOutInterface;
    delete pReceiverParam;
}

void
CDRMReceiver::SetReceiverMode(ERecMode eNewMode)
{
    if (eReceiverMode!=eNewMode || eNewReceiverMode != RM_NONE)
        eNewReceiverMode = eNewMode;
}

void
CDRMReceiver::SetAMDemodType(CAMDemodulation::EDemodType eNew)
{
    AMDemodulation.SetDemodType(eNew);
    switch (eNew)
    {
    case CAMDemodulation::DT_AM:
        AMDemodulation.SetFilterBW(iBwAM);
        break;

    case CAMDemodulation::DT_LSB:
        AMDemodulation.SetFilterBW(iBwLSB);
        break;

    case CAMDemodulation::DT_USB:
        AMDemodulation.SetFilterBW(iBwUSB);
        break;

    case CAMDemodulation::DT_CW:
        AMDemodulation.SetFilterBW(iBwCW);
        break;

    case CAMDemodulation::DT_FM:
        AMDemodulation.SetFilterBW(iBwFM);
        break;
    }
}

void
CDRMReceiver::SetAMFilterBW(int value)
{
    /* Store filter bandwidth for this demodulation type */
    switch (AMDemodulation.GetDemodType())
    {
    case CAMDemodulation::DT_AM:
        iBwAM = value;
        break;

    case CAMDemodulation::DT_LSB:
        iBwLSB = value;
        break;

    case CAMDemodulation::DT_USB:
        iBwUSB = value;
        break;

    case CAMDemodulation::DT_CW:
        iBwCW = value;
        break;

    case CAMDemodulation::DT_FM:
        iBwFM = value;
        break;
    }
    AMDemodulation.SetFilterBW(value);
}

void
CDRMReceiver::Run()
{
    _BOOLEAN bEnoughData = TRUE;
    _BOOLEAN bFrameToSend = FALSE;
    size_t i;
    /* Check for parameter changes from RSCI or GUI thread --------------- */
    /* The parameter changes are done through flags, the actual initialization
     * is done in this (the working) thread to avoid problems with shared data */
    if (eNewReceiverMode != RM_NONE)
        InitReceiverMode();

    CParameter & ReceiverParam = *pReceiverParam;

#ifdef HAVE_LIBGPS
//TODO locking
    gps_data_t* gps_data = &ReceiverParam.gps_data;
    int result=0;
    if(ReceiverParam.restart_gpsd)
    {
        stringstream s;
        s <<  ReceiverParam.gps_port;
        if(gps_data->gps_fd != -1) (void)gps_close(gps_data);
#if GPSD_API_MAJOR_VERSION < 5
        result = gps_open_r(ReceiverParam.gps_host.c_str(), s.str().c_str(), gps_data);
        result = gps_stream(gps_data, WATCH_ENABLE|POLL_NONBLOCK, NULL);
#else
        result = gps_open(ReceiverParam.gps_host.c_str(), s.str().c_str(), gps_data);
        result = gps_stream(gps_data, WATCH_ENABLE, NULL);
#endif
        ReceiverParam.restart_gpsd = false;
    }
    if(ReceiverParam.use_gpsd)
#if GPSD_API_MAJOR_VERSION < 5
        result = gps_poll(gps_data);
#else
        result = gps_read(gps_data);
#endif
    else
        if(gps_data->gps_fd != -1) (void)gps_close(gps_data);
#endif

    if (bRestartFlag) /* new acquisition requested by GUI */
    {
        bRestartFlag = FALSE;
        SetInStartMode();
    }

    /* Input - from upstream RSCI or input and demodulation from sound card / file */

    if (upstreamRSCI.GetInEnabled() == TRUE)
    {
        RSIPacketBuf.Clear();
        upstreamRSCI.ReadData(ReceiverParam, RSIPacketBuf);
        if (RSIPacketBuf.GetFillLevel() > 0)
        {
            time_keeper = time(NULL);
            DecodeRSIMDI.ProcessData(ReceiverParam, RSIPacketBuf, FACDecBuf, SDCDecBuf, MSCDecBuf);
            PlotManager.UpdateParamHistoriesRSIIn();
            bFrameToSend = TRUE;
        }
        else
        {
            time_t now = time(NULL);
            if ((now - time_keeper) > 2)
            {
                ReceiverParam.ReceiveStatus.Interface.SetStatus(NOT_PRESENT);
                ReceiverParam.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
                ReceiverParam.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
                ReceiverParam.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
                ReceiverParam.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
                ReceiverParam.ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
                ReceiverParam.ReceiveStatus.MOT.SetStatus(NOT_PRESENT);
            }
        }
    }
    else
    {
        ReceiveData.ReadData(ReceiverParam, RecDataBuf);

        // Split samples, one output to the demodulation, another for IQ recording
        if (SplitForIQRecord.ProcessData(ReceiverParam, RecDataBuf, DemodDataBuf, IQRecordDataBuf))
        {
            bEnoughData = TRUE;
        }

        switch (eReceiverMode)
        {
        case RM_DRM:
            DemodulateDRM(bEnoughData);
            DecodeDRM(bEnoughData, bFrameToSend);
            break;
        case RM_AM:
            DemodulateAM(bEnoughData);
            DecodeAM(bEnoughData);
            break;
        case RM_FM:
            DemodulateFM(bEnoughData);
            DecodeFM(bEnoughData);
            break;
        case RM_NONE:
            break;
        }
    }

    /* Split the data for downstream RSCI and local processing. TODO make this conditional */
    switch (eReceiverMode)
    {
    case RM_DRM:
        SplitFAC.ProcessData(ReceiverParam, FACDecBuf, FACUseBuf, FACSendBuf);

        /* if we have an SDC block, make a copy and keep it until the next frame is to be sent */
        if (SDCDecBuf.GetFillLevel() == ReceiverParam.iNumSDCBitsPerSFrame)
        {
            SplitSDC.ProcessData(ReceiverParam, SDCDecBuf, SDCUseBuf, SDCSendBuf);
        }

        for (i = 0; i < MSCDecBuf.size(); i++)
        {
            SplitMSC[i].ProcessData(ReceiverParam, MSCDecBuf[i], MSCUseBuf[i], MSCSendBuf[i]);
        }
        break;
    case RM_AM:
        SplitAudio.ProcessData(ReceiverParam, AMAudioBuf, AudSoDecBuf, AMSoEncBuf);
        break;
    case RM_FM:
        SplitAudio.ProcessData(ReceiverParam, AMAudioBuf, AudSoDecBuf, AMSoEncBuf);
        break;
    case RM_NONE:
        break;
    }

    /* decoding */
    while (bEnoughData && (ReceiverParam.eRunState==CParameter::RUNNING))
    {
        /* Init flag */
        bEnoughData = FALSE;

        // Write output I/Q file
        if (WriteIQFile.WriteData(ReceiverParam, IQRecordDataBuf))
        {
            bEnoughData = TRUE;
        }

        switch (eReceiverMode)
        {
        case RM_DRM:
            UtilizeDRM(bEnoughData);
            break;
        case RM_AM:
            UtilizeAM(bEnoughData);
            break;
        case RM_FM:
            UtilizeFM(bEnoughData);
            break;
        case RM_NONE:
            break;
        }
    }

    /* output to downstream RSCI */
    if (downstreamRSCI.GetOutEnabled())
    {
        switch (eReceiverMode)
        {
        case RM_DRM:
            if (ReceiverParam.eAcquiState == AS_NO_SIGNAL)
            {
                /* we will get one of these between each FAC block, and occasionally we */
                /* might get two, so don't start generating free-wheeling RSCI until we've. */
                /* had three in a row */
                if (FreqSyncAcq.GetUnlockedFrameBoundary())
                {
                    if (iUnlockedCount < MAX_UNLOCKED_COUNT)
                        iUnlockedCount++;
                    else
                        downstreamRSCI.SendUnlockedFrame(ReceiverParam);
                }
            }
            else if (bFrameToSend)
            {
                downstreamRSCI.SendLockedFrame(ReceiverParam, FACSendBuf, SDCSendBuf, MSCSendBuf);
                iUnlockedCount = 0;
                bFrameToSend = FALSE;
            }
            break;
        case RM_AM:
        case RM_FM:
            /* Encode audio for RSI output */
            if (AudioSourceEncoder.ProcessData(ReceiverParam, AMSoEncBuf, MSCSendBuf[0]))
                bFrameToSend = TRUE;

            if (bFrameToSend)
                downstreamRSCI.SendAMFrame(ReceiverParam, MSCSendBuf[0]);
            break;
        case RM_NONE:
            break;
        }
    }
    // check for RSCI commands
    if (downstreamRSCI.GetInEnabled())
    {
        downstreamRSCI.poll();
    }

    /* Play and/or save the audio */
    if (iAudioStreamID != STREAM_ID_NOT_USED || (eReceiverMode == RM_AM) || (eReceiverMode == RM_FM))

    {
        if (WriteData.WriteData(ReceiverParam, AudSoDecBuf))
        {
            bEnoughData = TRUE;
        }
    }
}

void
CDRMReceiver::DemodulateDRM(_BOOLEAN& bEnoughData)
{
    CParameter & ReceiverParam = *pReceiverParam;

    /* Resample input DRM-stream -------------------------------- */
    if (InputResample.ProcessData(ReceiverParam, DemodDataBuf, InpResBuf))
    {
        bEnoughData = TRUE;
    }

    /* Frequency synchronization acquisition -------------------- */
    if (FreqSyncAcq.ProcessData(ReceiverParam, InpResBuf, FreqSyncAcqBuf))
    {
        bEnoughData = TRUE;
    }

    /* Time synchronization ------------------------------------- */
    if (TimeSync.ProcessData(ReceiverParam, FreqSyncAcqBuf, TimeSyncBuf))
    {
        bEnoughData = TRUE;
        /* Use count of OFDM-symbols for detecting
         * aquisition state for acquisition detection
         * only if no signal was decoded before */
        if (ReceiverParam.eAcquiState == AS_NO_SIGNAL)
        {
            /* Increment symbol counter and check if bound is reached */
            iAcquDetecCnt++;

            if (iAcquDetecCnt > NUM_OFDMSYM_U_ACQ_WITHOUT)
                SetInStartMode();
        }
    }

    /* OFDM-demodulation ---------------------------------------- */
    if (OFDMDemodulation.
            ProcessData(ReceiverParam, TimeSyncBuf, OFDMDemodBuf))
    {
        bEnoughData = TRUE;
    }

    /* Synchronization in the frequency domain (using pilots) --- */
    if (SyncUsingPil.
            ProcessData(ReceiverParam, OFDMDemodBuf, SyncUsingPilBuf))
    {
        bEnoughData = TRUE;
    }

    /* Channel estimation and equalisation ---------------------- */
    if (ChannelEstimation.
            ProcessData(ReceiverParam, SyncUsingPilBuf, ChanEstBuf))
    {
        bEnoughData = TRUE;

        /* If this module has finished, all synchronization units
           have also finished their OFDM symbol based estimates.
           Update synchronization parameters histories */
        PlotManager.UpdateParamHistories(eReceiverState);
    }

    /* Demapping of the MSC, FAC, SDC and pilots off the carriers */
    if (OFDMCellDemapping.ProcessData(ReceiverParam, ChanEstBuf,
                                      MSCCarDemapBuf,
                                      FACCarDemapBuf, SDCCarDemapBuf))
    {
        bEnoughData = TRUE;
    }

}

void
CDRMReceiver::DecodeDRM(_BOOLEAN& bEnoughData, _BOOLEAN& bFrameToSend)
{
    CParameter & ReceiverParam = *pReceiverParam;

    /* FAC ------------------------------------------------------ */
    if (FACMLCDecoder.ProcessData(ReceiverParam, FACCarDemapBuf, FACDecBuf))
    {
        bEnoughData = TRUE;
        bFrameToSend = TRUE;
    }

    /* SDC ------------------------------------------------------ */
    if (SDCMLCDecoder.ProcessData(ReceiverParam, SDCCarDemapBuf, SDCDecBuf))
    {
        bEnoughData = TRUE;
    }

    /* MSC ------------------------------------------------------ */

    /* Symbol de-interleaver */
    if (SymbDeinterleaver.ProcessData(ReceiverParam, MSCCarDemapBuf, DeintlBuf))
    {
        bEnoughData = TRUE;
    }

    /* MLC decoder */
    if (MSCMLCDecoder.ProcessData(ReceiverParam, DeintlBuf, MSCMLCDecBuf))
    {
        bEnoughData = TRUE;
    }

    /* MSC demultiplexer (will leave FAC & SDC alone! */
    if (MSCDemultiplexer.ProcessData(ReceiverParam, MSCMLCDecBuf, MSCDecBuf))
    {
        bEnoughData = TRUE;
    }
}

void
CDRMReceiver::UtilizeDRM(_BOOLEAN& bEnoughData)
{
    CParameter & ReceiverParam = *pReceiverParam;

    if (UtilizeFACData.WriteData(ReceiverParam, FACUseBuf))
    {
        bEnoughData = TRUE;

        /* Use information of FAC CRC for detecting the acquisition
           requirement */
        DetectAcquiFAC();
#if 0
        saveSDCtoFile();
#endif
    }

    if (UtilizeSDCData.WriteData(ReceiverParam, SDCUseBuf))
    {
        bEnoughData = TRUE;
    }

    /* Data decoding */
    if (iDataStreamID != STREAM_ID_NOT_USED)
    {
        if (DataDecoder.WriteData(ReceiverParam, MSCUseBuf[iDataStreamID]))
            bEnoughData = TRUE;
    }
    /* Source decoding (audio) */
    if (iAudioStreamID != STREAM_ID_NOT_USED)
    {
        if (AudioSourceDecoder.ProcessData(ReceiverParam,
                                           MSCUseBuf[iAudioStreamID],
                                           AudSoDecBuf))
        {
            bEnoughData = TRUE;

            /* Store the number of correctly decoded audio blocks for
             *                            the history */
            PlotManager.SetCurrentCDAud(AudioSourceDecoder.GetNumCorDecAudio());
        }
    }
}

void
CDRMReceiver::DemodulateAM(_BOOLEAN& bEnoughData)
{
    CParameter & ReceiverParam = *pReceiverParam;

    /* The incoming samples are split 2 ways.
       One set is passed to the existing AM demodulator.
       The other set is passed to the new AMSS demodulator.
       The AMSS and AM demodulators work completely independently
     */
    if (Split.ProcessData(ReceiverParam, DemodDataBuf, AMDataBuf, AMSSDataBuf))
    {
        bEnoughData = TRUE;
    }

    /* AM demodulation ------------------------------------------ */
    if (AMDemodulation.ProcessData(ReceiverParam, AMDataBuf, AMAudioBuf))
    {
        bEnoughData = TRUE;
    }

    /* AMSS phase demodulation */
    if (AMSSPhaseDemod.ProcessData(ReceiverParam, AMSSDataBuf, AMSSPhaseBuf))
    {
        bEnoughData = TRUE;
    }
}

void
CDRMReceiver::DecodeAM(_BOOLEAN& bEnoughData)
{
    CParameter & ReceiverParam = *pReceiverParam;

    /* AMSS resampling */
    if (InputResample.ProcessData(ReceiverParam, AMSSPhaseBuf, AMSSResPhaseBuf))
    {
        bEnoughData = TRUE;
    }

    /* AMSS bit extraction */
    if (AMSSExtractBits.
            ProcessData(ReceiverParam, AMSSResPhaseBuf, AMSSBitsBuf))
    {
        bEnoughData = TRUE;
    }

    /* AMSS data decoding */
    if (AMSSDecode.ProcessData(ReceiverParam, AMSSBitsBuf, SDCDecBuf))
    {
        bEnoughData = TRUE;
    }
}

void
CDRMReceiver::UtilizeAM(_BOOLEAN& bEnoughData)
{
    CParameter & ReceiverParam = *pReceiverParam;

    if (UtilizeSDCData.WriteData(ReceiverParam, SDCDecBuf))
    {
        bEnoughData = TRUE;
    }
}

void CDRMReceiver::DemodulateFM(_BOOLEAN& bEnoughData)
{
    if (ConvertAudio.ProcessData(*pReceiverParam, DemodDataBuf, AMAudioBuf))
    {
        bEnoughData = TRUE;
        iAudioStreamID = 0; // TODO
    }
}

void CDRMReceiver::DecodeFM(_BOOLEAN& bEnoughData)
{
    (void)bEnoughData;
}

void CDRMReceiver::UtilizeFM(_BOOLEAN& bEnoughData)
{
    (void)bEnoughData;
}

void
CDRMReceiver::DetectAcquiFAC()
{
    /* If upstreamRSCI in is enabled, do not check for acquisition state because we want
       to stay in tracking mode all the time */
    if (upstreamRSCI.GetInEnabled() == TRUE)
        return;

    /* Acquisition switch */
    if (!UtilizeFACData.GetCRCOk())
    {
        /* Reset "good signal" count */
        iGoodSignCnt = 0;

        iAcquRestartCnt++;

        /* Check situation when receiver must be set back in start mode */
        if ((pReceiverParam->eAcquiState == AS_WITH_SIGNAL)
                && (iAcquRestartCnt > NUM_FAC_FRA_U_ACQ_WITH))
        {
            SetInStartMode();
        }
    }
    else
    {
        /* Set the receiver state to "with signal" not until two successive FAC
           frames are "ok", because there is only a 8-bit CRC which is not good
           for many bit errors. But it is very unlikely that we have two
           successive FAC blocks "ok" if no good signal is received */
        if (iGoodSignCnt > 0)
        {
            pReceiverParam->eAcquiState = AS_WITH_SIGNAL;

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

            iGoodSignCnt++;
        }
    }
}

void
CDRMReceiver::InitReceiverMode()
{
    switch (eNewReceiverMode)
    {
    case RM_AM:
    case RM_FM:
        if (pAMParam == NULL)
        {
            /* its the first time we have been in AM mode */
            if (pDRMParam == NULL)
            {
                /* DRM Mode was never invoked so we get to claim the default parameter instance */
                pAMParam = pReceiverParam;
            }
            else
            {
                /* change from DRM to AM Mode - we better have our own copy
                 * but make sure we inherit the initial settings of the default
                 */
                pAMParam = new CParameter(*pDRMParam);
            }
        }
        else
        {
            /* we have been in AM mode before, and have our own parameters but
             * we might need some state from the DRM mode params
             */
            switch (eReceiverMode)
            {
            case RM_AM:
                /* AM to AM switch */
                break;
            case RM_FM:
                /* AM to FM switch - re-acquisition requested - no special action */
                break;
            case RM_DRM:
                /* DRM to AM switch - grab some common stuff */
                pAMParam->rSigStrengthCorrection = pDRMParam->rSigStrengthCorrection;
                pAMParam->bMeasurePSD = pDRMParam->bMeasurePSD;
                pAMParam->bMeasurePSDAlways = pDRMParam->bMeasurePSDAlways;
                pAMParam->bMeasureInterference = pDRMParam->bMeasureInterference;
                pAMParam->FrontEndParameters = pDRMParam->FrontEndParameters;
                pAMParam->gps_data = pDRMParam->gps_data;
                pAMParam->use_gpsd = pDRMParam->use_gpsd;
                pAMParam->sSerialNumber = pDRMParam->sSerialNumber;
                pAMParam->sReceiverID  = pDRMParam->sReceiverID;
                pAMParam->sDataFilesDirectory = pDRMParam->sDataFilesDirectory;
                break;
            case RM_NONE:
                /* Start from cold in AM mode - no special action */
                break;
            }
        }
        pAMParam->eReceiverMode = eNewReceiverMode;
        pReceiverParam = pAMParam;

        if (pReceiverParam == NULL)
            throw CGenErr("Something went terribly wrong in the Receiver");

        /* Tell the SDC decoder that it's AMSS to decode (no AFS index) */
        UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_AMSS);

        /* Set the receive status - this affects the RSI output */
        pAMParam->ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
        pAMParam->ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
        pAMParam->ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
        pAMParam->ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
        pAMParam->ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
        pAMParam->ReceiveStatus.MOT.SetStatus(NOT_PRESENT);
        break;
    case RM_DRM:
        if (pDRMParam == NULL)
        {
            /* its the first time we have been in DRM mode */
            if (pAMParam == NULL)
            {
                /* AM Mode was never invoked so we get to claim the default parameter instance */
                pDRMParam = pReceiverParam;
            }
            else
            {
                /* change from AM to DRM Mode - we better have our own copy
                 * but make sure we inherit the initial settings of the default
                 */
                pDRMParam = new CParameter(*pAMParam);
            }
        }
        else
        {
            /* we have been in DRM mode before, and have our own parameters but
             * we might need some state from the AM mode params
             */
            switch (eReceiverMode)
            {
            case RM_AM:
            case RM_FM:
                /* AM to DRM switch - grab some common stuff */
                pDRMParam->rSigStrengthCorrection = pAMParam->rSigStrengthCorrection;
                pDRMParam->bMeasurePSD = pAMParam->bMeasurePSD;
                pDRMParam->bMeasurePSDAlways = pAMParam->bMeasurePSDAlways;
                pDRMParam->bMeasureInterference = pAMParam->bMeasureInterference;
                pDRMParam->FrontEndParameters = pAMParam->FrontEndParameters;
                pDRMParam->gps_data = pAMParam->gps_data;
                pDRMParam->use_gpsd = pAMParam->use_gpsd;
                pDRMParam->sSerialNumber = pAMParam->sSerialNumber;
                pDRMParam->sReceiverID  = pAMParam->sReceiverID;
                pDRMParam->sDataFilesDirectory = pAMParam->sDataFilesDirectory;
                break;
            case RM_DRM:
                /* DRM to DRM switch - re-acquisition requested - no special action */
                break;
            case RM_NONE:
                /* Start from cold in DRM mode - no special action */
                break;
            }
        }
        pDRMParam->eReceiverMode = RM_DRM;
        pReceiverParam = pDRMParam;

        if (pReceiverParam == NULL)
            throw CGenErr("Something went terribly wrong in the Receiver");

        UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_DRM);
        break;
    case RM_NONE:
        return;
    }

    eReceiverMode = eNewReceiverMode;
    /* Reset new mode flag */
    eNewReceiverMode = RM_NONE;

    /* Init all modules */
    SetInStartMode();

    if (upstreamRSCI.GetOutEnabled() == TRUE)
    {
        upstreamRSCI.SetReceiverMode(eReceiverMode);
    }
}

void
CDRMReceiver::Start()
{
    // set this here to make sure we are in a QThread
    if(rsiOrigin != "")
        upstreamRSCI.SetOrigin(rsiOrigin);

    /* Set run flag so that the thread can work */
    pReceiverParam->eRunState = CParameter::RUNNING;

    // set the frequency from the command line or ini file
    int iFreqkHz = pReceiverParam->GetFrequency();
    if(iFreqkHz!=-1)
    {
        SetFrequency(iFreqkHz);
    }

    do
    {
        Run();
    }
    while (pReceiverParam->eRunState == CParameter::RUNNING);

    pSoundInInterface->Close();
    pSoundOutInterface->Close();

    pReceiverParam->eRunState = CParameter::STOPPED;
}

void
CDRMReceiver::Stop()
{
    pReceiverParam->eRunState = CParameter::STOP_REQUESTED;
}

void
CDRMReceiver::SetAMDemodAcq(_REAL rNewNorCen)
{
    /* Set the frequency where the AM demodulation should look for the
       aquisition. Receiver must be in AM demodulation mode */
    if (eReceiverMode == RM_AM)
    {
        AMDemodulation.SetAcqFreq(rNewNorCen);
        AMSSPhaseDemod.SetAcqFreq(rNewNorCen);
    }
}

void
CDRMReceiver::SetInStartMode()
{
    CParameter & ReceiverParam = *pReceiverParam;

    iUnlockedCount = MAX_UNLOCKED_COUNT;

    ReceiverParam.Lock();
    /* Load start parameters for all modules */

    /* Define with which parameters the receiver should try to decode the
       signal. If we are correct with our assumptions, the receiver does not
       need to reinitialize */
    ReceiverParam.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);

    /* Set initial MLC parameters */
    ReceiverParam.SetInterleaverDepth(CParameter::SI_LONG);
    ReceiverParam.SetMSCCodingScheme(CS_3_SM);
    ReceiverParam.SetSDCCodingScheme(CS_2_SM);

    /* Select the service we want to decode. Always zero, because we do not
       know how many services are transmitted in the signal we want to
       decode */

    /* TODO: if service 0 is not used but another service is the audio service
     * we have a problem. We should check as soon as we have information about
     * services if service 0 is really the audio service
     */

    /* Set the following parameters to zero states (initial states) --------- */
    ReceiverParam.ResetServicesStreams();
    ReceiverParam.ResetCurSelAudDatServ();

    /* Protection levels */
    ReceiverParam.MSCPrLe.iPartA = 0;
    ReceiverParam.MSCPrLe.iPartB = 1;
    ReceiverParam.MSCPrLe.iHierarch = 0;

    /* Number of audio and data services */
    ReceiverParam.iNumAudioService = 0;
    ReceiverParam.iNumDataService = 0;

    /* We start with FAC ID = 0 (arbitrary) */
    ReceiverParam.iFrameIDReceiv = 0;

    /* Set synchronization parameters */
    ReceiverParam.rResampleOffset = rInitResampleOffset;	/* Initial resample offset */
    ReceiverParam.rFreqOffsetAcqui = (_REAL) 0.0;
    ReceiverParam.rFreqOffsetTrack = (_REAL) 0.0;
    ReceiverParam.iTimingOffsTrack = 0;

    ReceiverParam.Unlock();

    /* Initialization of the modules */
    InitsForAllModules();

    /* Activate acquisition */
    FreqSyncAcq.StartAcquisition();
    TimeSync.StartAcquisition();
    ChannelEstimation.GetTimeSyncTrack()->StopTracking();
    ChannelEstimation.StartSaRaOffAcq();
    ChannelEstimation.GetTimeWiener()->StopTracking();

    SyncUsingPil.StartAcquisition();
    SyncUsingPil.StopTrackPil();

    ReceiverParam.Lock();
    /* Set flag that no signal is currently received */
    ReceiverParam.eAcquiState = AS_NO_SIGNAL;

    /* Set flag for receiver state */
    eReceiverState = RS_ACQUISITION;

    /* Reset counters for acquisition decision, "good signal" and delayed
       tracking mode counter */
    iAcquRestartCnt = 0;
    iAcquDetecCnt = 0;
    iGoodSignCnt = 0;
    iDelayedTrackModeCnt = NUM_FAC_DEL_TRACK_SWITCH;

    /* Reset GUI lights */
    ReceiverParam.ReceiveStatus.Interface.SetStatus(NOT_PRESENT);
    ReceiverParam.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
    ReceiverParam.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
    ReceiverParam.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
    ReceiverParam.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
    ReceiverParam.ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
    ReceiverParam.ReceiveStatus.MOT.SetStatus(NOT_PRESENT);

    ReceiverParam.Unlock();

    /* In case upstreamRSCI is enabled, go directly to tracking mode, do not activate the
       synchronization units */
    if (upstreamRSCI.GetInEnabled() == TRUE)
    {
        /* We want to have as low CPU usage as possible, therefore set the
           synchronization units in a state where they do only a minimum
           work */
        FreqSyncAcq.StopAcquisition();
        TimeSync.StopTimingAcqu();
        InputResample.SetSyncInput(TRUE);
        SyncUsingPil.SetSyncInput(TRUE);

        /* This is important so that always the same amount of module input
           data is queried, otherwise it could be that amount of input data is
           set to zero and the receiver gets into an infinite loop */
        TimeSync.SetSyncInput(TRUE);

        /* Always tracking mode for upstreamRSCI */
        ReceiverParam.Lock();
        ReceiverParam.eAcquiState = AS_WITH_SIGNAL;
        ReceiverParam.Unlock();

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
CDRMReceiver::InitsForAllModules()
{
    if (downstreamRSCI.GetOutEnabled())
    {
        pReceiverParam->bMeasureDelay = TRUE;
        pReceiverParam->bMeasureDoppler = TRUE;
        pReceiverParam->bMeasureInterference = TRUE;
        pReceiverParam->bMeasurePSD = TRUE;
    }
    else
    {
        pReceiverParam->bMeasureDelay = FALSE;
        pReceiverParam->bMeasureDoppler = FALSE;
        pReceiverParam->bMeasureInterference = FALSE;
        if(pReceiverParam->bMeasurePSDAlways)
            pReceiverParam->bMeasurePSD = TRUE;
        else
            pReceiverParam->bMeasurePSD = FALSE;
    }

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
    ConvertAudio.SetInitFlag();

    ReceiveData.SetSoundInterface(pSoundInInterface);
    ReceiveData.SetInitFlag();
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
    AMDemodulation.SetInitFlag();

    SplitForIQRecord.SetInitFlag();
    WriteIQFile.SetInitFlag();
    /* AMSS */
    AMSSPhaseDemod.SetInitFlag();
    AMSSExtractBits.SetInitFlag();
    AMSSDecode.SetInitFlag();

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
    AMSSPhaseBuf.Clear();
    AMSSResPhaseBuf.Clear();
    AMSSBitsBuf.Clear();

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

/* -----------------------------------------------------------------------------
   Initialization routines for the modules. We have to look into the modules
   and decide on which parameters the modules depend on */
void
CDRMReceiver::InitsForWaveMode()
{
    /* Reset averaging of the parameter histories (needed, e.g., because the
       number of OFDM symbols per DRM frame might have changed) */
    PlotManager.Init();

    /* After a new robustness mode was detected, give the time synchronization
       a bit more time for its job */
    iAcquDetecCnt = 0;

    /* Set init flags */
    ReceiveData.SetSoundInterface(pSoundInInterface);
    ReceiveData.SetInitFlag();
    InputResample.SetInitFlag();
    FreqSyncAcq.SetInitFlag();
    Split.SetInitFlag();
    AMDemodulation.SetInitFlag();
    AudioSourceEncoder.SetInitFlag();

    SplitForIQRecord.SetInitFlag();
    WriteIQFile.SetInitFlag();

    AMSSPhaseDemod.SetInitFlag();
    AMSSExtractBits.SetInitFlag();
    AMSSDecode.SetInitFlag();

    TimeSync.SetInitFlag();
    OFDMDemodulation.SetInitFlag();
    SyncUsingPil.SetInitFlag();
    ChannelEstimation.SetInitFlag();
    OFDMCellDemapping.SetInitFlag();
    SymbDeinterleaver.SetInitFlag();	// Because of "iNumUsefMSCCellsPerFrame"
    MSCMLCDecoder.SetInitFlag();	// Because of "iNumUsefMSCCellsPerFrame"
    SDCMLCDecoder.SetInitFlag();	// Because of "iNumSDCCellsPerSFrame"
}

void
CDRMReceiver::InitsForSpectrumOccup()
{
    /* Set init flags */
    FreqSyncAcq.SetInitFlag();	// Because of bandpass filter
    OFDMDemodulation.SetInitFlag();
    SyncUsingPil.SetInitFlag();
    ChannelEstimation.SetInitFlag();
    OFDMCellDemapping.SetInitFlag();
    SymbDeinterleaver.SetInitFlag();	// Because of "iNumUsefMSCCellsPerFrame"
    MSCMLCDecoder.SetInitFlag();	// Because of "iNumUsefMSCCellsPerFrame"
    SDCMLCDecoder.SetInitFlag();	// Because of "iNumSDCCellsPerSFrame"
}

/* SDC ---------------------------------------------------------------------- */
void
CDRMReceiver::InitsForSDCCodSche()
{
    /* Set init flags */
    SDCMLCDecoder.SetInitFlag();

#ifdef USE_DD_WIENER_FILT_TIME
    ChannelEstimation.SetInitFlag();
#endif
}

void
CDRMReceiver::InitsForNoDecBitsSDC()
{
    /* Set init flag */
    SplitSDC.SetInitFlag();
    UtilizeSDCData.SetInitFlag();
}

/* MSC ---------------------------------------------------------------------- */
void
CDRMReceiver::InitsForInterlDepth()
{
    /* Can be absolutely handled seperately */
    SymbDeinterleaver.SetInitFlag();
}

void
CDRMReceiver::InitsForMSCCodSche()
{
    /* Set init flags */
    MSCMLCDecoder.SetInitFlag();
    MSCDemultiplexer.SetInitFlag();	// Not sure if really needed, look at code! TODO

#ifdef USE_DD_WIENER_FILT_TIME
    ChannelEstimation.SetInitFlag();
#endif
}

void
CDRMReceiver::InitsForMSC()
{
    /* Set init flags */
    MSCMLCDecoder.SetInitFlag();

    InitsForMSCDemux();
}

void
CDRMReceiver::InitsForMSCDemux()
{
    /* Set init flags */
    DecodeRSIMDI.SetInitFlag();
    MSCDemultiplexer.SetInitFlag();
    for (size_t i = 0; i < MSCDecBuf.size(); i++)
    {
        SplitMSC[i].SetStream(i);
        SplitMSC[i].SetInitFlag();
    }
    InitsForAudParam();
    InitsForDataParam();

    /* Reset value used for the history because if an audio service was selected
       but then only a data service is selected, the value would remain with the
       last state */
    PlotManager.SetCurrentCDAud(0);
}

void
CDRMReceiver::InitsForAudParam()
{
    for (size_t i = 0; i < MSCDecBuf.size(); i++)
    {
        MSCDecBuf[i].Clear();
        MSCUseBuf[i].Clear();
        MSCSendBuf[i].Clear();
    }

    /* Set init flags */
    DecodeRSIMDI.SetInitFlag();
    MSCDemultiplexer.SetInitFlag();
    int a = pReceiverParam->GetCurSelAudioService();
    iAudioStreamID = pReceiverParam->GetAudioParam(a).iStreamID;
    pReceiverParam->SetNumAudioDecoderBits(pReceiverParam->
                                           GetStreamLen(iAudioStreamID) *
                                           SIZEOF__BYTE);
    AudioSourceDecoder.SetInitFlag();
}

void
CDRMReceiver::InitsForDataParam()
{
    /* Set init flags */
    DecodeRSIMDI.SetInitFlag();
    MSCDemultiplexer.SetInitFlag();
    int d = pReceiverParam->GetCurSelDataService();
    iDataStreamID = pReceiverParam->GetDataParam(d).iStreamID;
    pReceiverParam->SetNumDataDecoderBits(pReceiverParam->
                                          GetStreamLen(iDataStreamID) *
                                          SIZEOF__BYTE);
    DataDecoder.SetInitFlag();
}

void CDRMReceiver::SetFrequency(int iNewFreqkHz)
{
    pReceiverParam->Lock();
    pReceiverParam->SetFrequency(iNewFreqkHz);
    /* clear out AMSS data and re-initialise AMSS acquisition */
    if (pReceiverParam->eReceiverMode == RM_AM)
        pReceiverParam->ResetServicesStreams();
    pReceiverParam->Unlock();

    if (upstreamRSCI.GetOutEnabled() == TRUE)
    {
        upstreamRSCI.SetFrequency(iNewFreqkHz);
    }

#ifdef HAVE_LIBHAMLIB
    if(pRig)
        pRig->SetFrequency(iNewFreqkHz);
#endif

    if (downstreamRSCI.GetOutEnabled() == TRUE)
        downstreamRSCI.NewFrequency(*pReceiverParam);

    /* tell the IQ file writer that freq has changed in case it needs to start a new file */
    WriteIQFile.NewFrequency(*pReceiverParam);
}

void
CDRMReceiver::SetIQRecording(_BOOLEAN bON)
{
    if (bON)
        WriteIQFile.StartRecording(*pReceiverParam);
    else
        WriteIQFile.StopRecording();
}

void
CDRMReceiver::SetRSIRecording(_BOOLEAN bOn, const char cProfile)
{
    downstreamRSCI.SetRSIRecording(*pReceiverParam, bOn, cProfile);
}

/* TEST store information about alternative frequency transmitted in SDC */
void
CDRMReceiver::saveSDCtoFile()
{
    CParameter & ReceiverParam = *pReceiverParam;
    static FILE *pFile = NULL;

    if (pFile == NULL)
        pFile = fopen("test/altfreq.dat", "w");

    ReceiverParam.Lock();
    size_t inum = ReceiverParam.AltFreqSign.vecMultiplexes.size();
    for (size_t z = 0; z < inum; z++)
    {
        fprintf(pFile, "sync:%d sr:", ReceiverParam.AltFreqSign.vecMultiplexes[z].bIsSyncMultplx);

        for (int k = 0; k < 4; k++)
            fprintf(pFile, "%d", ReceiverParam.AltFreqSign.vecMultiplexes[z].  veciServRestrict[k]);
        fprintf(pFile, " fr:");

        for (size_t kk = 0; kk < ReceiverParam.AltFreqSign.vecMultiplexes[z].veciFrequencies.size(); kk++)
            fprintf(pFile, "%d ", ReceiverParam.AltFreqSign.vecMultiplexes[z].  veciFrequencies[kk]);

        fprintf(pFile, " rID:%d sID:%d   /   ",
                ReceiverParam.AltFreqSign.vecMultiplexes[z].iRegionID,
                ReceiverParam.AltFreqSign.vecMultiplexes[z].iScheduleID);
    }
    ReceiverParam.Unlock();
    fprintf(pFile, "\n");
    fflush(pFile);
}

void
CDRMReceiver::LoadSettings(CSettings& s)
{
    string str;
    CReceiveData::EInChanSel defaultInChanSel = CReceiveData::CS_MIX_CHAN;

    /* Serial Number */
    string sValue = s.Get("Receiver", "serialnumber");
    if (sValue != "")
    {
        // Pad to a minimum of 6 characters
        while (sValue.length() < 6)
            sValue += "_";
        pReceiverParam->sSerialNumber = sValue;
    }

    pReceiverParam->GenerateReceiverID();

    /* Data files directory */
    string sDataFilesDirectory = s.Get(
                                     "Receiver", "datafilesdirectory", pReceiverParam->sDataFilesDirectory);
    // remove trailing slash if there
    size_t p = sDataFilesDirectory.find_last_not_of("/\\");
    if (p != string::npos)
        sDataFilesDirectory.erase(p+1);

    pReceiverParam->sDataFilesDirectory = sDataFilesDirectory;
    s.Put("Receiver", "datafilesdirectory", pReceiverParam->sDataFilesDirectory);

    /* Receiver ------------------------------------------------------------- */

    /* if 0 then only measure PSD when RSCI in use otherwise always measure it */
    pReceiverParam->bMeasurePSDAlways =	s.Get("Receiver", "measurepsdalways", 0);

    /* upstream RSCI or sound card, etc. */
    str = s.Get("command", "rsiin");
    if (str != "")
    {
        rsiOrigin = str;
    }
    else
    {
        /* input from file or sound card */
        string strInFile = s.Get("command", string("fileio"));
        if (strInFile != "")
        {
            // It's an I/Q or I/F file
            delete pSoundInInterface;
            CAudioFileIn *pf = new CAudioFileIn;
            pf->SetFileName(strInFile);
            string ext;
            size_t p = str.rfind('.');
            if (p != string::npos)
                ext = str.substr(p + 1);
            if (ext.substr(0, 2) == "iq")
                defaultInChanSel = CReceiveData::CS_IQ_POS_ZERO;
            if (ext.substr(0, 2) == "IQ")
                defaultInChanSel = CReceiveData::CS_IQ_POS_ZERO;
            pSoundInInterface = pf;
        }
        else /* sound card */
        {
            /* Sound In device */
            delete pSoundInInterface;
            pSoundInInterface = new CSoundIn;
            pSoundInInterface->SetDev(s.Get("Receiver", "snddevin", int(0)));
        }
    }

    /* Flip spectrum flag */
    ReceiveData.SetFlippedSpectrum(s.Get("Receiver", "flipspectrum", FALSE));

    CReceiveData::EInChanSel inChanSel = (CReceiveData::EInChanSel)s.Get("command", "inchansel", int(defaultInChanSel));
    ReceiveData.SetInChanSel(inChanSel);

    CWriteData::EOutChanSel outChanSel = (CWriteData::EOutChanSel)s.Get("command", "outchansel", CWriteData::CS_BOTH_BOTH);
    WriteData.SetOutChanSel(outChanSel);

    /* AM Parameters */

    /* AGC */
    AMDemodulation.SetAGCType((CAGC::EType)s.Get("AM Demodulation", "agc", 0));

    /* noise reduction */
    AMDemodulation.SetNoiRedType((CAMDemodulation::ENoiRedType)s.Get("AM Demodulation", "noisered", 0));

    /* pll enabled/disabled */
    AMDemodulation.EnablePLL(s.Get("AM Demodulation", "enablepll", 0));

    /* auto frequency acquisition */
    AMDemodulation.EnableAutoFreqAcq(s.Get("AM Demodulation", "autofreqacq", 0));

    /* demodulation */
    CAMDemodulation::EDemodType DemodType
    = (CAMDemodulation::EDemodType)s.Get("AM Demodulation", "demodulation", CAMDemodulation::DT_AM);

    AMDemodulation.SetDemodType(DemodType);

    iBwAM = s.Get("AM Demodulation", "filterbwam", 10000);
    iBwUSB = s.Get("AM Demodulation", "filterbwusb", 5000);
    iBwLSB = s.Get("AM Demodulation", "filterbwlsb", 5000);
    iBwCW = s.Get("AM Demodulation", "filterbwcw", 150);
    iBwFM = s.Get("AM Demodulation", "filterbwfm", 6000);

    /* Load user's saved filter bandwidth for the demodulation type. */
    switch (DemodType)
    {
    case CAMDemodulation::DT_AM:
        AMDemodulation.SetFilterBW(iBwAM);
        break;

    case CAMDemodulation::DT_LSB:
        AMDemodulation.SetFilterBW(iBwLSB);
        break;

    case CAMDemodulation::DT_USB:
        AMDemodulation.SetFilterBW(iBwUSB);
        break;

    case CAMDemodulation::DT_CW:
        AMDemodulation.SetFilterBW(iBwCW);
        break;

    case CAMDemodulation::DT_FM:
        AMDemodulation.SetFilterBW(iBwFM);
        break;
    }

    /* Sound Out device */
    pSoundOutInterface->SetDev(s.Get("Receiver", "snddevout", int(0)));

    str = s.Get("command", "rciout");
    if (str != "")
        upstreamRSCI.SetDestination(str);

    /* downstream RSCI */
    for (int i = 0; i<MAX_NUM_RSI_SUBSCRIBERS; i++)
    {
        stringstream ss;
        ss << "rsiout" << i;
        str = s.Get("command", ss.str());
        ss.str("");
        ss << "rsioutprofile" << i;
        string profile = s.Get("command", ss.str(), string("A"));
        ss.str("");
        ss << "rciin" << i;
        string origin = s.Get("command", ss.str());
        if (str == "")
        {
	    // allow control without status
	    if(origin != "")
		    downstreamRSCI.AddSubscriber("", origin, ' ');
        }
        else
        {
            downstreamRSCI.AddSubscriber(str, origin, profile[0]);
        }
    }
    /* RSCI File Recording */
    str = s.Get("command", "rsirecordprofile");
    string s2 = s.Get("command", "rsirecordtype");
    if (str != "" || s2 != "")
        downstreamRSCI.SetRSIRecording(*pReceiverParam, TRUE, str[0], s2);

    /* IQ File Recording */
    if (s.Get("command", "recordiq", false))
        WriteIQFile.StartRecording(*pReceiverParam);

    /* Mute audio flag */
    WriteData.MuteAudio(s.Get("Receiver", "muteaudio", FALSE));

    /* Output to File */
    str = s.Get("command", "writewav");
    if (str != "")
        WriteData.StartWriteWaveFile(str);

    /* Reverberation flag */
    AudioSourceDecoder.SetReverbEffect(s.Get("Receiver", "reverb", TRUE));

    /* Bandpass filter flag */
    FreqSyncAcq.SetRecFilter(s.Get("Receiver", "filter", FALSE));

    /* Set parameters for frequency acquisition search window if needed */
    _REAL rFreqAcSeWinSize = s.Get("command", "fracwinsize", _REAL(SOUNDCRD_SAMPLE_RATE / 2));
    _REAL rFreqAcSeWinCenter = s.Get("command", "fracwincent", _REAL(SOUNDCRD_SAMPLE_RATE / 4));
    /* Set new parameters */
    FreqSyncAcq.SetSearchWindow(rFreqAcSeWinCenter, rFreqAcSeWinSize);

    /* Modified metrics flag */
    ChannelEstimation.SetIntCons(s.Get("Receiver", "modmetric", FALSE));

    /* Number of iterations for MLC setting */
    MSCMLCDecoder.SetNumIterations(s.Get("Receiver", "mlciter", 0));

    SetReceiverMode(ERecMode(s.Get("Receiver", "mode", int(0))));

    /* Tuned Frequency */
    pReceiverParam->SetFrequency(s.Get("Receiver", "frequency", 0));

    /* Front-end - combine into Hamlib? */
    CFrontEndParameters& FrontEndParameters = pReceiverParam->FrontEndParameters;

    FrontEndParameters.eSMeterCorrectionType =
        CFrontEndParameters::ESMeterCorrectionType(s.Get("FrontEnd", "smetercorrectiontype", 0));

    FrontEndParameters.rSMeterBandwidth = s.Get("FrontEnd", "smeterbandwidth", 0.0);

    FrontEndParameters.rDefaultMeasurementBandwidth = s.Get("FrontEnd", "defaultmeasurementbandwidth", 0);

    FrontEndParameters.bAutoMeasurementBandwidth = s.Get("FrontEnd", "automeasurementbandwidth", TRUE);

    FrontEndParameters.rCalFactorDRM = s.Get("FrontEnd", "calfactordrm", 0.0);

    FrontEndParameters.rCalFactorAM = s.Get("FrontEnd", "calfactoram", 0.0);

    FrontEndParameters.rIFCentreFreq = s.Get("FrontEnd", "ifcentrefrequency", SOUNDCRD_SAMPLE_RATE / 4);

    /* Latitude string (used to be just for log file) */
    double latitude, longitude;
    latitude = s.Get("GPS", "latitude", s.Get("Logfile", "latitude", 1000.0));
    /* Longitude string */
    longitude = s.Get("GPS", "longitude", s.Get("Logfile", "longitude", 1000.0));
    pReceiverParam->Lock();
    if(-90.0 <= latitude && latitude <= 90.0 && -180.0 <= longitude  && longitude <= 180.0)
    {
        pReceiverParam->gps_data.set = LATLON_SET;
        pReceiverParam->gps_data.fix.latitude = latitude;
        pReceiverParam->gps_data.fix.longitude = longitude;
    }
    else {
        pReceiverParam->gps_data.set = 0;
    }
    bool use_gpsd = s.Get("GPS", "usegpsd", false);
    pReceiverParam->use_gpsd=use_gpsd;
    string host = s.Get("GPS", "host", string("localhost"));
    pReceiverParam->gps_host = host;
    pReceiverParam->gps_port = s.Get("GPS", "port", string("2947"));
    if(use_gpsd)
        pReceiverParam->restart_gpsd=true;
    pReceiverParam->Unlock();
}

void
CDRMReceiver::SaveSettings(CSettings& s)
{

    s.Put("Receiver", "mode", int(eReceiverMode));

    /* Receiver ------------------------------------------------------------- */

    /* if 0 then only measure PSD when RSCI in use otherwise always measure it */
    s.Put("Receiver", "measurepsdalways", pReceiverParam->bMeasurePSDAlways);

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

    /* Sound In device */
    s.Put("Receiver", "snddevin", pSoundInInterface->GetDev());

    /* Sound Out device */
    s.Put("Receiver", "snddevout", pSoundOutInterface->GetDev());

    /* Number of iterations for MLC setting */
    s.Put("Receiver", "mlciter", MSCMLCDecoder.GetInitNumIterations());

    /* Tuned Frequency */
    s.Put("Receiver", "frequency", pReceiverParam->GetFrequency());

    /* AM Parameters */

    /* AGC */
    s.Put("AM Demodulation", "agc", AMDemodulation.GetAGCType());

    /* noise reduction */
    s.Put("AM Demodulation", "noisered", AMDemodulation.GetNoiRedType());

    /* pll enabled/disabled */
    s.Put("AM Demodulation", "enablepll", AMDemodulation.PLLEnabled());

    /* auto frequency acquisition */
    s.Put("AM Demodulation", "autofreqacq", AMDemodulation.AutoFreqAcqEnabled());

    /* demodulation */
    s.Put("AM Demodulation", "demodulation", AMDemodulation.GetDemodType());

    s.Put("AM Demodulation", "filterbwam", iBwAM);
    s.Put("AM Demodulation", "filterbwusb", iBwUSB);
    s.Put("AM Demodulation", "filterbwlsb", iBwLSB);
    s.Put("AM Demodulation", "filterbwcw", iBwCW);
    s.Put("AM Demodulation", "filterbwfm", iBwFM);

    /* Front-end - combine into Hamlib? */
    s.Put("FrontEnd", "smetercorrectiontype", int(pReceiverParam->FrontEndParameters.eSMeterCorrectionType));

    s.Put("FrontEnd", "smeterbandwidth", int(pReceiverParam->FrontEndParameters.rSMeterBandwidth));

    s.Put("FrontEnd", "defaultmeasurementbandwidth", int(pReceiverParam->FrontEndParameters.rDefaultMeasurementBandwidth));

    s.Put("FrontEnd", "automeasurementbandwidth", pReceiverParam->FrontEndParameters.bAutoMeasurementBandwidth);

    s.Put("FrontEnd", "calfactordrm", int(pReceiverParam->FrontEndParameters.rCalFactorDRM));

    s.Put("FrontEnd", "calfactoram", int(pReceiverParam->FrontEndParameters.rCalFactorAM));

    s.Put("FrontEnd", "ifcentrefrequency", int(pReceiverParam->FrontEndParameters.rIFCentreFreq));

    /* Serial Number */
    s.Put("Receiver", "serialnumber", pReceiverParam->sSerialNumber);

    s.Put("Receiver", "datafilesdirectory", pReceiverParam->sDataFilesDirectory);

    /* GPS */
    if(pReceiverParam->gps_data.set & LATLON_SET) {
	s.Put("GPS", "latitude", pReceiverParam->gps_data.fix.latitude);
	s.Put("GPS", "longitude", pReceiverParam->gps_data.fix.longitude);
    }
    s.Put("GPS", "usegpsd", pReceiverParam->use_gpsd);
    s.Put("GPS", "host", pReceiverParam->gps_host);
    s.Put("GPS", "port", pReceiverParam->gps_port);
}

void CConvertAudio::InitInternal(CParameter& Parameter)
{
    (void)Parameter;
    iInputBlockSize = Parameter.CellMappingTable.iSymbolBlockSize;
    iOutputBlockSize = 2*iInputBlockSize;
    iMaxOutputBlockSize = 2 * int((_REAL) SOUNDCRD_SAMPLE_RATE * (_REAL) 0.4 /* 400 ms */);
}

void CConvertAudio::ProcessDataInternal(CParameter& Parameter)
{
    (void)Parameter;
    for (int i = 0; i < this->iInputBlockSize; i++)
    {
        (*this->pvecOutputData)[2*i] = _SAMPLE((*this->pvecInputData)[i]);
        (*this->pvecOutputData)[2*i+1] = _SAMPLE((*this->pvecInputData)[i]);
    }
}
