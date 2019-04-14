/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#include "TransmDlg.h"
#if QT_VERSION < 0x040000
# include <qevent.h>
# include <qpopupmenu.h>
# include <qbuttongroup.h>
# include <qmultilineedit.h>
# include <qlistview.h>
# include <qfiledialog.h>
# include <qpopupmenu.h>
# include <qprogressbar.h>
# include <qwhatsthis.h>
#else
# include <QCloseEvent>
# include <QTreeWidget>
# include <QFileDialog>
# include <QTextEdit>
# include <QProgressBar>
# include <QHeaderView>
# include <QWhatsThis>
# define CHECK_PTR(x) Q_CHECK_PTR(x)
# include "SoundCardSelMenu.h"
#endif


/* TODO to be moved in DialogUtil.h */
#if QT_VERSION < 0x040000
# if QT_VERSION < 0x030000
#  define ButtonGroupGetCurrentId(c) (c->selected()?c->id(c->selected()):int(-1))
# else
#  define ButtonGroupGetCurrentId(c) (c)->selectedId()
# endif
# define ComboBoxClear(c) (c)->clear()
# define ComboBoxInsertItem(c, t, i) (c)->insertItem(t, i)
# define ComboBoxSetCurrentItem(c, i) (c)->setCurrentItem(i)
# define ProgressBarSetRange(c, r) (c)->setTotalSteps(r)
# define ProgressBarSetValue(c, v) (c)->setProgress(v)
# define FromUtf8(s) QString::fromUtf8(s)
# define TextEditClear(c) (c)->clear()
# define TextEditClearModified(c) (c)->setEdited(FALSE)
# define TextEditIsModified(c) (c)->edited()
# define ToUtf8(s) (s).latin1()
# define WhatsThis(c, s) QWhatsThis::add(c ,s)
#else
# define ButtonGroupGetCurrentId(c) (c)->checkedId()
# define ComboBoxClear(c) (c)->clear()
# define ComboBoxInsertItem(c, t, i) (c)->insertItem(i, t)
# define ComboBoxSetCurrentItem(c, i) (c)->setCurrentIndex(i)
# define ProgressBarSetRange(c, r) (c)->setRange(0, r)
# define ProgressBarSetValue(c, v) (c)->setValue(v)
# define FromUtf8(s) QString::fromUtf8(s)
# define TextEditClear(c) (c)->clear()
# define TextEditClearModified(c)
# define TextEditIsModified(c) !(c)->toPlainText().isEmpty()
# define ToUtf8(s) (s).toUtf8().constData()
# define WhatsThis(c, s) (c)->setWhatsThis(s)
#endif


TransmDialog::TransmDialog(CSettings& Settings,
	QWidget* parent, const char* name, bool modal, Qt::WindowFlags f)
	:
	TransmDlgBase(parent, name, modal, f),
	TransThread(Settings),
	DRMTransmitter(TransThread.DRMTransmitter),
	Settings(*DRMTransmitter.GetSettings()),
#ifdef ENABLE_TRANSM_CODECPARAMS
	CodecDlg(NULL),
#endif
	bIsStarted(FALSE),
	vecstrTextMessage(1) /* 1 for new text */,
	iIDCurrentText(0), iServiceDescr(0), bCloseRequested(FALSE)
#ifdef ENABLE_TRANSM_CODECPARAMS
	, iButtonCodecState(0)
#endif
{
	/* Load transmitter settings */
	DRMTransmitter.LoadSettings();

	/* Recover window size and position */
	CWinGeom s;
	Settings.Get("Transmit Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

#if QT_VERSION < 0x040000
	/* Set the window title */
	SetDialogCaption(this, tr("Dream DRM Transmitter"));
#endif

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Set controls to custom behavior */
#if QT_VERSION < 0x040000
	MultiLineEditTextMessage->setWordWrap(QMultiLineEdit::WidgetWidth);
#endif

	/* Init controls with default settings */
	ButtonStartStop->setText(tr("&Start"));
	OnButtonClearAllText();
	UpdateMSCProtLevCombo();

	/* Init progress bar for input signal level */
	ProgrInputLevel->setScale(-50.0, 0.0);
    ProgrInputLevel->setOrientation(Qt::Horizontal);
    ProgrInputLevel->setScalePosition(QwtThermo::LeadingScale);
	ProgrInputLevel->setAlarmLevel(-5.0);
	QPalette newPalette = palette();
	newPalette.setColor(QPalette::Base, newPalette.color(QPalette::Window));
	newPalette.setColor(QPalette::ButtonText, QColor(0, 190, 0));
	newPalette.setColor(QPalette::Highlight,  QColor(255, 0, 0));
	ProgrInputLevel->setPalette(newPalette);

	/* Init progress bar for current transmitted picture */
	ProgressBarSetRange(ProgressBarCurPict, 100);
	ProgressBarSetValue(ProgressBarCurPict, 0);
	TextLabelCurPict->setText("");

	/* Output mode (real valued, I / Q or E / P) */
	switch (TransThread.DRMTransmitter.GetTransData()->GetIQOutput())
	{
	case CTransmitData::OF_REAL_VAL:
		RadioButtonOutReal->setChecked(TRUE);
		break;

	case CTransmitData::OF_IQ_POS:
		RadioButtonOutIQPos->setChecked(TRUE);
		break;

	case CTransmitData::OF_IQ_NEG:
		RadioButtonOutIQNeg->setChecked(TRUE);
		break;

	case CTransmitData::OF_EP:
		RadioButtonOutEP->setChecked(TRUE);
		break;
	}

	/* Output High Quality I/Q */
	CheckBoxHighQualityIQ->setEnabled(TransThread.DRMTransmitter.GetTransData()->GetIQOutput() != CTransmitData::OF_REAL_VAL);
	CheckBoxHighQualityIQ->setChecked(TransThread.DRMTransmitter.GetTransData()->GetHighQualityIQ());

	/* Output Amplified */
	CheckBoxAmplifiedOutput->setEnabled(TransThread.DRMTransmitter.GetTransData()->GetIQOutput() != CTransmitData::OF_EP);
	CheckBoxAmplifiedOutput->setChecked(TransThread.DRMTransmitter.GetTransData()->GetAmplifiedOutput());

	/* Don't lock the Parameter object since the working thread is stopped */
	CParameter& Parameters = *DRMTransmitter.GetParameters();

	/* Transmission of current time */
	switch (Parameters.eTransmitCurrentTime)
	{
	case CParameter::CT_OFF:
		RadioButtonCurTimeOff->setChecked(TRUE);
		break;

	case CParameter::CT_LOCAL:
		RadioButtonCurTimeLocal->setChecked(TRUE);
		break;

	case CParameter::CT_UTC:
		RadioButtonCurTimeUTC->setChecked(TRUE);
		break;

	case CParameter::CT_UTC_OFFSET:
		RadioButtonCurTimeUTCOffset->setChecked(TRUE);
	}

	/* Robustness mode */
	switch (Parameters.GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		RadioButtonRMA->setChecked(TRUE);
		break;

	case RM_ROBUSTNESS_MODE_B:
		RadioButtonRMB->setChecked(TRUE);
		break;

	case RM_ROBUSTNESS_MODE_C:
		RadioButtonBandwidth45->setEnabled(FALSE);
		RadioButtonBandwidth5->setEnabled(FALSE);
		RadioButtonBandwidth9->setEnabled(FALSE);
		RadioButtonBandwidth18->setEnabled(FALSE);
		RadioButtonRMC->setChecked(TRUE);
		break;

	case RM_ROBUSTNESS_MODE_D:
		RadioButtonBandwidth45->setEnabled(FALSE);
		RadioButtonBandwidth5->setEnabled(FALSE);
		RadioButtonBandwidth9->setEnabled(FALSE);
		RadioButtonBandwidth18->setEnabled(FALSE);
		RadioButtonRMD->setChecked(TRUE);
		break;

	case RM_NO_MODE_DETECTED:
		break;
	}

	/* Bandwidth */
	switch (Parameters.GetSpectrumOccup())
	{
	case SO_0:
		RadioButtonBandwidth45->setChecked(TRUE);
		break;

	case SO_1:
		RadioButtonBandwidth5->setChecked(TRUE);
		break;

	case SO_2:
		RadioButtonBandwidth9->setChecked(TRUE);
		break;

	case SO_3:
		RadioButtonBandwidth10->setChecked(TRUE);
		break;

	case SO_4:
		RadioButtonBandwidth18->setChecked(TRUE);
		break;

	case SO_5:
		RadioButtonBandwidth20->setChecked(TRUE);
		break;
	}


#if QT_VERSION < 0x040000
	/* Set button group IDs */
	ButtonGroupBandwidth->insert(RadioButtonBandwidth45, 0);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth5, 1);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth9, 2);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth10, 3);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth18, 4);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth20, 5);
