/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
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

#include "TransmitterMainWindow.h"
#include <QHostAddress>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QCloseEvent>
#include "DialogUtil.h"
#include "../DrmTransmitterInterface.h"
#include "../Parameter.h"
#include <sstream>
#include <fstream>

TransmitterMainWindow::TransmitterMainWindow(CDRMTransmitterInterface& tx, CSettings& NSettings,
	QWidget* parent, const char* name, Qt::WFlags f
	):
	QMainWindow(parent, f),
	Ui_TransmitterMainWindow(),
	DRMTransmitter(tx), Settings(NSettings),
    Timer(), bIsStarted(false),
    channelEditor(*this),
    streamEditor(*this),
    audioComponentEditor(*this),
    dataComponentEditor(*this),
    servicesEditor(*this),
    cofdmEditor(*this),
    mdiInputEditor(*this),
    mdiOutputEditor(this)
{
    setupUi(this);
    channelEditor.setupUi();
    streamEditor.setupUi();
    audioComponentEditor.setupUi();
    dataComponentEditor.setupUi();
    servicesEditor.setupUi();
    cofdmEditor.setupUi();
    mdiInputEditor.setupUi();

	/* recover window size and position */
	CWinGeom s;
	Settings.Get("Transmit Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

    setWindowTitle(tr("Dream DRM Transmitter"));

	/* Init controls with default settings */
	ButtonStartStop->setText(tr("&Start"));

	/* Init progress bar for input signal level */
	ProgrInputLevel->setRange(-50.0, 0.0);
	ProgrInputLevel->setOrientation(Qt::Horizontal, QwtThermo::BottomScale);
	ProgrInputLevel->setFillColor(QColor(0, 190, 0));
	ProgrInputLevel->setAlarmLevel(-5.0);
	ProgrInputLevel->setAlarmColor(QColor(255, 0, 0));

	/* Init progress bar for current transmitted picture */
	ProgressBarCurPict->setRange(0, 100);
	ProgressBarCurPict->setValue(0);
	TextLabelCurPict->setText("");

	/* Enable all controls */
	EnableAllControlsForSet();

	/* Connections ---------------------------------------------------------- */

    /* File Menu */
	connect(action_Exit, SIGNAL(triggered()), this, SLOT(OnButtonClose()));

    /* Help Menu */
    CAboutDlg* pAboutDlg = new CAboutDlg(this);
	connect(actionWhats_This, SIGNAL(triggered()), this, SLOT(OnHelpWhatsThis()));
	connect(actionAbout, SIGNAL(triggered()), pAboutDlg, SLOT(show()));

	/* General */
	connect(ButtonStartStop, SIGNAL(clicked()), this, SLOT(OnButtonStartStop()));
	connect(buttonClose, SIGNAL(clicked()), this, SLOT(OnButtonClose()));

    QButtonGroup* modeGroup = new QButtonGroup(this);
    modeGroup->addButton(RadioButtonTransmitter, CDRMTransmitterInterface::T_TX);
    modeGroup->addButton(RadioButtonEncoder, CDRMTransmitterInterface::T_ENC);
    modeGroup->addButton(RadioButtonModulator, CDRMTransmitterInterface::T_MOD);

	connect(modeGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioMode(int)));

	connect(&channelEditor, SIGNAL(MSCCapacityChanged()), this, SLOT(OnMSCCapacityChanged()));
	connect(this, SIGNAL(MSCCapacityChanged(int)), &channelEditor, SLOT(OnMSCCapacityChanged(int)));
	connect(this, SIGNAL(MSCCapacityChanged(int)), &streamEditor, SLOT(OnMSCCapacityChanged(int)));

	connect(&channelEditor, SIGNAL(SDCCapacityChanged()), this, SLOT(OnSDCCapacityChanged()));
	connect(this, SIGNAL(SDCCapacityChanged(int)), &channelEditor, SLOT(OnSDCCapacityChanged(int)));

	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	GetFrom(DRMTransmitter);
	OnRadioMode(0);

	Timer.stop();
}

TransmitterMainWindow::~TransmitterMainWindow()
{
}

void
TransmitterMainWindow::closeEvent(QCloseEvent* ce)
{
	/* Save window geometry data */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("Transmit Dialog", s);

	/* Stop transmitter if needed */
	if (bIsStarted == true)
		OnButtonStartStop();
	else
		PutTo(DRMTransmitter); // so Transmitter save settings has the latest info
	ce->accept();
}

void TransmitterMainWindow::OnMSCCapacityChanged()
{
    channelEditor.PutTo(DRMTransmitter);
    DRMTransmitter.CalculateChannelCapacities();
    emit MSCCapacityChanged(DRMTransmitter.NumBitsMSC());
}

void TransmitterMainWindow::OnSDCCapacityChanged()
{
    channelEditor.PutTo(DRMTransmitter);
    DRMTransmitter.CalculateChannelCapacities();
    emit SDCCapacityChanged(DRMTransmitter.NumBitsSDCsuperFrame());
}

void TransmitterMainWindow::OnButtonClose()
{
    (void)this->close();
}

void TransmitterMainWindow::OnTimer()
{
    ProgrInputLevel->setValue(DRMTransmitter.GetLevelMeter());
    string strCPictureName;
    _REAL rCPercent;

    /* Activate progress bar for slide show pictures only if current state
       can be queried */
    if (DRMTransmitter.GetTransStat(strCPictureName, rCPercent))
    {
        /* Enable controls */
        ProgressBarCurPict->setEnabled(true);
        TextLabelCurPict->setEnabled(true);

        /* We want to file name, not the complete path -> "QFileInfo" */
        QFileInfo FileInfo(strCPictureName.c_str());

        /* Show current file name and percentage */
        TextLabelCurPict->setText(FileInfo.fileName());
        ProgressBarCurPict->setValue(int(rCPercent * 100)); /* % */
    }
    else
    {
        /* Disable controls */
        ProgressBarCurPict->setEnabled(false);
        TextLabelCurPict->setEnabled(false);
    }
}

void
TransmitterMainWindow::GetFrom(const CDRMTransmitterInterface& DRMTransmitter)
{
	vector<string> s;
	switch(DRMTransmitter.GetOperatingMode())
	{
	case CDRMTransmitterInterface::T_ENC:
		channelEditor.GetFrom(DRMTransmitter);
		streamEditor.GetFrom(DRMTransmitter);
        audioComponentEditor.GetFrom(DRMTransmitter);
		dataComponentEditor.GetFrom(DRMTransmitter);
		servicesEditor.GetFrom(DRMTransmitter);
		DRMTransmitter.GetMDIOut(s);
		mdiOutputEditor.load(s);
		RadioButtonEncoder->setChecked(true);
		break;
	case CDRMTransmitterInterface::T_MOD:
		mdiInputEditor.GetFrom(DRMTransmitter);
		cofdmEditor.GetFrom(DRMTransmitter);
		RadioButtonModulator->setChecked(true);
		break;
	case CDRMTransmitterInterface::T_TX:
		channelEditor.GetFrom(DRMTransmitter);
		streamEditor.GetFrom(DRMTransmitter);
        audioComponentEditor.GetFrom(DRMTransmitter);
		dataComponentEditor.GetFrom(DRMTransmitter);
		servicesEditor.GetFrom(DRMTransmitter);
		cofdmEditor.GetFrom(DRMTransmitter);
		RadioButtonTransmitter->setChecked(true);
		break;
	}
}

