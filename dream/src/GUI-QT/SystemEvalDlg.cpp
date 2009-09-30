/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *
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

#include "SystemEvalDlg.h"
#include "DRMPlot.h"
#include "DialogUtil.h"
#include "../util/Settings.h"
#include "../GPSReceiver.h"
#include <QDateTime>
#include <QWhatsThis>
#include <QMessageBox>
#include <iostream>

SystemEvalDlg::SystemEvalDlg(ReceiverInterface& NDRMR, CSettings& NSettings,
    QWidget* parent, const char*, bool, Qt::WFlags f)
: QDialog(parent, f), Ui_SystemEvalDlg(),
    Receiver(NDRMR), Settings(NSettings), pGPSReceiver(NULL),
    plot(NULL),plots(),
    timer(NULL), timerTuning(NULL), timerLineEditFrequency(NULL),
    bTuningInProgress(false)
{
    setupUi(this);

    timer = new QTimer(this);
    timerTuning = new QTimer(this);
    timerLineEditFrequency = new QTimer(this);

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));
    connect(timer, SIGNAL(timeout()), this, SLOT(OnTimer()));
    connect(CheckBoxMuteAudio, SIGNAL(clicked()), this, SLOT(OnCheckBoxMuteAudio()));
    connect(CheckBoxSaveAudioWave, SIGNAL(clicked()), this, SLOT(OnCheckSaveAudioWav()));
    connect(CheckBoxReverb, SIGNAL(clicked()), this, SLOT(OnCheckBoxReverb()));

    AddWhatsThisHelp();

    InitialiseLEDs();
    InitialiseGPS();
    InitialiseMERetc();
    InitialiseFAC();
    InitialisePlots();
    InitialiseFrequency();
}

SystemEvalDlg::~SystemEvalDlg()
{
	if (pGPSReceiver)
		delete pGPSReceiver;
}

void SystemEvalDlg::showEvent(QShowEvent*)
{
	timer->start(GUI_CONTROL_UPDATE_TIME);
    showPlots();
}

void SystemEvalDlg::hideEvent(QHideEvent*)
{
    hidePlots();
	timer->stop();
}

void SystemEvalDlg::OnTimer()
{
    CParameter& Parameters = *(Receiver.GetParameters());

	Parameters.Lock();
	UpdateLEDs(Parameters);
	UpdateMERetc(Parameters);
	UpdateFAC(Parameters);
	UpdateGPS(Parameters);
	Parameters.Unlock();
	UpdateFrequency();
}

///////////////////////// Status LEDs //////////////////////////////////////

void SystemEvalDlg::InitialiseLEDs()
{
	/* Update times for colour LEDs */
	LEDFAC->SetUpdateTime(1500);
	LEDSDC->SetUpdateTime(1500);
	LEDMSC->SetUpdateTime(600);
	LEDFrameSync->SetUpdateTime(600);
	LEDTimeSync->SetUpdateTime(600);
	LEDIOInterface->SetUpdateTime(2000); /* extra long -> red light stays long */
}

void SystemEvalDlg::UpdateLEDs(CParameter& Parameters)
{
    int iCurSelAudioServ = Parameters.GetCurSelAudioService();
    if(Parameters.Service[iCurSelAudioServ].eAudDataFlag == SF_DATA)
	SetStatus(LEDMSC, Parameters.ReceiveStatus.MOT.GetStatus());
    else
	SetStatus(LEDMSC, Parameters.ReceiveStatus.Audio.GetStatus());
    SetStatus(LEDSDC, Parameters.ReceiveStatus.SDC.GetStatus());
    SetStatus(LEDFAC, Parameters.ReceiveStatus.FAC.GetStatus());
    SetStatus(LEDFrameSync, Parameters.ReceiveStatus.FSync.GetStatus());
    SetStatus(LEDTimeSync, Parameters.ReceiveStatus.TSync.GetStatus());
    SetStatus(LEDIOInterface, Parameters.ReceiveStatus.Interface.GetStatus());
}