#endif
	/* MSC interleaver mode */
	ComboBoxInsertItem(ComboBoxMSCInterleaver, tr("2 s (Long Interleaving)"), 0);
	ComboBoxInsertItem(ComboBoxMSCInterleaver, tr("400 ms (Short Interleaving)"), 1);

	switch (Parameters.eSymbolInterlMode)
	{
	case CParameter::SI_LONG:
		ComboBoxSetCurrentItem(ComboBoxMSCInterleaver, 0);
		break;

	case CParameter::SI_SHORT:
		ComboBoxSetCurrentItem(ComboBoxMSCInterleaver, 1);
		break;
	}

	/* MSC Constellation Scheme */
	ComboBoxInsertItem(ComboBoxMSCConstellation, tr("SM 16-QAM"), 0);
	ComboBoxInsertItem(ComboBoxMSCConstellation, tr("SM 64-QAM"), 1);

// These modes should not be used right now, TODO
// DF: I reenabled those, because it seems to work, at least with dream
	ComboBoxInsertItem(ComboBoxMSCConstellation, tr("HMsym 64-QAM"), 2);
	ComboBoxInsertItem(ComboBoxMSCConstellation, tr("HMmix 64-QAM"), 3);

	switch (Parameters.eMSCCodingScheme)
	{
	case CS_1_SM:
		break;

	case CS_2_SM:
		ComboBoxSetCurrentItem(ComboBoxMSCConstellation, 0);
		break;

	case CS_3_SM:
		ComboBoxSetCurrentItem(ComboBoxMSCConstellation, 1);
		break;

	case CS_3_HMSYM:
		ComboBoxSetCurrentItem(ComboBoxMSCConstellation, 2);
		break;

	case CS_3_HMMIX:
		ComboBoxSetCurrentItem(ComboBoxMSCConstellation, 3);
		break;
	}

	/* SDC Constellation Scheme */
	ComboBoxInsertItem(ComboBoxSDCConstellation, tr("4-QAM"), 0);
	ComboBoxInsertItem(ComboBoxSDCConstellation, tr("16-QAM"), 1);

	switch (Parameters.eSDCCodingScheme)
	{
	case CS_1_SM:
		ComboBoxSetCurrentItem(ComboBoxSDCConstellation, 0);
		break;

	case CS_2_SM:
		ComboBoxSetCurrentItem(ComboBoxSDCConstellation, 1);
		break;

	case CS_3_SM:
	case CS_3_HMSYM:
	case CS_3_HMMIX:
		break;
	}


	/* Service parameters --------------------------------------------------- */
	/* Service label */
	CService& Service = Parameters.Service[0]; // TODO
	QString label = FromUtf8(Service.strLabel.c_str());
	LineEditServiceLabel->setText(label);

	/* Service ID */
	LineEditServiceID->setText(QString().setNum((int) Service.iServiceID, 16));


	int i;
	/* Language */
	for (i = 0; i < LEN_TABLE_LANGUAGE_CODE; i++)
		ComboBoxInsertItem(ComboBoxLanguage, strTableLanguageCode[i].c_str(), i);

	ComboBoxSetCurrentItem(ComboBoxLanguage, Service.iLanguage);

	/* Program type */
	for (i = 0; i < LEN_TABLE_PROG_TYPE_CODE; i++)
		ComboBoxInsertItem(ComboBoxProgramType, strTableProgTypCod[i].c_str(), i);

	/* Service description */
	iServiceDescr = Service.iServiceDescr;
	ComboBoxSetCurrentItem(ComboBoxProgramType, iServiceDescr);

	/* Sound card IF */
	LineEditSndCrdIF->setText(QString().number(
		TransThread.DRMTransmitter.GetCarOffset(), 'f', 2));

	/* Clear list box for file names */
	OnButtonClearAllFileNames();

	/* Set up file list box columns */
#if QT_VERSION < 0x040000
	/* We assume that one column is already there */
	ListViewFileNames->setColumnText(0, "File Name");
	ListViewFileNames->addColumn("Size [KB]");
	ListViewFileNames->addColumn("Full Path");
#endif

	/* Disable other three services */
#if QT_VERSION < 0x040000
	TabWidgetServices->setTabEnabled(tabService2, FALSE);
	TabWidgetServices->setTabEnabled(tabService3, FALSE);
	TabWidgetServices->setTabEnabled(tabService4, FALSE);
#else
	TabWidgetServices->setTabEnabled(1, FALSE);
	TabWidgetServices->setTabEnabled(2, FALSE);
	TabWidgetServices->setTabEnabled(3, FALSE);
#endif
	CheckBoxEnableService->setChecked(TRUE);
	CheckBoxEnableService->setEnabled(FALSE);


	/* Let this service be an audio service for initialization */
	/* Set audio enable check box */
	CheckBoxEnableAudio->setChecked(TRUE);
	EnableAudio(TRUE);
	CheckBoxEnableData->setChecked(FALSE);
	EnableData(FALSE);

#ifdef ENABLE_TRANSM_CODECPARAMS
	/* Setup audio codec check boxes */
	switch (Service.AudioParam.eAudioCoding)
	{
	case CAudioParam::AC_AAC:
		RadioButtonFAAC->setChecked(TRUE);
		ShowButtonCodec(FALSE, 1);
		break;

	case CAudioParam::AC_OPUS:
		RadioButtonOPUS->setChecked(TRUE);
		ShowButtonCodec(TRUE, 1);
		break;

	default:
		ShowButtonCodec(FALSE, 1);
		break;
	}