void
TransmitterMainWindow::PutTo(CDRMTransmitterInterface& DRMTransmitter) const
{
	CDRMTransmitterInterface::ETxOpMode eMod = CDRMTransmitterInterface::T_TX;

	if(RadioButtonTransmitter->isChecked())
        eMod = CDRMTransmitterInterface::T_TX;

	if(RadioButtonEncoder->isChecked())
        eMod = CDRMTransmitterInterface::T_ENC;

	if(RadioButtonModulator->isChecked())
        eMod = CDRMTransmitterInterface::T_MOD;

	DRMTransmitter.SaveSettings(Settings);
	DRMTransmitter.SetOperatingMode(eMod);
	DRMTransmitter.LoadSettings(Settings);
	vector<string> MDIoutAddr;

    switch(eMod)
	{
    case CDRMTransmitterInterface::T_ENC:
		channelEditor.PutTo(DRMTransmitter);
		streamEditor.PutTo(DRMTransmitter);
		audioComponentEditor.PutTo(DRMTransmitter);
		dataComponentEditor.PutTo(DRMTransmitter);
		servicesEditor.PutTo(DRMTransmitter);
		mdiOutputEditor.save(MDIoutAddr);
		DRMTransmitter.SetMDIOut(MDIoutAddr);
		break;
    case CDRMTransmitterInterface::T_TX:
		channelEditor.PutTo(DRMTransmitter);
		streamEditor.PutTo(DRMTransmitter);
		audioComponentEditor.PutTo(DRMTransmitter);
		dataComponentEditor.PutTo(DRMTransmitter);
		servicesEditor.PutTo(DRMTransmitter);
		cofdmEditor.PutTo(DRMTransmitter);
		break;
    case CDRMTransmitterInterface::T_MOD:
		mdiInputEditor.PutTo(DRMTransmitter);
		cofdmEditor.PutTo(DRMTransmitter);
	}
}

void
TransmitterMainWindow::OnRadioMode(int)
{
	TabWidgetConfigure->clear();

	if(RadioButtonTransmitter->isChecked())
	{
		TabWidgetConfigure->addTab(Channel, tr("Channel"));
		TabWidgetConfigure->addTab(Streams, tr("Streams"));
		TabWidgetConfigure->addTab(Audio, tr("Audio"));
		TabWidgetConfigure->addTab(Data, tr("Data"));
		TabWidgetConfigure->addTab(Services, tr("Services"));
		TabWidgetConfigure->addTab(COFDM, tr("COFDM"));
		TabWidgetConfigure->setCurrentWidget(Channel);
	}
	if(RadioButtonEncoder->isChecked())
	{
		TabWidgetConfigure->addTab(Channel, tr("Channel"));
		TabWidgetConfigure->addTab(Streams, tr("Streams"));
		TabWidgetConfigure->addTab(Audio, tr("Audio"));
		TabWidgetConfigure->addTab(Data, tr("Data"));
		TabWidgetConfigure->addTab(Services, tr("Services"));
		TabWidgetConfigure->addTab(MDIOut, tr("MDI Output"));
		TabWidgetConfigure->setCurrentWidget(Channel);
	}
	if(RadioButtonModulator->isChecked())
	{
		TabWidgetConfigure->addTab(MDIIn, tr("MDI Input"));
		TabWidgetConfigure->addTab(COFDM, tr("COFDM"));
		TabWidgetConfigure->setCurrentWidget(COFDM);
	}
}

void TransmitterMainWindow::OnButtonStartStop()
{
	if (bIsStarted == true)
	{
	    Timer.stop();
		/* Stop transmitter */
		DRMTransmitter.Stop();
		(void)DRMTransmitter.wait(5000);
		if(!DRMTransmitter.isFinished())
		{
			QMessageBox::critical(this, "Dream", tr("Exit"),
				tr("Termination of working thread failed"));
		}

		ButtonStartStop->setText(tr("&Start"));

		EnableAllControlsForSet();

		bIsStarted = false;
	}
	else
	{
		PutTo(DRMTransmitter);
		DRMTransmitter.start();
		ButtonStartStop->setText(tr("&Stop"));
		DisableAllControlsForSet();
		bIsStarted = true;

        if(RadioButtonModulator->isChecked()==false)
            Timer.start(GUI_CONTROL_UPDATE_TIME);
	}
}

void TransmitterMainWindow::DisableAllControlsForSet()
{
	Channel->setEnabled(false);
	Streams->setEnabled(false);
	Audio->setEnabled(false);
	Data->setEnabled(false);
	Services->setEnabled(false);
	MDIOut->setEnabled(false);
	MDIIn->setEnabled(false);
	COFDM->setEnabled(false);
}

void TransmitterMainWindow::EnableAllControlsForSet()
{
	Channel->setEnabled(true);
	Streams->setEnabled(true);
	Audio->setEnabled(true);
	Data->setEnabled(true);
	Services->setEnabled(true);
	MDIOut->setEnabled(true);
	MDIIn->setEnabled(true);
	COFDM->setEnabled(true);

	/* Reset status bars */
	ProgrInputLevel->setValue(RET_VAL_LOG_0);
	ProgressBarCurPict->setValue(0);
	TextLabelCurPict->setText("");
}

void TransmitterMainWindow::OnHelpWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}

ChannelEditor::ChannelEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void ChannelEditor::setupUi()
{
	/* MSC interleaver mode */
	ui.ComboBoxMSCInterleaver->insertItem(0, tr("2 s (Long Interleaving)"), QVariant(SI_LONG));
	ui.ComboBoxMSCInterleaver->insertItem(0, tr("400 ms (Short Interleaving)"), QVariant(SI_SHORT));

	/* MSC Constellation Scheme */
	ui.ComboBoxMSCConstellation->insertItem(0, tr("SM 16-QAM"), QVariant(CS_2_SM));
	ui.ComboBoxMSCConstellation->insertItem(1, tr("SM 64-QAM"), QVariant(CS_3_SM));

// These modes should not be used right now, TODO
//	ui.ComboBoxMSCConstellation->insertItem(2, tr("HMsym 64-QAM"), QVariant(CS_3_HMSYM));
//	ui.ComboBoxMSCConstellation->insertItem(3, tr("HMmix 64-QAM"), QVariant(CS_3_HMMIX));

	/* SDC Constellation Scheme */
	ui.ComboBoxSDCConstellation->insertItem(0, tr("4-QAM"), QVariant(CS_1_SM));
	ui.ComboBoxSDCConstellation->insertItem(1, tr("16-QAM"), QVariant(CS_2_SM));

	/* PutTo button group IDs */

    soGroup = new QButtonGroup(this);
    soGroup->addButton(ui.RadioButtonBandwidth45, SO_0);
    soGroup->addButton(ui.RadioButtonBandwidth5, SO_1);
    soGroup->addButton(ui.RadioButtonBandwidth9, SO_2);
    soGroup->addButton(ui.RadioButtonBandwidth10, SO_3);
    soGroup->addButton(ui.RadioButtonBandwidth18, SO_4);
    soGroup->addButton(ui.RadioButtonBandwidth20, SO_5);
	connect(soGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioBandwidth(int)));

    robustnessGroup = new QButtonGroup(this);
    robustnessGroup->addButton(ui.RadioButtonRMA, RM_ROBUSTNESS_MODE_A);
    robustnessGroup->addButton(ui.RadioButtonRMB, RM_ROBUSTNESS_MODE_B);
    robustnessGroup->addButton(ui.RadioButtonRMC, RM_ROBUSTNESS_MODE_C);
    robustnessGroup->addButton(ui.RadioButtonRMD, RM_ROBUSTNESS_MODE_D);
	connect(robustnessGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioRobustnessMode(int)));

	connect(ui.ComboBoxMSCInterleaver, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCInterleaverActivated(int)));
	connect(ui.ComboBoxMSCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCConstellationActivated(int)));
	connect(ui.ComboBoxMSCProtLev, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCProtLevActivated(int)));
	connect(ui.ComboBoxSDCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxSDCConstellationActivated(int)));
}

void
ChannelEditor::GetFrom(const CDRMTransmitterInterface& DRMTransmitter)
{
    CChannel channel;
	 DRMTransmitter.GetChannel(channel);

    robustnessGroup->button(channel.eRobustness)->setChecked(true);
	soGroup->button(channel.eSpectrumOccupancy)->setChecked(true);

	switch (channel.eInterleaverDepth)
	{
	case SI_LONG:
		ui.ComboBoxMSCInterleaver->setCurrentIndex(0);
		break;

	case SI_SHORT:
		ui.ComboBoxMSCInterleaver->setCurrentIndex(1);
		break;
	}

	switch (channel.eMSCmode)
	{
	case CS_1_SM:
		break;

	case CS_2_SM:
		ui.ComboBoxMSCConstellation->setCurrentIndex(0);
		break;

	case CS_3_SM:
		ui.ComboBoxMSCConstellation->setCurrentIndex(1);
		break;

	case CS_3_HMSYM:
//		ui.ComboBoxMSCConstellation->setCurrentIndex(2);
		break;

	case CS_3_HMMIX:
//		ui.ComboBoxMSCConstellation->setCurrentIndex(3);
		break;
	}

	/* MSC Protection Level */
	UpdateMSCProtLevCombo(); /* options depend on MSC Constellation */
	CMSCParameters msc;
	DRMTransmitter.GetMSC(msc);
	ui.ComboBoxMSCProtLev->setCurrentIndex(msc.ProtectionLevel.iPartB);

	switch (channel.eSDCmode)
	{
	case CS_1_SM:
		ui.ComboBoxSDCConstellation->setCurrentIndex(0);
		break;

	case CS_2_SM:
		ui.ComboBoxSDCConstellation->setCurrentIndex(1);
		break;
	default:
		;
	}
	emit MSCCapacityChanged();
	emit SDCCapacityChanged();
}

