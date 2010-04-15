/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable, Andrea Russo
 *
 * Description:
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

#ifndef __RECEIVERSETTINGSDLG_H
#define __RECEIVERSETTINGSDLG_H

#include "../util/Settings.h"
#include "../util/Hamlib.h"
#include "../selectioninterface.h"
#include "../GPSData.h"
#include "ui_ReceiverSettingsDlg.h"
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include <QAbstractItemModel>

/* Definitions ****************************************************************/

/* Classes ********************************************************************/

#include "../util/Hamlib.h"
#include <set>

class RigTypesModel : public QAbstractItemModel
{
public:

    RigTypesModel();
    virtual ~RigTypesModel() {}

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    QModelIndex index(int row, int column,
		  const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    void load();

protected:

    struct model_index { int parent; virtual ~model_index(){} };
    struct rig : public model_index { string name; rig_model_t model; };
    struct make : public model_index {
	string name;
	vector<rig> model;
    };
    vector<make> rigs;
    static int rig_enumerator(const rig_caps *, void *);
};

class RigModel : public QAbstractItemModel
{
public:

    RigModel(CSettings& s);
    virtual ~RigModel() {}

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    QModelIndex index(int row, int column,
		  const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    void add(rig_model_t, bool=false);
    void remove(int);

    struct parms
    {
    	parms():levels(),mode_for_drm(RIG_MODE_NONE),width_for_drm(0),offset(0){}
    	map<string,int> levels;
    	rmode_t mode_for_drm;
    	pbwidth_t width_for_drm;
    	int offset;
    };

protected:

    CSettings&  settings;
    set<int> rigs;
    map<rig_model_t,parms> unmodified, modified;
};

class SoundChoice : public QAbstractItemModel
{
public:

    SoundChoice(const CSelectionInterface&, bool t=false);
    virtual ~SoundChoice() {}

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    QModelIndex index(int row, int column,
		  const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent(const QModelIndex &child) const;
    void update();

protected:
    bool isTree;
    const CSelectionInterface& interface;
    struct port { string name; int index; int parent; };
    struct card : public port { vector<port> members;};
    vector<card> cards;
};


class ReceiverSettingsDlg : public QDialog, public Ui_ReceiverSettingsDlg
{
	Q_OBJECT

public:

    ReceiverSettingsDlg(CSettings&, CGPSData&,
			CSelectionInterface&, CSelectionInterface&,
			QWidget* parent = 0, Qt::WFlags f = 0);
    virtual ~ReceiverSettingsDlg();

protected:
    void		showEvent(QShowEvent* pEvent);
    void		hideEvent(QHideEvent* pEvent);

    bool		ValidInput(const QLineEdit* le);
    void		loadDRMSettings();
    void		saveDRMSettings();
    void		loadOutputSettings();
    void		saveOutputSettings();
    void		loadLogfileSettings();
    void		saveLogfileSettings();
    void		loadGPSSettings();
    void		saveGPSSettings();
    void		setLatLngDlg(double latitude, double longitude);

    void		AddWhatsThisHelp();

    CSettings&	Settings;
    bool		loading;
    QTimer		TimerRig;
    int			iWantedrigModel;
    QButtonGroup	*bgTimeInterp, *bgFreqInterp, *bgTiSync;
    QButtonGroup	*bgriq, *bglrm, *bgiq;

    RigTypesModel	RigTypes;
    RigModel		Rigs;
    CGPSData&		GPSData;
    SoundChoice*	soundinputs;
    SoundChoice*	soundoutputs;

signals:
    void 		StartStopLog(bool);
    void 		LogPosition(bool);
    void 		LogSigStr(bool);
    void 		SetLogStartDelay(long);
    void 		StartStopGPS(bool);
    void 		ShowHideGPS(bool);
    void 		ReConfigureReceiver();

public slots:
    void 		OnLineEditLatDegChanged(const QString& str);
    void 		OnLineEditLatMinChanged(const QString& str);
    void 		OnComboBoxNSHighlighted(int iID);
    void 		OnLineEditLngDegChanged(const QString& str);
    void 		OnLineEditLngMinChanged(const QString& str);
    void 		OnComboBoxEWHighlighted(int iID);
    void 		SetLatLng();
    void 		OnCheckBoxUseGPS();
    void 		OnCheckBoxDisplayGPS();
    void 		OnSelTimeInterp(int);
    void 		OnSelFrequencyInterp(int);
    void 		OnSelTiSync(int);
    void 		OnSliderIterChange(int value);
    void 		OnButtonDRMApply();
    void 		OnButtonAMApply();
    void 		OnButtonFMApply();
    void 		OnSliderLogStartDelayChange(int value);
    void 		OnCheckWriteLog();
    void 		OnCheckBoxLogLatLng();
    void 		OnCheckBoxLogSigStr();
    void 		OnCheckModiMetric();
    void 		OnRigTypeSelected(const QModelIndex&);
    void 		OnRigSelected(const QModelIndex&);
    void 		OnCheckEnableSMeterToggled(bool);
    void 		OnButtonAddRig();
    void		OnButtonRemoveRig();
    void		OnButtonConfigureRig();
    void		OnButtonChooseAudioFile();
    void		OnCheckBoxMuteAudio();
    void		OnCheckSaveAudioWav();
    void		OnCheckBoxReverb();
    void		OnAudioSelected(const QModelIndex&);
};

#endif
