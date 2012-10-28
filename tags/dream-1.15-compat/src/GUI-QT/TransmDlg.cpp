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
# include <qpopupmenu.h>
# include <qbuttongroup.h>
# include <qwhatsthis.h>
# include <qmultilineedit.h>
# include <qlistview.h>
# include <qfiledialog.h>
# include <qpopupmenu.h>
# include <qprogressbar.h>
#else
# include <QWhatsThis>
# include <QFileDialog>
# include <QHideEvent>
# define CHECK_PTR(x) Q_CHECK_PTR(x)
#endif

TransmDialog::TransmDialog(CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f)
	:
	TransmDlgBase(parent, name, modal, f),
	Settings(NSettings), bIsStarted(FALSE),
	vecstrTextMessage(1) /* 1 for new text */, iIDCurrentText(0)
{
	/* recover window size and position */
	CWinGeom s;
	Settings.Get("Transmit Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	SetDialogCaption(this, tr("Dream DRM Transmitter"));

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Set controls to custom behavior */
#if QT_VERSION < 0x040000
	MultiLineEditTextMessage->setWordWrap(QMultiLineEdit::WidgetWidth);
	MultiLineEditTextMessage->setEdited(FALSE);
	ComboBoxTextMessage->insertItem(tr("new"), 0);
#else
	// TODO put in ui file
#endif
	UpdateMSCProtLevCombo();

	/* Init controls with default settings */
	ButtonStartStop->setText(tr("&Start"));

	/* Init progress bar for input signal level */
	ProgrInputLevel->setRange(-50.0, 0.0);
#if QT_VERSION < 0x040000
	ProgrInputLevel->setOrientation(QwtThermo::Horizontal, QwtThermo::Bottom);
#else
	ProgrInputLevel->setOrientation(Qt::Horizontal, QwtThermo::BottomScale);
#endif
        QBrush fillBrush(QColor(0, 190, 0));
        ProgrInputLevel->setFillBrush(fillBrush);
	ProgrInputLevel->setAlarmLevel(-5.0);
        QBrush alarmBrush(QColor(255, 0, 0));
        ProgrInputLevel->setAlarmBrush(alarmBrush);

	/* Init progress bar for current transmitted picture */
#if QT_VERSION < 0x040000
	ProgressBarCurPict->setTotalSteps(100);
	ProgressBarCurPict->setProgress(0);
#else
	ProgressBarCurPict->setRange(0,100);
	ProgressBarCurPict->setValue(0);
#endif
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

	/* Don't lock the Parameter object since the working thread is stopped */

	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

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
		RadioButtonRMC->setChecked(TRUE);
		break;

	case RM_ROBUSTNESS_MODE_D:
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
	/* MSC interleaver mode */
	ComboBoxMSCInterleaver->insertItem(tr("2 s (Long Interleaving)"), 0);
	ComboBoxMSCInterleaver->insertItem(tr("400 ms (Short Interleaving)"), 1);

	switch (Parameters.eSymbolInterlMode)
	{
	case CParameter::SI_LONG:
		ComboBoxMSCInterleaver->setCurrentItem(0);
		break;

	case CParameter::SI_SHORT:
		ComboBoxMSCInterleaver->setCurrentItem(1);
		break;
	}

	/* MSC Constellation Scheme */
	ComboBoxMSCConstellation->insertItem(tr("SM 16-QAM"), 0);
	ComboBoxMSCConstellation->insertItem(tr("SM 64-QAM"), 1);

// These modes should not be used right now, TODO
//	ComboBoxMSCConstellation->insertItem(tr("HMsym 64-QAM"), 2);
//	ComboBoxMSCConstellation->insertItem(tr("HMmix 64-QAM"), 3);

	switch (Parameters.eMSCCodingScheme)
	{
	case CS_1_SM:
		break;

	case CS_2_SM:
		ComboBoxMSCConstellation->setCurrentItem(0);
		break;

	case CS_3_SM:
		ComboBoxMSCConstellation->setCurrentItem(1);
		break;

	case CS_3_HMSYM:
//		ComboBoxMSCConstellation->setCurrentItem(2);
		break;

	case CS_3_HMMIX:
		ComboBoxMSCConstellation->setCurrentItem(3);
		break;
	}

	/* SDC Constellation Scheme */
	ComboBoxSDCConstellation->insertItem(tr("4-QAM"), 0);
	ComboBoxSDCConstellation->insertItem(tr("16-QAM"), 1);

	switch (Parameters.eSDCCodingScheme)
	{
	case CS_1_SM:
		ComboBoxSDCConstellation->setCurrentItem(0);
		break;

	case CS_2_SM:
		ComboBoxSDCConstellation->setCurrentItem(1);
		break;

	case CS_3_SM:
	case CS_3_HMSYM:
	case CS_3_HMMIX:
		break;
	}
#else
	// TODO
#endif

	/* Service parameters --------------------------------------------------- */
	/* Service label */
	CService& Service = Parameters.Service[0];
	QString label = QString::fromUtf8(Service.strLabel.c_str());
	LineEditServiceLabel->setText(label);

	/* Service ID */
	LineEditServiceID->setText(QString().setNum((int) Service.iServiceID, 16));

#if QT_VERSION < 0x040000

	int i;
	/* Language */
	for (i = 0; i < LEN_TABLE_LANGUAGE_CODE; i++)
		ComboBoxLanguage->insertItem(strTableLanguageCode[i].c_str(), i);

	ComboBoxLanguage->setCurrentItem(Service.iLanguage);

	/* Program type */
	for (i = 0; i < LEN_TABLE_PROG_TYPE_CODE; i++)
		ComboBoxProgramType->insertItem(strTableProgTypCod[i].c_str(), i);

	ComboBoxProgramType->setCurrentItem(Service.iServiceDescr);
#else
	// TODO
#endif

	/* Sound card IF */
	LineEditSndCrdIF->setText(QString().number(
		TransThread.DRMTransmitter.GetCarOffset(), 'f', 2));

#if QT_VERSION < 0x040000
	/* Clear list box for file names and set up columns */
	ListViewFileNames->clear();

	/* We assume that one column is already there */
	ListViewFileNames->setColumnText(0, "File Name");
	ListViewFileNames->addColumn("Size [KB]");
	ListViewFileNames->addColumn("Full Path");

	/* Disable other three services */
	TabWidgetServices->setTabEnabled(tabService2, FALSE);
	TabWidgetServices->setTabEnabled(tabService3, FALSE);
	TabWidgetServices->setTabEnabled(tabService4, FALSE);
#else
	// TODO
#endif
	CheckBoxEnableService->setChecked(TRUE);
	CheckBoxEnableService->setEnabled(FALSE);


	/* Let this service be an audio service for initialization */
	/* Set audio enable check box */
	CheckBoxEnableAudio->setChecked(TRUE);
	EnableAudio(TRUE);
	CheckBoxEnableData->setChecked(FALSE);
	EnableData(FALSE);


	/* Add example text message at startup ---------------------------------- */
	/* Activate text message */
	EnableTextMessage(TRUE);
	CheckBoxEnableTextMessage->setChecked(TRUE);

	/* Add example text in internal container */
	vecstrTextMessage.Add(tr("Dream DRM Transmitter\x0B\x0AThis is a test transmission").latin1());

	/* Insert item in combo box, display text and set item to our text */
#if QT_VERSION < 0x040000
	ComboBoxTextMessage->insertItem(QString().setNum(1), 1);
	ComboBoxTextMessage->setCurrentItem(1);
	MultiLineEditTextMessage->insertLine(vecstrTextMessage[1].c_str());
#else
		// TODO
#endif
	iIDCurrentText = 1;

	/* Now make sure that the text message flag is activated in global struct */
	Service.AudioParam.bTextflag = TRUE;


	/* Enable all controls */
	EnableAllControlsForSet();


	/* Set Menu ***************************************************************/
	/* Settings menu  ------------------------------------------------------- */
#if QT_VERSION < 0x040000
	pSettingsMenu = new QPopupMenu(this);
	CHECK_PTR(pSettingsMenu);
	pSettingsMenu->insertItem(tr("&Sound Card Selection"),
		new CSoundCardSelMenu(
			TransThread.DRMTransmitter.GetSoundInInterface(),
			TransThread.DRMTransmitter.GetSoundOutInterface(), this)
	);
	/* Main menu bar */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&Settings"), pSettingsMenu);
	pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	TransmDlgBaseLayout->setMenuBar(pMenu);
#else
//TODO
#endif


	/* Connections ---------------------------------------------------------- */
	connect(ButtonStartStop, SIGNAL(clicked()),
		this, SLOT(OnButtonStartStop()));
	connect(PushButtonAddText, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddText()));
	connect(PushButtonClearAllText, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllText()));
	connect(PushButtonAddFile, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddFileName()));
	connect(PushButtonClearAllFileNames, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllFileNames()));
	connect(CheckBoxEnableTextMessage, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableTextMessage(bool)));
	connect(CheckBoxEnableAudio, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableAudio(bool)));
	connect(CheckBoxEnableData, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableData(bool)));

	/* Combo boxes */
	connect(ComboBoxMSCInterleaver, SIGNAL(highlighted(int)),
		this, SLOT(OnComboBoxMSCInterleaverHighlighted(int)));
	connect(ComboBoxMSCConstellation, SIGNAL(highlighted(int)),
		this, SLOT(OnComboBoxMSCConstellationHighlighted(int)));
	connect(ComboBoxSDCConstellation, SIGNAL(highlighted(int)),
		this, SLOT(OnComboBoxSDCConstellationHighlighted(int)));
	connect(ComboBoxLanguage, SIGNAL(highlighted(int)),
		this, SLOT(OnComboBoxLanguageHighlighted(int)));
	connect(ComboBoxProgramType, SIGNAL(highlighted(int)),
		this, SLOT(OnComboBoxProgramTypeHighlighted(int)));
	connect(ComboBoxTextMessage, SIGNAL(highlighted(int)),
		this, SLOT(OnComboBoxTextMessageHighlighted(int)));
	connect(ComboBoxMSCProtLev, SIGNAL(highlighted(int)),
		this, SLOT(OnComboBoxMSCProtLevHighlighted(int)));

	/* Button groups */
	connect(ButtonGroupRobustnessMode, SIGNAL(clicked(int)),
		this, SLOT(OnRadioRobustnessMode(int)));
	connect(ButtonGroupBandwidth, SIGNAL(clicked(int)),
		this, SLOT(OnRadioBandwidth(int)));
	connect(ButtonGroupBandwidth, SIGNAL(clicked(int)),
		this, SLOT(OnRadioBandwidth(int)));
	connect(ButtonGroupOutput, SIGNAL(clicked(int)),
		this, SLOT(OnRadioOutput(int)));

	/* Line edits */
	connect(LineEditServiceLabel, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceLabel(const QString&)));
	connect(LineEditServiceID, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceID(const QString&)));
	connect(LineEditSndCrdIF, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedSndCrdIF(const QString&)));

	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));


	/* Set timer for real-time controls */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

