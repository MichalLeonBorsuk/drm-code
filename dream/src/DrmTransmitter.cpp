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
#include <sstream>

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
    /* Set flag to request stop */
    TransmParam.eRunState = CParameter::STOP_REQUESTED;
}

void CDRMTransmitter::Run()
{
    /*
    	The hand over of data is done via an intermediate-buffer. The calling
    	convention is always "input-buffer, output-buffer". Additional, the
    	DRM-parameters are fed to the function
    */
    for (;;)
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


        /* Soft stop **********************************************************/
        if (CanSoftStopExit())
            break;

        /* Transmit the signal ************************************************/
        TransmitData.WriteData(TransmParam, OFDMModBuf);
    }

    /* Closing the sound interfaces */
    if (pSoundInInterface) pSoundInInterface->Close();
    if (pSoundOutInterface) pSoundOutInterface->Close();

    /* Set flag to stopped */
    TransmParam.eRunState = CParameter::STOPPED;
}

#if 1
/* Flavour 1: Stop at the frame boundary (worst case delay one frame) */
_BOOLEAN CDRMTransmitter::CanSoftStopExit()
{
    /* Set new symbol flag */
    const _BOOLEAN bNewSymbol = OFDMModBuf.GetFillLevel() != 0;

    if (bNewSymbol)
    {
        /* Number of symbol by frame */
        const int iSymbolPerFrame = TransmParam.CellMappingTable.iNumSymPerFrame;

        /* Set stop requested flag */
        const _BOOLEAN bStopRequested = TransmParam.eRunState != CParameter::RUNNING;

        /* The soft stop is always started at the beginning of a new frame */
        if ((bStopRequested && iSoftStopSymbolCount == 0) || iSoftStopSymbolCount < 0)
        {
            /* Data in OFDM buffer are set to zero */
            OFDMModBuf.QueryWriteBuffer()->Reset(_COMPLEX());

            /* The zeroing will continue until the frame end */
            if (--iSoftStopSymbolCount < -iSymbolPerFrame)
                return TRUE; /* End of frame reached, signal that loop exit must be done */
        }
        else
        {
            /* Update the symbol counter to keep track of frame beginning */
            if (++iSoftStopSymbolCount >= iSymbolPerFrame)
                iSoftStopSymbolCount = 0;
        }
    }
    return FALSE; /* Signal to continue the normal operation */
}
#endif
#if 0
/* Flavour 2: Stop at the symbol boundary (worst case delay two frame) */
_BOOLEAN CDRMTransmitter::CanSoftStopExit()
{
    /* Set new symbol flag */
    const _BOOLEAN bNewSymbol = OFDMModBuf.GetFillLevel() != 0;

    if (bNewSymbol)
    {
        /* Set stop requested flag */
        const _BOOLEAN bStopRequested = TransmParam.eRunState != CParameter::RUNNING;

        /* Check if stop is requested */
        if (bStopRequested || iSoftStopSymbolCount < 0)
        {
            /* Reset the counter if positif */
            if (iSoftStopSymbolCount > 0)
                iSoftStopSymbolCount = 0;

            /* Data in OFDM buffer are set to zero */
            OFDMModBuf.QueryWriteBuffer()->Reset(_COMPLEX());

            /* Zeroing only this symbol, the next symbol will be an exiting one */
            if (--iSoftStopSymbolCount < -1)
            {
                TransmitData.FlushData();
                return TRUE; /* Signal that a loop exit must be done */
            }
        }
        else
        {
            /* Number of symbol by frame */
            const int iSymbolPerFrame = TransmParam.CellMappingTable.iNumSymPerFrame;

            /* Update the symbol counter to keep track of frame beginning */
            if (++iSoftStopSymbolCount >= iSymbolPerFrame)
                iSoftStopSymbolCount = 0;
        }
    }
    return FALSE; /* Signal to continue the normal operation */
}
#endif
#if 0
/* Flavour 3: The original behaviour: stop at the symbol boundary,
   without zeroing any symbol. Cause spreading of the spectrum on the
   entire bandwidth for the last symbol. */
