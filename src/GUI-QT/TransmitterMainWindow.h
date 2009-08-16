/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Julian Cable
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

#ifndef _TRANSMITTERMAINWINDOW_H
#define _TRANSMITTERMAINWINDOW_H

#include "../Parameter.h"
#include "ui_TransmitterMainWindow.h"
#include "../util/Utilities.h"
#include "DIoutputSelector.h"
#include <vector>
#include <QDialog>
#include <QTimer>
#include <QButtonGroup>
#include <QStandardItemModel>
#include <QStringListModel>

/* Classes ********************************************************************/
class CDRMTransmitterInterface;
class CSettings;

class ChannelEditor: public QObject
{
	Q_OBJECT
public:
    ChannelEditor(Ui_TransmitterMainWindow&);
    virtual ~ChannelEditor() {}

    void    setupUi();
	void	GetFrom(const CDRMTransmitterInterface&);
	void	PutTo(CDRMTransmitterInterface&) const;

public slots:
	void OnComboBoxSDCConstellationActivated(int iID);
	void OnComboBoxMSCInterleaverActivated(int iID);
	void OnComboBoxMSCConstellationActivated(int iID);
	void OnComboBoxMSCProtLevActivated(int iID);
	void OnRadioBandwidth(int iID);
	void OnRadioRobustnessMode(int iID);
    void OnMSCCapacityChanged(int);
    void OnSDCCapacityChanged(int);

signals:

    void MSCCapacityChanged();
    void SDCCapacityChanged();

protected:
	void	UpdateMSCProtLevCombo();

    Ui_TransmitterMainWindow& ui;
    QButtonGroup* robustnessGroup;
    QButtonGroup* soGroup;
};

class StreamEditor: public QObject
{
	Q_OBJECT
public:
    StreamEditor(Ui_TransmitterMainWindow&);
    virtual ~StreamEditor() {}

    void    setupUi();
	void	GetFrom(const CDRMTransmitterInterface&);
	void	PutTo(CDRMTransmitterInterface&) const;

public slots:
	void OnButtonAddStream();
	void OnButtonDeleteStream();
	void OnComboBoxStreamTypeActivated(int item);
	void OnComboBoxPacketsPerFrameActivated(const QString& str);
	void OnLineEditPacketLenChanged(const QString& str);
	void OnItemClicked(const QModelIndex&);
    void OnMSCCapacityChanged(int);
protected:
    Ui_TransmitterMainWindow& ui;
    QStandardItemModel* streams;
};

class AudioComponentEditor: public QObject
{
	Q_OBJECT
public:
    AudioComponentEditor(Ui_TransmitterMainWindow&);
    virtual ~AudioComponentEditor() {}

    void    setupUi();
	void	GetFrom(const CDRMTransmitterInterface&);
	void	PutTo(CDRMTransmitterInterface&) const;

public slots:
	void OnComboBoxSourceActivated(int iID);
	void OnToggleCheckBoxEnableTextMessage(bool bState);
	void OnPushButtonAddText();
	void OnPushButtonDeleteText();
	void OnButtonClearAllText();
	void OnButtonFileBrowse();
	void OnLineEditFileChanged(const QString& str);
	void OnToggleCheckBoxSourceIsFile(bool bState);
protected:
	void EnableTextMessage(const bool bFlag);
	void EnableAudio(const bool bFlag);

    Ui_TransmitterMainWindow& ui;
    QStandardItemModel* textMessages;
};

class DataComponentEditor: public QObject
{
	Q_OBJECT
public:
    DataComponentEditor(Ui_TransmitterMainWindow&);
    virtual ~DataComponentEditor() {}

    void    setupUi();
	void	GetFrom(const CDRMTransmitterInterface&);
	void	PutTo(CDRMTransmitterInterface&) const;

public slots:
	void OnPushButtonAddFileName();
	void OnButtonClearAllFileNames();
protected:
	void EnableData(const bool bFlag);
    void AddSlide(const QString& path);

    Ui_TransmitterMainWindow& ui;
    QStandardItemModel* pictures;
};

