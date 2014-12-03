/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Original Author(s):
 *	Volker Fischer
 *
 * Refactored by Julian Cable
 *
 * Description:
 *	Base class for plots
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
#include <QResizeEvent>
#include "receivercontroller.h"
#ifdef WITH_QCUSTOMPLOT
# include "cdrmplotqcp.h"
#else
# include "cdrmplotqwt.h"
#endif

/* Define the plot color profiles */
/* BLUEWHITE */
#define BLUEWHITE_MAIN_PEN_COLOR_PLOT			Qt::blue
#define BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION	Qt::blue
#define BLUEWHITE_BCKGRD_COLOR_PLOT				Qt::white
#define BLUEWHITE_MAIN_GRID_COLOR_PLOT			Qt::gray
#define BLUEWHITE_SPEC_LINE1_COLOR_PLOT			Qt::red
#define BLUEWHITE_SPEC_LINE2_COLOR_PLOT			Qt::black
#define BLUEWHITE_PASS_BAND_COLOR_PLOT			QColor(192, 192, 255, 125)

/* GREENBLACK */
#define GREENBLACK_MAIN_PEN_COLOR_PLOT			Qt::green
#define GREENBLACK_MAIN_PEN_COLOR_CONSTELLATION	Qt::red
#define GREENBLACK_BCKGRD_COLOR_PLOT			Qt::black
#define GREENBLACK_MAIN_GRID_COLOR_PLOT			QColor(128, 0, 0)
#define GREENBLACK_SPEC_LINE1_COLOR_PLOT		Qt::yellow
#define GREENBLACK_SPEC_LINE2_COLOR_PLOT		Qt::blue
#define GREENBLACK_PASS_BAND_COLOR_PLOT			QColor(0, 96, 0, 125)

/* BLACKGREY */
#define BLACKGREY_MAIN_PEN_COLOR_PLOT			Qt::black
#define BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION	Qt::green
#define BLACKGREY_BCKGRD_COLOR_PLOT				Qt::gray
#define BLACKGREY_MAIN_GRID_COLOR_PLOT			Qt::white
#define BLACKGREY_SPEC_LINE1_COLOR_PLOT			Qt::blue
#define BLACKGREY_SPEC_LINE2_COLOR_PLOT			Qt::yellow
#define BLACKGREY_PASS_BAND_COLOR_PLOT			QColor(128, 128, 128, 125)

CDRMPlot::CDRMPlot(QWidget* parent):QObject(parent),CurCharType(NONE_OLD)
{
#ifdef WITH_QCUSTOMPLOT
    CDRMPlotQCP* p = new CDRMPlotQCP(parent);
    connect(p, SIGNAL(plotClicked(double)), this, SLOT(on_plotClicked(double)));
    plot = p;
#else
    CDRMPlotQwt* p = new CDRMPlotQwt(parent);
    connect(p, SIGNAL(plotClicked(double)), this, SLOT(on_plotClicked(double)));
    plot = p;
#endif
    SetPlotStyle(0);
}

void CDRMPlot::on_plotClicked(double d)
{
    double df = 1000.0 * d; // kHz to Hz
    emit plotClicked(df);
}

CDRMPlot::~CDRMPlot()
{

}