void
ChannelEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter) const
{
	CMSCParameters MSCParameters;
	CChannel channel;
	/* Spectrum Occupancy */
    channel.eSpectrumOccupancy = ESpecOcc(soGroup->checkedId());

	/* MSC Protection Level */
	CMSCProtLev MSCPrLe;
	MSCPrLe.iPartB = ui.ComboBoxMSCProtLev->currentIndex();
	MSCParameters.ProtectionLevel = MSCPrLe;

	channel.eMSCmode
        = ECodScheme(ui.ComboBoxMSCConstellation->itemData(
            ui.ComboBoxMSCConstellation->currentIndex()
            ).toInt());

    channel.eInterleaverDepth
        = ESymIntMod(ui.ComboBoxMSCInterleaver->itemData(
            ui.ComboBoxMSCInterleaver->currentIndex()
    ).toInt());

    channel.eSDCmode
        = ECodScheme(ui.ComboBoxSDCConstellation->itemData(
            ui.ComboBoxSDCConstellation->currentIndex()
    ).toInt());

	/* Robustness Mode */
    channel.eRobustness = ERobMode(robustnessGroup->checkedId());

	DRMTransmitter.PutMSC(MSCParameters);
	DRMTransmitter.PutChannel(channel);
}

void ChannelEditor::OnComboBoxMSCInterleaverActivated(int)
{
}

void ChannelEditor::OnComboBoxSDCConstellationActivated(int)
{
	emit SDCCapacityChanged();
}

void ChannelEditor::OnComboBoxMSCConstellationActivated(int)
{
	emit MSCCapacityChanged();
}

void ChannelEditor::OnComboBoxMSCProtLevActivated(int)
{
	emit MSCCapacityChanged();
}

void ChannelEditor::UpdateMSCProtLevCombo()
{
	if(ui.ComboBoxMSCConstellation->currentIndex()==0)
	{
		/* Only two protection levels possible in 16 QAM mode */
		ui.ComboBoxMSCProtLev->clear();
		ui.ComboBoxMSCProtLev->insertItem(0, "0", 0);
		ui.ComboBoxMSCProtLev->insertItem(1, "1", 1);
	}
	else
	{
		/* Four protection levels defined */
		ui.ComboBoxMSCProtLev->clear();
		ui.ComboBoxMSCProtLev->insertItem(0, "0", 0);
		ui.ComboBoxMSCProtLev->insertItem(1, "1", 1);
		ui.ComboBoxMSCProtLev->insertItem(2, "2", 2);
		ui.ComboBoxMSCProtLev->insertItem(3, "3", 3);
	}

	/* PutTo protection level to 1 */
	ui.ComboBoxMSCProtLev->setCurrentIndex(1);
}

void ChannelEditor::OnRadioBandwidth(int)
{
	emit MSCCapacityChanged();
}

void ChannelEditor::OnRadioRobustnessMode(int iID)
{
	/* Check, which bandwidth's are possible with this robustness mode */
	switch (iID)
	{
	case 0:
		/* All bandwidth modes are possible */
		ui.RadioButtonBandwidth45->setEnabled(true);
		ui.RadioButtonBandwidth5->setEnabled(true);
		ui.RadioButtonBandwidth9->setEnabled(true);
		ui.RadioButtonBandwidth10->setEnabled(true);
		ui.RadioButtonBandwidth18->setEnabled(true);
		ui.RadioButtonBandwidth20->setEnabled(true);
		break;

	case 1:
		/* All bandwidth modes are possible */
		ui.RadioButtonBandwidth45->setEnabled(true);
		ui.RadioButtonBandwidth5->setEnabled(true);
		ui.RadioButtonBandwidth9->setEnabled(true);
		ui.RadioButtonBandwidth10->setEnabled(true);
		ui.RadioButtonBandwidth18->setEnabled(true);
		ui.RadioButtonBandwidth20->setEnabled(true);
		break;

	case 2:
		/* Only 10 and 20 kHz possible in robustness mode C */
		ui.RadioButtonBandwidth45->setEnabled(false);
		ui.RadioButtonBandwidth5->setEnabled(false);
		ui.RadioButtonBandwidth9->setEnabled(false);
		ui.RadioButtonBandwidth10->setEnabled(true);
		ui.RadioButtonBandwidth18->setEnabled(false);
		ui.RadioButtonBandwidth20->setEnabled(true);

		/* PutTo check on a default value to be sure we are "in range" */
		ui.RadioButtonBandwidth10->setChecked(true);
		break;

	case 3:
		/* Only 10 and 20 kHz possible in robustness mode D */
		ui.RadioButtonBandwidth45->setEnabled(false);
		ui.RadioButtonBandwidth5->setEnabled(false);
		ui.RadioButtonBandwidth9->setEnabled(false);
		ui.RadioButtonBandwidth10->setEnabled(true);
		ui.RadioButtonBandwidth18->setEnabled(false);
		ui.RadioButtonBandwidth20->setEnabled(true);

		/* PutTo check on a default value to be sure we are "in range" */
		ui.RadioButtonBandwidth10->setChecked(true);
		break;
	}
	emit MSCCapacityChanged();
	emit SDCCapacityChanged();
}

void
ChannelEditor::OnMSCCapacityChanged(int iBitsMSC)
{
	ui.TextLabelMSCCapBits->setText(QString::number(iBitsMSC));
	ui.TextLabelMSCCapBytes->setText(QString::number(iBitsMSC/8));
}

void
ChannelEditor::OnSDCCapacityChanged(int iBitsSDC)
{
	ui.TextLabelSDCCapBits->setText(QString::number(iBitsSDC));
	ui.TextLabelSDCCapBytes->setText(QString::number(iBitsSDC/8));
}

StreamEditor::StreamEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void StreamEditor::setupUi()
{
    streams = new QStandardItemModel();
	QStringList labels;
	labels << tr("Stream") << tr("Type")
            << tr("Bytes per Frame") << tr("Bytes per Packet") << tr("Packets per Frame");
	streams->setHorizontalHeaderLabels(labels);
    ui.treeViewStreams->setModel(streams);
	connect(ui.ButtonAddStream, SIGNAL(clicked()),
		this, SLOT(OnButtonAddStream()));
	connect(ui.ButtonDeleteStream, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteStream()));
	connect(ui.ComboBoxStreamType, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxStreamTypeActivated(int)));
	connect(ui.ComboBoxPacketsPerFrame, SIGNAL(activated(const QString&)),
		this, SLOT(OnComboBoxPacketsPerFrameActivated(const QString&)));
	connect(ui.LineEditPacketLen, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditPacketLenChanged(const QString&)));
	connect(ui.treeViewStreams, SIGNAL(clicked(const QModelIndex&)),
        this, SLOT(OnItemClicked(const QModelIndex&)));

    ui.ButtonDeleteStream->setEnabled(false);
}