_BOOLEAN CDRMTransmitter::CanSoftStopExit()
{
    return TransmParam.eRunState != CParameter::RUNNING;
}
#endif

void CDRMTransmitter::Init()
{
    /* (Re)Initialization of the buffers */
    CarMapBuf.Clear();
    SDCMapBuf.Clear();
    MLCEncBuf.Clear();
    IntlBuf.Clear();
    GenFACDataBuf.Clear();
    FACMapBuf.Clear();
    GenSDCDataBuf.Clear();
    OFDMModBuf.Clear();
    AudSrcBuf.Clear();
    DataBuf.Clear();

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

    /* Initialize the soft stop */
    InitSoftStop();
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

void CDRMTransmitter::LoadSettings(CSettings& s)
{
    const char *Transmitter = "Transmitter";
    std::ostringstream oss;
    string value, service;

    /* Sound card input device id */
    pSoundInInterface->SetDev(s.Get(Transmitter, "snddevin", int(0)));

    /* Sound card output device id */
    pSoundOutInterface->SetDev(s.Get(Transmitter, "snddevout", int(0)));
#if 0 // TODO
    /* Sound clock drift adjustment */
    _BOOLEAN bEnabled = s.Get(Transmitter, "sndclkadj", int(0));
    ((CSoundOutPulse*)pSoundOutInterface)->EnableClockDriftAdj(bEnabled);
#endif
    /* Robustness mode and spectrum occupancy */
    ERobMode eRobustnessMode = RM_ROBUSTNESS_MODE_B;
    ESpecOcc eSpectOccup = SO_3;
    /* Robustness mode */
    value = s.Get(Transmitter, "robustness", string("RM_ROBUSTNESS_MODE_B"));
    if      (value == "RM_ROBUSTNESS_MODE_A") { eRobustnessMode = RM_ROBUSTNESS_MODE_A; }
    else if (value == "RM_ROBUSTNESS_MODE_B") { eRobustnessMode = RM_ROBUSTNESS_MODE_B; }
    else if (value == "RM_ROBUSTNESS_MODE_C") { eRobustnessMode = RM_ROBUSTNESS_MODE_C; }
    else if (value == "RM_ROBUSTNESS_MODE_D") { eRobustnessMode = RM_ROBUSTNESS_MODE_D; }
    /* Spectrum occupancy */
    value = s.Get(Transmitter, "spectocc", string("SO_3"));
    if      (value == "SO_0") { eSpectOccup = SO_0; }
    else if (value == "SO_1") { eSpectOccup = SO_1; }
    else if (value == "SO_2") { eSpectOccup = SO_2; }
    else if (value == "SO_3") { eSpectOccup = SO_3; }
    else if (value == "SO_4") { eSpectOccup = SO_4; }
    else if (value == "SO_5") { eSpectOccup = SO_5; }
    TransmParam.InitCellMapTable(eRobustnessMode, eSpectOccup);

    /* Protection level for MSC */
    TransmParam.MSCPrLe.iPartB = s.Get(Transmitter, "protlevel", int(1));

    /* Interleaver mode of MSC service */
    value = s.Get(Transmitter, "interleaver", string("SI_LONG"));
    if      (value == "SI_SHORT") { TransmParam.eSymbolInterlMode = CParameter::SI_SHORT; }
    else if (value == "SI_LONG")  { TransmParam.eSymbolInterlMode = CParameter::SI_LONG;  }

    /* MSC modulation scheme */
    value = s.Get(Transmitter, "msc", string("CS_3_SM"));
    if      (value == "CS_2_SM")    { TransmParam.eMSCCodingScheme = CS_2_SM;    }
    else if (value == "CS_3_SM")    { TransmParam.eMSCCodingScheme = CS_3_SM;    }
    else if (value == "CS_3_HMSYM") { TransmParam.eMSCCodingScheme = CS_3_HMSYM; }
    else if (value == "CS_3_HMMIX") { TransmParam.eMSCCodingScheme = CS_3_HMMIX; }

    /* SDC modulation scheme */
    value = s.Get(Transmitter, "sdc", string("CS_2_SM"));
    if      (value == "CS_1_SM") { TransmParam.eSDCCodingScheme = CS_1_SM; }
    else if (value == "CS_2_SM") { TransmParam.eSDCCodingScheme = CS_2_SM; }

    /* IF frequency */
    SetCarOffset(s.Get(Transmitter, "iffreq", double(GetCarOffset())));

    /* IF format */
    value = s.Get(Transmitter, "ifformat", string("OF_REAL_VAL"));
    if      (value == "OF_REAL_VAL") { GetTransData()->SetIQOutput(CTransmitData::OF_REAL_VAL); }
    else if (value == "OF_IQ_POS")   { GetTransData()->SetIQOutput(CTransmitData::OF_IQ_POS);   }
    else if (value == "OF_IQ_NEG")   { GetTransData()->SetIQOutput(CTransmitData::OF_IQ_NEG);   }
    else if (value == "OF_EP")       { GetTransData()->SetIQOutput(CTransmitData::OF_EP);       }
#if 0 // TODO
    /* Transmission of current time */
    value = s.Get(Transmitter, "currenttime", string("CT_OFF"));
    if      (value == "CT_OFF")        { TransmParam.eTransmitCurrentTime = CParameter::CT_OFF;        }
    else if (value == "CT_LOCAL")      { TransmParam.eTransmitCurrentTime = CParameter::CT_LOCAL;      }
    if      (value == "CT_UTC")        { TransmParam.eTransmitCurrentTime = CParameter::CT_UTC;        }
    else if (value == "CT_UTC_OFFSET") { TransmParam.eTransmitCurrentTime = CParameter::CT_UTC_OFFSET; }
#endif
    /**********************/
    /* Service parameters */
    for (int i=0; i<1/*MAX_NUM_SERVICES*/; i++) // TODO
    {
        oss << Transmitter << " Service " << i+1;
        string service = oss.str();

        CService& Service = TransmParam.Service[i];

        /* Service ID */
        Service.iServiceID = s.Get(service, "id", int(Service.iServiceID));

        /* Service label data */
        Service.strLabel = s.Get(service, "label", string(Service.strLabel));

        /* Service description */
        Service.iServiceDescr = s.Get(service, "description", int(Service.iServiceDescr));

        /* Language */
        Service.iLanguage = s.Get(service, "language", int(Service.iLanguage));
#if 0 // TODO
        /* Audio codec */
        value = s.Get(service, "codec", string("faac"));
        if      (value == "faac") { Service.AudioParam.eAudioCoding = CAudioParam::AC_AAC;  }
        else if (value == "Opus") { Service.AudioParam.eAudioCoding = CAudioParam::AC_OPUS; }

        /* Opus Codec Channels */
        value = s.Get(service, "Opus_Channels", string("OC_STEREO"));
        if      (value == "OC_MONO")   { Service.AudioParam.eOPUSChan = CAudioParam::OC_MONO;   }
        else if (value == "OC_STEREO") { Service.AudioParam.eOPUSChan = CAudioParam::OC_STEREO; }

        /* Opus Codec Bandwith */
        value = s.Get(service, "Opus_Bandwith", string("OB_FB"));
        if      (value == "OB_NB")  { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_NB;  }
        else if (value == "OB_MB")  { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_MB;  }
        else if (value == "OB_WB")  { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_WB;  }
        else if (value == "OB_SWB") { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_SWB; }
        else if (value == "OB_FB")  { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_FB;  }

        /* Opus Forward Error Correction */
        value = s.Get(service, "Opus_FEC", string("0"));
        if      (value == "0") { Service.AudioParam.bOPUSForwardErrorCorrection = FALSE; }
        else if (value == "1") { Service.AudioParam.bOPUSForwardErrorCorrection = TRUE;  }

        /* Opus encoder signal type */
        value = s.Get(service, "Opus_Signal", string("OG_MUSIC"));
        if      (value == "OG_VOICE") { Service.AudioParam.eOPUSSignal = CAudioParam::OG_VOICE; }
        else if (value == "OG_MUSIC") { Service.AudioParam.eOPUSSignal = CAudioParam::OG_MUSIC; }

        /* Opus encoder intended application */
        value = s.Get(service, "Opus_Application", string("OA_AUDIO"));
        if      (value == "OA_VOIP")  { Service.AudioParam.eOPUSApplication = CAudioParam::OA_VOIP;  }
        else if (value == "OA_AUDIO") { Service.AudioParam.eOPUSApplication = CAudioParam::OA_AUDIO; }
#endif
    }
}

void CDRMTransmitter::SaveSettings(CSettings& s)
{
    const char *Transmitter = "Transmitter";
    std::ostringstream oss;
    string value, service;

    /* Sound card input device id */
    s.Put(Transmitter, "snddevin", int(pSoundInInterface->GetDev()));

    /* Sound card output device id */
    s.Put(Transmitter, "snddevout", int(pSoundOutInterface->GetDev()));
#if 0 // TODO
    /* Sound clock drift adjustment */
    s.Put(Transmitter, "sndclkadj", int(((CSoundOutPulse*)pSoundOutInterface)->IsClockDriftAdjEnabled()));
#endif
    /* Robustness mode */
    switch (TransmParam.GetWaveMode()) {
    case RM_ROBUSTNESS_MODE_A: value = "RM_ROBUSTNESS_MODE_A"; break;
    case RM_ROBUSTNESS_MODE_B: value = "RM_ROBUSTNESS_MODE_B"; break;
    case RM_ROBUSTNESS_MODE_C: value = "RM_ROBUSTNESS_MODE_C"; break;
    case RM_ROBUSTNESS_MODE_D: value = "RM_ROBUSTNESS_MODE_D"; break;
    default: value = ""; }
    s.Put(Transmitter, "robustness", value);
	
    /* Spectrum occupancy */
    switch (TransmParam.GetSpectrumOccup()) {
    case SO_0: value = "SO_0"; break;
    case SO_1: value = "SO_1"; break;
    case SO_2: value = "SO_2"; break;
    case SO_3: value = "SO_3"; break;
    case SO_4: value = "SO_4"; break;
    case SO_5: value = "SO_5"; break;
    default: value = ""; }
    s.Put(Transmitter, "spectocc", value);

    /* Protection level for MSC */
    s.Put(Transmitter, "protlevel", int(TransmParam.MSCPrLe.iPartB));

    /* Interleaver mode of MSC service */
    switch (TransmParam.eSymbolInterlMode) {
    case CParameter::SI_SHORT: value = "SI_SHORT"; break;
    case CParameter::SI_LONG:  value = "SI_LONG";  break;
    default: value = ""; }
    s.Put(Transmitter, "interleaver", value);

    /* MSC modulation scheme */
    switch (TransmParam.eMSCCodingScheme) {
    case CS_2_SM:    value = "CS_2_SM";    break;
    case CS_3_SM:    value = "CS_3_SM";    break;
    case CS_3_HMSYM: value = "CS_3_HMSYM"; break;
    case CS_3_HMMIX: value = "CS_3_HMMIX"; break;
    default: value = ""; }
    s.Put(Transmitter, "msc", value);

    /* SDC modulation scheme */
    switch (TransmParam.eSDCCodingScheme) {
    case CS_1_SM: value = "CS_1_SM"; break;
    case CS_2_SM: value = "CS_2_SM"; break;
    default: value = ""; }
    s.Put(Transmitter, "sdc", value);

    /* IF frequency */
    s.Put(Transmitter, "iffreq", double(GetCarOffset()));

    /* IF format */
    switch (GetTransData()->GetIQOutput()) {
    case CTransmitData::OF_REAL_VAL: value = "OF_REAL_VAL"; break;
    case CTransmitData::OF_IQ_POS:   value = "OF_IQ_POS";   break;
    case CTransmitData::OF_IQ_NEG:   value = "OF_IQ_NEG";   break;
    case CTransmitData::OF_EP:       value = "OF_EP";       break;
    default: value = ""; }
    s.Put(Transmitter, "ifformat", value);
#if 0 // TODO
    /* Transmission of current time */
    switch (TransmParam.eTransmitCurrentTime) {
    case CParameter::CT_OFF:        value = "CT_OFF";        break;
    case CParameter::CT_LOCAL:      value = "CT_LOCAL";      break;
    case CParameter::CT_UTC:        value = "CT_UTC";        break;
    case CParameter::CT_UTC_OFFSET: value = "CT_UTC_OFFSET"; break;
    default: value = ""; }
    s.Put(Transmitter, "currenttime", value);
#endif
    /**********************/
    /* Service parameters */
    for (int i=0; i<1/*MAX_NUM_SERVICES*/; i++) // TODO
    {
        oss << Transmitter << " Service " << i+1;
        string service = oss.str();

        CService& Service = TransmParam.Service[i];

        /* Service ID */
        s.Put(service, "id", int(Service.iServiceID));

        /* Service label data */
        s.Put(service, "label", string(Service.strLabel));

        /* Service description */
        s.Put(service, "description", int(Service.iServiceDescr));

        /* Language */
        s.Put(service, "language", int(Service.iLanguage));
#if 0 // TODO
        /* Audio codec */
        switch (Service.AudioParam.eAudioCoding) {
        case CAudioParam::AC_AAC:  value = "faac"; break;
        case CAudioParam::AC_OPUS: value = "Opus"; break;
        default: value = ""; }
        s.Put(service, "codec", value);

        /* Opus Codec Channels */
        switch (Service.AudioParam.eOPUSChan) {
        case CAudioParam::OC_MONO:   value = "OC_MONO";   break;
        case CAudioParam::OC_STEREO: value = "OC_STEREO"; break;
        default: value = ""; }
        s.Put(service, "Opus_Channels", value);

        /* Opus Codec Bandwith */
        switch (Service.AudioParam.eOPUSBandwidth) {
        case CAudioParam::OB_NB:  value = "OB_NB";  break;
        case CAudioParam::OB_MB:  value = "OB_MB";  break;
        case CAudioParam::OB_WB:  value = "OB_WB";  break;
        case CAudioParam::OB_SWB: value = "OB_SWB"; break;
        case CAudioParam::OB_FB:  value = "OB_FB";  break;
        default: value = ""; }
        s.Put(service, "Opus_Bandwith", value);

        /* Opus Forward Error Correction */
        switch (Service.AudioParam.bOPUSForwardErrorCorrection) {
        case FALSE: value = "0"; break;
        case TRUE:  value = "1"; break;
        default: value = ""; }
        s.Put(service, "Opus_FEC", value);

        /* Opus encoder signal type */
        switch (Service.AudioParam.eOPUSSignal) {
        case CAudioParam::OG_VOICE: value = "OG_VOICE"; break;
        case CAudioParam::OG_MUSIC: value = "OG_MUSIC"; break;
        default: value = ""; }
        s.Put(service, "Opus_Signal", value);

        /* Opus encoder intended application */
        switch (Service.AudioParam.eOPUSApplication) {
        case CAudioParam::OA_VOIP:  value = "OA_VOIP";  break;
        case CAudioParam::OA_AUDIO: value = "OA_AUDIO"; break;
        default: value = ""; }
        s.Put(service, "Opus_Application", value);
#endif
    }
}
