/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Original Author(s):
 *	Volker Fischer
 *
 * Qwt 5-6 conversion Author(s):
 *  David Flamand
 *
 * Description:
 *	Custom settings of the qwt-plot, Support Qwt version 5.0.0 to 6.1.0(+)
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

#include "DRMPlot.h"
#include <QTreeWidget>
#include "receivercontroller.h"

/* Define the plot color profiles */
/* BLUEWHITE */
#define BLUEWHITE_MAIN_PEN_COLOR_PLOT			Qt::blue
#define BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION	Qt::blue
#define BLUEWHITE_BCKGRD_COLOR_PLOT				Qt::white
#define BLUEWHITE_MAIN_GRID_COLOR_PLOT			Qt::gray
#define BLUEWHITE_SPEC_LINE1_COLOR_PLOT			Qt::red
#define BLUEWHITE_SPEC_LINE2_COLOR_PLOT			Qt::black
#define BLUEWHITE_PASS_BAND_COLOR_PLOT			QColor(192, 192, 255)

/* GREENBLACK */
#define GREENBLACK_MAIN_PEN_COLOR_PLOT			Qt::green
#define GREENBLACK_MAIN_PEN_COLOR_CONSTELLATION	Qt::red
#define GREENBLACK_BCKGRD_COLOR_PLOT			Qt::black
#define GREENBLACK_MAIN_GRID_COLOR_PLOT			QColor(128, 0, 0)
#define GREENBLACK_SPEC_LINE1_COLOR_PLOT		Qt::yellow
#define GREENBLACK_SPEC_LINE2_COLOR_PLOT		Qt::blue
#define GREENBLACK_PASS_BAND_COLOR_PLOT			QColor(0, 96, 0)

/* BLACKGREY */
#define BLACKGREY_MAIN_PEN_COLOR_PLOT			Qt::black
#define BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION	Qt::green
#define BLACKGREY_BCKGRD_COLOR_PLOT				Qt::gray
#define BLACKGREY_MAIN_GRID_COLOR_PLOT			Qt::white
#define BLACKGREY_SPEC_LINE1_COLOR_PLOT			Qt::blue
#define BLACKGREY_SPEC_LINE2_COLOR_PLOT			Qt::yellow
#define BLACKGREY_PASS_BAND_COLOR_PLOT			QColor(128, 128, 128)