void
StreamEditor::GetFrom(const CDRMTransmitterInterface& DRMTransmitter)
{
	CMSCParameters MSCParameters;
	DRMTransmitter.GetMSC(MSCParameters);

    size_t n = MSCParameters.Stream.size();

    streams->setRowCount(0);

    if(n==0) // default to 1 audio stream filling MSC
    {
        ui.ComboBoxStreamType->setCurrentIndex(0);
        ui.LineEditBytesPerFrame->setText(ui.TextLabelMSCBytesTotal->text());
        ui.LineEditPacketLen->setText("");
		OnButtonAddStream();
        return;
    }

	for(size_t i=0; i<n; i++)
	{
		const CStream& stream = MSCParameters.Stream[i];
        int bytes = stream.iLenPartB; // EEP Only
        if(stream.iLenPartA != 0)
        {
            QMessageBox::information(NULL, "Dream", tr("UEP stream read from transmitter - ignored"),
            QMessageBox::Ok);
        }
		ui.ComboBoxStream->setCurrentIndex(i);
		if(stream.eAudDataFlag == SF_AUDIO)
		{
			ui.ComboBoxStreamType->setCurrentIndex(0);
			ui.LineEditPacketLen->setText("");
		}
		else
		{
			if(stream.ePacketModInd == PM_PACKET_MODE)
			{
				ui.ComboBoxStreamType->setCurrentIndex(1);
				ui.LineEditPacketLen->setText(QString::number(stream.iPacketLen));
			}
			else
			{
				ui.ComboBoxStreamType->setCurrentIndex(2);
				ui.LineEditPacketLen->setText("");
				ui.ComboBoxPacketsPerFrame->clear();
			}
		}
		ui.LineEditBytesPerFrame->setText(QString::number(bytes));
		OnButtonAddStream();
	}
}

void
StreamEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter) const
{
	CMSCParameters MSCParameters;
	DRMTransmitter.GetMSC(MSCParameters);

    int n = streams->rowCount();
	MSCParameters.Stream.resize(n);
	for (int i=0; i<n; i++)
	{
	    int iStreamID = streams->item(i, 0)->text().toInt();
	    QString type = streams->item(i, 1)->text();
	    int bytes = streams->item(i, 2)->text().toInt();
        CStream& stream = MSCParameters.Stream[iStreamID];
        switch(ui.ComboBoxStreamType->findText(type))
        {
            case 0:
                stream.eAudDataFlag = SF_AUDIO;
                break;
            case 1:
                stream.eAudDataFlag = SF_DATA;
                stream.ePacketModInd = PM_PACKET_MODE;
                stream.iPacketLen = streams->item(i, 3)->text().toInt();
                break;
            case 2:
                stream.eAudDataFlag = SF_DATA;
                stream.ePacketModInd = PM_SYNCHRON_STR_MODE;
        }
        stream.iLenPartA = 0; // EEP
        stream.iLenPartB = bytes;
	}
}

void
StreamEditor::OnLineEditPacketLenChanged(const QString& str)
{
	ui.ComboBoxPacketsPerFrame->clear();
	if(str=="")
		return;
	size_t packet_len = str.toInt();
	if(packet_len>=3) // minumum size of a packet
	{
        size_t bytes = ui.TextLabelMSCBytesAvailable->text().toInt();
        size_t max_packets = bytes/(packet_len);
        for(size_t i=1; i<=max_packets; i++)
            ui.ComboBoxPacketsPerFrame->addItem(QString::number(i));
        ui.ComboBoxPacketsPerFrame->setCurrentIndex(0);
	}
}

void
StreamEditor::OnComboBoxPacketsPerFrameActivated(const QString& str)
{
	if(ui.ComboBoxPacketsPerFrame->currentText()=="-")
		return;
	size_t packet_len = ui.LineEditPacketLen->text().toInt();
	size_t packets = str.toInt();
	ui.LineEditBytesPerFrame->setText(QString::number(packets*packet_len));
}

void
StreamEditor::OnComboBoxStreamTypeActivated(int item)
{
	size_t bytes = ui.TextLabelMSCBytesAvailable->text().toInt();
	switch(item)
	{
	case 0: // audio
	case 2: // data_stream
		ui.LineEditPacketLen->setText("");
		ui.LineEditPacketLen->setEnabled(false);
		ui.ComboBoxPacketsPerFrame->clear();
		ui.ComboBoxPacketsPerFrame->setEnabled(false);
		ui.LineEditBytesPerFrame->setEnabled(true);
		break;
	case 1: // data_packet
		ui.LineEditPacketLen->setText(QString::number(int(bytes)));
		ui.LineEditPacketLen->setEnabled(true);
		ui.ComboBoxPacketsPerFrame->setEnabled(true);
		ui.LineEditBytesPerFrame->setEnabled(false);
		break;
	}
    ui.LineEditBytesPerFrame->setText(QString::number(bytes));
}

void
StreamEditor::OnItemClicked(const QModelIndex& index)
{
    int row = index.row();
    QString type =  streams->item(row, 1)->text();
	ui.LineEditBytesPerFrame->setText(streams->item(row, 2)->text());
    int iType = ui.ComboBoxStreamType->findText(type);
    ui.ComboBoxStreamType->setCurrentIndex(iType);
    if(iType == 1) // data packet
    {
        ui.LineEditPacketLen->setText(streams->item(row, 3)->text());
        int f = ui.ComboBoxPacketsPerFrame->findText(streams->item(row, 4)->text());
        ui.ComboBoxPacketsPerFrame->setCurrentIndex(f);
    }
    //ui.treeViewStreams->setCurrentCell(row, 0, QItemSelectionModel::Rows|QItemSelectionModel::ClearAndSelect);
}

void
StreamEditor::OnButtonAddStream()
{
	int bytes=0;
	QString plen, packets;
	switch(ui.ComboBoxStreamType->currentIndex())
	{
	case 0: // audio
	case 2: // data_stream
        bytes = ui.LineEditBytesPerFrame->text().toInt();
		break;
	case 1: // data_packet
        plen = ui.LineEditPacketLen->text();
        packets = ui.ComboBoxPacketsPerFrame->currentText();
        bytes = plen.toInt()*packets.toInt();
	}

    int row = streams->rowCount();
    cerr << row << endl;
    streams->setRowCount(row+1);
    streams->setItem(row, 0, new QStandardItem(ui.ComboBoxStream->currentText()));
    streams->setItem(row, 1, new QStandardItem(ui.ComboBoxStreamType->currentText()));
    streams->setItem(row, 2, new QStandardItem(QString::number(bytes)));
    streams->setItem(row, 3, new QStandardItem(plen));
    streams->setItem(row, 4, new QStandardItem(packets));

	ui.ComboBoxStream->removeItem(ui.ComboBoxStream->currentIndex());
	ui.ComboBoxStream->setCurrentIndex(0);
	ui.ComboBoxStreamType->setCurrentIndex(0);
	int availbytes = ui.TextLabelMSCBytesAvailable->text().toInt();
	ui.TextLabelMSCBytesAvailable->setText(QString::number(availbytes-bytes));
    ui.LineEditBytesPerFrame->setText(ui.TextLabelMSCBytesAvailable->text());
    ui.ButtonDeleteStream->setEnabled(true);
    //ui.treeViewStreams->setCurrentCell(row, 0, QItemSelectionModel::Rows|QItemSelectionModel::ClearAndSelect);
}

void
StreamEditor::OnButtonDeleteStream()
{

    int row = ui.treeViewStreams->currentIndex().row();

    QString stream = streams->item(row, 0)->text();
    int bytes = streams->item(row, 2)->text().toInt();
    streams->removeRow(row);
    ui.ComboBoxStream->addItem(stream);
    int availbytes = ui.TextLabelMSCBytesAvailable->text().toInt();
    ui.TextLabelMSCBytesAvailable->setText(QString::number(availbytes+bytes));
    ui.LineEditBytesPerFrame->setText(ui.TextLabelMSCBytesAvailable->text());
    if(streams->rowCount()<1)
        ui.ButtonDeleteStream->setEnabled(false);
    else // select a row near the deleted row
    {
        if(row>0)
            row--;
        else
            row++;
        //ui.treeViewStreams->setCurrentIndex(QModelIndex(row,0));
    }
}

void
StreamEditor::OnMSCCapacityChanged(int iBitsMSC)
{
	ui.TextLabelMSCBytesTotal->setText(QString::number(iBitsMSC/8));
	int used = 0;
	int n = streams->rowCount();
	for (int i=0; i<n; i++)
	{
	    used +=  streams->item(i, 2)->text().toInt();
	}
	ui.TextLabelMSCBytesAvailable->setText(QString::number(iBitsMSC/8 - used));
	ui.ComboBoxStreamType->setCurrentIndex(0);
}