//TODO	CAudioSourceEncoder& AudioSourceEncoder = *DRMTransmitter.GetAudSrcEnc();
//TODO	if (!AudioSourceEncoder.FaacCodecSupported())
//TODO		RadioButtonFAAC->hide();
//TODO	if (!AudioSourceEncoder.OpusCodecSupported())
//TODO		RadioButtonOPUS->hide();
#else
# if QT_VERSION < 0x040000
	ButtonGroupCodec->hide();
	ButtonCodec->hide();
# else
	GroupBoxCodec->setVisible(FALSE);
	ButtonCodec->setVisible(FALSE);
# endif
#endif

	/* Add example text message at startup ---------------------------------- */
	/* Activate text message */
	EnableTextMessage(TRUE);
	CheckBoxEnableTextMessage->setChecked(TRUE);

	/* Add example text in internal container */
	vecstrTextMessage.Add(
		ToUtf8(tr("Dream DRM Transmitter\x0B\x0AThis is a test transmission")));

	/* Insert item in combo box, display text and set item to our text */
	ComboBoxInsertItem(ComboBoxTextMessage, QString().setNum(1), 1);
	ComboBoxSetCurrentItem(ComboBoxTextMessage, 1);

	/* Update the TextEdit with the default text */
	OnComboBoxTextMessageActivated(1);

	/* Now make sure that the text message flag is activated in global struct */
	Service.AudioParam.bTextflag = TRUE;


	/* Enable all controls */
	EnableAllControlsForSet();


	/* Set check box remove path */
	CheckBoxRemovePath->setChecked(TRUE);
	OnToggleCheckBoxRemovePath(TRUE);


	/* Set Menu ***************************************************************/
#if QT_VERSION < 0x040000
	/* Settings menu  ------------------------------------------------------- */
	pSettingsMenu = new QPopupMenu(this);
	CHECK_PTR(pSettingsMenu);
	pSettingsMenu->insertItem(tr("&Sound Card Selection"),
		new CSoundCardSelMenu(DRMTransmitter, this));

	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&Settings"), pSettingsMenu);
	pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	TransmDlgBaseLayout->setMenuBar(pMenu);

#else
	CFileMenu* pFileMenu = new CFileMenu(DRMTransmitter, this, menu_Settings);

	menu_Settings->addMenu(new CSoundCardSelMenu(DRMTransmitter, pFileMenu, this));

	connect(actionAbout_Dream, SIGNAL(triggered()), &AboutDlg, SLOT(show()));
	connect(actionWhats_This, SIGNAL(triggered()), this, SLOT(on_actionWhats_This()));
#endif


	/* Connections ---------------------------------------------------------- */
	/* Push buttons */
	connect(ButtonStartStop, SIGNAL(clicked()),
		this, SLOT(OnButtonStartStop()));
#ifdef ENABLE_TRANSM_CODECPARAMS
	connect(ButtonCodec, SIGNAL(clicked()),
		this, SLOT(OnButtonCodec()));
#endif
	connect(PushButtonAddText, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddText()));
	connect(PushButtonClearAllText, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllText()));
	connect(PushButtonAddFile, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddFileName()));
	connect(PushButtonClearAllFileNames, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllFileNames()));

	/* Check boxes */
	connect(CheckBoxHighQualityIQ, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxHighQualityIQ(bool)));
	connect(CheckBoxAmplifiedOutput, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxAmplifiedOutput(bool)));
	connect(CheckBoxEnableTextMessage, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableTextMessage(bool)));
	connect(CheckBoxEnableAudio, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableAudio(bool)));
	connect(CheckBoxEnableData, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableData(bool)));
	connect(CheckBoxRemovePath, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxRemovePath(bool)));

	/* Combo boxes */
	connect(ComboBoxMSCInterleaver, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCInterleaverActivated(int)));
	connect(ComboBoxMSCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCConstellationActivated(int)));
	connect(ComboBoxSDCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxSDCConstellationActivated(int)));
	connect(ComboBoxLanguage, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxLanguageActivated(int)));
	connect(ComboBoxProgramType, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxProgramTypeActivated(int)));
	connect(ComboBoxTextMessage, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxTextMessageActivated(int)));
	connect(ComboBoxMSCProtLev, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCProtLevActivated(int)));

	/* Button groups */
#if QT_VERSION < 0x040000
	connect(ButtonGroupRobustnessMode, SIGNAL(clicked(int)),
		this, SLOT(OnRadioRobustnessMode(int)));
	connect(ButtonGroupBandwidth, SIGNAL(clicked(int)),
		this, SLOT(OnRadioBandwidth(int)));
	connect(ButtonGroupOutput, SIGNAL(clicked(int)),
		this, SLOT(OnRadioOutput(int)));
# ifdef ENABLE_TRANSM_CODECPARAMS
	connect(ButtonGroupCodec, SIGNAL(clicked(int)),
		this, SLOT(OnRadioCodec(int)));
# endif
	connect(ButtonGroupCurrentTime, SIGNAL(clicked(int)),
		this, SLOT(OnRadioCurrentTime(int)));
#else
	connect(ButtonGroupRobustnessMode, SIGNAL(buttonClicked(int)),
		this, SLOT(OnRadioRobustnessMode(int)));
	connect(ButtonGroupBandwidth, SIGNAL(buttonClicked(int)),
		this, SLOT(OnRadioBandwidth(int)));
	connect(ButtonGroupOutput, SIGNAL(buttonClicked(int)),
		this, SLOT(OnRadioOutput(int)));
# ifdef ENABLE_TRANSM_CODECPARAMS
	connect(ButtonGroupCodec, SIGNAL(buttonClicked(int)),
		this, SLOT(OnRadioCodec(int)));
# endif
	connect(ButtonGroupCurrentTime, SIGNAL(buttonClicked(int)),
		this, SLOT(OnRadioCurrentTime(int)));
#endif

	/* Line edits */
	connect(LineEditServiceLabel, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceLabel(const QString&)));
	connect(LineEditServiceID, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceID(const QString&)));
	connect(LineEditSndCrdIF, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedSndCrdIF(const QString&)));

	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerStop, SIGNAL(timeout()),
		this, SLOT(OnTimerStop()));


	/* Set timer for real-time controls */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

TransmDialog::~TransmDialog()
{
#ifdef ENABLE_TRANSM_CODECPARAMS
	/* Destroy codec dialog if exist */
	if (CodecDlg)
		delete CodecDlg;
#endif

	/* Save window position and size */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("Transmit Dialog", s);

	/* Stop transmitter */
	if (bIsStarted == TRUE)
		TransThread.Stop();

	/* Restore the service description, may
	   have been reset by data service */
	CParameter& Parameters = *DRMTransmitter.GetParameters();
	Parameters.Lock();
	CService& Service = Parameters.Service[0]; // TODO
	Service.iServiceDescr = iServiceDescr;

	/* Save transmitter settings */
	DRMTransmitter.SaveSettings();

	Parameters.Unlock();
}

void TransmDialog::closeEvent(QCloseEvent* ce)
{
	bCloseRequested = TRUE;
	if (bIsStarted == TRUE)
	{
		OnButtonStartStop();
		ce->ignore();
	}
	else
		ce->accept();
}

void TransmDialog::on_actionWhats_This()
{
	QWhatsThis::enterWhatsThisMode();
}