void CDRMPlot::SetupChart(const ECharType eNewType)
{
    if(eNewType == CurCharType)
        return;

    clearPlots();

    switch (eNewType)
    {
    case AVERAGED_IR:
        setupBasicPlot("Channel Impulse Response","Time [ms]", "Input IR [dB]", "Input IR", 0.0, 1.0, -20.0, 45.0);
        addxMarker(SpecLine1ColorPlot, 1.0);
        addxMarker(SpecLine1ColorPlot, 2.0);
        addxMarker(SpecLine2ColorPlot, 3.0);
        addxMarker(SpecLine2ColorPlot, 4.0);
        addyMarker(SpecLine1ColorPlot, 5.0);
        break;

    case TRANSFERFUNCTION:
        setupBasicPlot("Channel Transfer Function / Group Delay","Carrier Index", "TF [dB]", "Transf. Fct.", 0.0, 1.0, -85.0, -35.0);
        add2ndGraph("Group Delay [ms]", "Group Del.", -50.0, 50.0);
        break;

    case POWER_SPEC_DENSITY:
        setupBasicPlot("Shifted Power Spectral Density of Input Signal","Frequency [kHz]","Input Spectrum [dB]","Input Spectrum", 0.0, 24.0, -120.0, 1.0);
        addxMarker(SpecLine1ColorPlot, 1.0);
        break;

    case SNR_SPECTRUM:
        setupBasicPlot("SNR Spectrum (Weighted MER on MSC Cells)","Carrier Index","WMER [dB]","SNR Spectrum", 0.0, 1.0, 20.0, 25.0);
        break;

    case INPUTSPECTRUM_NO_AV:
        setupBasicPlot("Input Spectrum","Frequency [kHz]","Input Spectrum [dB]","Input Spectrum", 0.0, 24.0, -120.0, 1.0);
        addxMarker(SpecLine1ColorPlot, 1.0);
        break;

    case INP_SPEC_WATERF:
        setupWaterfall();
        break;

    case INPUT_SIG_PSD:
        setupBasicPlot("Input PSD","Frequency [kHz]", "Input PSD [dB]", "Input PSD", 0.0, 24.0, -120.0, 0.0);
        addxMarker(SpecLine1ColorPlot, 1.0);
        break;

    case INPUT_SIG_PSD_ANALOG:
        break;

    case AUDIO_SPECTRUM:
        setupBasicPlot("Audio Spectrum", "Frequency [kHz]", "AS [dB]", "Audio Spectrum", 0.0, 24.0, -100.0, -20.0);
        break;

    case FREQ_SAM_OFFS_HIST:
        setupBasicPlot("Rel. Frequency Offset / Sample Rate Offset History",
                       "Time [ms]", "Freq. Offset [Hz]", "Freq", 0.0, 4.0, -2.0, 0.0);
        add2ndGraph("Sample Rate Offset [Hz]", "Samp.", 0.0, 1.0);
        break;

    case DOPPLER_DELAY_HIST:
        setupBasicPlot("Delay / Doppler History",
                       "Time [min]", "Delay [ms]", "Delay", 0.0, 1.0, 0.0, 10.0);
        add2ndGraph("Doppler [Hz]", "Doppler", 0.0, 4.0);
        break;

    case SNR_AUDIO_HIST:
        setupBasicPlot("SNR / Correctly Decoded Audio History",
                       "Time [min]", "SNR [dB[", "SNR", 0.0, 1.0, 0.0, 1.0);
        add2ndGraph("Corr. Dec. Audio / DRM-Frame", "Audio", 0.0, 1.0);
        break;

    case FAC_CONSTELLATION:
        setupConstPlot("FAC Constellation", CS_2_SM);
        addConstellation("FAC", 0);
        break;

    case SDC_CONSTELLATION:
        setupConstPlot("SDC Constellation", CS_2_SM);
        addConstellation("SDC", 1);
        break;

    case MSC_CONSTELLATION:
        setupConstPlot("MSC Constellation", CS_3_SM);
        addConstellation("MSC", 2);
        break;

    case ALL_CONSTELLATION:
        setupConstPlot("FAC / SDC / MSC Constellation", CS_3_SM);
        addConstellation("FAC", 0);
        addConstellation("SDC", 1);
        addConstellation("MSC", 2);
        break;

    case NONE_OLD:
        break;
    }
    CurCharType = eNewType;
    replot();
}

