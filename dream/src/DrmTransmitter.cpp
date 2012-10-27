/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM-transmitter
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

#include "DrmTransmitter.h"
#include "sound.h"

/* Implementation *************************************************************/
void CDRMTransmitter::Start()
{
    /* Set run flag */
    TransmParam.eRunState = CParameter::RUNNING;

    /* Initialization of the modules */
    Init();

    /* Start the transmitter run routine */
    Run();
}

void CDRMTransmitter::Stop()
{
    TransmParam.eRunState = CParameter::STOP_REQUESTED;

    if (pSoundInInterface) pSoundInInterface->Close();
    if (pSoundOutInterface) pSoundOutInterface->Close();
}

void CDRMTransmitter::Run()
{
    /*
    	The hand over of data is done via an intermediate-buffer. The calling
    	convention is always "input-buffer, output-buffer". Additional, the
    	DRM-parameters are fed to the function
    */
    while (TransmParam.eRunState == CParameter::RUNNING)
    {
        /* MSC ****************************************************************/
        /* Read the source signal */
        ReadData.ReadData(TransmParam, DataBuf);

        /* Audio source encoder */
        AudioSourceEncoder.ProcessData(TransmParam, DataBuf, AudSrcBuf);

        /* MLC-encoder */
        MSCMLCEncoder.ProcessData(TransmParam, AudSrcBuf, MLCEncBuf);

        /* Convolutional interleaver */
        SymbInterleaver.ProcessData(TransmParam, MLCEncBuf, IntlBuf);


        /* FAC ****************************************************************/
        GenerateFACData.ReadData(TransmParam, GenFACDataBuf);
        FACMLCEncoder.ProcessData(TransmParam, GenFACDataBuf, FACMapBuf);


        /* SDC ****************************************************************/
        GenerateSDCData.ReadData(TransmParam, GenSDCDataBuf);
        SDCMLCEncoder.ProcessData(TransmParam, GenSDCDataBuf, SDCMapBuf);


        /* Mapping of the MSC, FAC, SDC and pilots on the carriers ************/
        OFDMCellMapping.ProcessData(TransmParam, IntlBuf, FACMapBuf, SDCMapBuf,
                                    CarMapBuf);


        /* OFDM-modulation ****************************************************/
        OFDMModulation.ProcessData(TransmParam, CarMapBuf, OFDMModBuf);


        /* Transmit the signal ************************************************/
        TransmitData.WriteData(TransmParam, OFDMModBuf);
    }
    TransmParam.eRunState = CParameter::STOPPED;
}

void CDRMTransmitter::Init()
{
    /* Defines number of cells, important! */
    OFDMCellMapping.Init(TransmParam, CarMapBuf);

    /* Defines number of SDC bits per super-frame */
    SDCMLCEncoder.Init(TransmParam, SDCMapBuf);

    MSCMLCEncoder.Init(TransmParam, MLCEncBuf);
    SymbInterleaver.Init(TransmParam, IntlBuf);
    GenerateFACData.Init(TransmParam, GenFACDataBuf);
    FACMLCEncoder.Init(TransmParam, FACMapBuf);
    GenerateSDCData.Init(TransmParam, GenSDCDataBuf);
    OFDMModulation.Init(TransmParam, OFDMModBuf);
    AudioSourceEncoder.Init(TransmParam, AudSrcBuf);
    ReadData.Init(TransmParam, DataBuf);
    TransmitData.Init(TransmParam);
}

