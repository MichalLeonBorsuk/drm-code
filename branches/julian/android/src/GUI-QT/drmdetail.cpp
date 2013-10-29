#include "drmdetail.h"
#include "ui_drmdetail.h"
#include <../util-QT/Util.h>

DRMDetail::DRMDetail(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DRMDetail)
{
    ui->setupUi(this);
    /* Update times for colour LEDs */
    ui->LEDFAC->SetUpdateTime(1500);
    ui->LEDSDC->SetUpdateTime(1500);
    ui->LEDMSC->SetUpdateTime(600);
    ui->LEDFrameSync->SetUpdateTime(600);
    ui->LEDTimeSync->SetUpdateTime(600);
    ui->LEDIOInterface->SetUpdateTime(2000); /* extra long -> red light stays long */
}

DRMDetail::~DRMDetail()
{
    delete ui;
}

void DRMDetail::updateDisplay(CParameter& Parameters, _REAL freqOffset, EAcqStat acqState, bool rsciMode)
{
    SetStatus(ui->LEDFAC, Parameters.ReceiveStatus.FAC.GetStatus());
    SetStatus(ui->LEDSDC, Parameters.ReceiveStatus.SDC.GetStatus());
    // TODO Data Broadcasts
    int iShortID = Parameters.GetCurSelAudioService();
    SetStatus(ui->LEDMSC, Parameters.AudioComponentStatus[iShortID].GetStatus());
    SetStatus(ui->LEDFrameSync, Parameters.ReceiveStatus.FSync.GetStatus());
    SetStatus(ui->LEDTimeSync, Parameters.ReceiveStatus.TSync.GetStatus());
    ETypeRxStatus soundCardStatusI = Parameters.ReceiveStatus.InterfaceI.GetStatus(); /* Input */
    ETypeRxStatus soundCardStatusO = Parameters.ReceiveStatus.InterfaceO.GetStatus(); /* Output */
    SetStatus(ui->LEDIOInterface, soundCardStatusO == NOT_PRESENT || (soundCardStatusI != NOT_PRESENT && soundCardStatusI != RX_OK) ? soundCardStatusI : soundCardStatusO);

    /* Show SNR if receiver is in tracking mode */
    if (acqState == AS_WITH_SIGNAL)
    {
        /* Get a consistant snapshot */

        /* We only get SNR from a local DREAM Front-End */
        _REAL rSNR = Parameters.GetSNR();
        if (rSNR >= 0.0)
        {
            /* SNR */
            ui->ValueSNR->setText("<b>" +
                              QString().setNum(rSNR, 'f', 1) + " dB</b>");
        }
        else
        {
            ui->ValueSNR->setText("<b>---</b>");
        }
        /* We get MER from a local DREAM Front-End or an RSCI input but not an MDI input */
        _REAL rMER = Parameters.rMER;
        if (rMER >= 0.0 )
        {
            ui->ValueMERWMER->setText(QString().
                                  setNum(Parameters.rWMERMSC, 'f', 1) + " dB / "
                                  + QString().setNum(rMER, 'f', 1) + " dB");
        }
        else
        {
            ui->ValueMERWMER->setText("---");
        }

        /* Doppler estimation (assuming Gaussian doppler spectrum) */
        if (Parameters.rSigmaEstimate >= 0.0)
        {
            /* Plot delay and Doppler values */
            ui->ValueWiener->setText(
                QString().setNum(Parameters.rSigmaEstimate, 'f', 2) + " Hz / "
                + QString().setNum(Parameters.rMinDelay, 'f', 2) + " ms");
        }
        else
        {
            /* Plot only delay, Doppler not available */
            ui->ValueWiener->setText("--- / "
                                 + QString().setNum(Parameters.rMinDelay, 'f', 2) + " ms");
        }

        /* Sample frequency offset estimation */
        const _REAL rCurSamROffs = Parameters.rResampleOffset;

        /* Display value in [Hz] and [ppm] (parts per million) */
        ui->ValueSampFreqOffset->setText(
            QString().setNum(rCurSamROffs, 'f', 2) + " Hz (" +
            QString().setNum((int) (rCurSamROffs / Parameters.GetSigSampleRate() * 1e6))
            + " ppm)");

    }
    else
    {
        ui->ValueSNR->setText("<b>---</b>");
        ui->ValueMERWMER->setText("---");
        ui->ValueWiener->setText("--- / ---");
        ui->ValueSampFreqOffset->setText("---");
    }

#ifdef _DEBUG_
    ui->TextFreqOffset->setText("DC: " +
                            QString().setNum(DRMReceiver.GetReceiveData()->
                                    ConvertFrequency(Parameters.GetDCFrequency()), 'f', 3) + " Hz ");

    /* Metric values */
    ui->ValueFreqOffset->setText(tr("Metrics [dB]: MSC: ") +
                             QString().setNum(
                                 DRMReceiver.GetMSCMLC()->GetAccMetric(), 'f', 2) +	"\nSDC: " +
                             QString().setNum(
                                 DRMReceiver.GetSDCMLC()->GetAccMetric(), 'f', 2) +	" / FAC: " +
                             QString().setNum(
                                 DRMReceiver.GetFACMLC()->GetAccMetric(), 'f', 2));
#else
    /* DC frequency */
    ui->ValueFreqOffset->setText(QString().setNum(freqOffset, 'f', 2) + " Hz");
#endif

    /* _WIN32 fix because in Visual c++ the GUI files are always compiled even
       if QT_GUI_LIB is set or not (problem with MDI in DRMReceiver) */
#ifdef QT_GUI_LIB
    /* If MDI in is enabled, do not show any synchronization parameter */
    if (rsciMode == TRUE)
    {
        ui->ValueSNR->setText("<b>---</b>");
        if (Parameters.vecrRdelThresholds.GetSize() > 0)
            ui->ValueWiener->setText(QString().setNum(Parameters.rRdop, 'f', 2) + " Hz / "
                                 + QString().setNum(Parameters.vecrRdelIntervals[0], 'f', 2) + " ms ("
                                 + QString().setNum(Parameters.vecrRdelThresholds[0]) + "%)");
        else
            ui->ValueWiener->setText(QString().setNum(Parameters.rRdop, 'f', 2) + " Hz / ---");

        ui->ValueSampFreqOffset->setText("---");
        ui->ValueFreqOffset->setText("---");
    }
#endif


    /* FAC info static ------------------------------------------------------ */
    QString strFACInfo;

    /* Robustness mode #################### */
    strFACInfo = GetRobModeStr(Parameters.GetWaveMode()) + " / " + GetSpecOccStr(Parameters.GetSpectrumOccup());

    //FACDRMModeBWL->setText(tr("DRM Mode / Bandwidth:")); /* Label */
    ui->FACDRMModeBWV->setText(strFACInfo); /* Value */


    /* Interleaver Depth #################### */
    switch (Parameters.eSymbolInterlMode)
    {
    case CParameter::SI_LONG:
        strFACInfo = tr("2 s (Long Interleaving)");
        break;

    case CParameter::SI_SHORT:
        strFACInfo = tr("400 ms (Short Interleaving)");
        break;

    default:
        strFACInfo = "?";
    }

    //FACInterleaverDepthL->setText(tr("Interleaver Depth:")); /* Label */
    ui->FACInterleaverDepthV->setText(strFACInfo); /* Value */


    /* SDC, MSC mode #################### */
    /* SDC */
    switch (Parameters.eSDCCodingScheme)
    {
    case CS_1_SM:
        strFACInfo = "4-QAM / ";
        break;

    case CS_2_SM:
        strFACInfo = "16-QAM / ";
        break;

    default:
        strFACInfo = "? / ";
    }

    /* MSC */
    switch (Parameters.eMSCCodingScheme)
    {
    case CS_2_SM:
        strFACInfo += "SM 16-QAM";
        break;

    case CS_3_SM:
        strFACInfo += "SM 64-QAM";
        break;

    case CS_3_HMSYM:
        strFACInfo += "HMsym 64-QAM";
        break;

    case CS_3_HMMIX:
        strFACInfo += "HMmix 64-QAM";
        break;

    default:
        strFACInfo += "?";
    }

    //FACSDCMSCModeL->setText(tr("SDC / MSC Mode:")); /* Label */
    ui->FACSDCMSCModeV->setText(strFACInfo); /* Value */


    /* Code rates #################### */
    strFACInfo = QString().setNum(Parameters.MSCPrLe.iPartB);
    strFACInfo += " / ";
    strFACInfo += QString().setNum(Parameters.MSCPrLe.iPartA);

    //FACCodeRateL->setText(tr("Prot. Level (B / A):")); /* Label */
    ui->FACCodeRateV->setText(strFACInfo); /* Value */


    /* Number of services #################### */
    strFACInfo = tr("Audio: ");
    strFACInfo += QString().setNum(Parameters.iNumAudioService);
    strFACInfo += tr(" / Data: ");
    strFACInfo += QString().setNum(Parameters.iNumDataService);

    //FACNumServicesL->setText(tr("Number of Services:")); /* Label */
    ui->FACNumServicesV->setText(strFACInfo); /* Value */


    /* Time, date #################### */
    if ((Parameters.iUTCHour == 0) &&
            (Parameters.iUTCMin == 0) &&
            (Parameters.iDay == 0) &&
            (Parameters.iMonth == 0) &&
            (Parameters.iYear == 0))
    {
        /* No time service available */
        strFACInfo = tr("Service not available");
    }
    else
    {
#ifdef GUI_QT_DATE_TIME_TYPE
        /* QT type of displaying date and time */
        QDateTime DateTime;
        DateTime.setDate(QDate(Parameters.iYear,
                               Parameters.iMonth,
                               Parameters.iDay));
        DateTime.setTime(QTime(Parameters.iUTCHour,
                               Parameters.iUTCMin));

        strFACInfo = DateTime.toString();
#else
        /* Set time and date */
        QString strMin;
        const int iMin = Parameters.iUTCMin;

        /* Add leading zero to number smaller than 10 */
        if (iMin < 10)
            strMin = "0";
        else
            strMin = "";

        strMin += QString().setNum(iMin);

        strFACInfo =
            /* Time */
            QString().setNum(Parameters.iUTCHour) + ":" +
            strMin + "  -  " +
            /* Date */
            QString().setNum(Parameters.iMonth) + "/" +
            QString().setNum(Parameters.iDay) + "/" +
            QString().setNum(Parameters.iYear);
#endif
        /* Add UTC offset if available */
        if (Parameters.bValidUTCOffsetAndSense)
            strFACInfo += QString(" %1%2%3%4")
                .arg(tr("UTC"))
                .arg(Parameters.iUTCSense ? "-" : "+")
                .arg(Parameters.iUTCOff / 2, 0, 10)
                .arg(Parameters.iUTCOff & 1 ? ".5" : "");
    }

    //FACTimeDateL->setText(tr("Received time - date:")); /* Label */
    ui->FACTimeDateV->setText(strFACInfo); /* Value */
}