void CDRMPlot::SetupChart(const ECharType eNewType, int sampleRate)
{
    if(eNewType == CurCharType)
        return;

    double sr = double(sampleRate) / 2000.0;

    plot->clearPlots();

    switch (eNewType)
    {
    case AVERAGED_IR:
        plot->setupBasicPlot("Channel Impulse Response","Time [ms]", "Input IR [dB]", "Input IR",
                             -3.0, 8.0, -20.0, 45.0, MainPenColorPlot, BckgrdColorPlot);
        // left bound of guard interval
        plot->addxMarker(SpecLine1ColorPlot, 1.0);
        // right bound of guard interval
        plot->addxMarker(SpecLine1ColorPlot, 2.0);
        // Estimated beginning of impulse response
        plot->addxMarker(SpecLine2ColorPlot, 3.0);
        // Estimated end of impulse response
        plot->addxMarker(SpecLine2ColorPlot, 4.0);
        /* the peak detection bound from timing tracking */
        plot->addyMarker(SpecLine1ColorPlot, 30.0);
        break;

    case TRANSFERFUNCTION:
        plot->setupBasicPlot("Channel Transfer Function / Group Delay","Carrier Index", "TF [dB]", "Transf. Fct.",
                             0.0, 200.0,
                       MIN_VAL_SHIF_PSD_Y_AXIS_DB, MAX_VAL_SHIF_PSD_Y_AXIS_DB,
                              MainPenColorPlot, BckgrdColorPlot);
        plot->add2ndGraph("Group Delay [ms]", "Group Del.", -50.0, 50.0, SpecLine1ColorPlot);
        break;

    case POWER_SPEC_DENSITY:
        plot->setupBasicPlot("Shifted Power Spectral Density of Input Signal","Frequency [kHz]","PSD [dB]","Input Spectrum", 0.0, sr,
                       MIN_VAL_SHIF_PSD_Y_AXIS_DB, MAX_VAL_SHIF_PSD_Y_AXIS_DB, MainPenColorPlot, BckgrdColorPlot);
        plot->addxMarker(SpecLine1ColorPlot, 1.0);
        break;

    case SNR_SPECTRUM:
        plot->setupBasicPlot("SNR Spectrum (Weighted MER on MSC Cells)","Carrier Index","WMER [dB]","SNR Spectrum",0.0, 10.0, 20.0, 25.0,  MainPenColorPlot, BckgrdColorPlot);
        plot->setAutoScalePolicy(Plot::left, Plot::enlarge, 0.0);
        plot->setAutoScalePolicy(Plot::bottom, Plot::last, 0.0);
        break;

    case INPUTSPECTRUM_NO_AV:
        plot->setupBasicPlot("Input Spectrum","Frequency [kHz]","Input Spectrum [dB]","Input Spectrum", 0.0, sr,
                       MIN_VAL_INP_SPEC_Y_AXIS_DB, 1.0,  MainPenColorPlot, BckgrdColorPlot);
        plot->addxMarker(SpecLine1ColorPlot, 1.0);
        break;

    case INP_SPEC_WATERF:
        plot->setupWaterfall(sr);
        break;

    case INPUT_SIG_PSD:
        plot->setupBasicPlot("Input PSD","Frequency [kHz]", "Input PSD [dB]", "Input PSD", 0.0, sr,
                       MIN_VAL_INP_SPEC_Y_AXIS_DB, MAX_VAL_INP_SPEC_Y_AXIS_DB,  MainPenColorPlot, BckgrdColorPlot);
        plot->addxMarker(SpecLine1ColorPlot, 1.0);
        break;

    case INPUT_SIG_PSD_ANALOG:
        plot->setupBasicPlot("Input PSD","Frequency [kHz]", "Input PSD [dB]", "Input PSD", 0.0, sr,
                       MIN_VAL_INP_SPEC_Y_AXIS_DB, MAX_VAL_INP_SPEC_Y_AXIS_DB,  MainPenColorPlot, BckgrdColorPlot);
        plot->addxMarker(SpecLine1ColorPlot, 1.0);
        plot->addBwMarker(PassBandColorPlot);
        break;

    case AUDIO_SPECTRUM:
        plot->setupBasicPlot("Audio Spectrum", "Frequency [kHz]", "AS [dB]", "Audio Spectrum",
                             0.0, 20.0, -100.0, -20.0, MainPenColorPlot, BckgrdColorPlot);
        break;

    case FREQ_SAM_OFFS_HIST:
        plot->setupBasicPlot("Rel. Frequency Offset / Sample Rate Offset History",
                       "Time [ms]", "Freq. Offset [Hz]", "Freq", 0.0, 4.0, -2.0, 0.0, MainPenColorPlot, BckgrdColorPlot);
        plot->add2ndGraph("Sample Rate Offset [Hz]", "Samp.", 0.0, 1.0, SpecLine1ColorPlot);
        plot->setAutoScalePolicy(Plot::left, Plot::min, 1.0);
        plot->setAutoScalePolicy(Plot::right, Plot::min, 1.0);
        plot->setAutoScalePolicy(Plot::bottom, Plot::first, 0.0);
        break;

    case DOPPLER_DELAY_HIST:
        plot->setupBasicPlot("Delay / Doppler History",
                       "Time [min]", "Delay [ms]", "Delay", -15.0, 0.0, 0.0, 10.0, MainPenColorPlot, BckgrdColorPlot);
        plot->add2ndGraph("Doppler [Hz]", "Doppler", 0.0, 4.0, SpecLine1ColorPlot);
        break;

    case SNR_AUDIO_HIST:
        plot->setupBasicPlot("SNR / Correctly Decoded Audio History",
                       "Time [min]", "SNR [dB[", "SNR", -15.0, 0.0, 0.0, 1.0, MainPenColorPlot, BckgrdColorPlot);
        plot->add2ndGraph("Corr. Dec. Audio / DRM-Frame", "Audio", 0.0, 1.0, SpecLine1ColorPlot);
        /* Ratio between the maximum values for audio and SNR. The ratio should be
           chosen so that the audio curve is not in the same range as the SNR curve
           under "normal" conditions to increase readability of curves.
           Since at very low SNRs, no audio can received anyway so we do not have to
           check whether the audio y-scale is in range of the curve */
        plot->setAutoScalePolicy(Plot::left, Plot::fit, 5.0);
        plot->setAutoScalePolicy(Plot::right, Plot::fit, 7.5); // TODO
        plot->setAutoScalePolicy(Plot::bottom, Plot::first, 0.0);
        break;

    case FAC_CONSTELLATION:
        plot->setupConstPlot("FAC Constellation");
        plot->setQAMGrid(2.0 / sqrt(2.0), 1, 4); /* QAM4 */
        plot->addConstellation("FAC", 0);
        break;

    case SDC_CONSTELLATION:
        plot->setupConstPlot("SDC Constellation");
        plot->setQAMGrid(4.0 / sqrt(10.0), 2, 4); /* QAM16 */
        plot->addConstellation("SDC", 1);
        break;

    case MSC_CONSTELLATION:
        plot->setupConstPlot("MSC Constellation");
        plot->setQAMGrid(8.0 / sqrt(42.0), 4, 4); /* QAM64 */
        plot->addConstellation("MSC", 2);
        break;

    case ALL_CONSTELLATION:
        plot->setupConstPlot("FAC / SDC / MSC Constellation");
        plot->setQAMGrid(8.0 / sqrt(42.0), 4, 4); /* QAM64 */
        plot->addConstellation("FAC", 0);
        plot->addConstellation("SDC", 1);
        plot->addConstellation("MSC", 2);
        break;

    case NONE_OLD:
        break;
    }
    CurCharType = eNewType;
    addWhatsThisHelp();
    plot->replot();
}