TransmDialog::~TransmDialog()
{
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
}

void TransmDialog::OnHelpWhatsThis()
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
#if QT_VERSION < 0x040000
			ProgressBarCurPict->setProgress((int) (rCPercent * 100)); /* % */
#else
			ProgressBarCurPict->setValue((int) (rCPercent * 100)); /* % */
#endif
		}
		else
		{
			/* Disable controls */
			ProgressBarCurPict->setEnabled(FALSE);
			TextLabelCurPict->setEnabled(FALSE);
		}
	}
}

void TransmDialog::OnButtonStartStop()
{
	int i;

	if (bIsStarted == TRUE)
	{
		/* Stop transmitter */
		TransThread.Stop();

		ButtonStartStop->setText(tr("&Start"));

		EnableAllControlsForSet();

		bIsStarted = FALSE;
	}
	else
	{
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
				SetPicFileName(strFileName.latin1(), strFormat.latin1());
		}
#endif

		TransThread.start();

		ButtonStartStop->setText(tr("&Stop"));

		DisableAllControlsForSet();

		bIsStarted = TRUE;
	}
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
#if QT_VERSION < 0x040000
		MultiLineEditTextMessage->setEnabled(TRUE);
#endif
		PushButtonAddText->setEnabled(TRUE);
		PushButtonClearAllText->setEnabled(TRUE);

		/* Set text message flag in global struct */
		Parameters.Service[0].AudioParam.bTextflag = TRUE;
	}
	else
	{
		/* Disable text message controls */
		ComboBoxTextMessage->setEnabled(FALSE);
#if QT_VERSION < 0x040000
		MultiLineEditTextMessage->setEnabled(FALSE);
#endif
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
	}
	else
	{
		/* Set audio enable check box */
		CheckBoxEnableData->setChecked(TRUE);
		EnableData(TRUE);
	}
}