void SystemEvalDlg::SetStatus(CMultColorLED* LED, ETypeRxStatus state)
{
	switch(state)
	{
	case NOT_PRESENT:
		LED->Reset(); /* GREY */
		break;

	case CRC_ERROR:
		LED->SetLight(CMultColorLED::RL_RED); /* RED */
		break;

	case DATA_ERROR:
		LED->SetLight(CMultColorLED::RL_YELLOW); /* YELLOW */
		break;

	case RX_OK:
		LED->SetLight(CMultColorLED::RL_GREEN); /* GREEN */
		break;
	}
}

/////////////////////////// GPS /////////////////////////////////////////

void SystemEvalDlg::InitialiseGPS()
{
    bool b = Settings.Get("GPS", "usegpsd", false);
	EnableGPS(b);
	ShowGPS(b);
}

void SystemEvalDlg::EnableGPS(bool b)
{
    CParameter& Parameters = *Receiver.GetParameters();
    Parameters.Lock();
    if(b)
    {
	Parameters.GPSData.SetGPSSource(CGPSData::GPS_SOURCE_GPS_RECEIVER);
	Parameters.Unlock();
	if(pGPSReceiver == NULL)
	{
	    string host = Settings.Get("GPS", "host", string("localhost"));
	    int port = Settings.Get("GPS", "port", 2947);
	    pGPSReceiver = new CGPSReceiver(Parameters, host, port);
	}
    }
    else
    {
	Parameters.GPSData.SetGPSSource(CGPSData::GPS_SOURCE_MANUAL_ENTRY);
	Parameters.Unlock();
	if(pGPSReceiver)
	{
		delete pGPSReceiver;
		pGPSReceiver = NULL;
	}
    }
}

void SystemEvalDlg::UpdateGPS(CParameter& Parameters)
{
	/* display GPS info */

	switch (Parameters.GPSData.GetStatus())
	{
		case CGPSData::GPS_RX_NOT_CONNECTED:
			LEDGPS->SetLight(CMultColorLED::RL_RED); // Red
			break;

		case CGPSData::GPS_RX_NO_DATA:
			LEDGPS->SetLight(CMultColorLED::RL_YELLOW); // Yellow
			break;

		case CGPSData::GPS_RX_DATA_AVAILABLE:
			LEDGPS->SetLight(CMultColorLED::RL_GREEN); // Green
			break;
	}

	if (Parameters.GPSData.GetPositionAvailable())
	{
		double latitude, longitude;
		Parameters.GPSData.GetLatLongDegrees(latitude, longitude);
		GPSLatV->setText(QString("%1\260").arg(latitude, 0, 'f', 6));
		GPSLngV->setText(QString("%1\260").arg(longitude, 0, 'f', 6));
	}
	else
	{
		GPSLatV->setText("?");
		GPSLngV->setText("?");
	}

	//if (Parameters.GPSData.GetAltitudeAvailable())
		//qStrPosition += QString("  Alt: %1 m").arg(Parameters.GPSData.GetAltitudeMetres(), 0, 'f', 0);

	if (Parameters.GPSData.GetSpeedAvailable())
		GPSSpeedV->setText(QString("%1 m/s").arg(Parameters.GPSData.GetSpeedMetresPerSecond(), 0, 'f', 1));
	else
		GPSSpeedV->setText("?");

	if (Parameters.GPSData.GetHeadingAvailable())
		GPSHeadingV->setText(QString("%1\260").arg(Parameters.GPSData.GetHeadingDegrees()));
	else
		GPSHeadingV->setText("?");

	if (Parameters.GPSData.GetTimeAndDateAvailable())
		GPSTimeDateV->setText(Parameters.GPSData.GetTimeDate().c_str());
	else
		GPSTimeDateV->setText("?");

/*
	if (Parameters.GPSData.GetSatellitesVisibleAvailable())
		qStrTime += "Satellites: " + QString().setNum(Parameters.GPSData.GetSatellitesVisible());
	else
		qStrTime += "Satellites: ?";

	TextLabelGPSTime->setText(qStrTime);
*/

}

void SystemEvalDlg::ShowGPS(bool b)
{
	if(b)
		FrameGPS->show();
	else
		FrameGPS->hide();
}


//////////////////////////////// MER, etc. ////////////////////////////////////////

void SystemEvalDlg::InitialiseMERetc()
{
}