void CDRMPlot::update(ReceiverController* rc)
{
    CVector<_COMPLEX> veccData;
    CVector<_REAL> vecrData, vecrData2, vecrScale;
    _REAL  rLowerBound, rHigherBound, rStartGuard, rEndGuard, rPDSBegin, rPDSEnd, rCenterFreq, rBandwidth, rFreqAcquVal;

    CParameter& Parameters = *rc->getReceiver()->GetParameters();

    Parameters.Lock();
    _REAL rDCFrequency = Parameters.GetDCFrequency();
    iSigSampleRate = Parameters.GetSigSampleRate();
    Parameters.Unlock();

    switch (CurCharType)
    {
    case AVERAGED_IR:
        rc->getReceiver()->GetPlotManager()->GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
            rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);
        plot->setData(0, vecrData, vecrScale);
        plot->setxMarker(0, rStartGuard);
        plot->setxMarker(1, rEndGuard);
        plot->setxMarker(2, rPDSBegin);
        plot->setxMarker(3, rPDSEnd);
        plot->setyMarker(0, Max(rLowerBound, rHigherBound));
        break;

    case TRANSFERFUNCTION:
        rc->getReceiver()->GetPlotManager()->GetTransferFunction(vecrData, vecrData2, vecrScale);
        plot->setData(0, vecrData, vecrScale);
        plot->setData(1, vecrData2, vecrScale);
        break;

    case POWER_SPEC_DENSITY:
        rc->getReceiver()->GetOFDMDemod()->GetPowDenSpec(vecrData, vecrScale);
        plot->setData(0, vecrData, vecrScale);
        break;

    case SNR_SPECTRUM:
        rc->getReceiver()->GetPlotManager()->GetSNRProfile(vecrData, vecrScale);
        plot->setData(0, vecrData, vecrScale);
        break;

    case INPUTSPECTRUM_NO_AV:
        rc->getReceiver()->GetReceiveData()->GetInputSpec(vecrData, vecrScale);
        plot->setxMarker(0, rDCFrequency);
        plot->setData(0, vecrData, vecrScale);
        break;

    case INP_SPEC_WATERF:
        rc->getReceiver()->GetReceiveData()->GetInputSpec(vecrData, vecrScale);
        plot->updateWaterfall(vecrData, vecrScale);
        break;

    case INPUT_SIG_PSD:
        rc->getReceiver()->GetPlotManager()->GetInputPSD(vecrData, vecrScale);
        plot->setxMarker(0, rDCFrequency);
        plot->setData(0, vecrData, vecrScale);
        break;

    case INPUT_SIG_PSD_ANALOG:
        rc->getReceiver()->GetReceiveData()->GetInputPSD(vecrData, vecrScale);
        rc->getReceiver()->GetAMDemod()->GetBWParameters(rCenterFreq, rBandwidth);
        {
            double f1 = rc->getReceiver()->GetAMDemod()->GetCurMixFreqOffs();
            double f2 = rc->getReceiver()->GetReceiveData()->ConvertFrequency(f1) / 1000.0;
            plot->setxMarker(0, f2);
        }
        {
            double rCenterFreq, rBandwidth;
            double c, b;
            rc->getReceiver()->GetAMDemod()->GetBWParameters(rCenterFreq, rBandwidth);
            int sampleRate = rc->getReceiver()->GetParameters()->GetSigSampleRate();
            c = rc->getReceiver()->GetReceiveData()->ConvertFrequency(rCenterFreq * sampleRate) / 1000.0;
            b = rc->getReceiver()->GetReceiveData()->ConvertFrequency(rBandwidth * sampleRate) / 1000.0;
            plot->setBwMarker(0, c, b);
        }
        plot->setData(0, vecrData, vecrScale);
        break;

    case AUDIO_SPECTRUM:
        if(rc->getReceiver()->GetParameters()->audiodecoder.empty())
        {
            vecrData.Init(0); vecrScale.Init(0);
            plot->setData(0, vecrData, vecrScale, "No codec");
        }
        else
        {
            rc->getReceiver()->GetWriteData()->GetAudioSpec(vecrData, vecrScale);
            plot->setData(0, vecrData, vecrScale, "AS [dB]");
        }
        break;

    case FREQ_SAM_OFFS_HIST:
        rc->getReceiver()->GetPlotManager()->GetFreqSamOffsHist(vecrData, vecrData2, vecrScale, rFreqAcquVal);
        // TODO AutoScale(vecrData, vecrData2, vecrScale);
        plot->setData(0, vecrData, vecrScale,
            QString("Freq. Offset [Hz] rel. to %1 Hz").arg(rc->getReceiver()->GetReceiveData()->ConvertFrequency(rFreqAcquVal)));
        plot->setData(1, vecrData2, vecrScale);
        break;

    case DOPPLER_DELAY_HIST:
        rc->getReceiver()->GetPlotManager()->GetDopplerDelHist(vecrData, vecrData2, vecrScale);
        plot->setData(0, vecrData, vecrScale);
        plot->setData(1, vecrData2, vecrScale);
        break;

    case SNR_AUDIO_HIST:
        rc->getReceiver()->GetPlotManager()->GetSNRHist(vecrData, vecrData2, vecrScale);
        // TODO AutoScale2(vecrData, vecrData2, vecrScale);
        plot->setData(0, vecrData, vecrScale);
        plot->setData(1, vecrData2, vecrScale);
        break;

    case FAC_CONSTELLATION:
        rc->getReceiver()->GetFACMLC()->GetVectorSpace(veccData);
        plot->setData(0, veccData);
        break;

    case SDC_CONSTELLATION:
        switch (rc->getReceiver()->GetParameters()->eSDCCodingScheme)
        {
        case CS_1_SM:
            plot->setQAMGrid(2.0 / sqrt(2.0), 1, 4); /* QAM4 */
            break;

        case CS_2_SM:
            plot->setQAMGrid(4.0 / sqrt(10.0), 2, 4); /* QAM16 */
            break;

        default:
            plot->setQAMGrid(8.0 / sqrt(42.0), 4, 4); /* QAM64 */
            break;
        }
        rc->getReceiver()->GetSDCMLC()->GetVectorSpace(veccData);
        plot->setData(0, veccData);
        break;

    case MSC_CONSTELLATION:
        switch (rc->getReceiver()->GetParameters()->eMSCCodingScheme)
        {
        case CS_1_SM:
            plot->setQAMGrid(2.0 / sqrt(2.0), 1, 4); /* QAM4 */
            break;

        case CS_2_SM:
            plot->setQAMGrid(4.0 / sqrt(10.0), 2, 4); /* QAM16 */
            break;

        default:
            plot->setQAMGrid(8.0 / sqrt(42.0), 4, 4); /* QAM64 */
            break;
        }
        rc->getReceiver()->GetMSCMLC()->GetVectorSpace(veccData);
        plot->setData(0, veccData);
        break;

    case ALL_CONSTELLATION:
        rc->getReceiver()->GetFACMLC()->GetVectorSpace(veccData);
        plot->setData(0, veccData);
        rc->getReceiver()->GetSDCMLC()->GetVectorSpace(veccData);
        plot->setData(1, veccData);
        rc->getReceiver()->GetMSCMLC()->GetVectorSpace(veccData);
        plot->setData(2, veccData);
        break;

    case NONE_OLD:
        break;
    }
    plot->replot();
}