void TransmDialog::EnableAudio(const _BOOLEAN bFlag)
{
	if (bFlag == TRUE)
	{
		/* Enable audio controls */
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

		/* Programme Type code, get it from combo box */
#if QT_VERSION < 0x040000
		Parameters.Service[0].iServiceDescr
				= ComboBoxProgramType->currentItem();
#else
		// TODO
#endif
		Parameters.Unlock();
	}
	else
	{
		/* Disable audio controls */
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
	if (bFlag == TRUE)
	{
		/* Enable data controls */
#if QT_VERSION < 0x040000
		ListViewFileNames->setEnabled(TRUE);
#endif
		PushButtonClearAllFileNames->setEnabled(TRUE);
		PushButtonAddFile->setEnabled(TRUE);

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
	else
	{
		/* Disable data controls */
#if QT_VERSION < 0x040000
		ListViewFileNames->setEnabled(FALSE);
#endif
		PushButtonClearAllFileNames->setEnabled(FALSE);
		PushButtonAddFile->setEnabled(FALSE);
	}
}

_BOOLEAN TransmDialog::GetMessageText(const int iID)
{
	_BOOLEAN bTextIsNotEmpty = TRUE;
#if QT_VERSION < 0x040000

	/* Check if text control is not empty */
	if (MultiLineEditTextMessage->edited())
	{
		/* Check size of container. If not enough space, enlarge */
		if (iID == vecstrTextMessage.Size())
			vecstrTextMessage.Enlarge(1);

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
	}
	else
		bTextIsNotEmpty = FALSE;

#else
	// TODO
	(void)iID;
#endif

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
#if QT_VERSION < 0x040000
			ComboBoxTextMessage->insertItem(QString().setNum(iNewID), iNewID);
			/* Clear added text */
			MultiLineEditTextMessage->clear();
			MultiLineEditTextMessage->setEdited(FALSE);
#else
			// TODO
			(void)iNewID;
#endif
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
	ComboBoxTextMessage->clear();
#if QT_VERSION < 0x040000
	ComboBoxTextMessage->insertItem(tr("new"), 0);
	/* Clear multi line edit */
	MultiLineEditTextMessage->clear();
	MultiLineEditTextMessage->setEdited(FALSE);
#else
	// TODO
#endif
}

void TransmDialog::OnPushButtonAddFileName()
{
	/* Show "open file" dialog. Let the user select more than one file */
#if QT_VERSION < 0x040000
	QStringList list = QFileDialog::getOpenFileNames(tr("Image Files (*.png *.jpg *.jpeg *.jfif)"), NULL, this);

	/* Check if user not hit the cancel button */
	if (!list.isEmpty())
	{
		/* Insert all selected file names */
		for (QStringList::Iterator it = list.begin(); it != list.end(); it++)
		{
			QFileInfo FileInfo((*it));

			/* Insert list view item. The object which is created here will be
			   automatically destroyed by QT when the parent
			   ("ListViewFileNames") is destroyed */
			ListViewFileNames->insertItem(
				new QListViewItem(ListViewFileNames, FileInfo.fileName(),
				QString().setNum((float) FileInfo.size() / 1000.0, 'f', 2),
				FileInfo.filePath()));
		}
	}
#else
	// TODO
#endif
}

void TransmDialog::OnButtonClearAllFileNames()
{
	/* Clear list box for file names */
#if QT_VERSION < 0x040000
	ListViewFileNames->clear();
#endif
}

void TransmDialog::OnComboBoxTextMessageHighlighted(int iID)
{
	iIDCurrentText = iID;

	/* Set text control with selected message */
#if QT_VERSION < 0x040000
	MultiLineEditTextMessage->clear();
	MultiLineEditTextMessage->setEdited(FALSE);
#endif
	if (iID != 0)
	{
		/* Write stored text in multi line edit control */
#if QT_VERSION < 0x040000
		MultiLineEditTextMessage->insertLine(vecstrTextMessage[iID].c_str());
#else
		// TODO
#endif
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
#if QT_VERSION < 0x040000
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();

	Parameters.Lock();
	/* Set additional text for log file. Conversion from QString to STL
	   string is needed (done with .utf8() function of QT string) */
	Parameters.Service[0].strLabel = strLabel.utf8().data();
	Parameters.Unlock();
#else
	// TODO
	(void)strLabel;
#endif
}

void TransmDialog::OnComboBoxMSCInterleaverHighlighted(int iID)
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

void TransmDialog::OnComboBoxMSCConstellationHighlighted(int iID)
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

void TransmDialog::OnComboBoxMSCProtLevHighlighted(int iID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	Parameters.MSCPrLe.iPartB = iID;
	Parameters.Unlock();
}

void TransmDialog::UpdateMSCProtLevCombo()
{
#if QT_VERSION < 0x040000
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	if (Parameters.eMSCCodingScheme == CS_2_SM)
	{
		/* Only two protection levels possible in 16 QAM mode */
		ComboBoxMSCProtLev->clear();
		ComboBoxMSCProtLev->insertItem("0", 0);
		ComboBoxMSCProtLev->insertItem("1", 1);
	}
	else
	{
		/* Four protection levels defined */
		ComboBoxMSCProtLev->clear();
		ComboBoxMSCProtLev->insertItem("0", 0);
		ComboBoxMSCProtLev->insertItem("1", 1);
		ComboBoxMSCProtLev->insertItem("2", 2);
		ComboBoxMSCProtLev->insertItem("3", 3);
	}

	/* Set protection level to 1 */
	ComboBoxMSCProtLev->setCurrentItem(1);
	Parameters.MSCPrLe.iPartB = 1;
	Parameters.Unlock();
#else
	// TODO
#endif
}

void TransmDialog::OnComboBoxSDCConstellationHighlighted(int iID)
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

void TransmDialog::OnComboBoxLanguageHighlighted(int iID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	Parameters.Service[0].iLanguage = iID;
	Parameters.Unlock();
}

void TransmDialog::OnComboBoxProgramTypeHighlighted(int iID)
{
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	Parameters.Service[0].iServiceDescr = iID;
	Parameters.Unlock();
}

void TransmDialog::OnRadioOutput(int iID)
{
	switch (iID)
	{
	case 0:
		/* Button "Real Valued" */
		TransThread.DRMTransmitter.GetTransData()->
			SetIQOutput(CTransmitData::OF_REAL_VAL);
		break;

	case 1:
		/* Button "I / Q (pos)" */
		TransThread.DRMTransmitter.GetTransData()->
			SetIQOutput(CTransmitData::OF_IQ_POS);
		break;

	case 2:
		/* Button "I / Q (neg)" */
		TransThread.DRMTransmitter.GetTransData()->
			SetIQOutput(CTransmitData::OF_IQ_NEG);
		break;

	case 3:
		/* Button "E / P" */
		TransThread.DRMTransmitter.GetTransData()->
			SetIQOutput(CTransmitData::OF_EP);
		break;
	}
}

void TransmDialog::OnRadioRobustnessMode(int iID)
{
	/* Check, which bandwith's are possible with this robustness mode */
	switch (iID)
	{
	case 0:
		/* All bandwidth modes are possible */
		RadioButtonBandwidth45->setEnabled(TRUE);
		RadioButtonBandwidth5->setEnabled(TRUE);
		RadioButtonBandwidth9->setEnabled(TRUE);
		RadioButtonBandwidth10->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(TRUE);
		RadioButtonBandwidth20->setEnabled(TRUE);
		break;

	case 1:
		/* All bandwidth modes are possible */
		RadioButtonBandwidth45->setEnabled(TRUE);
		RadioButtonBandwidth5->setEnabled(TRUE);
		RadioButtonBandwidth9->setEnabled(TRUE);
		RadioButtonBandwidth10->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(TRUE);
		RadioButtonBandwidth20->setEnabled(TRUE);
		break;

	case 2:
		/* Only 10 and 20 kHz possible in robustness mode C */
		RadioButtonBandwidth45->setEnabled(FALSE);
		RadioButtonBandwidth5->setEnabled(FALSE);
		RadioButtonBandwidth9->setEnabled(FALSE);
		RadioButtonBandwidth10->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(FALSE);
		RadioButtonBandwidth20->setEnabled(TRUE);

		/* Set check on a default value to be sure we are "in range" */
		RadioButtonBandwidth10->setChecked(TRUE);
		OnRadioBandwidth(3); /* TODO better solution for that */
		break;

	case 3:
		/* Only 10 and 20 kHz possible in robustness mode D */
		RadioButtonBandwidth45->setEnabled(FALSE);
		RadioButtonBandwidth5->setEnabled(FALSE);
		RadioButtonBandwidth9->setEnabled(FALSE);
		RadioButtonBandwidth10->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(FALSE);
		RadioButtonBandwidth20->setEnabled(TRUE);

		/* Set check on a default value to be sure we are "in range" */
		RadioButtonBandwidth10->setChecked(TRUE);
		OnRadioBandwidth(3); /* TODO better solution for that */
		break;
	}


	/* Set new parameters */
	ERobMode eNewRobMode = RM_NO_MODE_DETECTED;

	switch (iID)
	{
	case 0:
		eNewRobMode = RM_ROBUSTNESS_MODE_A;
		break;

	case 1:
		eNewRobMode = RM_ROBUSTNESS_MODE_B;
		break;

	case 2:
		eNewRobMode = RM_ROBUSTNESS_MODE_C;
		break;

	case 3:
		eNewRobMode = RM_ROBUSTNESS_MODE_D;
		break;
	}

	/* Set new robustness mode. Spectrum occupancy is the same as before */
	CParameter& Parameters = *TransThread.DRMTransmitter.GetParameters();
	Parameters.Lock();
	Parameters.InitCellMapTable(eNewRobMode, Parameters.GetSpectrumOccup());
	Parameters.Unlock();
}

void TransmDialog::OnRadioBandwidth(int iID)
{
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

	Parameters.Lock();

	/* Set new spectrum occupancy. Robustness mode is the same as before */
	Parameters.InitCellMapTable(Parameters.GetWaveMode(), eNewSpecOcc);

	Parameters.Unlock();
}

void TransmDialog::DisableAllControlsForSet()
{
	GroupBoxChanParam->setEnabled(FALSE);
	TabWidgetServices->setEnabled(FALSE);
#if QT_VERSION < 0x040000
	ButtonGroupOutput->setEnabled(FALSE);
#endif
	GroupInput->setEnabled(TRUE); /* For run-mode */
}

void TransmDialog::EnableAllControlsForSet()
{
	GroupBoxChanParam->setEnabled(TRUE);
	TabWidgetServices->setEnabled(TRUE);
#if QT_VERSION < 0x040000
	ButtonGroupOutput->setEnabled(TRUE);
#else
	// TODO
#endif
	GroupInput->setEnabled(FALSE); /* For run-mode */

	/* Reset status bars */
	ProgrInputLevel->setValue(RET_VAL_LOG_0);
#if QT_VERSION < 0x040000
	ProgressBarCurPict->setProgress(0);
#else
	ProgressBarCurPict->setValue(0);
#endif
	TextLabelCurPict->setText("");
}

void TransmDialog::AddWhatsThisHelp()
{
#if QT_VERSION < 0x040000
	/* Dream Logo */
	QWhatsThis::add(PixmapLabelDreamLogo,
		tr("<b>Dream Logo:</b> This is the official logo of "
		"the Dream software."));

	/* Input Level */
	QWhatsThis::add(ProgrInputLevel,
		tr("<b>Input Level:</b> The input level meter shows "
		"the relative input signal peak level in dB. If the level is too high, "
		"the meter turns from green to red."));

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

	QWhatsThis::add(RadioButtonRMA, strRobustnessMode);
	QWhatsThis::add(RadioButtonRMB, strRobustnessMode);
	QWhatsThis::add(RadioButtonRMC, strRobustnessMode);
	QWhatsThis::add(RadioButtonRMD, strRobustnessMode);

	/* Bandwidth */
	const QString strBandwidth =
		tr("<b>DRM Bandwidth:</b> The bandwith is the gross "
		"bandwidth of the generated DRM signal. Not all DRM robustness mode / "
		"bandwidth constellations are possible, e.g., DRM robustness mode D "
		"and C are only defined for the bandwidths 10 kHz and 20 kHz.");

	QWhatsThis::add(RadioButtonBandwidth45, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth5, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth9, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth10, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth18, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth20, strBandwidth);

	/* Output intermediate frequency of DRM signal */
	const QString strOutputIF =
		tr("<b>Output intermediate frequency of DRM signal:</b> "
		"Set the output intermediate frequency (IF) of generated DRM signal "
		"in the 'sound-card pass-band'. In some DRM modes, the IF is located "
		"at the edge of the DRM signal, in other modes it is centered. The IF "
		"should be chosen that the DRM signal lies entirely inside the "
		"sound-card bandwidth.");

	QWhatsThis::add(TextLabelIF, strOutputIF);
	QWhatsThis::add(LineEditSndCrdIF, strOutputIF);
	QWhatsThis::add(TextLabelIFUnit, strOutputIF);

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

	QWhatsThis::add(RadioButtonOutReal, strOutputFormat);
	QWhatsThis::add(RadioButtonOutIQPos, strOutputFormat);
	QWhatsThis::add(RadioButtonOutIQNeg, strOutputFormat);
	QWhatsThis::add(RadioButtonOutEP, strOutputFormat);

	/* MSC interleaver mode */
	const QString strInterleaver =
		tr("<b>MSC interleaver mode:</b> The symbol "
		"interleaver depth can be either short (approx. 400 ms) or long "
		"(approx. 2 s). The longer the interleaver the better the channel "
		"decoder can correct errors from slow fading signals. But the longer "
		"the interleaver length the longer the delay until (after a "
		"re-synchronization) audio can be heard.");

	QWhatsThis::add(TextLabelInterleaver, strInterleaver);
	QWhatsThis::add(ComboBoxMSCInterleaver, strInterleaver);
#else
	// TODO
#endif
}