CDRMTransmitter::CDRMTransmitter() :
        TransmParam(NULL),
        pSoundInInterface(new CSoundIn), pSoundOutInterface(new CSoundOut),
        ReadData(pSoundInInterface), TransmitData(pSoundOutInterface),
        rDefCarOffset((_REAL) VIRTUAL_INTERMED_FREQ),
        // UEP only works with Dream receiver, FIXME! -> disabled for now
        bUseUEP(FALSE)
{
    /* Init streams */
    TransmParam.ResetServicesStreams();

    /* Init frame ID counter (index) */
    TransmParam.iFrameIDTransm = 0;

    /* Date, time. TODO: use computer system time... */
    TransmParam.iDay = 0;
    TransmParam.iMonth = 0;
    TransmParam.iYear = 0;
    TransmParam.iUTCHour = 0;
    TransmParam.iUTCMin = 0;


    /**************************************************************************/
    /* Robustness mode and spectrum occupancy. Available transmission modes:
       RM_ROBUSTNESS_MODE_A: Gaussian channels, with minor fading,
       RM_ROBUSTNESS_MODE_B: Time and frequency selective channels, with longer
       delay spread,
       RM_ROBUSTNESS_MODE_C: As robustness mode B, but with higher Doppler
       spread,
       RM_ROBUSTNESS_MODE_D: As robustness mode B, but with severe delay and
       Doppler spread.
       Available bandwidths:
       SO_0: 4.5 kHz, SO_1: 5 kHz, SO_2: 9 kHz, SO_3: 10 kHz, SO_4: 18 kHz,
       SO_5: 20 kHz */
    TransmParam.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);

    /* Protection levels for MSC. Depend on the modulation scheme. Look at
       TableMLC.h, iCodRateCombMSC16SM, iCodRateCombMSC64SM,
       iCodRateCombMSC64HMsym, iCodRateCombMSC64HMmix for available numbers */
    TransmParam.MSCPrLe.iPartA = 0;
    TransmParam.MSCPrLe.iPartB = 1;
    TransmParam.MSCPrLe.iHierarch = 0;

    /* Either one audio or one data service can be chosen */
    _BOOLEAN bIsAudio = TRUE;

    CService Service;

    /* In the current version only one service and one stream is supported. The
       stream IDs must be 0 in both cases */
    if (bIsAudio == TRUE)
    {
        /* Audio */
        TransmParam.SetNumOfServices(1,0);
        TransmParam.SetCurSelAudioService(0);

        CAudioParam AudioParam;

        AudioParam.iStreamID = 0;

        /* Text message */
        AudioParam.bTextflag = TRUE;

        TransmParam.SetAudioParam(0, AudioParam);

        TransmParam.SetAudDataFlag(0,  CService::SF_AUDIO);

        /* Programme Type code (see TableFAC.h, "strTableProgTypCod[]") */
        Service.iServiceDescr = 15; /* 15 -> other music */

        TransmParam.SetCurSelAudioService(0);
    }
    else
    {
        /* Data */
        TransmParam.SetNumOfServices(0,1);
        TransmParam.SetCurSelDataService(0);

        TransmParam.SetAudDataFlag(0,  CService::SF_DATA);

        CDataParam DataParam;

        DataParam.iStreamID = 0;

        /* Init SlideShow application */
        DataParam.iPacketLen = 45; /* TEST */
        DataParam.eDataUnitInd = CDataParam::DU_DATA_UNITS;
        DataParam.eAppDomain = CDataParam::AD_DAB_SPEC_APP;
        TransmParam.SetDataParam(0, DataParam);

        /* The value 0 indicates that the application details are provided
           solely by SDC data entity type 5 */
        Service.iServiceDescr = 0;
    }

    /* Init service parameters, 24 bit unsigned integer number */
    Service.iServiceID = 0;

    /* Service label data. Up to 16 bytes defining the label using UTF-8
       coding */
    Service.strLabel = "Dream Test";

    /* Language (see TableFAC.h, "strTableLanguageCode[]") */
    Service.iLanguage = 5; /* 5 -> english */

    TransmParam.SetServiceParameters(0, Service);

    /* Interleaver mode of MSC service. Long interleaving (2 s): SI_LONG,
       short interleaving (400 ms): SI_SHORT */
    TransmParam.eSymbolInterlMode = CParameter::SI_LONG;

    /* MSC modulation scheme. Available modes:
       16-QAM standard mapping (SM): CS_2_SM,
       64-QAM standard mapping (SM): CS_3_SM,
       64-QAM symmetrical hierarchical mapping (HMsym): CS_3_HMSYM,
       64-QAM mixture of the previous two mappings (HMmix): CS_3_HMMIX */
    TransmParam.eMSCCodingScheme = CS_3_SM;

    /* SDC modulation scheme. Available modes:
       4-QAM standard mapping (SM): CS_1_SM,
       16-QAM standard mapping (SM): CS_2_SM */
    TransmParam.eSDCCodingScheme = CS_2_SM;

    /* Set desired intermedia frequency (IF) in Hertz */
    SetCarOffset(12000.0); /* Default: "VIRTUAL_INTERMED_FREQ" */

    if (bUseUEP == TRUE)
    {
        // TEST
        TransmParam.SetStreamLen(0, 80, 0);
    }
    else
    {
        /* Length of part B is set automatically (equal error protection (EEP),
           if "= 0"). Sets the number of bytes, should not exceed total number
           of bytes available in MSC block */
        TransmParam.SetStreamLen(0, 0, 0);
    }
}