void SystemEvalDlg::UpdateMERetc(CParameter& Parameters)
{
    _REAL rSigStr=0.0;

	bool bValid = Parameters.Measurements.SigStrstat.getCurrent(rSigStr);
    if (bValid)
	ValueRF->setText(QString().setNum(rSigStr, 'f', 1) + " dBuV");
    else
	ValueRF->setText("---");

    /* Show SNR if receiver is in tracking mode */
    if (Receiver.GetAcquiState() == AS_WITH_SIGNAL)
    {
	/* Get a consistant snapshot */

	/* We only get SNR from a local DREAM Front-End */
	_REAL rSNR = Parameters.GetSNR();
	if (rSNR >= 0.0)
	{
	    /* SNR */
	    ValueSNR->setText("<b>" + QString().setNum(rSNR, 'f', 1) + " dB</b>");
	}
	else
	{
	    ValueSNR->setText("<b>---</b>");
	}

	/* We get MER from a local DREAM Front-End or an RSCI input but not an MDI input */
	_REAL rMER=0.0, rWMERMSC=0.0;
	QString sMER="---", sWMERMSC="---";
	if (Parameters.Measurements.WMERMSC.get(rWMERMSC))
	{
	    sWMERMSC = QString().setNum(rWMERMSC, 'f', 1) + " dB";
	}
	if (Parameters.Measurements.MER.get(rMER))
	{
	    sMER = QString().setNum(rMER, 'f', 1) + " dB";
	}
	ValueMERWMER->setText("<b>"+sWMERMSC+" / "+sMER+"</b>");

	/* Doppler estimation (assuming Gaussian doppler spectrum) */
	QString sdoppler = "";
	QString sdelay = "";
	_REAL rVal;
	vector<_REAL> vecrVal;
	vector<CMeasurements::CRdel> rdel;
	if(Parameters.Measurements.Rdop.get(rVal))
	{
	    sdoppler += QString().setNum(rVal, 'f', 2) + " ";
	}
	if(Parameters.Measurements.Doppler.get(rVal))
	{
	    sdoppler += QString().setNum(rVal, 'f', 2) + " ";
	}
	if(Parameters.Measurements.Rdel.get(rdel))
	{
	    sdelay += QString().setNum(rdel[0].interval, 'f', 2) + " ";
	}
	if (Parameters.Measurements.Delay.get(rVal))
	{
	    sdelay += QString().setNum(rVal, 'f', 2) + " ";
	}
	if(sdoppler=="")
	    sdoppler = "---";
	else
	    sdoppler += "Hz";
	if(sdelay=="")
	    sdelay = "---";
	else
	    sdelay += "ms";
	ValueWiener->setText(sdoppler+" / "+sdelay);

	/* Sample frequency offset estimation */
	_REAL rCurSamROffs = 0.0;
	bool b = Parameters.Measurements.SampleFrequencyOffset.get(rCurSamROffs);
		if(b)
		{
			/* Display value in [Hz] and [ppm] (parts per million) */
			ValueSampFreqOffset->setText(
				QString().setNum(rCurSamROffs, 'f', 2) + " Hz (" +
				QString().setNum((int) (rCurSamROffs / SOUNDCRD_SAMPLE_RATE * 1e6))
				+ " ppm)");
		}
		else
		{
			ValueSampFreqOffset->setText("--");
		}
    }
    else
    {
	ValueSNR->setText("<b>---</b>");
	ValueMERWMER->setText("<b>---</b>");
	ValueWiener->setText("--- / ---");
	ValueSampFreqOffset->setText("---");
    }

#ifdef _DEBUG_
	TextFreqOffset->setText("DC: " + QString().setNum(Parameters.  GetDCFrequency(), 'f', 3) + " Hz ");

	/* Metric values */
	ValueFreqOffset->setText(tr("Metrics [dB]: MSC: ")
	+ QString().setNum(Receiver.GetMSCMLC()->GetAccMetric(), 'f', 2)
	+ "\nSDC: " + QString().setNum( Receiver.GetSDCMLC()->GetAccMetric(), 'f', 2)
	+ " / FAC: " + QString().setNum( Receiver.GetFACMLC()->GetAccMetric(), 'f', 2));
#else
	/* DC frequency */
	ValueFreqOffset->setText(QString().setNum(Parameters.GetDCFrequency(), 'f', 2)+" Hz");
#endif


}