AudioComponentEditor::AudioComponentEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void AudioComponentEditor::setupUi()
{
    ui.ComboBoxAudioStreamNo->clear();
	ui.ComboBoxAudioStreamNo->insertItem(0, "-", -1);
	ui.ComboBoxAudioStreamNo->insertItem(1, "0", 0);
	ui.ComboBoxAudioStreamNo->insertItem(2, "1", 1);
	ui.ComboBoxAudioStreamNo->insertItem(3, "2", 2);
	ui.ComboBoxAudioStreamNo->insertItem(4, "3", 3);

	textMessages = new QStandardItemModel();
	QStringList labels;
	labels << tr("Message");
	textMessages->setHorizontalHeaderLabels(labels);
	ui.treeViewTextMessages->setModel(textMessages);
	/* NOT implemented yet */
    ui.ComboBoxCodec->setEnabled(false);
    ui.ComboBoxAudioMode->setEnabled(false);
    ui.ComboBoxAudioBitrate->setEnabled(false);
    ui.CheckBoxSBR->setEnabled(false);

	connect(ui.ComboBoxAudioSource, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxSourceActivated(int)));
	connect(ui.PushButtonAudioSourceFileBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonFileBrowse()));
	connect(ui.CheckBoxAudioSourceIsFile, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxSourceIsFile(bool)));
	connect(ui.LineEditAudioSourceFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditFileChanged(const QString&)));

	connect(ui.PushButtonAddText, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddText()));
	connect(ui.PushButtonDeleteText, SIGNAL(clicked()),
		this, SLOT(OnPushButtonDeleteText()));
	connect(ui.PushButtonClearAllText, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllText()));
	connect(ui.CheckBoxEnableTextMessage, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableTextMessage(bool)));
}

void
AudioComponentEditor::GetFrom(const CDRMTransmitterInterface& DRMTransmitter)
{
	vector<string> vecAudioDevices;
	DRMTransmitter.GetSoundInChoices(vecAudioDevices);
	ui.ComboBoxAudioSource->clear();
	for (size_t t = 0; t < vecAudioDevices.size(); t++)
	{
		ui.ComboBoxAudioSource->insertItem(t, QString(vecAudioDevices[t].c_str()), t);
	}
	ui.ComboBoxAudioSource->setCurrentIndex(0);
    string fn = DRMTransmitter.GetReadFromFile();
    ui.LineEditAudioSourceFile->setText(fn.c_str());
    if(fn == "")
    {
        int iAudSrc = DRMTransmitter.GetSoundInInterface();
        if((iAudSrc>=0) && (iAudSrc<ui.ComboBoxAudioSource->count()))
        {
            ui.ComboBoxAudioSource->setCurrentIndex(iAudSrc);
        }
        else
        {
            int n = ui.ComboBoxAudioSource->count();
            if(n>0)
                ui.ComboBoxAudioSource->setCurrentIndex(n-1);
        }
    }

	map<int,CAudioParam> a;
	DRMTransmitter.GetAudio(a);
    for(map<int,CAudioParam>::const_iterator i=a.begin(); i!=a.end(); i++)
    {
        ui.ComboBoxAudioStreamNo->setCurrentIndex(ui.ComboBoxAudioStreamNo->findText(QString::number(i->first)));
        if(i->second.bTextflag == true)
        {
            /* Activate text message */
            EnableTextMessage(true);
            vector<string> msg;
            DRMTransmitter.GetTextMessages(msg);
            for(size_t j=0; j<msg.size(); j++)
            {
                QList<QStandardItem*> l;
                l.append(new QStandardItem(QString::fromUtf8(msg[j].c_str())));
                textMessages->appendRow(l);
            }
        }
        else
        {
            EnableTextMessage(false);
        }
    }
}

void
AudioComponentEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter) const
{
    int iStreamNo = ui.ComboBoxAudioStreamNo->currentText().toInt();

	if(ui.CheckBoxAudioSourceIsFile->isChecked())
	{
		const char * fname = ui.LineEditAudioSourceFile->text().toUtf8().constData();
		cerr << fname << endl;
		DRMTransmitter.SetReadFromFile(fname);
	}
	else
	{
		int iAudSrc = ui.ComboBoxAudioSource->currentIndex();
		DRMTransmitter.SetSoundInInterface(iAudSrc);
	}

	map<int,CAudioParam> a;

	a[iStreamNo].bTextflag = ui.CheckBoxEnableTextMessage->isChecked();

	if(a[iStreamNo].bTextflag)
	{
		DRMTransmitter.ClearTextMessages();
        for (int i=0; i<textMessages->rowCount(); i++)
        {
			DRMTransmitter.AddTextMessage(textMessages->item(i,0)->text().toUtf8().constData());
        }
	}

	/* TODO - let the user choose */
	bool b = true;
	if (b) // DRMTransmitter.GetParameters()->GetStreamLen(0) > 7000)
		a[iStreamNo].eAudioSamplRate = CAudioParam::AS_24KHZ;
	else
		a[iStreamNo].eAudioSamplRate = CAudioParam::AS_12KHZ;
	DRMTransmitter.PutAudio(a);
}

void AudioComponentEditor::OnComboBoxSourceActivated(int)
{
}

void AudioComponentEditor::OnButtonFileBrowse()
{
	QString s( QFileDialog::getOpenFileName(NULL,
		QString::null, "Wave Files (*.wav)") );
	if ( s.isEmpty() )
		return;
    ui.LineEditAudioSourceFile->setText(s);
}

void AudioComponentEditor::OnLineEditFileChanged(const QString&)
{
}

void AudioComponentEditor::OnToggleCheckBoxSourceIsFile(bool)
{
}

void AudioComponentEditor::OnToggleCheckBoxEnableTextMessage(bool bState)
{
	EnableTextMessage(bState);
}

void AudioComponentEditor::EnableTextMessage(const bool bFlag)
{
    ui.CheckBoxEnableTextMessage->setChecked(bFlag);
    ui.PushButtonAddText->setEnabled(bFlag);
    ui.PushButtonDeleteText->setEnabled(bFlag);
    ui.PushButtonClearAllText->setEnabled(bFlag);
}

void AudioComponentEditor::OnPushButtonAddText()
{
	QString msg = ui.LineEditTextMessage->text();
	if(msg != "")
	{
	    QList<QStandardItem*> l;
	    l.push_back(new QStandardItem(msg));
	    textMessages->appendRow(l);
	}
}

void AudioComponentEditor::OnPushButtonDeleteText()
{
    textMessages->removeRow(ui.treeViewTextMessages->currentIndex().row());
}

void AudioComponentEditor::OnButtonClearAllText()
{
    textMessages->removeRows(0, textMessages->rowCount());
}

DataComponentEditor::DataComponentEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void DataComponentEditor::setupUi()
{
    ui.ComboBoxDataStreamNo->clear();
	ui.ComboBoxDataStreamNo->insertItem(0, "-", -1);
	ui.ComboBoxDataStreamNo->insertItem(1, "0", 0);
	ui.ComboBoxDataStreamNo->insertItem(2, "1", 1);
	ui.ComboBoxDataStreamNo->insertItem(3, "2", 2);
	ui.ComboBoxDataStreamNo->insertItem(4, "3", 3);

    ui.ComboBoxDataPacketId->clear();
	ui.ComboBoxDataPacketId->insertItem(0, "-", -1);
	ui.ComboBoxDataPacketId->insertItem(1, "0", 0);
	ui.ComboBoxDataPacketId->insertItem(2, "1", 1);
	ui.ComboBoxDataPacketId->insertItem(3, "2", 2);
	ui.ComboBoxDataPacketId->insertItem(4, "3", 3);

    pictures = new QStandardItemModel();
	QStringList labels;
	labels << tr("File Name") << tr("Size [KB]") << tr("Full Path");
	pictures->setHorizontalHeaderLabels(labels);
	ui.treeViewPictures->setModel(pictures);

	connect(ui.PushButtonAddFile, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddFileName()));
	connect(ui.PushButtonClearAllFileNames, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllFileNames()));
}