void TransmDialog::OnTimer()
{
	/* Set value for input level meter (only in "start" mode) */
	if (bIsStarted == TRUE)
	{
		ProgrInputLevel->
			setValue(TransThread.DRMTransmitter.GetReadData()->GetLevelMeter());

		string strCPictureName;
		_REAL rCPercent;

		/* Activate progress bar for slide show pictures only if current state
		   can be queried and if data service is active
		   (check box is checked) */
		if ((TransThread.DRMTransmitter.GetAudSrcEnc()->
			GetTransStat(strCPictureName, rCPercent) ==	TRUE) &&
			(CheckBoxEnableData->isChecked()))
		{
			/* Enable controls */
			ProgressBarCurPict->setEnabled(TRUE);
			TextLabelCurPict->setEnabled(TRUE);

			/* We want to file name, not the complete path -> "QFileInfo" */
			QFileInfo FileInfo(strCPictureName.c_str());

			/* Show current file name and percentage */
			TextLabelCurPict->setText(FileInfo.fileName());
			ProgressBarSetValue(ProgressBarCurPict, (int) (rCPercent * 100)); /* % */
		}
		else
		{
			/* Disable controls */
			ProgressBarCurPict->setEnabled(FALSE);
			TextLabelCurPict->setEnabled(FALSE);
		}
	}
}

void TransmDialog::OnTimerStop()
{
#if QT_VERSION < 0x040000
	if (!TransThread.running())
#else
	if (!TransThread.isRunning())
#endif
	{
		TimerStop.stop();

		bIsStarted = FALSE;

		if (bCloseRequested)
		{
			close();
		}
		else
		{
			ButtonStartStop->setText(tr("&Start"));

			EnableAllControlsForSet();
		}
	}
}

void TransmDialog::OnButtonStartStop()
{
	if (!TimerStop.isActive())
	{
		if (bIsStarted == TRUE)
		{
			ButtonStartStop->setText(tr("Stopping..."));

			/* Request a transmitter stop */
			TransThread.Stop();

			/* Start the timer for polling the thread state */
			TimerStop.start(50);
		}
		else
		{
			int i;

			/* Start transmitter */
			/* Set text message */
			TransThread.DRMTransmitter.GetAudSrcEnc()->ClearTextMessage();

			for (i = 1; i < vecstrTextMessage.Size(); i++)
				TransThread.DRMTransmitter.GetAudSrcEnc()->
					SetTextMessage(vecstrTextMessage[i]);

			/* Set file names for data application */
			TransThread.DRMTransmitter.GetAudSrcEnc()->ClearPicFileNames();

#if QT_VERSION < 0x040000
			/* Iteration through list view items. Code based on QT sample code for
			   list view items */
			QListViewItemIterator it(ListViewFileNames);

			for (; it.current(); it++)
			{
				/* Complete file path is in third column */
				const QString strFileName = it.current()->text(2);

				/* Extract format string */
				QFileInfo FileInfo(strFileName);
				const QString strFormat = FileInfo.extension(FALSE);

				TransThread.DRMTransmitter.GetAudSrcEnc()->
					SetPicFileName(ToUtf8(strFileName), ToUtf8(strFormat));
			}
#else
			/* Iteration through table widget items */
			int count = TreeWidgetFileNames->topLevelItemCount();

			for (i = 0; i <count; i++)
			{
				/* Complete file path is in third column */
				QTreeWidgetItem* item = TreeWidgetFileNames->topLevelItem(i);
				if (item)
				{
					/* Get the file path  */
					const QString strFileName = item->text(2);

					/* Extract format string */
					QFileInfo FileInfo(strFileName);
					const QString strFormat = FileInfo.suffix();

					TransThread.DRMTransmitter.GetAudSrcEnc()->
						SetPicFileName(ToUtf8(strFileName), ToUtf8(strFormat));
				}
			}
#endif

			TransThread.start();

			ButtonStartStop->setText(tr("&Stop"));

			DisableAllControlsForSet();

			bIsStarted = TRUE;
		}
	}
}

void TransmDialog::OnToggleCheckBoxHighQualityIQ(bool bState)
{
	TransThread.DRMTransmitter.GetTransData()->SetHighQualityIQ(bState);
}

void TransmDialog::OnToggleCheckBoxAmplifiedOutput(bool bState)
{
	TransThread.DRMTransmitter.GetTransData()->SetAmplifiedOutput(bState);
}

void TransmDialog::OnToggleCheckBoxEnableTextMessage(bool bState)
{
	EnableTextMessage(bState);
}

void TransmDialog::EnableTextMessage(const _BOOLEAN bFlag)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	Parameters.Lock();

	if (bFlag == TRUE)
	{
		/* Enable text message controls */
		ComboBoxTextMessage->setEnabled(TRUE);
		MultiLineEditTextMessage->setEnabled(TRUE);
		PushButtonAddText->setEnabled(TRUE);
		PushButtonClearAllText->setEnabled(TRUE);

		/* Set text message flag in global struct */
		Parameters.Service[0].AudioParam.bTextflag = TRUE;
	}
	else
	{
		/* Disable text message controls */
		ComboBoxTextMessage->setEnabled(FALSE);
		MultiLineEditTextMessage->setEnabled(FALSE);
		PushButtonAddText->setEnabled(FALSE);
		PushButtonClearAllText->setEnabled(FALSE);

		/* Set text message flag in global struct */
		Parameters.Service[0].AudioParam.bTextflag = FALSE;
	}

	Parameters.Unlock();
}

void TransmDialog::OnToggleCheckBoxEnableAudio(bool bState)
{
	EnableAudio(bState);

	if (bState)
	{
		/* Set audio enable check box */
		CheckBoxEnableData->setChecked(FALSE);
		EnableData(FALSE);
#ifdef ENABLE_TRANSM_CODECPARAMS
		ShowButtonCodec(TRUE, 2);
#endif
	}
	else
	{
		/* Set audio enable check box */
		CheckBoxEnableData->setChecked(TRUE);
		EnableData(TRUE);
#ifdef ENABLE_TRANSM_CODECPARAMS
		ShowButtonCodec(FALSE, 2);
#endif
	}
}

void TransmDialog::EnableAudio(const _BOOLEAN bFlag)
{
	if (bFlag == TRUE)
	{
		/* Enable audio controls */
#if QT_VERSION < 0x040000
		ButtonGroupCodec->setEnabled(TRUE);
#else
		GroupBoxCodec->setEnabled(TRUE);
#endif
		GroupBoxTextMessage->setEnabled(TRUE);
		ComboBoxProgramType->setEnabled(TRUE);

		CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

		Parameters.Lock();

		/* Only one audio service */
		Parameters.iNumAudioService = 1;
		Parameters.iNumDataService = 0;

		/* Audio flag of this service */
		Parameters.Service[0].eAudDataFlag = CService::SF_AUDIO;

		/* Always use stream number 0 right now, TODO */
		Parameters.Service[0].AudioParam.iStreamID = 0;

		/* Programme Type code */
		Parameters.Service[0].iServiceDescr = iServiceDescr;

		Parameters.Unlock();
	}
	else
	{
		/* Disable audio controls */
#if QT_VERSION < 0x040000
		ButtonGroupCodec->setEnabled(FALSE);
#else
		GroupBoxCodec->setEnabled(FALSE);
#endif
		GroupBoxTextMessage->setEnabled(FALSE);
		ComboBoxProgramType->setEnabled(FALSE);
	}
}