//////////////////////////////// FAC Data ////////////////////////////////////////

void SystemEvalDlg::InitialiseFAC()
{
}

void SystemEvalDlg::UpdateFAC(CParameter& Parameters)
{
	/* FAC info static ------------------------------------------------------ */
	QString strFACInfo;

	/* Robustness mode #################### */
	QChar robm = QString("ABCD")[Parameters.Channel.eRobustness];

    const float so[] = {4.5,5,9,10,18,20};
    float spectrum_occupancy = so[Parameters.Channel.eSpectrumOccupancy];

    strFACInfo = robm+QString(" / %1 kHz").arg(spectrum_occupancy);

	FACDRMModeBWV->setText(strFACInfo); /* Value */

	/* Interleaver Depth #################### */
	switch (Parameters.Channel.eInterleaverDepth)
	{
	case SI_LONG:
		strFACInfo = tr("2 s (Long Interleaving)");
		break;

	case SI_SHORT:
		strFACInfo = tr("400 ms (Short Interleaving)");
		break;
	}

	FACInterleaverDepthV->setText(strFACInfo); /* Value */


	/* SDC, MSC mode #################### */
	/* SDC */
	switch (Parameters.Channel.eSDCmode)
	{
	case CS_1_SM:
		strFACInfo = "4-QAM / ";
		break;

	case CS_2_SM:
		strFACInfo = "16-QAM / ";
		break;

	default:
		break;
	}

	/* MSC */
	switch (Parameters.Channel.eMSCmode)
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
		break;
	}

	FACSDCMSCModeV->setText(strFACInfo); /* Value */

	/* Code rates #################### */
	strFACInfo = QString().setNum(Parameters.MSCParameters.ProtectionLevel.iPartB);
	strFACInfo += " / ";
	strFACInfo += QString().setNum(Parameters.MSCParameters.ProtectionLevel.iPartA);

	FACCodeRateV->setText(strFACInfo); /* Value */

	/* Number of services #################### */
	strFACInfo = tr("Audio: ");
	strFACInfo += QString().setNum(Parameters.FACParameters.iNumAudioServices);
	strFACInfo += tr(" / Data: ");
	strFACInfo +=QString().setNum(Parameters.FACParameters.iNumDataServices);

	FACNumServicesV->setText(strFACInfo); /* Value */

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
		/* QT type of displaying date and time */
		QDateTime DateTime;
		DateTime.setDate(QDate(Parameters.iYear,
			Parameters.iMonth,
			Parameters.iDay));
		DateTime.setTime(QTime(Parameters.iUTCHour,
			Parameters.iUTCMin));

		strFACInfo = DateTime.toString();
	}

	FACTimeDateV->setText(strFACInfo); /* Value */
}

///////////////////////  Plots /////////////////////////////////////

void SystemEvalDlg::InitialisePlots()
{
	plot = new CDRMPlot(MainPlot, Receiver.GetParameters());
	plot->stop();

	/* Connect controls ----------------------------------------------------- */

	/* Chart selector Tree Widget */
	connect(ChartSelector, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
		this, SLOT(OnItemClicked (QTreeWidgetItem*, int)));
	connect(ChartSelector,
		SIGNAL(customContextMenuRequested ( const QPoint&)),
		this, SLOT(OnCustomContextMenuRequested ( const QPoint&)));
	ChartSelector->hideColumn(1);
}

void SystemEvalDlg::showPlots()
{
	/* Restore chart windows */
	const size_t iNumChartWin = Settings.Get("GUI System Evaluation", "numchartwin", 0);
	for (size_t i = 0; i < iNumChartWin; i++)
	{
		stringstream s;
		s << "Chart Window " << i;
		newPlot(0, s.str());
	}
	/* Restore main plot */
	plot->load(Settings, "GUI System Evaluation");
	CDRMPlot::EPlotType pt = plot->GetChartType();
	if(pt == CDRMPlot::NONE_OLD)
	{
		pt = CDRMPlot::INPUT_SIG_PSD;
		plot->SetupChart(pt);
	}
	try {
	    plot->start();
	} catch(const char* e)
	{
	    QMessageBox::information(this, "Problem", e);
	}
	QString pts = QString::number(int(pt));
	QList<QTreeWidgetItem *> l = ChartSelector->findItems(pts, Qt::MatchRecursive|Qt::MatchFixedString, 1);
	if(l.size()==1)
	{
		ChartSelector->scrollToItem(l[0]);
		ChartSelector->setCurrentItem(l[0]);
	}
}