void
DataComponentEditor::GetFrom(const CDRMTransmitterInterface& DRMTransmitter)
{
	map<int, map<int, CDataParam> > d;
	DRMTransmitter.GetData(d);
	for(map<int, map<int, CDataParam> >::const_iterator i=d.begin();
                                                        i!=d.end(); i++)
	{
	    int stream = i->first;
	    for(map<int, CDataParam>::const_iterator j=i->second.begin(); j!=i->second.end(); j++)
	    {
	        int packetId = j->first;
            if(j->second.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
            {
                // we only do slideshow!
                int idx = ui.ComboBoxDataStreamNo->findData(stream);
                ui.ComboBoxDataStreamNo->setCurrentIndex(idx);
                idx = ui.ComboBoxDataPacketId->findData(packetId);
                ui.ComboBoxDataPacketId->setCurrentIndex(idx);
                /* file names for data application */
                map<string,string> m;
                DRMTransmitter.GetPics(m);

                pictures->setRowCount(0);
                for (map<string,string>::const_iterator p=m.begin(); p!=m.end(); p++)
                {
                    AddSlide(p->first.c_str());
                }
                break; // one is enough!
            }
	    }
	}
}

void
DataComponentEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter) const
{

    if(ui.ComboBoxDataStreamNo->currentText() == "-")
        return;

    if(ui.ComboBoxDataPacketId->currentText() == "-")
        return;

    int iStreamNo = ui.ComboBoxDataStreamNo->itemData(ui.ComboBoxDataStreamNo->currentIndex()).toInt();
    int iPacketId = ui.ComboBoxDataPacketId->itemData(ui.ComboBoxDataPacketId->currentIndex()).toInt();

	map<int, map<int, CDataParam> > dataParam;

	/* Init SlideShow application */
	dataParam[iStreamNo][iPacketId].eAppDomain = CDataParam::AD_DAB_SPEC_APP;
	dataParam[iStreamNo][iPacketId].ePacketModInd = PM_PACKET_MODE;
	dataParam[iStreamNo][iPacketId].eDataUnitInd = CDataParam::DU_DATA_UNITS;

	DRMTransmitter.PutData(dataParam);

	/* file names for data application */
	DRMTransmitter.ClearPics();

	for(int i=0; i<pictures->rowCount(); i++)
	{
		/* Complete file path is in third column */
		const QString strFileName = pictures->item(i, 3)->text();

		/* Extract format string */
		QFileInfo FileInfo(strFileName);
		const QString strFormat = FileInfo.suffix();

		DRMTransmitter.AddPic(strFileName.toUtf8().constData(), strFormat.toUtf8().constData());
	}
}

void DataComponentEditor::AddSlide(const QString& path)
{
    QFileInfo FileInfo(path);

    QList<QStandardItem*> l;
    l.append(new QStandardItem(FileInfo.fileName()));
    l.append(new QStandardItem(QString().setNum((float) FileInfo.size() / 1000.0, 'f', 2)));
    l.append(new QStandardItem(FileInfo.filePath()));
    pictures->appendRow(l);
}

void DataComponentEditor::OnPushButtonAddFileName()
{
	/* Show "open file" dialog. Let the user select more than one file */
	QStringList list = QFileDialog::getOpenFileNames(NULL,
		tr("Image Files (*.png *.jpg *.jpeg *.jfif)"));

	/* Check if user not hit the cancel button */
	if (!list.isEmpty())
	{
		/* Insert all selected file names */
		for (QStringList::Iterator it = list.begin(); it != list.end(); it++)
		{
		    AddSlide(*it);
		}
	}
}

void DataComponentEditor::OnButtonClearAllFileNames()
{
	/* Clear list box for file names */
	pictures->setRowCount(0);
}

void DataComponentEditor::EnableData(const bool bFlag)
{
	if (bFlag == true)
	{
		/* Enable data controls */
		ui.treeViewPictures->setEnabled(true);
		ui.PushButtonClearAllFileNames->setEnabled(true);
		ui.PushButtonAddFile->setEnabled(true);
	}
	else
	{
		/* Disable data controls */
		ui.treeViewPictures->setEnabled(false);
		ui.PushButtonClearAllFileNames->setEnabled(false);
		ui.PushButtonAddFile->setEnabled(false);
	}
}

ServicesEditor::ServicesEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void ServicesEditor::setupUi()
{
	/* Language */
	for (int i = 0; i < LEN_TABLE_LANGUAGE_CODE; i++)
		ui.ComboBoxFACLanguage->insertItem(i, strTableLanguageCode[i].c_str(), i);

	/* Program type */
	for (int i = 0; i < LEN_TABLE_PROG_TYPE_CODE; i++)
		ui.ComboBoxProgramType->insertItem(i, strTableProgTypCod[i].c_str(), i);

	/* services */
	ui.ComboBoxStreamType->addItem(tr("audio"));
	ui.ComboBoxStreamType->addItem(tr("data packet"));
	ui.ComboBoxStreamType->addItem(tr("data stream"));
	ui.ComboBoxStream->insertItem(0, "0", 0);
	ui.ComboBoxStream->insertItem(1, "1", 1);
	ui.ComboBoxStream->insertItem(2, "2", 2);
	ui.ComboBoxStream->insertItem(3, "3", 3);

    services = new QStandardItemModel();
	QStringList labels;
	labels << tr("short ID") << tr("Label") << tr("Service ID")
            << tr("FAC Lang") << tr("SDC Lang") << tr("Country") << tr("Type")
            << tr("Audio Stream") << tr("Data Stream") << tr("Packet ID");
	services->setHorizontalHeaderLabels(labels);
	ui.treeViewServices->setModel(services);

    ui.ComboBoxServiceAudioStream->clear();
    ui.ComboBoxServiceDataStream->clear();
	for(int i=0; i<4; i++)
	{
		QString s("0");
		s[0] = '0'+i;
		ui.ComboBoxShortID->insertItem(i,s,i);
		ui.ComboBoxServiceAudioStream->insertItem(i,s,i);
		ui.ComboBoxServiceDataStream->insertItem(i,s,i);
	}

    ui.ComboBoxServicePacketID->clear();
	ui.ComboBoxServicePacketID->insertItem(0, "0", 0);
	ui.ComboBoxServicePacketID->insertItem(1, "0", 0);

    ui.ComboBoxAppType->clear();
    ui.ComboBoxAppType->addItem(tr("Normal"));
    ui.ComboBoxAppType->addItem(tr("Engineering Test"));
	for (int t = 1; t < 31; t++)
	{
	    QString reserved = QString(tr("Reserved"))+" (%1)";
		ui.ComboBoxAppType->addItem(reserved.arg(t));
	}

	connect(ui.PushButtonAddService, SIGNAL(clicked()),
		this, SLOT(OnButtonAdd()));
	connect(ui.PushButtonDeleteService, SIGNAL(clicked()),
		this, SLOT(OnButtonDelete()));
	connect(ui.LineEditServiceLabel, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedLabel(const QString&)));
	connect(ui.LineEditServiceID, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceID(const QString&)));
	connect(ui.treeViewServices, SIGNAL(clicked(const QModelIndex&)),
		this, SLOT(OnItemClicked(const QModelIndex&)));
}