void CDRMPlot::update(ReceiverController* rc)
{
    CVector<_COMPLEX> veccData;
    CVector<_REAL> vecrData, vecrData2, vecrScale;
    _REAL  rLowerBound, rHigherBound, rStartGuard, rEndGuard, rPDSBegin, rPDSEnd, rCenterFreq, rBandwidth, rFreqAcquVal;

    CParameter& Parameters = *rc->getReceiver()->GetParameters();

    Parameters.Lock();
    _REAL rDCFrequency = Parameters.GetDCFrequency();
    ECodScheme eSDCCodingScheme = Parameters.eSDCCodingScheme;
    ECodScheme eMSCCodingScheme = Parameters.eMSCCodingScheme;
    bool bAudioDecoder = !Parameters.audiodecoder.empty();
    int iAudSampleRate = Parameters.GetAudSampleRate();
    int iSigSampleRate = Parameters.GetSigSampleRate();
    int iChanMode = (int)rc->getReceiver()->GetReceiveData()->GetInChanSel();
    Parameters.Unlock();

    switch (CurCharType)
    {
    case AVERAGED_IR:
        rc->getReceiver()->GetPlotManager()->GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
            rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);
        setData(0, vecrData, vecrScale, true);
        setxMarker(0, rStartGuard);
        setxMarker(1, rEndGuard);
        setxMarker(2, rPDSBegin);
        setxMarker(3, rPDSEnd);
        setyMarker(0, Max(rLowerBound, rHigherBound));
        break;

    case TRANSFERFUNCTION:
        rc->getReceiver()->GetPlotManager()->GetTransferFunction(vecrData, vecrData2, vecrScale);
        setData(0, vecrData, vecrScale, true);
        setData(1, vecrData2, vecrScale);
        break;

    case POWER_SPEC_DENSITY:
        rc->getReceiver()->GetOFDMDemod()->GetPowDenSpec(vecrData, vecrScale);
        setData(0, vecrData, vecrScale);
        break;

    case SNR_SPECTRUM:
        rc->getReceiver()->GetPlotManager()->GetSNRProfile(vecrData, vecrScale);
        setData(0, vecrData, vecrScale, true);
        break;

    case INPUTSPECTRUM_NO_AV:
        rc->getReceiver()->GetReceiveData()->GetInputSpec(vecrData, vecrScale);
        setxMarker(0, rDCFrequency);
        setData(0, vecrData, vecrScale);
        break;

    case INP_SPEC_WATERF:
        rc->getReceiver()->GetReceiveData()->GetInputSpec(vecrData, vecrScale);
        updateWaterfall(vecrData, vecrScale);
        break;

    case INPUT_SIG_PSD:
        rc->getReceiver()->GetPlotManager()->GetInputPSD(vecrData, vecrScale);
        setxMarker(0, rDCFrequency);
        setData(0, vecrData, vecrScale);
        break;

    case INPUT_SIG_PSD_ANALOG:
        rc->getReceiver()->GetReceiveData()->GetInputPSD(vecrData, vecrScale);
        rc->getReceiver()->GetAMDemod()->GetBWParameters(rCenterFreq, rBandwidth);
        setxMarker(0, rc->getReceiver()->GetAMDemod()->GetCurMixFreqOffs());
        setxMarker(0, rCenterFreq, rBandwidth);
        setData(0, vecrData, vecrScale);
        break;

    case AUDIO_SPECTRUM:
        if(rc->getReceiver()->GetParameters()->audiodecoder.empty())
        {
            vecrData.Init(0); vecrScale.Init(0);
            setData(0, vecrData, vecrScale, true, "No codec");
        }
        else
        {
            rc->getReceiver()->GetWriteData()->GetAudioSpec(vecrData, vecrScale);
            setData(0, vecrData, vecrScale, true, "AS [dB]");
        }
        break;

    case FREQ_SAM_OFFS_HIST:
        rc->getReceiver()->GetPlotManager()->GetFreqSamOffsHist(vecrData, vecrData2, vecrScale, rFreqAcquVal);
        // TODO AutoScale(vecrData, vecrData2, vecrScale);
        setData(0, vecrData, vecrScale, true,
            QString("Freq. Offset [Hz] rel. to %1 Hz").arg(rc->getReceiver()->GetReceiveData()->ConvertFrequency(rFreqAcquVal)));
        setData(1, vecrData2, vecrScale, true);
        break;

    case DOPPLER_DELAY_HIST:
        rc->getReceiver()->GetPlotManager()->GetDopplerDelHist(vecrData, vecrData2, vecrScale);
        setData(0, vecrData, vecrScale, true);
        setData(1, vecrData2, vecrScale);
        break;

    case SNR_AUDIO_HIST:
        rc->getReceiver()->GetPlotManager()->GetSNRHist(vecrData, vecrData2, vecrScale);
        // TODO AutoScale2(vecrData, vecrData2, vecrScale);
        setData(0, vecrData, vecrScale, true);
        setData(1, vecrData2, vecrScale);
        break;

    case FAC_CONSTELLATION:
        rc->getReceiver()->GetFACMLC()->GetVectorSpace(veccData);
        setData(0, veccData);
        break;

    case SDC_CONSTELLATION:
        rc->getReceiver()->GetSDCMLC()->GetVectorSpace(veccData);
        setQAMGrid(rc->getReceiver()->GetParameters()->eSDCCodingScheme);
        setData(0, veccData);
        break;

    case MSC_CONSTELLATION:
        rc->getReceiver()->GetMSCMLC()->GetVectorSpace(veccData);
        setQAMGrid(rc->getReceiver()->GetParameters()->eMSCCodingScheme);
        setData(0, veccData);
        break;

    case ALL_CONSTELLATION:
        rc->getReceiver()->GetFACMLC()->GetVectorSpace(veccData);
        setData(0, veccData);
        rc->getReceiver()->GetSDCMLC()->GetVectorSpace(veccData);
        setData(1, veccData);
        rc->getReceiver()->GetMSCMLC()->GetVectorSpace(veccData);
        setData(2, veccData);
        break;

    case NONE_OLD:
        break;
    }
    replot();
}