class ServicesEditor: public QObject
{
	Q_OBJECT
public:
    ServicesEditor(Ui_TransmitterMainWindow&);
    virtual ~ServicesEditor() {}

    void    setupUi();
	void	GetFrom(const CDRMTransmitterInterface&);
	void	PutTo(CDRMTransmitterInterface&) const;

public slots:
	void OnTextChangedLabel(const QString& strLabel);
	void OnTextChangedServiceID(const QString& strID);
	void OnButtonAdd();
	void OnButtonDelete();
	void OnItemClicked(const QModelIndex&);
protected:
    Ui_TransmitterMainWindow& ui;
    QStandardItemModel* services;
};

class COFDMEditor: public QObject
{
	Q_OBJECT
public:
    COFDMEditor(Ui_TransmitterMainWindow&);
    virtual ~COFDMEditor() {}

    void    setupUi();
	void	GetFrom(const CDRMTransmitterInterface&);
	void	PutTo(CDRMTransmitterInterface&) const;

public slots:
	void OnComboBoxDestActivated(int iID);
	void OnComboBoxFileDestActivated(int iID);
	void OnTextChangedSndCrdIF(const QString& strIF);
	void OnButtonAddAudio();
	void OnLineEditFileChanged(const QString& str);
	void OnButtonAddFile();
	void OnButtonDeleteSelected();
	void OnButtonBrowse();
	void OnItemClicked(const QModelIndex&);
protected:
    Ui_TransmitterMainWindow& ui;
    QButtonGroup* outputGroup;
    QStandardItemModel* destinations;
};

class MDIInputEditor: public QObject
{
	Q_OBJECT
public:
    MDIInputEditor(Ui_TransmitterMainWindow&);
    virtual ~MDIInputEditor() {}

    void    setupUi();
	void	GetFrom(const CDRMTransmitterInterface&);
	void	PutTo(CDRMTransmitterInterface&) const;

public slots:
	void OnLineEditPortChanged(const QString& str);
	void OnToggleCheckBoxMcast(bool bState);
	void OnLineEditGroupChanged(const QString& str);
	void OnComboBoxInterfaceActivated(int iID);
	void OnLineEditFileChanged(const QString& str);
	void OnButtonBrowse();
	void OnToggleCheckBoxReadFile(bool bState);
protected:
    Ui_TransmitterMainWindow& ui;
	vector<CIpIf>       vecIpIf;
};

class TransmitterMainWindow : public QMainWindow, public Ui_TransmitterMainWindow
{
	Q_OBJECT

public:
	TransmitterMainWindow(
		CDRMTransmitterInterface& tx,
		CSettings&,
		QWidget* parent=0, const char* name=0, Qt::WFlags f=0);
	virtual ~TransmitterMainWindow();
	/* dummy assignment operator to help MSVC8 */
	TransmitterMainWindow& operator=(const TransmitterMainWindow&)
	{ throw "should not happen"; return *this;}

	void	GetFrom(const CDRMTransmitterInterface&);
	void	PutTo(CDRMTransmitterInterface&) const;

protected:

	void		DisableAllControlsForSet();
	void		EnableAllControlsForSet();

	void		closeEvent(QCloseEvent* ce);

	CDRMTransmitterInterface&	DRMTransmitter;
	CSettings&			Settings;
	QTimer				Timer;
	bool			    bIsStarted;

    ChannelEditor       channelEditor;
    StreamEditor        streamEditor;
    AudioComponentEditor    audioComponentEditor;
    DataComponentEditor dataComponentEditor;
    ServicesEditor      servicesEditor;
    COFDMEditor         cofdmEditor;
    MDIInputEditor      mdiInputEditor;
    DIoutputSelector     mdiOutputEditor;

public slots:
	void OnRadioMode(int iID);
	void OnButtonStartStop();
	void OnButtonClose();
	void OnTimer();
	void OnHelpWhatsThis();
    void OnMSCCapacityChanged();
    void OnSDCCapacityChanged();
signals:
    void MSCCapacityChanged(int);
    void SDCCapacityChanged(int);
};

#endif