void CDRMPlot::setupTreeWidget(QTreeWidget* tw)
{
    /* Set the Char Type of each selectable item */
    QTreeWidgetItemIterator it(tw, QTreeWidgetItemIterator::NoChildren);
    for (; *it; it++)
    {
        if ((*it)->text(0) == QObject::QObject::tr("SNR Spectrum"))
            (*it)->setData(0,  Qt::UserRole, SNR_SPECTRUM);
        if ((*it)->text(0) == QObject::QObject::tr("Audio Spectrum"))
            (*it)->setData(0,  Qt::UserRole, AUDIO_SPECTRUM);
        if ((*it)->text(0) == QObject::QObject::tr("Shifted PSD"))
            (*it)->setData(0,  Qt::UserRole, POWER_SPEC_DENSITY);
        if ((*it)->text(0) == QObject::QObject::tr("Waterfall Input Spectrum"))
            (*it)->setData(0,  Qt::UserRole, INP_SPEC_WATERF);
        if ((*it)->text(0) == QObject::QObject::tr("Input Spectrum"))
            (*it)->setData(0,  Qt::UserRole, INPUTSPECTRUM_NO_AV);
        if ((*it)->text(0) == QObject::QObject::tr("Input PSD"))
            (*it)->setData(0,  Qt::UserRole, INPUT_SIG_PSD);
        if ((*it)->text(0) == QObject::QObject::tr("MSC"))
            (*it)->setData(0,  Qt::UserRole, MSC_CONSTELLATION);
        if ((*it)->text(0) == QObject::QObject::tr("SDC"))
            (*it)->setData(0,  Qt::UserRole, SDC_CONSTELLATION);
        if ((*it)->text(0) == QObject::QObject::tr("FAC"))
            (*it)->setData(0,  Qt::UserRole, FAC_CONSTELLATION);
        if ((*it)->text(0) == QObject::QObject::tr("FAC / SDC / MSC"))
            (*it)->setData(0,  Qt::UserRole, ALL_CONSTELLATION);
        if ((*it)->text(0) == QObject::QObject::tr("Frequency / Sample Rate"))
            (*it)->setData(0,  Qt::UserRole, FREQ_SAM_OFFS_HIST);
        if ((*it)->text(0) == QObject::QObject::tr("Delay / Doppler"))
            (*it)->setData(0,  Qt::UserRole, DOPPLER_DELAY_HIST);
        if ((*it)->text(0) == QObject::QObject::tr("SNR / Audio"))
            (*it)->setData(0,  Qt::UserRole, SNR_AUDIO_HIST);
        if ((*it)->text(0) == QObject::QObject::tr("Transfer Function"))
            (*it)->setData(0,  Qt::UserRole, TRANSFERFUNCTION);
        if ((*it)->text(0) == QObject::QObject::tr("Impulse Response"))
            (*it)->setData(0,  Qt::UserRole, AVERAGED_IR);
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
        BckgrdColorPlot = GREENBLACK_BCKGRD_COLOR_PLOT;
        MainGridColorPlot = GREENBLACK_MAIN_GRID_COLOR_PLOT;
        SpecLine1ColorPlot = GREENBLACK_SPEC_LINE1_COLOR_PLOT;
        SpecLine2ColorPlot = GREENBLACK_SPEC_LINE2_COLOR_PLOT;
        PassBandColorPlot = GREENBLACK_PASS_BAND_COLOR_PLOT;
        break;

    case 2:
        MainPenColorPlot = BLACKGREY_MAIN_PEN_COLOR_PLOT;
        BckgrdColorPlot = BLACKGREY_BCKGRD_COLOR_PLOT;
        MainGridColorPlot = BLACKGREY_MAIN_GRID_COLOR_PLOT;
        SpecLine1ColorPlot = BLACKGREY_SPEC_LINE1_COLOR_PLOT;
        SpecLine2ColorPlot = BLACKGREY_SPEC_LINE2_COLOR_PLOT;
        PassBandColorPlot = BLACKGREY_PASS_BAND_COLOR_PLOT;
        break;

    case 0: /* 0 is default */
    default:
        MainPenColorPlot = BLUEWHITE_MAIN_PEN_COLOR_PLOT;
        BckgrdColorPlot = BLUEWHITE_BCKGRD_COLOR_PLOT;
        MainGridColorPlot = BLUEWHITE_MAIN_GRID_COLOR_PLOT;
        SpecLine1ColorPlot = BLUEWHITE_SPEC_LINE1_COLOR_PLOT;
        SpecLine2ColorPlot = BLUEWHITE_SPEC_LINE2_COLOR_PLOT;
        PassBandColorPlot = BLUEWHITE_PASS_BAND_COLOR_PLOT;
        break;
    }
    plot->applyColors(MainPenColorPlot, BckgrdColorPlot);
}