void TransmDialog::OnToggleCheckBoxEnableData(bool bState)
{
	EnableData(bState);

	if (bState)
	{
		/* Set audio enable check box */
		CheckBoxEnableAudio->setChecked(FALSE);
		EnableAudio(FALSE);
	}
	else
	{
		/* Set audio enable check box */
		CheckBoxEnableAudio->setChecked(TRUE);
		EnableAudio(TRUE);
	}
}

void TransmDialog::EnableData(const _BOOLEAN bFlag)
{
	/* Enable/Disable data controls */
	CheckBoxRemovePath->setEnabled(bFlag);
#if QT_VERSION < 0x040000
	ListViewFileNames->setEnabled(bFlag);
#else
	TreeWidgetFileNames->setEnabled(bFlag);
#endif
	PushButtonClearAllFileNames->setEnabled(bFlag);
	PushButtonAddFile->setEnabled(bFlag);

	if (bFlag == TRUE)
	{
		CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

		Parameters.Lock();

		/* Only one data service */
		Parameters.iNumAudioService = 0;
		Parameters.iNumDataService = 1;

		/* Data flag for this service */
		Parameters.Service[0].eAudDataFlag = CService::SF_DATA;

		/* Always use stream number 0, TODO */
		Parameters.Service[0].DataParam.iStreamID = 0;

		/* Init SlideShow application */
		Parameters.Service[0].DataParam.iPacketLen = 45; /* TEST */
		Parameters.Service[0].DataParam.eDataUnitInd = CDataParam::DU_DATA_UNITS;
		Parameters.Service[0].DataParam.eAppDomain = CDataParam::AD_DAB_SPEC_APP;

		/* The value 0 indicates that the application details are provided
		   solely by SDC data entity type 5 */
		Parameters.Service[0].iServiceDescr = 0;

		Parameters.Unlock();
	}
}

_BOOLEAN TransmDialog::GetMessageText(const int iID)
{
	_BOOLEAN bTextIsNotEmpty = TRUE;

	/* Check if text control is not empty */
	if (TextEditIsModified(MultiLineEditTextMessage))
	{
		/* Check size of container. If not enough space, enlarge */
		if (iID == vecstrTextMessage.Size())
			vecstrTextMessage.Enlarge(1);

		/* DF: I did some test on both Qt3 and Qt4, and
		   UTF8 char are well preserved */
#if QT_VERSION < 0x040000
		/* First line */
		vecstrTextMessage[iID] = MultiLineEditTextMessage->textLine(0).utf8().data();
		/* Other lines */
		const int iNumLines = MultiLineEditTextMessage->numLines();

		for (int i = 1; i < iNumLines; i++)
		{
			/* Insert line break */
			vecstrTextMessage[iID].append("\x0A");

			/* Insert text of next line */
			vecstrTextMessage[iID].
				append(MultiLineEditTextMessage->textLine(i).utf8());
		}
#else
		/* Get the text from MultiLineEditTextMessage */
		QString text = MultiLineEditTextMessage->toPlainText();

		/* Each line is already separated by a newline char,
		   so no special processing is further required */

		/* Save the text */
		vecstrTextMessage[iID] = ToUtf8(text);
#endif

	}
	else
		bTextIsNotEmpty = FALSE;
	return bTextIsNotEmpty;
}

void TransmDialog::OnPushButtonAddText()
{
	if (iIDCurrentText == 0)
	{
		/* Add new message */
		if (GetMessageText(vecstrTextMessage.Size()) == TRUE)
		{
			/* If text was not empty, add new text in combo box */
			const int iNewID = vecstrTextMessage.Size() - 1;
			ComboBoxInsertItem(ComboBoxTextMessage, QString().setNum(iNewID), iNewID);
			/* Clear added text */
			TextEditClear(MultiLineEditTextMessage);
			TextEditClearModified(MultiLineEditTextMessage);
		}
	}
	else
	{
		/* Text was modified */
		GetMessageText(iIDCurrentText);
	}
}

void TransmDialog::OnButtonClearAllText()
{
	/* Clear container */
	vecstrTextMessage.Init(1);
	iIDCurrentText = 0;

	/* Clear combo box */
	ComboBoxClear(ComboBoxTextMessage);
	ComboBoxInsertItem(ComboBoxTextMessage, "new", 0);
	/* Clear multi line edit */
	TextEditClear(MultiLineEditTextMessage);
	TextEditClearModified(MultiLineEditTextMessage);
}

void TransmDialog::OnToggleCheckBoxRemovePath(bool bState)
{
	TransThread.DRMTransmitter.GetAudSrcEnc()->SetPathRemoval(bState);
}

void TransmDialog::OnPushButtonAddFileName()
{
	/* Show "open file" dialog. Let the user select more than one file */
#if QT_VERSION < 0x040000
	QStringList list = QFileDialog::getOpenFileNames(tr("Image Files (*.png *.jpg *.jpeg *.jfif)"), NULL, this);
#else
	QStringList list = QFileDialog::getOpenFileNames(this, tr("Add Files"), NULL, tr("Image Files (*.png *.jpg *.jpeg *.jfif)"));
#endif

	/* Check if user not hit the cancel button */
	if (!list.isEmpty())
	{
		/* Insert all selected file names */
		for (QStringList::Iterator it = list.begin(); it != list.end(); it++)
		{
			QFileInfo FileInfo((*it));

#if QT_VERSION < 0x040000
			/* Insert list view item. The object which is created
			   here will be automatically destroyed by QT when
			   the parent ("ListViewFileNames") is destroyed */
			ListViewFileNames->insertItem(
				new QListViewItem(ListViewFileNames, FileInfo.fileName(),
				QString().setNum((float) FileInfo.size() / 1000.0, 'f', 2),
				FileInfo.filePath()));
#else
			/* Insert tree widget item. The objects which is created
			   here will be automatically destroyed by QT when
			   the parent ("TreeWidgetFileNames") is destroyed */
			QTreeWidgetItem* item = new QTreeWidgetItem();
			if (item)
			{
				item->setText(0, FileInfo.fileName());
				item->setText(1, QString().setNum((float) FileInfo.size() / 1000.0, 'f', 2));
				item->setText(2, FileInfo.filePath());
				TreeWidgetFileNames->addTopLevelItem(item);
			}
#endif
		}
#if QT_VERSION >= 0x040000
		/* Resize columns to content */
		for (int i = 0; i < 3; i++)
			TreeWidgetFileNames->resizeColumnToContents(i);
#endif
	}
}

void TransmDialog::OnButtonClearAllFileNames()
{
#if QT_VERSION < 0x040000
	/* Clear list box for file names */
	ListViewFileNames->clear();
#else
	/* Remove all items */
	TreeWidgetFileNames->clear();
	/* Resize columns */
	for (int i = 0; i < 3; i++)
		TreeWidgetFileNames->resizeColumnToContents(i);
#endif
}