QString	DRMDetail::GetRobModeStr(ERobMode e)
{
    switch (e)
    {
    case RM_ROBUSTNESS_MODE_A:
        return "A";
        break;

    case RM_ROBUSTNESS_MODE_B:
        return "B";
        break;

    case RM_ROBUSTNESS_MODE_C:
        return "C";
        break;

    case RM_ROBUSTNESS_MODE_D:
        return "D";
        break;

    default:
        return "?";
    }
}

QString	DRMDetail::GetSpecOccStr(ESpecOcc e)
{
    switch (e)
    {
    case SO_0:
        return "4.5 kHz";
        break;

    case SO_1:
        return "5 kHz";
        break;

    case SO_2:
        return "9 kHz";
        break;

    case SO_3:
        return "10 kHz";
        break;

    case SO_4:
        return "18 kHz";
        break;

    case SO_5:
        return "20 kHz";
        break;

    default:
        return "?";
    }
}

void DRMDetail::AddWhatsThisHelp()
{
    /*
        This text was taken from the only documentation of Dream software
    */
    /* DC Frequency Offset */
    const QString strDCFreqOffs =
        tr("<b>DC Frequency Offset:</b> This is the "
           "estimation of the DC frequency offset. This offset corresponds "
           "to the resulting sound card intermedia frequency of the front-end. "
           "This frequency is not restricted to a certain value. The only "
           "restriction is that the DRM spectrum must be completely inside the "
           "bandwidth of the sound card.");

    ui->TextFreqOffset->setWhatsThis(strDCFreqOffs);
    ui->ValueFreqOffset->setWhatsThis(strDCFreqOffs);

    /* Sample Frequency Offset */
    const QString strFreqOffset =
        tr("<b>Sample Frequency Offset:</b> This is the "
           "estimation of the sample rate offset between the sound card sample "
           "rate of the local computer and the sample rate of the D / A (digital "
           "to analog) converter in the transmitter. Usually the sample rate "
           "offset is very constant for a given sound card. Therefore it is "
           "useful to inform the Dream software about this value at application "
           "startup to increase the acquisition speed and reliability.");

    ui->TextSampFreqOffset->setWhatsThis(strFreqOffset);
    ui->ValueSampFreqOffset->setWhatsThis(strFreqOffset);

    /* Doppler / Delay */
    const QString strDopplerDelay =
        tr("<b>Doppler / Delay:</b> The Doppler frequency "
           "of the channel is estimated for the Wiener filter design of channel "
           "estimation in time direction. If linear interpolation is set for "
           "channel estimation in time direction, this estimation is not updated. "
           "The Doppler frequency is an indication of how fast the channel varies "
           "with time. The higher the frequency, the faster the channel changes "
           "are.<br>The total delay of the Power Delay Spectrum "
           "(PDS) is estimated from the impulse response estimation derived from "
           "the channel estimation. This delay corresponds to the range between "
           "the two vertical dashed black lines in the Impulse Response (IR) "
           "plot.");

    ui->TextWiener->setWhatsThis(strDopplerDelay);
    ui->ValueWiener->setWhatsThis(strDopplerDelay);

    /* I / O Interface LED */
    const QString strLEDIOInterface =
        tr("<b>I / O Interface LED:</b> This LED shows the "
           "current status of the sound card interface. The yellow light shows "
           "that the audio output was corrected. Since the sample rate of the "
           "transmitter and local computer are different, from time to time the "
           "audio buffers will overflow or under run and a correction is "
           "necessary. When a correction occurs, a \"click\" sound can be heard. "
           "The red light shows that a buffer was lost in the sound card input "
           "stream. This can happen if a thread with a higher priority is at "
           "100% and the Dream software cannot read the provided blocks fast "
           "enough. In this case, the Dream software will instantly loose the "
           "synchronization and has to re-synchronize. Another reason for red "
           "light is that the processor is too slow for running the Dream "
           "software.");

    ui->TextLabelLEDIOInterface->setWhatsThis(strLEDIOInterface);
    ui->LEDIOInterface->setWhatsThis(strLEDIOInterface);

    /* Time Sync Acq LED */
    const QString strLEDTimeSyncAcq =
        tr("<b>Time Sync Acq LED:</b> This LED shows the "
           "state of the timing acquisition (search for the beginning of an OFDM "
           "symbol). If the acquisition is done, this LED will stay green.");

    ui->TextLabelLEDTimeSyncAcq->setWhatsThis(strLEDTimeSyncAcq);
    ui->LEDTimeSync->setWhatsThis(strLEDTimeSyncAcq);

    /* Frame Sync LED */
    const QString strLEDFrameSync =
        tr("<b>Frame Sync LED:</b> The DRM frame "
           "synchronization status is shown with this LED. This LED is also only "
           "active during acquisition state of the Dream receiver. In tracking "
           "mode, this LED is always green.");

    ui->TextLabelLEDFrameSync->setWhatsThis(strLEDFrameSync);
    ui->LEDFrameSync->setWhatsThis(strLEDFrameSync);

    /* FAC CRC LED */
    const QString strLEDFACCRC =
        tr("<b>FAC CRC LED:</b> This LED shows the Cyclic "
           "Redundancy Check (CRC) of the Fast Access Channel (FAC) of DRM. FAC "
           "is one of the three logical channels and is always modulated with a "
           "4-QAM. If the FAC CRC check was successful, the receiver changes to "
           "tracking mode. The FAC LED is the indication whether the receiver "
           "is synchronized to a DRM transmission or not.<br>"
           "The bandwidth of the DRM signal, the constellation scheme of MSC and "
           "SDC channels and the interleaver depth are some of the parameters "
           "which are provided by the FAC.");

    ui->TextLabelLEDFACCRC->setWhatsThis(strLEDFACCRC);
    ui->LEDFAC->setWhatsThis(strLEDFACCRC);

    /* SDC CRC LED */
    const QString strLEDSDCCRC =
        tr("<b>SDC CRC LED:</b> This LED shows the CRC "
           "check result of the Service Description Channel (SDC) which is one "
           "logical channel of the DRM stream. This data is transmitted in "
           "approx. 1 second intervals and contains information about station "
           "label, audio and data format, etc. The error protection is normally "
           "lower than the protection of the FAC. Therefore this LED will turn "
           "to red earlier than the FAC LED in general.<br>If the CRC check "
           "is ok but errors in the content were detected, the LED turns "
           "yellow.");

    ui->TextLabelLEDSDCCRC->setWhatsThis(strLEDSDCCRC);
    ui->LEDSDC->setWhatsThis(strLEDSDCCRC);

    /* MSC CRC LED */
    const QString strLEDMSCCRC =
        tr("<b>MSC CRC LED:</b> This LED shows the status "
           "of the Main Service Channel (MSC). This channel contains the actual "
           "audio and data bits. The LED shows the CRC check of the AAC core "
           "decoder. The SBR has a separate CRC, but this status is not shown "
           "with this LED. If SBR CRC is wrong but the AAC CRC is ok one can "
           "still hear something (of course, the high frequencies are not there "
           "in this case). If this LED turns red, interruptions of the audio are "
           "heard. The yellow light shows that only one 40 ms audio frame CRC "
           "was wrong. This causes usually no hearable artifacts.");

    ui->TextLabelLEDMSCCRC->setWhatsThis(strLEDMSCCRC);
    ui->LEDMSC->setWhatsThis(strLEDMSCCRC);

    /* SNR */
    const QString strSNREst =
        tr("<b>SNR:</b> Signal to Noise Ratio (SNR) "
           "estimation based on FAC cells. Since the FAC cells are only "
           "located approximately in the region 0-5 kHz relative to the DRM DC "
           "frequency, it may happen that the SNR value is very high "
           "although the DRM spectrum on the left side of the DRM DC frequency "
           "is heavily distorted or disturbed by an interferer so that the true "
           "overall SNR is lower as indicated by the SNR value. Similarly, "
           "the SNR value might show a very low value but audio can still be "
           "decoded if only the right side of the DRM spectrum is degraded "
           "by an interferer.");

    ui->ValueSNR->setWhatsThis(strSNREst);
    ui->TextSNRText->setWhatsThis(strSNREst);

    /* MSC WMER / MSC MER */
    const QString strMERWMEREst =
        tr("<b>MSC WMER / MSC MER:</b> Modulation Error Ratio (MER) and "
           "weighted MER (WMER) calculated on the MSC cells is shown. The MER is "
           "calculated as follows: For each equalized MSC cell (only MSC cells, "
           "no FAC cells, no SDC cells, no pilot cells), the error vector from "
           "the nearest ideal point of the constellation diagram is measured. The "
           "squared magnitude of this error is found, and a mean of the squared "
           "errors is calculated (over one frame). The MER is the ratio in [dB] "
           "of the mean of the squared magnitudes of the ideal points of the "
           "constellation diagram to the mean squared error. This gives an "
           "estimate of the ratio of the total signal power to total noise "
           "power at the input to the equalizer for channels with flat frequency "
           "response.<br> In case of the WMER, the calculations of the means are "
           "multiplied by the squared magnitude of the estimated channel "
           "response.<br>For more information see ETSI TS 102 349.");

    ui->ValueMERWMER->setWhatsThis(strMERWMEREst);
    ui->TextMERWMER->setWhatsThis(strMERWMEREst);

    /* DRM Mode / Bandwidth */
    const QString strRobustnessMode =
        tr("<b>DRM Mode / Bandwidth:</b> In a DRM system, "
           "four possible robustness modes are defined to adapt the system to "
           "different channel conditions. According to the DRM standard:<ul>"
           "<li><i>Mode A:</i> Gaussian channels, with "
           "minor fading</li><li><i>Mode B:</i> Time "
           "and frequency selective channels, with longer delay spread</li>"
           "<li><i>Mode C:</i> As robustness mode B, but "
           "with higher Doppler spread</li>"
           "<li><i>Mode D:</i> As robustness mode B, but "
           "with severe delay and Doppler spread</li></ul>The "
           "bandwith is the gross bandwidth of the current DRM signal");

    ui->FACDRMModeBWL->setWhatsThis(strRobustnessMode);
    ui->FACDRMModeBWV->setWhatsThis(strRobustnessMode);

    /* Interleaver Depth */
    const QString strInterleaver =
        tr("<b>Interleaver Depth:</b> The symbol "
           "interleaver depth can be either short (approx. 400 ms) or long "
           "(approx. 2 s). The longer the interleaver the better the channel "
           "decoder can correct errors from slow fading signals. But the "
           "longer the interleaver length the longer the delay until (after a "
           "re-synchronization) audio can be heard.");

    ui->FACInterleaverDepthL->setWhatsThis(strInterleaver);
    ui->FACInterleaverDepthV->setWhatsThis(strInterleaver);

    /* SDC / MSC Mode */
    const QString strSDCMSCMode =
        tr("<b>SDC / MSC Mode:</b> Shows the modulation "
           "type of the SDC and MSC channel. For the MSC channel, some "
           "hierarchical modes are defined which can provide a very strong "
           "protected service channel.");

    ui->FACSDCMSCModeL->setWhatsThis(strSDCMSCMode);
    ui->FACSDCMSCModeV->setWhatsThis(strSDCMSCMode);

    /* Prot. Level (B/A) */
    const QString strProtLevel =
        tr("<b>Prot. Level (B/A):</b> The error protection "
           "level of the channel coder. For 64-QAM, there are four protection "
           "levels defined in the DRM standard. Protection level 0 has the "
           "highest protection whereas level 3 has the lowest protection. The "
           "letters A and B are the names of the higher and lower protected parts "
           "of a DRM block when Unequal Error Protection (UEP) is used. If Equal "
           "Error Protection (EEP) is used, only the protection level of part B "
           "is valid.");

    ui->FACCodeRateL->setWhatsThis(strProtLevel);
    ui->FACCodeRateV->setWhatsThis(strProtLevel);

    /* Number of Services */
    const QString strNumServices =
        tr("<b>Number of Services:</b> This shows the "
           "number of audio and data services transmitted in the DRM stream. "
           "The maximum number of streams is four.");

    ui->FACNumServicesL->setWhatsThis(strNumServices);
    ui->FACNumServicesV->setWhatsThis(strNumServices);

    /* Received time - date */
    const QString strTimeDate =
        tr("<b>Received time - date:</b> This label shows "
           "the received time and date in UTC. This information is carried in "
           "the SDC channel.");

    ui->FACTimeDateL->setWhatsThis(strTimeDate);
    ui->FACTimeDateV->setWhatsThis(strTimeDate);

}