void CDRMPlot::setupTreeWidget(QTreeWidget* tw)
{
    /* Set the Char Type of each selectable item */
    QTreeWidgetItemIterator it(tw, QTreeWidgetItemIterator::NoChildren);
    for (; *it; it++)
    {
        if ((*it)->text(0) == QObject::tr("SNR Spectrum"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::SNR_SPECTRUM);
        if ((*it)->text(0) == QObject::tr("Audio Spectrum"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::AUDIO_SPECTRUM);
        if ((*it)->text(0) == QObject::tr("Shifted PSD"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::POWER_SPEC_DENSITY);
        if ((*it)->text(0) == QObject::tr("Waterfall Input Spectrum"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::INP_SPEC_WATERF);
        if ((*it)->text(0) == QObject::tr("Input Spectrum"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::INPUTSPECTRUM_NO_AV);
        if ((*it)->text(0) == QObject::tr("Input PSD"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::INPUT_SIG_PSD);
        if ((*it)->text(0) == QObject::tr("MSC"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::MSC_CONSTELLATION);
        if ((*it)->text(0) == QObject::tr("SDC"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::SDC_CONSTELLATION);
        if ((*it)->text(0) == QObject::tr("FAC"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::FAC_CONSTELLATION);
        if ((*it)->text(0) == QObject::tr("FAC / SDC / MSC"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::ALL_CONSTELLATION);
        if ((*it)->text(0) == QObject::tr("Frequency / Sample Rate"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::FREQ_SAM_OFFS_HIST);
        if ((*it)->text(0) == QObject::tr("Delay / Doppler"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::DOPPLER_DELAY_HIST);
        if ((*it)->text(0) == QObject::tr("SNR / Audio"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::SNR_AUDIO_HIST);
        if ((*it)->text(0) == QObject::tr("Transfer Function"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::TRANSFERFUNCTION);
        if ((*it)->text(0) == QObject::tr("Impulse Response"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::AVERAGED_IR);
    }

    /* Expand all items */
    tw->expandAll();
}

void CDRMPlot::SetPlotStyle(const int iNewStyleID)
{
    switch (iNewStyleID)
    {
    case 1:
        MainPenColorPlot = GREENBLACK_MAIN_PEN_COLOR_PLOT;
        MainPenColorConst = GREENBLACK_MAIN_PEN_COLOR_CONSTELLATION;
        BckgrdColorPlot = GREENBLACK_BCKGRD_COLOR_PLOT;
        MainGridColorPlot = GREENBLACK_MAIN_GRID_COLOR_PLOT;
        SpecLine1ColorPlot = GREENBLACK_SPEC_LINE1_COLOR_PLOT;
        SpecLine2ColorPlot = GREENBLACK_SPEC_LINE2_COLOR_PLOT;
        PassBandColorPlot = GREENBLACK_PASS_BAND_COLOR_PLOT;
        break;

    case 2:
        MainPenColorPlot = BLACKGREY_MAIN_PEN_COLOR_PLOT;
        MainPenColorConst = BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION;
        BckgrdColorPlot = BLACKGREY_BCKGRD_COLOR_PLOT;
        MainGridColorPlot = BLACKGREY_MAIN_GRID_COLOR_PLOT;
        SpecLine1ColorPlot = BLACKGREY_SPEC_LINE1_COLOR_PLOT;
        SpecLine2ColorPlot = BLACKGREY_SPEC_LINE2_COLOR_PLOT;
        PassBandColorPlot = BLACKGREY_PASS_BAND_COLOR_PLOT;
        break;

    case 0: /* 0 is default */
    default:
        MainPenColorPlot = BLUEWHITE_MAIN_PEN_COLOR_PLOT;
        MainPenColorConst = BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION;
        BckgrdColorPlot = BLUEWHITE_BCKGRD_COLOR_PLOT;
        MainGridColorPlot = BLUEWHITE_MAIN_GRID_COLOR_PLOT;
        SpecLine1ColorPlot = BLUEWHITE_SPEC_LINE1_COLOR_PLOT;
        SpecLine2ColorPlot = BLUEWHITE_SPEC_LINE2_COLOR_PLOT;
        PassBandColorPlot = BLUEWHITE_PASS_BAND_COLOR_PLOT;
        break;
    }
    applyColors();
}