#ifdef ENABLE_TRANSM_CODECPARAMS
void TransmDialog::OnButtonCodec()
{
	/* Create Codec Dialog if NULL */
	if (!CodecDlg)
	{
		CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
# if QT_VERSION < 0x040000
		CodecDlg = new CodecParams(Settings, Parameters, this, NULL, FALSE, Qt::WStyle_MinMax);
# else
		CodecDlg = new CodecParams(Settings, Parameters, this, NULL, FALSE, Qt::Dialog);
# endif
	}
	/* Toggle the visibility */
	if (CodecDlg)
		CodecDlg->Toggle();
}
#else
# if QT_VERSION < 0x040000
void TransmDialog::OnButtonCodec() {}
# endif
#endif

void TransmDialog::OnComboBoxTextMessageActivated(int iID)
{
	iIDCurrentText = iID;

	/* Set text control with selected message */
	TextEditClear(MultiLineEditTextMessage);
	TextEditClearModified(MultiLineEditTextMessage);
	if (iID != 0)
	{
		/* Get the text */
		QString text = FromUtf8(vecstrTextMessage[iID].c_str());

		/* Write stored text in multi line edit control */
		MultiLineEditTextMessage->setText(text);
	}
}

void TransmDialog::OnTextChangedSndCrdIF(const QString& strIF)
{
	/* Convert string to floating point value "toFloat()" */
	TransThread.DRMTransmitter.SetCarOffset(strIF.toFloat());
}

void TransmDialog::OnTextChangedServiceID(const QString& strID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	if(strID.length()<6)
        return;

	/* Convert hex string to unsigned integer "toUInt()" */
	bool ok;
	uint32_t iServiceID = strID.toUInt(&ok, 16);
	if(ok == false)
        return;

	Parameters.Lock();

	Parameters.Service[0].iServiceID = iServiceID;

	Parameters.Unlock();
}

void TransmDialog::OnTextChangedServiceLabel(const QString& strLabel)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	Parameters.Lock();
	/* Set additional text for log file. */
	Parameters.Service[0].strLabel = ToUtf8(strLabel);
	Parameters.Unlock();
}

void TransmDialog::OnComboBoxMSCInterleaverActivated(int iID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	Parameters.Lock();
	switch (iID)
	{
	case 0:
		Parameters.eSymbolInterlMode = CParameter::SI_LONG;
		break;

	case 1:
		Parameters.eSymbolInterlMode = CParameter::SI_SHORT;
		break;
	}
	Parameters.Unlock();
}

void TransmDialog::OnComboBoxMSCConstellationActivated(int iID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	Parameters.Lock();
	switch (iID)
	{
	case 0:
		Parameters.eMSCCodingScheme = CS_2_SM;
		break;

	case 1:
		Parameters.eMSCCodingScheme = CS_3_SM;
		break;

	case 2:
		Parameters.eMSCCodingScheme = CS_3_HMSYM;
		break;

	case 3:
		Parameters.eMSCCodingScheme = CS_3_HMMIX;
		break;
	}
	Parameters.Unlock();

	/* Protection level must be re-adjusted when constelletion mode was
	   changed */
	UpdateMSCProtLevCombo();
}

void TransmDialog::OnComboBoxMSCProtLevActivated(int iID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	Parameters.MSCPrLe.iPartB = iID;
	Parameters.Unlock();
}

void TransmDialog::UpdateMSCProtLevCombo()
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	if (Parameters.eMSCCodingScheme == CS_2_SM)
	{
		/* Only two protection levels possible in 16 QAM mode */
		ComboBoxClear(ComboBoxMSCProtLev);
		ComboBoxInsertItem(ComboBoxMSCProtLev, "0", 0);
		ComboBoxInsertItem(ComboBoxMSCProtLev, "1", 1);
		/* Set protection level to 1 if greater than 1 */
		if (Parameters.MSCPrLe.iPartB > 1)
			Parameters.MSCPrLe.iPartB = 1;
	}
	else
	{
		/* Four protection levels defined */
		ComboBoxClear(ComboBoxMSCProtLev);
		ComboBoxInsertItem(ComboBoxMSCProtLev, "0", 0);
		ComboBoxInsertItem(ComboBoxMSCProtLev, "1", 1);
		ComboBoxInsertItem(ComboBoxMSCProtLev, "2", 2);
		ComboBoxInsertItem(ComboBoxMSCProtLev, "3", 3);
	}
	Parameters.Unlock();
	ComboBoxSetCurrentItem(ComboBoxMSCProtLev, Parameters.MSCPrLe.iPartB);
}

void TransmDialog::OnComboBoxSDCConstellationActivated(int iID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	switch (iID)
	{
	case 0:
		Parameters.eSDCCodingScheme = CS_1_SM;
		break;

	case 1:
		Parameters.eSDCCodingScheme = CS_2_SM;
		break;
	}
	Parameters.Unlock();
}

void TransmDialog::OnComboBoxLanguageActivated(int iID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	Parameters.Service[0].iLanguage = iID;
	Parameters.Unlock();
}

void TransmDialog::OnComboBoxProgramTypeActivated(int iID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	Parameters.Service[0].iServiceDescr = iID;
	Parameters.Unlock();
	iServiceDescr = iID;
}

void TransmDialog::OnRadioRobustnessMode(int iID)
{
#if QT_VERSION >= 0x040000
	iID = -iID - 2; // TODO understand why
#endif
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	/* Set current bandwidth */
	Parameters.Lock();
	ESpecOcc eCurSpecOcc = Parameters.GetSpectrumOccup();
	Parameters.Unlock();

	/* Set default new robustness mode */
	ERobMode eNewRobMode = RM_ROBUSTNESS_MODE_B;

	/* Check, which bandwith's are possible with this robustness mode */
	switch (iID)
	{
	case 0:
		/* All bandwidth modes are possible */
		RadioButtonBandwidth45->setEnabled(TRUE);
		RadioButtonBandwidth5->setEnabled(TRUE);
		RadioButtonBandwidth9->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(TRUE);
		/* Set new robustness mode */
		eNewRobMode = RM_ROBUSTNESS_MODE_A;
		break;

	case 1:
		/* All bandwidth modes are possible */
		RadioButtonBandwidth45->setEnabled(TRUE);
		RadioButtonBandwidth5->setEnabled(TRUE);
		RadioButtonBandwidth9->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(TRUE);
		/* Set new robustness mode */
		eNewRobMode = RM_ROBUSTNESS_MODE_B;
		break;

	case 2:
		/* Only 10 and 20 kHz possible in robustness mode C */
		RadioButtonBandwidth45->setEnabled(FALSE);
		RadioButtonBandwidth5->setEnabled(FALSE);
		RadioButtonBandwidth9->setEnabled(FALSE);
		RadioButtonBandwidth18->setEnabled(FALSE);
		/* Set check on the nearest bandwidth if "out of range" */
		if (eCurSpecOcc == SO_0 || eCurSpecOcc == SO_1 ||
			eCurSpecOcc == SO_2 || eCurSpecOcc == SO_4)
		{
			if (eCurSpecOcc == SO_4)
				RadioButtonBandwidth20->setChecked(TRUE);
			else
				RadioButtonBandwidth10->setChecked(TRUE);
			OnRadioBandwidth(ButtonGroupGetCurrentId(ButtonGroupBandwidth));
		}
		/* Set new robustness mode */
		eNewRobMode = RM_ROBUSTNESS_MODE_C;
		break;

	case 3:
		/* Only 10 and 20 kHz possible in robustness mode D */
		RadioButtonBandwidth45->setEnabled(FALSE);
		RadioButtonBandwidth5->setEnabled(FALSE);
		RadioButtonBandwidth9->setEnabled(FALSE);
		RadioButtonBandwidth18->setEnabled(FALSE);
		/* Set check on the nearest bandwidth if "out of range" */
		if (eCurSpecOcc == SO_0 || eCurSpecOcc == SO_1 ||
			eCurSpecOcc == SO_2 || eCurSpecOcc == SO_4)
		{
			if (eCurSpecOcc == SO_4)
				RadioButtonBandwidth20->setChecked(TRUE);
			else
				RadioButtonBandwidth10->setChecked(TRUE);
			OnRadioBandwidth(ButtonGroupGetCurrentId(ButtonGroupBandwidth));
		}
		/* Set new robustness mode */
		eNewRobMode = RM_ROBUSTNESS_MODE_D;
		break;
	}

	/* Set new robustness mode */
	Parameters.Lock();
	Parameters.SetWaveMode(eNewRobMode);
	Parameters.Unlock();
}