void
ServicesEditor::GetFrom(const CDRMTransmitterInterface& DRMTransmitter)
{
	vector<CService> Service;
	int a,d;
	DRMTransmitter.GetServices(Service, a, d);
	for(size_t i=0; i<Service.size(); i++)
	{
	    QList<QStandardItem*> l;
        l.push_back(new QStandardItem(QString::number(i)));
        l.push_back(new QStandardItem(QString::fromUtf8(Service[i].strLabel.c_str())));
        l.push_back(new QStandardItem(QString::number(ulong(Service[i].iServiceID), 16)));
        l.push_back(new QStandardItem(ui.ComboBoxFACLanguage->itemText(Service[i].iLanguage)));
        l.push_back(new QStandardItem(Service[i].strLanguageCode.c_str()));
        l.push_back(new QStandardItem(Service[i].strCountryCode.c_str()));
		if(Service[i].iAudioStream!=STREAM_ID_NOT_USED)
		{
		    /* audio overrides data */
            l.push_back(new QStandardItem(ui.ComboBoxProgramType->itemText(Service[i].iServiceDescr)));
		}
		else if(Service[i].iDataStream != STREAM_ID_NOT_USED)
		{
		    /* audio overrides data */
            l.push_back(new QStandardItem(ui.ComboBoxAppType->itemText(Service[i].iServiceDescr)));
		}
		else
		{
            l.push_back(new QStandardItem("")); // shouldn't happen
		}
		if(Service[i].iAudioStream!=STREAM_ID_NOT_USED)
		{
			l.push_back(new QStandardItem(QString::number(Service[i].iAudioStream)));
		}
		else
		{
            l.push_back(new QStandardItem(""));
		}
		if(Service[i].iDataStream != STREAM_ID_NOT_USED)
		{
			l.push_back(new QStandardItem(QString::number(Service[i].iDataStream)));
            l.push_back(new QStandardItem(QString::number(Service[i].iPacketID)));
		}
		else
		{
            l.push_back(new QStandardItem(""));
            l.push_back(new QStandardItem(""));
		}
        services->appendRow(l);
	}
	//ui.ListViewServices->setSelected(ui.ListViewServices->firstChild(), true);
}

void
ServicesEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter) const
{
	int iNumDataServices=0;
	int iNumAudioServices=0;

	vector<CService> s(4);
	for (int i=0; i<services->rowCount(); i++)
	{
		int iShortID = services->item(i, 0)->text().toUInt();
		CService Service;
		Service.strLabel = services->item(i, 1)->text().toUtf8().constData();
		Service.iServiceID = services->item(i, 2)->text().toULong(NULL, 16);
		QString lang = services->item(i, 3)->text();
        Service.iLanguage = ui.ComboBoxFACLanguage->findText(lang);
		Service.strLanguageCode = services->item(i, 4)->text().toUtf8().constData();
		Service.strCountryCode = services->item(i, 5)->text().toUtf8().constData();
        QString type = services->item(i, 6)->text();
		QString sA = services->item(i, 7)->text();
		QString sD = services->item(i, 8)->text();
		QString sP = services->item(i, 9)->text();
		if(sA=="-")
		{
			Service.iAudioStream = STREAM_ID_NOT_USED;
			iNumDataServices++;
			Service.eAudDataFlag = SF_DATA;
            Service.iServiceDescr = ui.ComboBoxAppType->findText(type);
		}
		else
		{
			Service.iAudioStream = sA.toUInt();
			iNumAudioServices++;
			Service.eAudDataFlag = SF_AUDIO;
            Service.iServiceDescr = ui.ComboBoxProgramType->findText(type);
		}
		if(sD=="-")
			Service.iDataStream = STREAM_ID_NOT_USED;
		else
			Service.iDataStream = sD.toUInt();
		if(sP=="-")
			Service.iPacketID = 4;
		else
			Service.iPacketID = sP.toUInt();
		s[iShortID] = Service;
	}
	DRMTransmitter.PutServices(s, iNumAudioServices, iNumDataServices);
}

void ServicesEditor::OnTextChangedServiceID(const QString& strID)
{
	(void)strID; // TODO
}

void ServicesEditor::OnTextChangedLabel(const QString& strLabel)
{
	(void)strLabel; // TODO
}

void ServicesEditor::OnButtonAdd()
{
    QList<QStandardItem*> l;
    l.push_back(new QStandardItem(ui.ComboBoxShortID->currentText()));
    l.push_back(new QStandardItem(ui.LineEditServiceLabel->text()));
    l.push_back(new QStandardItem(ui.LineEditServiceID->text()));
    l.push_back(new QStandardItem(ui.ComboBoxFACLanguage->currentText()));
    l.push_back(new QStandardItem(ui.LineEditSDCLanguage->text()));
    l.push_back(new QStandardItem(ui.LineEditCountry->text()));
    if(ui.CheckBoxAudioComp->isChecked())
    {
        /* audio overrides data */
        l.push_back(new QStandardItem(ui.ComboBoxProgramType->currentText()));
    }
    else if(ui.CheckBoxDataComp->isChecked())
    {
        l.push_back(new QStandardItem(ui.ComboBoxAppType->currentText()));
    }
    else
    {
        l.push_back(new QStandardItem(""));
    }
    if(ui.CheckBoxAudioComp->isChecked())
    {
        l.push_back(new QStandardItem(ui.ComboBoxServiceAudioStream->currentText()));
    }
    else
    {
        l.push_back(new QStandardItem(""));
    }
    if(ui.CheckBoxDataComp->isChecked())
    {
        l.push_back(new QStandardItem(ui.ComboBoxServiceDataStream->currentText()));
        l.push_back(new QStandardItem(ui.ComboBoxServicePacketID->currentText()));
    }
    else
    {
        l.push_back(new QStandardItem(""));
        l.push_back(new QStandardItem(""));
    }
    services->appendRow(l);
	ui.ComboBoxShortID->setCurrentIndex(0);
}

void ServicesEditor::OnButtonDelete()
{
    services->removeRow(ui.treeViewServices->currentIndex().row());
	ui.ComboBoxShortID->setCurrentIndex(0);
}

void ServicesEditor::OnItemClicked(const QModelIndex& index)
{
    int row = index.row();
    ui.ComboBoxShortID->setCurrentIndex(ui.ComboBoxShortID->findText(services->item(row,0)->text()));
	ui.LineEditServiceLabel->setText(services->item(row,1)->text());
	ui.LineEditServiceID->setText(services->item(row,2)->text());
    ui.ComboBoxFACLanguage->setCurrentIndex(ui.ComboBoxFACLanguage->findText(services->item(row,3)->text()));
	ui.LineEditSDCLanguage->setText(services->item(row,4)->text());
	ui.LineEditCountry->setText(services->item(row,5)->text());
	if(services->item(row,8)->text() != "")
	{
	    ui.CheckBoxDataComp->setChecked(true);
        ui.ComboBoxAppType->setCurrentIndex(ui.ComboBoxAppType->findText(services->item(row,6)->text()));
        ui.ComboBoxServiceDataStream->setCurrentIndex(ui.ComboBoxServiceDataStream->findText(services->item(row,7)->text()));
        ui.ComboBoxServicePacketID->setCurrentIndex(ui.ComboBoxServicePacketID->findText(services->item(row,8)->text()));
	}
	else
	{
	    ui.CheckBoxDataComp->setChecked(false);
	}
	if(services->item(row,7)->text() != "")
	{
	    ui.CheckBoxAudioComp->setChecked(true);
        ui.ComboBoxProgramType->setCurrentIndex(ui.ComboBoxProgramType->findText(services->item(row,6)->text()));
        ui.ComboBoxServiceAudioStream->setCurrentIndex(ui.ComboBoxServiceAudioStream->findText(services->item(row,7)->text()));
	}
	else
	{
	    ui.CheckBoxAudioComp->setChecked(false);
	}
}

COFDMEditor::COFDMEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void COFDMEditor::setupUi()
{
    outputGroup = new QButtonGroup(this);
    outputGroup->addButton(ui.RadioButtonOutReal, OF_REAL_VAL);
    outputGroup->addButton(ui.RadioButtonOutIQPos, OF_IQ_POS);
    outputGroup->addButton(ui.RadioButtonOutIQNeg, OF_IQ_NEG);
    outputGroup->addButton(ui.RadioButtonOutEP, OF_EP);

    destinations = new QStandardItemModel();
	QStringList labels;
	labels << tr("File or Device");
	destinations->setHorizontalHeaderLabels(labels);
    ui.treeViewCOFDMOutputs->setModel(destinations);

	connect(ui.PushButtonCOFDMAddAudio, SIGNAL(clicked()),
		this, SLOT(OnButtonAddAudio()));
	connect(ui.PushButtonCOFDMAddFile, SIGNAL(clicked()),
		this, SLOT(OnButtonAddFile()));
	connect(ui.PushButtonCOFDMDeleteSelected, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteSelected()));
	connect(ui.PushButtonCOFDMBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonBrowse()));
	connect(ui.ComboBoxCOFDMdest, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxDestActivated(int)));
	connect(ui.LineEditSndCrdIF, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedSndCrdIF(const QString&)));
	connect(ui.LineEditCOFDMOutputFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditFileChanged(const QString&)));
	connect(ui.treeViewCOFDMOutputs, SIGNAL(clicked(const QModelIndex&)),
		this, SLOT(OnItemClicked(const QModelIndex&)));
}