void SystemEvalDlg::hidePlots()
{
	plot->stop();
	plot->save(Settings, "GUI System Evaluation");

	int n = Settings.Get("GUI System Evaluation", "numchartwin", int(0));
	for (int i = 0; i < n; i++)
	{
	stringstream s;
	s << "Chart Window " << i;
	Settings.Clear(s.str());
	}

	/* Store size and position of all additional chart windows */
	n=0;
	for (size_t i = 0; i < plots.size(); i++)
	{
	stringstream s;
	s << "Chart Window " << n;
	if(plots[i]->save(Settings, s.str()))
	    n++;
	plots[i]->stop();
	}
	Settings.Put("GUI System Evaluation", "numchartwin", n);

	/* We do not need the pointers anymore, reset vector */
	plots.clear();
}

void SystemEvalDlg::OnItemClicked (QTreeWidgetItem * item, int)
{
    /* Get char type from selected item and setup chart */
    int pt = item->text(1).toInt();
    cerr << "chose plot type " << pt << endl;
    plot->SetupChart(CDRMPlot::EPlotType(pt));
    try {
	plot->start();
    } catch(const char* e)
    {
	QMessageBox::information(this, "Problem", e);
    }
}

void SystemEvalDlg::OnCustomContextMenuRequested ( const QPoint&)
{
    /* Get chart type from current selected list view item */
    // right clicking also selects so we don't need to use the point parameter
    QTreeWidgetItem* item = ChartSelector->currentItem();
    if (item != NULL)
    {
	/* Open new chart window */
	newPlot(item->text(1).toInt(), "");
    }
}

void SystemEvalDlg::newPlot(int pt, const string& setting)
{
    QwtPlot* p = NULL; p = new QwtPlot(NULL); // quiet warning
    if(p==NULL)
	return; // should never happen, but language allows it
    p->setAttribute(Qt::WA_QuitOnClose, false);
    CDRMPlot* pNewPlot = new CDRMPlot(p, Receiver.GetParameters());
    plots.push_back(pNewPlot);
    if(setting == "")
    {
	pNewPlot->SetupChart(CDRMPlot::EPlotType(pt));
	p->resize(200, 150);
    }
    else
    {
		pNewPlot->load(Settings, setting);
    }
    try {
	pNewPlot->start();
    } catch(const char* e)
    {
    	QMessageBox::information(this, "Problem", e);
    }
    p->show();
}

////////////////////// Frequency Tuning Field /////////////////////

void SystemEvalDlg::InitialiseFrequency()
{
	/* If RSCI in is enabled, disable retuning */
	if (Receiver.UpstreamDIInputEnabled() == true)
	{
		EdtFrequency->setText("0");
		EdtFrequency->setEnabled(false);
	}
	else
	{
	connect(timerLineEditFrequency, SIGNAL(timeout()), this, SLOT(OnTimerLineEditFrequency()));
	connect(timerTuning, SIGNAL(timeout()), this, SLOT(OnTimerTuning()));

	connect( EdtFrequency, SIGNAL(textChanged(const QString&)),
	    this, SLOT(OnLineEditFrequencyChanged(const QString&)) );

	EdtFrequency->setValidator(new QIntValidator(100, 120000, EdtFrequency));
	}
}

void SystemEvalDlg::UpdateFrequency()
{
	/* Update frequency edit control
	 * frequency could be changed by schedule dialog
	 * or RSCI
	 * Don't update if already correct - would create loops
	 */
	int iFrequency = Receiver.GetFrequency();
	int iFreq = EdtFrequency->text().toInt();
	if(iFrequency != iFreq)
	{
		if(bTuningInProgress == false)
			EdtFrequency->setText(QString::number(iFrequency));
	}
	else
	{
		bTuningInProgress = false;
	}
}