void TransmDialog::OnRadioBandwidth(int iID)
{
#if QT_VERSION >= 0x040000
	iID = -iID - 2; // TODO understand why
	static const int table[6] = {0, 2, 4, 1, 3, 5};
	iID = table[(unsigned int)iID % (unsigned int)6];
#endif
	ESpecOcc eNewSpecOcc = SO_0;

	switch (iID)
	{
	case 0:
		eNewSpecOcc = SO_0;
		break;

	case 1:
		eNewSpecOcc = SO_1;
		break;

	case 2:
		eNewSpecOcc = SO_2;
		break;

	case 3:
		eNewSpecOcc = SO_3;
		break;

	case 4:
		eNewSpecOcc = SO_4;
		break;

	case 5:
		eNewSpecOcc = SO_5;
		break;
	}

	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	/* Set new spectrum occupancy */
	Parameters.Lock();
	Parameters.SetSpectrumOccup(eNewSpecOcc);
	Parameters.Unlock();
}

void TransmDialog::OnRadioOutput(int iID)
{
#if QT_VERSION >= 0x040000
	iID = -iID - 2; // TODO understand why
#endif
	switch (iID)
	{
	case 0:
		/* Button "Real Valued" */
		TransThread.DRMTransmitter.GetTransData()->
			SetIQOutput(CTransmitData::OF_REAL_VAL);
		CheckBoxAmplifiedOutput->setEnabled(true);
		CheckBoxHighQualityIQ->setEnabled(false);
		break;

	case 1:
		/* Button "I / Q (pos)" */
		TransThread.DRMTransmitter.GetTransData()->
			SetIQOutput(CTransmitData::OF_IQ_POS);
		CheckBoxAmplifiedOutput->setEnabled(true);
		CheckBoxHighQualityIQ->setEnabled(true);
		break;

	case 2:
		/* Button "I / Q (neg)" */
		TransThread.DRMTransmitter.GetTransData()->
			SetIQOutput(CTransmitData::OF_IQ_NEG);
		CheckBoxAmplifiedOutput->setEnabled(true);
		CheckBoxHighQualityIQ->setEnabled(true);
		break;

	case 3:
		/* Button "E / P" */
		TransThread.DRMTransmitter.GetTransData()->
			SetIQOutput(CTransmitData::OF_EP);
		CheckBoxAmplifiedOutput->setEnabled(false);
		CheckBoxHighQualityIQ->setEnabled(true);
		break;
	}
}

void TransmDialog::OnRadioCurrentTime(int iID)
{
#if QT_VERSION >= 0x040000
	iID = -iID - 2; // TODO understand why
#endif
	CParameter::ECurTime eCurTime = CParameter::CT_OFF;

	switch (iID)
	{
	case 0:
		eCurTime = CParameter::CT_OFF;
		break;

	case 1:
		eCurTime = CParameter::CT_LOCAL;
		break;

	case 2:
		eCurTime = CParameter::CT_UTC;
		break;

	case 3:
		eCurTime = CParameter::CT_UTC_OFFSET;
		break;
	}

	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	Parameters.Lock();

	/* Set transmission of current time in global struct */
	Parameters.eTransmitCurrentTime = eCurTime;

	Parameters.Unlock();
}

#ifdef ENABLE_TRANSM_CODECPARAMS
void TransmDialog::OnRadioCodec(int iID)
{
#if QT_VERSION >= 0x040000
	iID = -iID - 2; // TODO understand why
#endif
	CAudioParam::EAudCod eNewAudioCoding = CAudioParam::AC_AAC;

	switch (iID)
	{
	case 0:
		eNewAudioCoding = CAudioParam::AC_AAC;
		ShowButtonCodec(FALSE, 1);
		break;

	case 1:
		eNewAudioCoding = CAudioParam::AC_OPUS;
		ShowButtonCodec(TRUE, 1);
		break;
	}

	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	Parameters.Lock();

	/* Set new audio codec */
	Parameters.Service[0].AudioParam.eAudioCoding = eNewAudioCoding;

	Parameters.Unlock();
}
#else
# if QT_VERSION < 0x040000
void TransmDialog::OnRadioCodec(int iID) {(void)iID;}
# endif
#endif

void TransmDialog::DisableAllControlsForSet()
{
	TabWidgetEnableTabs(TabWidgetParam, FALSE);
	TabWidgetEnableTabs(TabWidgetServices, FALSE);

	GroupInput->setEnabled(TRUE); /* For run-mode */
}

void TransmDialog::EnableAllControlsForSet()
{
	TabWidgetEnableTabs(TabWidgetParam, TRUE);
	TabWidgetEnableTabs(TabWidgetServices, TRUE);

	GroupInput->setEnabled(FALSE); /* For run-mode */

	/* Reset status bars */
	ProgrInputLevel->setValue(RET_VAL_LOG_0);
	ProgressBarSetValue(ProgressBarCurPict, 0);
	TextLabelCurPict->setText("");
}

void TransmDialog::TabWidgetEnableTabs(QTabWidget *tabWidget, bool enable)
{
#if QT_VERSION < 0x030000
	tabWidget->setEnabled(enable);
#else
	for (int i = 0; i < tabWidget->count(); i++)
# if QT_VERSION < 0x040000
		tabWidget->page(i)->setEnabled(enable);
# else
		tabWidget->widget(i)->setEnabled(enable);
# endif
#endif
}

#ifdef ENABLE_TRANSM_CODECPARAMS
void TransmDialog::ShowButtonCodec(_BOOLEAN bShow, int iKey)
{
	_BOOLEAN bLastShow = iButtonCodecState == 0;
	if (bShow)
		iButtonCodecState &= ~iKey;
	else
		iButtonCodecState |= iKey;
	bShow = iButtonCodecState == 0;
	if (bShow != bLastShow)
	{
		if (bShow)
			ButtonCodec->show();
		else
			ButtonCodec->hide();
		if (CodecDlg)
			CodecDlg->Show(bShow);
	}
}
#endif