void
COFDMEditor::GetFrom(const CDRMTransmitterInterface& DRMTransmitter)
{

	/* Output mode (real valued, I / Q or E / P) */
    outputGroup->button(DRMTransmitter.GetOutputFormat())->setChecked(true);

	/* Sound card IF */
	ui.LineEditSndCrdIF->setText(QString().number(DRMTransmitter.GetCarrierOffset(), 'f', 2));

	/* Get sound device names */
	vector<string> vecAudioDevices;
	DRMTransmitter.GetSoundOutChoices(vecAudioDevices);
	for (size_t t = 0; t < vecAudioDevices.size(); t++)
	{
		ui.ComboBoxCOFDMdest->addItem(QString(vecAudioDevices[t].c_str()));
	}
	ui.ComboBoxCOFDMdest->setCurrentIndex(0);
	destinations->setRowCount(0);
	vector<string> COFDMOutputs;
	DRMTransmitter.GetCOFDMOutputs(COFDMOutputs);
	for(size_t i=0; i<COFDMOutputs.size(); i++)
	{
        QList<QStandardItem*> l;
        l.append(new QStandardItem(COFDMOutputs[i].c_str()));
        destinations->appendRow(l);
	}
}

void
COFDMEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter) const
{
	DRMTransmitter.SetCarrierOffset(ui.LineEditSndCrdIF->text().toFloat());
	DRMTransmitter.SetOutputFormat(EOutFormat(outputGroup->checkedId()));
	vector<string> COFDMOutputs;
	for(int i=0; i<destinations->rowCount(); i++)
	{
		/* Complete file path is in third column */
		const QString s = destinations->item(i, 0)->text();
		COFDMOutputs.push_back(s.toUtf8().constData());
	}
	DRMTransmitter.SetCOFDMOutputs(COFDMOutputs);
}

void COFDMEditor::OnComboBoxDestActivated(int)
{
}

void COFDMEditor::OnLineEditFileChanged(const QString&)
{
}

void COFDMEditor::OnComboBoxFileDestActivated(int)
{
}

void COFDMEditor::OnTextChangedSndCrdIF(const QString&)
{
}

void
COFDMEditor::OnButtonAddAudio()
{
    QList<QStandardItem*> l;
    l.append(new QStandardItem(ui.ComboBoxCOFDMdest->currentText()));
    destinations->appendRow(l);
}

void COFDMEditor::OnButtonAddFile()
{
	QString file = ui.LineEditCOFDMOutputFile->text();
	if(file != "")
	{
        QList<QStandardItem*> l;
        l.append(new QStandardItem(file));
        destinations->appendRow(l);
	}
}

void COFDMEditor::OnButtonDeleteSelected()
{
    int row = ui.treeViewCOFDMOutputs->currentIndex().row();
    destinations->removeRow(row);
}

void COFDMEditor::OnItemClicked(const QModelIndex& index)
{
    QString s = destinations->item(index.row(), 0)->text();
    int i = ui.ComboBoxCOFDMdest->findText(s);
    if(i>=0)
        ui.ComboBoxCOFDMdest->setCurrentIndex(i);
    else
        ui.LineEditCOFDMOutputFile->setText(s);
}

void COFDMEditor::OnButtonBrowse()
{
	QString s( QFileDialog::getSaveFileName(NULL,
		"cofdm.wav", "Wave Files (*.wav)" ) );
	if ( s.isEmpty() )
		return;
	ui.LineEditCOFDMOutputFile->setText(s);
}

MDIInputEditor::MDIInputEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u),vecIpIf()
{
}

void MDIInputEditor::setupUi()
{
	ui.LineEditMDIinGroup->setEnabled(false);
	ui.LineEditMDIinGroup->setInputMask("000.000.000.000;_");
	ui.LineEditMDIinPort->setInputMask("00009;_");

	GetNetworkInterfaces(vecIpIf);
	for(size_t i=0; i<vecIpIf.size(); i++)
	{
		ui.ComboBoxMDIinInterface->addItem(vecIpIf[i].name.c_str());
	}

	connect(ui.PushButtonMDIInBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonBrowse()));
	connect(ui.CheckBoxMDIinMcast, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxMcast(bool)));
	connect(ui.CheckBoxReadMDIFile, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxReadFile(bool)));
	connect(ui.ComboBoxMDIinInterface, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxInterfaceActivated(int)));
	connect(ui.LineEditMDIinPort, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditPortChanged(const QString&)));
	connect(ui.LineEditMDIinGroup, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditGroupChanged(const QString&)));
	connect(ui.LineEditMDIInputFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditFileChanged(const QString&)));
}

void
MDIInputEditor::GetFrom(const CDRMTransmitterInterface& DRMTransmitter)
{
	QString addr = DRMTransmitter.GetMDIIn().c_str();
	QFileInfo f(addr);

    ui.CheckBoxMDIinMcast->setChecked(false);

	if(f.exists())
	{
		ui.CheckBoxReadMDIFile->setChecked(true);
		ui.LineEditMDIInputFile->setText(addr);
		ui.LineEditMDIinPort->setText("");
        ui.ComboBoxMDIinInterface->setCurrentIndex(ui.ComboBoxMDIinInterface->findText("any"));
		ui.LineEditMDIinGroup->setText("");
	}
	else
	{
		ui.CheckBoxReadMDIFile->setChecked(false);
		ui.LineEditMDIInputFile->setText("");
		QString port,group,ifname="any";

		QStringList parts = addr.split(":", QString::KeepEmptyParts);
		switch(parts.count())
		{
		case 1:
			port = parts[0];
			break;
		case 2:
			group = parts[0];
			port = parts[1];
            ui.CheckBoxMDIinMcast->setChecked(true); // TODO - check address type
			break;
		case 3:
            for(size_t i=0; i<vecIpIf.size(); i++)
            {
                if(parts[0].toUInt()==vecIpIf[i].addr)
                {
                    ifname = vecIpIf[i].name.c_str();
                }
            }
			group = parts[1];
			port = parts[2];
            ui.CheckBoxMDIinMcast->setChecked(true);
			break;
		}
        ui.LineEditMDIinGroup->setText(group);
        ui.LineEditMDIinPort->setText(port);
        int item = ui.ComboBoxMDIinInterface->findText(ifname);
        ui.ComboBoxMDIinInterface->setCurrentIndex(item);
	}
}

void
MDIInputEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter) const
{
	if(ui.CheckBoxReadMDIFile->isChecked())
	{
		DRMTransmitter.SetMDIIn(ui.LineEditMDIInputFile->text().toUtf8().constData());
	}
	else
	{
		QString port = ui.LineEditMDIinPort->text();
		QString group = ui.LineEditMDIinGroup->text();
		QString iface = ui.ComboBoxMDIinInterface->currentText();
		QString addr=port;
		if(ui.CheckBoxMDIinMcast->isChecked())
			addr = group+":"+addr;
		if(iface!="any")
		{
			if(!ui.CheckBoxMDIinMcast->isChecked())
				addr = ":"+addr;
			addr = iface+":"+addr;
		}
		DRMTransmitter.SetMDIIn(addr.toUtf8().constData());
	}
}


void
MDIInputEditor::OnLineEditPortChanged(const QString&)
{
}

void
MDIInputEditor::OnToggleCheckBoxMcast(bool)
{
}

void
MDIInputEditor::OnLineEditGroupChanged(const QString&)
{
}

void
MDIInputEditor::OnComboBoxInterfaceActivated(int)
{
}

void
MDIInputEditor::OnLineEditFileChanged(const QString&)
{
}

void
MDIInputEditor::OnButtonBrowse()
{
	QString s( QFileDialog::getOpenFileName(NULL,
		"out.pcap", "Capture Files (*.pcap)" ) );
	if ( s.isEmpty() )
		return;
    ui.LineEditMDIInputFile->setText(s);
}

void
MDIInputEditor::OnToggleCheckBoxReadFile(bool)
{
}