void CDRMPlot::addWhatsThisHelp()
{
    QString strCurPlotHelp;

    switch (CurCharType)
    {
    case AVERAGED_IR:
        /* Impulse Response */
        strCurPlotHelp =
            QObject::tr("<b>Impulse Response:</b> This plot shows "
               "the estimated Impulse Response (IR) of the channel based on the "
               "channel estimation. It is the averaged, Hamming Window weighted "
               "Fourier back transformation of the transfer function. The length "
               "of PDS estimation and time synchronization tracking is based on "
               "this function. The two red dashed vertical lines show the "
               "beginning and the end of the guard-interval. The two black dashed "
               "vertical lines show the estimated beginning and end of the PDS of "
               "the channel (derived from the averaged impulse response "
               "estimation). If the \"First Peak\" timing tracking method is "
               "chosen, a bound for peak estimation (horizontal dashed red line) "
               "is shown. Only peaks above this bound are used for timing "
               "estimation.");
        break;

    case TRANSFERFUNCTION:
        /* Transfer Function */
        strCurPlotHelp =
            QObject::tr("<b>Transfer Function / Group Delay:</b> "
               "This plot shows the squared magnitude and the group delay of "
               "the estimated channel at each sub-carrier.");
        break;

    case FAC_CONSTELLATION:
    case SDC_CONSTELLATION:
    case MSC_CONSTELLATION:
    case ALL_CONSTELLATION:
        /* Constellations */
        strCurPlotHelp =
            QObject::tr("<b>FAC, SDC, MSC:</b> The plots show the "
               "constellations of the FAC, SDC and MSC logical channel of the DRM "
               "stream. Depending on the current transmitter settings, the SDC "
               "and MSC can have 4-QAM, 16-QAM or 64-QAM modulation.");
        break;

    case POWER_SPEC_DENSITY:
        /* Shifted PSD */
        strCurPlotHelp =
            QObject::tr("<b>Shifted PSD:</b> This plot shows the "
               "estimated Power Spectrum Density (PSD) of the input signal. The "
               "DC frequency (red dashed vertical line) is fixed at 6 kHz. If "
               "the frequency offset acquisition was successful, the rectangular "
               "DRM spectrum should show up with a center frequency of 6 kHz. "
               "This plot represents the frequency synchronized OFDM spectrum. "
               "If the frequency synchronization was successful, the useful "
               "signal really shows up only inside the actual DRM bandwidth "
               "since the side loops have in this case only energy between the "
               "samples in the frequency domain. On the sample positions outside "
               "the actual DRM spectrum, the DRM signal has zero crossings "
               "because of the orthogonality. Therefore this spectrum represents "
               "NOT the actual spectrum but the \"idealized\" OFDM spectrum.");
            break;

    case SNR_SPECTRUM:
        /* SNR Spectrum (Weighted MER on MSC Cells) */
        strCurPlotHelp =
            QObject::tr("<b>SNR Spectrum (Weighted MER on MSC Cells):</b> "
               "This plot shows the Weighted MER on MSC cells for each carrier "
               "separately.");
            break;

    case INPUTSPECTRUM_NO_AV:
        /* Input Spectrum */
        strCurPlotHelp =
        QObject::tr("<b>Input Spectrum:</b> This plot shows the "
           "Fast Fourier Transformation (FFT) of the input signal. This plot "
           "is active in both modes, analog and digital. There is no "
           "averaging applied. The screen shot of the Evaluation Dialog shows "
           "the significant shape of a DRM signal (almost rectangular). The "
           "dashed vertical line shows the estimated DC frequency. This line "
           "is very important for the analog AM demodulation. Each time a "
           "new carrier frequency is acquired, the red line shows the "
           "selected AM spectrum. If more than one AM spectrums are within "
           "the sound card frequency range, the strongest signal is chosen.");
        break;

    case INPUT_SIG_PSD:
    case INPUT_SIG_PSD_ANALOG:
        /* Input PSD */
        strCurPlotHelp =
            QObject::tr("<b>Input PSD:</b> This plot shows the "
               "estimated power spectral density (PSD) of the input signal. The "
               "PSD is estimated by averaging some Hamming Window weighted "
               "Fourier transformed blocks of the input signal samples. The "
               "dashed vertical line shows the estimated DC frequency.");
        break;

    case AUDIO_SPECTRUM:
        /* Audio Spectrum */
        strCurPlotHelp =
            QObject::tr("<b>Audio Spectrum:</b> This plot shows the "
               "averaged audio spectrum of the currently played audio. With this "
               "plot the actual audio bandwidth can easily determined. Since a "
               "linear scale is used for the frequency axis, most of the energy "
               "of the signal is usually concentrated on the far left side of the "
               "spectrum.");
        break;

    case FREQ_SAM_OFFS_HIST:
        /* Frequency Offset / Sample Rate Offset History */
        strCurPlotHelp =
            QObject::tr("<b>Frequency Offset / Sample Rate Offset History:"
               "</b> The history "
               "of the values for frequency offset and sample rate offset "
               "estimation is shown. If the frequency offset drift is very small, "
               "this is an indication that the analog front end is of high "
               "quality.");
        break;

    case DOPPLER_DELAY_HIST:
        /* Doppler / Delay History */
        strCurPlotHelp =
            QObject::tr("<b>Doppler / Delay History:</b> "
               "The history of the values for the "
               "Doppler and Impulse response length is shown. Large Doppler "
               "values might be responsable for audio drop-outs.");
        break;

    case SNR_AUDIO_HIST:
        /* SNR History */
        strCurPlotHelp =
            QObject::tr("<b>SNR History:</b> "
               "The history of the values for the "
               "SNR and correctly decoded audio blocks is shown. The maximum "
               "achievable number of correctly decoded audio blocks per DRM "
               "frame is 10 or 5 depending on the audio sample rate (24 kHz "
               "or 12 kHz AAC core bandwidth).");
        break;

    case INP_SPEC_WATERF:
        /* Waterfall Display of Input Spectrum */
        strCurPlotHelp =
            QObject::tr("<b>Waterfall Display of Input Spectrum:</b> "
               "The input spectrum is displayed as a waterfall type. The "
               "different colors represent different levels.");
        break;

    case NONE_OLD:
        break;
    }

    plot->setWhatsThis(strCurPlotHelp);
}