void TransmDialog::AddWhatsThisHelp()
{
	/* Dream Logo */
	const QString strPixmapLabelDreamLogo =
		tr("<b>Dream Logo:</b> This is the official logo of "
		"the Dream software.");

	WhatsThis(PixmapLabelDreamLogo, strPixmapLabelDreamLogo);

	/* Input Level */
	const QString strInputLevel =
		tr("<b>Input Level:</b> The input level meter shows "
		"the relative input signal peak level in dB. If the level is too high, "
		"the meter turns from green to red.");

	WhatsThis(TextLabelAudioLevel, strInputLevel);
	WhatsThis(ProgrInputLevel, strInputLevel);

	/* Progress Bar */
	const QString strProgressBar =
		tr("<b>Progress Bar:</b> The progress bar shows "
		"the advancement of transmission of current file. "
		"Only meaningful when 'Data (SlideShow Application)' "
		"mode is enabled.");

	WhatsThis(ProgressBarCurPict, strProgressBar);

	/* DRM Robustness Mode */
	const QString strRobustnessMode =
		tr("<b>DRM Robustness Mode:</b> In a DRM system, "
		"four possible robustness modes are defined to adapt the system to "
		"different channel conditions. According to the DRM standard:"
		"<ul><li><i>Mode A:</i> Gaussian channels, with "
		"minor fading</li><li><i>Mode B:</i> Time "
		"and frequency selective channels, with longer delay spread"
		"</li><li><i>Mode C:</i> As robustness mode B, "
		"but with higher Doppler spread</li><li><i>Mode D:"
		"</i> As robustness mode B, but with severe delay and "
		"Doppler spread</li></ul>");

#if QT_VERSION < 0x040000
	WhatsThis(ButtonGroupRobustnessMode, strRobustnessMode);
#else
	WhatsThis(GroupBoxRobustnessMode, strRobustnessMode);
#endif
	WhatsThis(RadioButtonRMA, strRobustnessMode);
	WhatsThis(RadioButtonRMB, strRobustnessMode);
	WhatsThis(RadioButtonRMC, strRobustnessMode);
	WhatsThis(RadioButtonRMD, strRobustnessMode);

	/* Bandwidth */
	const QString strBandwidth =
		tr("<b>DRM Bandwidth:</b> The bandwith is the gross "
		"bandwidth of the generated DRM signal. Not all DRM robustness mode / "
		"bandwidth constellations are possible, e.g., DRM robustness mode D "
		"and C are only defined for the bandwidths 10 kHz and 20 kHz.");

#if QT_VERSION < 0x040000
	WhatsThis(ButtonGroupBandwidth, strBandwidth);
#else
	WhatsThis(GroupBoxBandwidth, strBandwidth);
#endif
	WhatsThis(RadioButtonBandwidth45, strBandwidth);
	WhatsThis(RadioButtonBandwidth5, strBandwidth);
	WhatsThis(RadioButtonBandwidth9, strBandwidth);
	WhatsThis(RadioButtonBandwidth10, strBandwidth);
	WhatsThis(RadioButtonBandwidth18, strBandwidth);
	WhatsThis(RadioButtonBandwidth20, strBandwidth);

	/* TODO: ComboBoxMSCConstellation, ComboBoxMSCProtLev,
	         ComboBoxSDCConstellation */

	/* MSC interleaver mode */
	const QString strInterleaver =
		tr("<b>MSC interleaver mode:</b> The symbol "
		"interleaver depth can be either short (approx. 400 ms) or long "
		"(approx. 2 s). The longer the interleaver the better the channel "
		"decoder can correct errors from slow fading signals. But the longer "
		"the interleaver length the longer the delay until (after a "
		"re-synchronization) audio can be heard.");

	WhatsThis(TextLabelInterleaver, strInterleaver);
	WhatsThis(ComboBoxMSCInterleaver, strInterleaver);

	/* Output intermediate frequency of DRM signal */
	const QString strOutputIF =
		tr("<b>Output intermediate frequency of DRM signal:</b> "
		"Set the output intermediate frequency (IF) of generated DRM signal "
		"in the 'sound-card pass-band'. In some DRM modes, the IF is located "
		"at the edge of the DRM signal, in other modes it is centered. The IF "
		"should be chosen that the DRM signal lies entirely inside the "
		"sound-card bandwidth.");

	WhatsThis(ButtonGroupIF, strOutputIF);
	WhatsThis(LineEditSndCrdIF, strOutputIF);
	WhatsThis(TextLabelIFUnit, strOutputIF);

	/* Output format */
	const QString strOutputFormat =
		tr("<b>Output format:</b> Since the sound-card "
		"outputs signals in stereo format, it is possible to output the DRM "
		"signal in three formats:<ul><li><b>real valued"
		"</b> output on both, left and right, sound-card "
		"channels</li><li><b>I / Q</b> output "
		"which is the in-phase and quadrature component of the complex "
		"base-band signal at the desired IF. In-phase is output on the "
		"left channel and quadrature on the right channel."
		"</li><li><b>E / P</b> output which is the "
		"envelope and phase on separate channels. This output type cannot "
		"be used if the Dream transmitter is regularly compiled with a "
		"sound-card sample rate of 48 kHz since the spectrum of these "
		"components exceed the bandwidth of 20 kHz.<br>The envelope signal "
		"is output on the left channel and the phase is output on the right "
		"channel.</li></ul>");

#if QT_VERSION < 0x040000
	WhatsThis(ButtonGroupOutput, strOutputFormat);
#else
	WhatsThis(GroupBoxOutput, strOutputFormat);
#endif
	WhatsThis(RadioButtonOutReal, strOutputFormat);
	WhatsThis(RadioButtonOutIQPos, strOutputFormat);
	WhatsThis(RadioButtonOutIQNeg, strOutputFormat);
	WhatsThis(RadioButtonOutEP, strOutputFormat);

	/* Current Time Transmission */
	const QString strCurrentTime =
		tr("<b>Current Time Transmission:</b> The current time is transmitted, "
		"four possible modes are defined:"
		"<ul><li><i>Off:</i> No time information is transmitted</li>"
		"<li><i>Local:</i> The local time is transmitted</li>"
		"<li><i>UTC:</i> The Coordinated Universal Time is transmitted</li>"
		"<li><i>UTC+Offset:</i> Same as UTC but with the addition of an offset "
		"in hours from local time</li></ul>");

# if QT_VERSION < 0x040000
	WhatsThis(ButtonGroupCurrentTime, strCurrentTime);
# else
	WhatsThis(GroupBoxCurrentTime, strCurrentTime);
# endif
	WhatsThis(RadioButtonCurTimeOff, strCurrentTime);
	WhatsThis(RadioButtonCurTimeLocal, strCurrentTime);
	WhatsThis(RadioButtonCurTimeUTC, strCurrentTime);
	WhatsThis(RadioButtonCurTimeUTCOffset, strCurrentTime);

	/* TODO: Services... */

	/* Data (SlideShow Application) */

	/* Remove path from filename */
	const QString strRemovePath =
		tr("<b>Remove path from filename:</b> Un-checking this box will "
		"transmit the full path of the image location on your computer. "
		"This might not be what you want.");

	WhatsThis(CheckBoxRemovePath, strRemovePath);
}