void SystemEvalDlg::OnLineEditFrequencyChanged(const QString&)
{
	// wait an inter-digit timeout
	timerLineEditFrequency->setSingleShot(true);
	timerLineEditFrequency->start(500);
	bTuningInProgress = true;
}

void SystemEvalDlg::OnTimerLineEditFrequency()
{
	// commit the frequency if different
	int iFreq = EdtFrequency->text().toInt();
	int iFrequency = Receiver.GetFrequency();
	if(iFreq != iFrequency)
	{
		Receiver.SetFrequency(iFreq);
		bTuningInProgress = true;
		timerTuning->setSingleShot(true);
		timerTuning->start(2000);
	}
}

void SystemEvalDlg::OnTimerTuning()
{
	bTuningInProgress = false;
}
void SystemEvalDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	Receiver.MuteAudio(CheckBoxMuteAudio->isChecked());
}

void SystemEvalDlg::OnCheckBoxReverb()
{
	/* Set parameter in working thread module */
	Receiver.SetReverbEffect(CheckBoxReverb->isChecked());
}

void SystemEvalDlg::OnCheckSaveAudioWav()
{
	OnSaveAudio(this, CheckBoxSaveAudioWave, Receiver);
}

void SystemEvalDlg::AddWhatsThisHelp()
{
/*
	This text was taken from the only documentation of Dream software
*/
	/* DC Frequency Offset */
	const QString strDCFreqOffs =
		tr("<b>DC Frequency Offset:</b> This is the "
		"estimation of the DC frequency offset. This offset corresponds "
		"to the resulting intermediate frequency of the front-end. "
		"This frequency is not restricted to a certain value. The only "
		"restriction is that the DRM spectrum must be completely inside the "
		"bandwidth of the sound card.");

	TextFreqOffset->setWhatsThis( strDCFreqOffs);
	ValueFreqOffset->setWhatsThis( strDCFreqOffs);

	/* Sample Frequency Offset */
	const QString strFreqOffset =
		tr("<b>Sample Frequency Offset:</b> This is the "
		"estimation of the sample rate offset between the sound card sample "
		"rate of the local computer and the sample rate of the D / A (digital "
		"to analog) converter in the transmitter. Usually the sample rate "
		"offset is very constant for a given sound card. Therefore it is "
		"useful to inform the Dream software about this value at application "
		"startup to increase the acquisition speed and reliability.");

	TextSampFreqOffset->setWhatsThis( strFreqOffset);
	ValueSampFreqOffset->setWhatsThis( strFreqOffset);

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

	TextWiener->setWhatsThis( strDopplerDelay);
	ValueWiener->setWhatsThis( strDopplerDelay);

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

	TextLabelLEDIOInterface->setWhatsThis( strLEDIOInterface);
	LEDIOInterface->setWhatsThis( strLEDIOInterface);

	/* Time Sync Acq LED */
	const QString strLEDTimeSyncAcq =
		tr("<b>Time Sync Acq LED:</b> This LED shows the "
		"state of the timing acquisition (search for the beginning of an OFDM "
		"symbol). If the acquisition is done, this LED will stay green.");

	TextLabelLEDTimeSyncAcq->setWhatsThis( strLEDTimeSyncAcq);
	LEDTimeSync->setWhatsThis( strLEDTimeSyncAcq);

	/* Frame Sync LED */
	const QString strLEDFrameSync =
		tr("<b>Frame Sync LED:</b> The DRM frame "
		"synchronization status is shown with this LED. This LED is also only "
		"active during acquisition state of the Dream receiver. In tracking "
		"mode, this LED is always green.");

	TextLabelLEDFrameSync->setWhatsThis( strLEDFrameSync);
	LEDFrameSync->setWhatsThis( strLEDFrameSync);

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

	TextLabelLEDFACCRC->setWhatsThis( strLEDFACCRC);
	LEDFAC->setWhatsThis( strLEDFACCRC);

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

	TextLabelLEDSDCCRC->setWhatsThis( strLEDSDCCRC);
	LEDSDC->setWhatsThis( strLEDSDCCRC);

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

	TextLabelLEDMSCCRC->setWhatsThis( strLEDMSCCRC);
	LEDMSC->setWhatsThis( strLEDMSCCRC);

	/* Freq */
	EdtFrequency->setWhatsThis(
		tr("<b>Freq:</b> In this edit control, the current "
		"selected frequency on the front-end can be specified. This frequency "
		"will be written into the log file."));

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

	ValueSNR->setWhatsThis( strSNREst);
	TextSNRText->setWhatsThis( strSNREst);

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

	ValueMERWMER->setWhatsThis( strMERWMEREst);
	TextMERWMER->setWhatsThis( strMERWMEREst);

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

	FACDRMModeBWL->setWhatsThis( strRobustnessMode);
	FACDRMModeBWV->setWhatsThis( strRobustnessMode);

	/* Interleaver Depth */
	const QString strInterleaver =
		tr("<b>Interleaver Depth:</b> The symbol "
		"interleaver depth can be either short (approx. 400 ms) or long "
		"(approx. 2 s). The longer the interleaver the better the channel "
		"decoder can correct errors from slow fading signals. But the "
		"longer the interleaver length the longer the delay until (after a "
		"re-synchronization) audio can be heard.");

	FACInterleaverDepthL->setWhatsThis( strInterleaver);
	FACInterleaverDepthV->setWhatsThis( strInterleaver);

	/* SDC / MSC Mode */
	const QString strSDCMSCMode =
		tr("<b>SDC / MSC Mode:</b> Shows the modulation "
		"type of the SDC and MSC channel. For the MSC channel, some "
		"hierarchical modes are defined which can provide a very strong "
		"protected service channel.");

	FACSDCMSCModeL->setWhatsThis( strSDCMSCMode);
	FACSDCMSCModeV->setWhatsThis( strSDCMSCMode);

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

	FACCodeRateL->setWhatsThis( strProtLevel);
	FACCodeRateV->setWhatsThis( strProtLevel);

	/* Number of Services */
	const QString strNumServices =
		tr("<b>Number of Services:</b> This shows the "
		"number of audio and data services transmitted in the DRM stream. "
		"The maximum number of streams is four.");

	FACNumServicesL->setWhatsThis( strNumServices);
	FACNumServicesV->setWhatsThis( strNumServices);

	/* Received time - date */
	const QString strTimeDate =
		tr("<b>Received time - date:</b> This label shows "
		"the received time and date in UTC. This information is carried in "
		"the SDC channel.");

	FACTimeDateL->setWhatsThis( strTimeDate);
	FACTimeDateV->setWhatsThis( strTimeDate);

	/* Mute Audio */
	CheckBoxMuteAudio->setWhatsThis(
		tr("<b>Mute Audio:</b> The audio can be muted by "
		"checking this box. The reaction of checking or unchecking this box "
		"is delayed by approx. 1 second due to the audio buffers."));

	/* Reverberation Effect */
	CheckBoxReverb->setWhatsThis(
		tr("<b>Reverberation Effect:</b> If this check box is checked, a "
		"reverberation effect is applied each time an audio drop-out occurs. "
		"With this effect it is possible to mask short drop-outs."));

	/* Save audio as wave */
	CheckBoxSaveAudioWave->setWhatsThis(
		tr("<b>Save Audio as WAV:</b> Save the audio signal "
		"as stereo, 16-bit, 48 kHz sample rate PCM wave file. Checking this "
		"box will let the user choose a file name for the recording."));

	/* TODO - Test the right click popup */

	/* Chart Selector */
	ChartSelector->setWhatsThis(
		tr("<b>Chart Selector:</b> With the chart selector "
		"different types of graphical display of parameters and receiver "
		"states can be chosen. The different plot types are sorted in "
		"different groups. To open a group just double-click on the group or "
		"click on the plus left of the group name. After clicking on an item "
		"it is possible to choose other items by using the up / down arrow "
		"keys. With these keys it is also possible to open and close the "
		"groups by using the left / right arrow keys.<br>A separate chart "
		"window for a selected item can be opened by right click on the item "
		"and click on the context menu item."));
}

void SystemEvalDlg::OnHelpWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}
