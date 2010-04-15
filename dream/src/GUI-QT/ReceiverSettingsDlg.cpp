/******************************************************************************\
 * British Broadcasting Corporation * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable, Andrea Russo
 *
 * Description:
 * Settings for the receiver
 * Perhaps this should be Receiver Controls rather than Settings
 * since selections take effect immediately and there is no apply/cancel
 * feature. This makes sense, since one wants enable/disable GPS, Rig, Smeter
 * to be instant and mute/savetofile etc.
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

#include "ReceiverSettingsDlg.h"
#include "rigconfigurationdialog.h"
#include "../GlobalDefinitions.h"
#include "../selectioninterface.h"
#include "../util/Utilities.h"
#include "DialogUtil.h"
#include <QMessageBox>
#include <QButtonGroup>
#include <QFileDialog>
#include <algorithm>
#include <iostream>

/* Implementation *************************************************************/

RigTypesModel::RigTypesModel():QAbstractItemModel(),rigs()
{
}

int
RigTypesModel::rowCount ( const QModelIndex & parent ) const
{
    if(parent.isValid())
    {
    	const model_index* r = (const model_index*)parent.internalPointer();
    	if(r->parent==-1) // its a make - what we expect!
    	{
		const make *m = dynamic_cast<const make*>(r);
		return m->model.size();
    	}
    	else // its a model - stop descending
    	{
		return 0;
    	}
    }
    else
    {
	return rigs.size();
    }
}

int
RigTypesModel::columnCount ( const QModelIndex & parent) const
{
    return 1;
}

QVariant
RigTypesModel::data ( const QModelIndex & index, int role) const
{
    const model_index* i = (const model_index*)index.internalPointer();
    if(i->parent==-1)
    {
	switch(role)
	{
	case Qt::DecorationRole:
	    break;
	case Qt::DisplayRole:
	    if( (index.column()==0) && (int(rigs.size())>index.row()) )
		return rigs[index.row()].name.c_str();
	    break;
	case Qt::UserRole:
	    break;
	case Qt::TextAlignmentRole:
	    break;
	}
    }
    else
    {
	const rig *r = reinterpret_cast<const rig*>(i);
	switch(role)
	{
	case Qt::DecorationRole:
	    break;
	case Qt::DisplayRole:
	    switch(index.column())
	    {
		case 0:
		    return r->name.c_str();
		break;
	    }
	    break;
	case Qt::UserRole:
	    {
		return r->model;
	    }
	    break;
	case Qt::TextAlignmentRole:
	    switch(index.column())
	    {
		case 1:
			return QVariant(Qt::AlignRight|Qt::AlignVCenter);
			break;
		default:
			return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
	    }
	}
    }
    return QVariant();
}

QVariant
RigTypesModel::headerData ( int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
	    return QVariant();
    if(orientation != Qt::Horizontal)
	    return QVariant();
    switch(section)
    {
	case 0: return tr("Rig Make/Model"); break;
    }
    return "";
}

QModelIndex
RigTypesModel::index(int row, int column, const QModelIndex &parent) const
{
    if(parent.isValid())
    {
    	const model_index* r = (const model_index*)parent.internalPointer();
    	if(r->parent==-1) // its a make - what we expect!
    	{
		const make *m = reinterpret_cast<const make*>(r);
		if(int(m->model.size())>row)
		{
		    return createIndex(row, column, (void*)&m->model[row]);
		}
    	} // else its wrong, drop through
    }
    else
    {
    	if(int(rigs.size())>row)
    	{
	    return createIndex(row, column, (void*)&rigs[row]);
    	}
    }
    return QModelIndex();
}

QModelIndex
RigTypesModel::parent(const QModelIndex &child) const
{
    if(child.isValid())
    {
    	const model_index* r = (const model_index*)child.internalPointer();
    	if(r->parent!=-1) // its a model - what we expect!
    	{
	    size_t p = r->parent;
	    return createIndex(p, 0, (void*)&rigs[p]);
    	} // else its wrong, drop through
    }
    return QModelIndex();
}

int RigTypesModel::rig_enumerator(const rig_caps *caps, void *data)
{
    CRigMap& map = *(CRigMap *)data;
    map.rigs[caps->mfg_name][caps->model_name] = caps->rig_model;
    return 1;	/* !=0, we want them all! */
}

void
RigTypesModel::load()
{
    CRigMap r;
    rig_list_foreach(rig_enumerator, &r);
    for(map<string,map<string,rig_model_t> >::const_iterator
	i=r.rigs.begin(); i!=r.rigs.end(); i++)
    {
	make m;
	m.name = i->first;
	m.parent = -1;
	size_t n = rigs.size();
	for(map<string,rig_model_t>::const_iterator
	    j = i->second.begin(); j!=i->second.end(); j++)
	{
	    rig r;
	    r.name = j->first;
	    r.model = j->second;
	    r.parent = n;
	    m.model.push_back(r);
	}
	rigs.push_back(m);
    }
    reset();
}

RigModel::RigModel(CSettings& ns) : QAbstractItemModel(), settings(ns),
rigs(),unmodified(),modified()
{
	unmodified[RIG_MODEL_G303].levels["ATT"]=0;
	unmodified[RIG_MODEL_G303].levels["AGC"]=3;

	modified[RIG_MODEL_G303].levels["ATT"]=0;
	modified[RIG_MODEL_G303].levels["AGC"]=3;

	unmodified[RIG_MODEL_G313].levels["ATT"]=0;
	unmodified[RIG_MODEL_G313].levels["AGC"]=3;

	modified[RIG_MODEL_G313].levels["ATT"]=0;
	modified[RIG_MODEL_G313].levels["AGC"]=3;

	unmodified[RIG_MODEL_AR7030].mode_for_drm = RIG_MODE_AM;
	unmodified[RIG_MODEL_AR7030].width_for_drm = 3000;
	unmodified[RIG_MODEL_AR7030].levels["AGC"]=5;

	modified[RIG_MODEL_AR7030].mode_for_drm = RIG_MODE_AM;
	modified[RIG_MODEL_AR7030].width_for_drm = 6000;
	modified[RIG_MODEL_AR7030].levels["AGC"]=5;

	unmodified[RIG_MODEL_NRD535].mode_for_drm = RIG_MODE_CW;
	unmodified[RIG_MODEL_NRD535].width_for_drm = 12000;
	unmodified[RIG_MODEL_NRD535].levels["CWPITCH"]=-5000;
	unmodified[RIG_MODEL_NRD535].levels["IF"]=-2000;
	unmodified[RIG_MODEL_NRD535].levels["AGC"]=3;
	unmodified[RIG_MODEL_NRD535].offset=3;

	modified[RIG_MODEL_NRD535].levels["AGC"]=3;
	modified[RIG_MODEL_NRD535].offset=3;

	unmodified[RIG_MODEL_RX320].mode_for_drm = RIG_MODE_AM;
	unmodified[RIG_MODEL_RX320].width_for_drm = 6000;
	unmodified[RIG_MODEL_RX320].levels["AF"]=1;
	unmodified[RIG_MODEL_RX320].levels["AGC"]=3;

	modified[RIG_MODEL_RX320].levels["AGC"]=3;

	unmodified[RIG_MODEL_RX340].mode_for_drm = RIG_MODE_USB;
	unmodified[RIG_MODEL_RX340].width_for_drm = 16000;
	unmodified[RIG_MODEL_RX340].levels["AF"]=1;
	unmodified[RIG_MODEL_RX340].levels["IF"]=2000;
	unmodified[RIG_MODEL_RX340].levels["AGC"]=3;
	unmodified[RIG_MODEL_RX340].offset=-12;

	modified[RIG_MODEL_RX340].levels["AGC"]=3;
	modified[RIG_MODEL_RX340].offset=-12;

	string s = settings.Get("Receiver", "rigs", string(""));
	if(s!="")
	{
	    stringstream ss(s);
	    while(ss)
	    {
		int i;
		ss >> i;
		rigs.insert(i);
	    }
	}
}

QModelIndex RigModel::index(int row, int column,
		  const QModelIndex &parent) const
{
    if(parent.isValid())
    {
    }
    else
    {
	int n = 0;
	for(set<int>::const_iterator i=rigs.begin(); i!=rigs.end(); i++)
	{
	    if(n==row)
		return createIndex(row, column, *i);
	    n++;
	}
    }
    return QModelIndex();
}

QModelIndex RigModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int RigModel::rowCount (const QModelIndex& parent) const
{
    if(parent.isValid())
    {
	return 0;
    }
    else
    {
	return rigs.size();
    }
}

int RigModel::columnCount (const QModelIndex& parent) const
{
	return 3;
}

QVariant RigModel::data (const QModelIndex& index, int role) const
{
    int id = int(index.internalId());
    if(rigs.count(id)==0)
	return QVariant();

    if(true) // no structure visible at the moment
    {
    	stringstream sec;
    	sec << "Rig-" << id;
    	string name;
	switch(role)
	{
	case Qt::DecorationRole:
	    break;
	case Qt::DisplayRole:
	    switch(index.column())
	    {
		case 0:
		    name = settings.Get(sec.str(), "model_name", string(""));
		    if(name != "")
			return name.c_str();
		break;
		case 1:
		    {
		    int model = settings.Get(sec.str(), "model", -1);
		    if(model != -1)
			return model;
		    }
		    break;

		case 2:
		    name = settings.Get(sec.str(), "status", string(""));
		    if(name != "")
			return name.c_str();
		    break;
	    }
	    break;
	case Qt::UserRole:
	    {
		QVariant var;
		var.setValue(id);
		return var;
	    }
	    break;
	case Qt::TextAlignmentRole:
	    switch(index.column())
	    {
		case 1:
			return QVariant(Qt::AlignRight|Qt::AlignVCenter);
			break;
		default:
			return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
	    }
	    break;
	}
    }
    else
    {
#if 0
	const rig *r = reinterpret_cast<const rig*>(i);
	switch(role)
	{
	case Qt::DecorationRole:
	    if(index.column()==0)
	    {
		QIcon icon;
		//icon.addPixmap(BitmCubeGreen);
		return icon;
	    }
	    break;
	case Qt::DisplayRole:
	    break;
	case Qt::UserRole:
	    break;
	case Qt::TextAlignmentRole:
	}
#endif
    }
    return QVariant();
}

QVariant RigModel::headerData ( int section, Qt::Orientation orientation, int role ) const
{
    if(role != Qt::DisplayRole)
	    return QVariant();
    if(orientation != Qt::Horizontal)
	    return QVariant();
    switch(section)
    {
	case 0: return tr("Rig"); break;
	case 1: return tr("ID"); break;
	case 2: return tr("Status"); break;
    }
    return "";
}

void
RigModel::add(rig_model_t m, bool modified_for_drm)
{
    CRig r(m);
    int n = 0;
    for(size_t i=0; i<100; i++)
    {
    	if(rigs.count(i) == 0)
	{
	    n = i;
	    break;
	}
    }
    rigs.insert(n);
    struct scollect {
	scollect():sep(""),riglist(){}
	void operator() (int i) {riglist << sep << i; sep = ",";}
	string sep;
	stringstream riglist;
    } collect;

    //for_each(rigs.begin(), rigs.end(), collect);
    for(set<int>::const_iterator i=rigs.begin(); i!=rigs.end(); i++)
    {
	collect(*i);
    }
    settings.Put("Receiver", "rigs", collect.riglist.str());
    stringstream sec;
    sec << "Rig-" << n;
    settings.Put(sec.str(), "model", m);
    settings.Put(sec.str(), "model_name", string(r.model_name));
    settings.Put(sec.str(), "status", string(rig_strstatus(r.status)));
    reset();
}

void
RigModel::remove(int id)
{
    for(set<int>::iterator i=rigs.begin(); i!=rigs.end(); i++)
    {
    	if((*i) == id)
    	{
		rigs.erase(i); // rig is still stored in receiver TODO
		break;
    	}
    }
    reset();
}

SoundChoice::SoundChoice(const CSelectionInterface& s, bool t)
:QAbstractItemModel(), isTree(t), interface(s), cards()
{
    update();
}

void SoundChoice::update()
{
    vector<string> s;
    interface.Enumerate(s);
    if(isTree)
    {
    	cards.resize(0);
    	map<string,int> apis;
	for(size_t i=0; i<s.size(); i++)
	{
	    size_t n = s[i].find(':');
	    if(n==string::npos)
	    {
		card  c;
		c.name = s[i];
		c.parent = -1;
		c.index = i;
		cards.push_back(c);
	    }
	    else
	    {
		string api = s[i].substr(0,n);
		map<string,int>::iterator it = apis.find(api);
		size_t cn=0;
		if(it==apis.end())
		{
			cn = cards.size();
			apis[api] = cn;
			card c;
			c.name = api;
			c.parent = -1;
			c.index = -1;
			cards.push_back(c);
		}
		else
		{
			cn = it->second;
		}
		port p;
		p.name = s[i].substr(n+1);
		p.index = i;
		p.parent = cn;
		cards[cn].members.push_back(p);
	    }
	}
    }
    else
    {
    	cards.resize(s.size());
	for(size_t i=0; i<s.size(); i++)
	{
		cards[i].name = s[i];
		cards[i].index = i;
		cards[i].parent = -1;
	}
    }
#if 0
    for(size_t i=0; i<cards.size(); i++)
    {
    	cerr << "{" << cards[i].name << " " << cards[i].index << " " << cards[i].parent << endl;
    	for(size_t j=0; j<cards[i].members.size(); j++)
    	{
		cerr << "[" << cards[i].members[j].name
		<< " " << cards[i].members[j].index
		<< " " << cards[i].members[j].parent << "]" << endl;
    	}
    	cerr << "}" << endl;
    }
#endif
}

int SoundChoice::rowCount (const QModelIndex& index) const
{
    int n=0;
    int t=0;
    if(isTree)
    {
    	if(index.isValid())
    	{
		const port* p = reinterpret_cast<const port*>(index.internalPointer());
		if(p)
		{
			if(p->parent >= 0)
			{
				n = cards[p->parent].members.size();
				t = 1;
			}
			else
			{
				n = cards.size();
				t = 2;
			}
		}
		else
		{
			n = 0;
			t = 4;
		}
    	}
    	else
    	{
		n = cards.size();
		t = 3;
    	}
    }
    else
    {
    	n = cards.size();
    }
    return n;
}

int SoundChoice::columnCount (const QModelIndex& index) const
{
    return 1;
}

QVariant SoundChoice::headerData ( int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole)
		return QVariant();
	if(orientation != Qt::Horizontal)
		return QVariant();
	return tr("Device");
}

bool SoundChoice::hasChildren ( const QModelIndex & parent) const
{
    if(!parent.isValid())
	return true; // root
    const port* p = reinterpret_cast<const port*>(parent.internalPointer());
    if(p && (p->parent==-1))
	return true;
    return false;
}

QVariant
SoundChoice::data (const QModelIndex& index, int role) const
{
    if(!index.isValid())
	return QVariant();
    const port* p = reinterpret_cast<const port*>(index.internalPointer());
    if(p==NULL)
	return QVariant();
    switch(role)
    {
	case Qt::DecorationRole:
	    break;
	case Qt::DisplayRole:
	    return p->name.c_str();
	    break;
	case Qt::UserRole:
	    return p->index;
	    break;
	case Qt::TextAlignmentRole:
	    return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
    }
    return QVariant();
}

QModelIndex SoundChoice::index(int row, int column, const QModelIndex &parent) const
{
    if(column!=0)
	return QModelIndex();
    if(parent.isValid())
    {
	const port* p = reinterpret_cast<const card*>(parent.internalPointer());
	if(p->parent != -1) // at the bottom
	{
	    return QModelIndex();
	}
	const card* c = reinterpret_cast<const card*>(p);
	if(int(c->members.size())<=row)
		return QModelIndex();
	return createIndex(row, 0, (void*)&c->members[row]);
    }
    int n = cards.size();
    if(n<=row)
    {
	return QModelIndex();
    }
    QModelIndex r = createIndex(row, 0, (void*)&cards[row]);
    return r;
}

QModelIndex SoundChoice::parent(const QModelIndex& index) const
{
	if(index.isValid())
	{
	    const port* p = reinterpret_cast<const port*>(index.internalPointer());
	    if (p)
	    {
		if(p->parent>=0)
		{
		    return createIndex(p->parent, 0, (void*)&cards[p->parent]);
		}
	    }
    	}
	return QModelIndex();
}

ReceiverSettingsDlg::ReceiverSettingsDlg(
    CSettings& NSettings, CGPSData& GPSD,
     CSelectionInterface& soundin,
     CSelectionInterface& soundout,
	QWidget* parent, Qt::WFlags f) :
	QDialog(parent, f), Ui_ReceiverSettingsDlg(),
	Settings(NSettings), loading(true),
	TimerRig(),iWantedrigModel(0),
	bgTimeInterp(NULL), bgFreqInterp(NULL), bgTiSync(NULL),
	bgriq(NULL), bglrm(NULL), bgiq(NULL),

	RigTypes(),Rigs(NSettings),GPSData(GPSD),soundinputs(),
	soundoutputs()
{
    setupUi(this);

    treeViewRigTypes->setModel(&RigTypes);
    RigTypes.load();
    treeViewRigs->setModel(&Rigs);

    /* Connections */

    connect(buttonClose, SIGNAL(clicked()), this, SLOT(close()) );
    connect(LineEditLatDegrees, SIGNAL(textChanged(const QString&)), SLOT(OnLineEditLatDegChanged(const QString&)));
    connect(LineEditLatMinutes, SIGNAL(textChanged(const QString&)), SLOT(OnLineEditLatMinChanged(const QString&)));
    connect(ComboBoxNS, SIGNAL(highlighted(int)), SLOT(OnComboBoxNSHighlighted(int)) );
    connect(LineEditLngDegrees, SIGNAL(textChanged(const QString&)), SLOT(OnLineEditLngDegChanged(const QString&)));
    connect(LineEditLngMinutes, SIGNAL(textChanged(const QString&)), SLOT(OnLineEditLngMinChanged(const QString&)));
    connect(ComboBoxEW, SIGNAL(highlighted(int)), SLOT(OnComboBoxEWHighlighted(int)) );

    connect(pushButtonChooseAudioFile, SIGNAL(clicked()), this, SLOT(OnButtonChooseAudioFile()) );

    connect(SliderLogStartDelay, SIGNAL(valueChanged(int)),
	    this, SLOT(OnSliderLogStartDelayChange(int)));

    connect(SliderNoOfIterations, SIGNAL(valueChanged(int)),
	    this, SLOT(OnSliderIterChange(int)));

    /* Radio buttons */
    bgTimeInterp = new QButtonGroup(this);
    bgTimeInterp->addButton(RadioButtonTiWiener, TWIENER);
    bgTimeInterp->addButton(RadioButtonTiLinear, TLINEAR);
    connect(bgTimeInterp, SIGNAL(buttonClicked(int)), SLOT(OnSelTimeInterp(int)));

    bgFreqInterp = new QButtonGroup(this);
    bgFreqInterp->addButton(RadioButtonFreqWiener, FWIENER);
    bgFreqInterp->addButton(RadioButtonFreqLinear, FLINEAR);
    bgFreqInterp->addButton(RadioButtonFreqDFT, FDFTFILTER);
    connect(bgFreqInterp, SIGNAL(buttonClicked(int)), SLOT(OnSelFrequencyInterp(int)));

    bgTiSync = new QButtonGroup(this);
    bgTiSync->addButton(RadioButtonTiSyncEnergy, TSENERGY);
    bgTiSync->addButton(RadioButtonTiSyncFirstPeak, TSFIRSTPEAK);
    connect(bgTiSync, SIGNAL(buttonClicked(int)), SLOT(OnSelTiSync(int)));

    soundinputs = new SoundChoice(soundin);
    widgetDRMInput->comboBoxRig->setModel(treeViewRigs->model());
    widgetDRMInput->comboBoxCard->setModel(soundinputs);

    widgetAMInput->comboBoxRig->setModel(treeViewRigs->model());
    widgetAMInput->comboBoxCard->setModel(soundinputs);

    widgetFMInput->comboBoxRig->setModel(treeViewRigs->model());
    widgetFMInput->comboBoxCard->setModel(soundinputs);
    //widgetFMInput->soundInputFrame->setEnabled(false);

    connect(pushButtonDRMApply, SIGNAL(clicked()), SLOT(OnButtonDRMApply()));
    connect(pushButtonAMApply, SIGNAL(clicked()), SLOT(OnButtonAMApply()));
    connect(pushButtonFMApply, SIGNAL(clicked()), SLOT(OnButtonFMApply()));

    /* Check boxes */
    connect(CheckBoxUseGPS, SIGNAL(clicked()), SLOT(OnCheckBoxUseGPS()) );
    connect(CheckBoxDisplayGPS, SIGNAL(clicked()), SLOT(OnCheckBoxDisplayGPS()) );
    connect(CheckBoxWriteLog, SIGNAL(clicked()), this, SLOT(OnCheckWriteLog()));
    connect(CheckBoxModiMetric, SIGNAL(clicked()), this, SLOT(OnCheckModiMetric()));
    connect(CheckBoxLogLatLng, SIGNAL(clicked()), this, SLOT(OnCheckBoxLogLatLng()));
    connect(CheckBoxLogSigStr, SIGNAL(clicked()), this, SLOT(OnCheckBoxLogSigStr()));

    connect(pushButtonAddRig, SIGNAL(clicked()), this, SLOT(OnButtonAddRig()));
    connect(pushButtonConfigureRig, SIGNAL(clicked()), this, SLOT(OnButtonConfigureRig()));

    connect(pushButtonRemoveRig, SIGNAL(clicked()), this, SLOT(OnButtonRemoveRig()));

    connect(treeViewRigTypes, SIGNAL(clicked (const QModelIndex&)),
	    this, SLOT(OnRigTypeSelected(const QModelIndex&)));
    connect(treeViewRigs, SIGNAL(clicked (const QModelIndex&)),
	    this, SLOT(OnRigSelected(const QModelIndex&)));

    checkBoxModified->setEnabled(false);

    soundoutputs = new SoundChoice(soundout, true);
    connect(CheckBoxMuteAudio, SIGNAL(clicked()), this, SLOT(OnCheckBoxMuteAudio()));
    connect(CheckBoxReverb, SIGNAL(clicked()), this, SLOT(OnCheckBoxReverb()));
    connect(CheckBoxRecordAudio, SIGNAL(clicked()), this, SLOT(OnCheckSaveAudioWav()));
    connect(treeViewAudio, SIGNAL(clicked (const QModelIndex&)),
	    this, SLOT(OnAudioSelected(const QModelIndex&)));
    treeViewAudio->setModel(soundoutputs);

    /* Set help text for the controls */
    AddWhatsThisHelp();
}

ReceiverSettingsDlg::~ReceiverSettingsDlg()
{
}

void ReceiverSettingsDlg::showEvent(QShowEvent*)
{
    loading = true; // prevent executive actions during reading state

    EModulationType modn = EModulationType(Settings.Get("Receiver", "modulation", int(NONE)));

    switch(modn)
    {
	case DRM:
	    pushButtonDRMApply->setText(tr("Apply"));
	    pushButtonAMApply->setText(tr("Save"));
	    pushButtonFMApply->setText(tr("Save"));
	    break;
	case WBFM:
	    pushButtonDRMApply->setText(tr("Save"));
	    pushButtonAMApply->setText(tr("Save"));
	    pushButtonFMApply->setText(tr("Apply"));
	    break;
	case NONE:
	    pushButtonDRMApply->setText(tr("Save"));
	    pushButtonAMApply->setText(tr("Save"));
	    pushButtonFMApply->setText(tr("Save"));
	    break;
	default:
	    pushButtonDRMApply->setText(tr("Save"));
	    pushButtonAMApply->setText(tr("Apply"));
	    pushButtonFMApply->setText(tr("Save"));
    }

    /* DRM ----------------------------------------------------------------- */
    QAbstractButton* button = NULL;
    button = bgTimeInterp->button(Settings.Get("Input-DRM", "timeinterpolation", 0));
    if(button) button->setChecked(true);
    button = bgFreqInterp->button(Settings.Get("Input-DRM", "frequencyinterpolation", 0));
    if(button) button->setChecked(true);
    button = bgTiSync->button(Settings.Get("Input-DRM", "tracking", 0));
    if(button) button->setChecked(true);

    CheckBoxModiMetric->setChecked(Settings.Get("Input-DRM", "modmetric", false));
    SliderNoOfIterations->setValue(Settings.Get("Input-DRM", "mlciter", 0));

    pushButtonConfigureRig->setEnabled(false);

    /* Input ----------------------------------------------------------------- */

    widgetDRMInput->load(Settings);
    widgetAMInput->load(Settings);
    widgetFMInput->load(Settings);

    /* DRM tab */
    loadDRMSettings();

    /* Output */
    loadOutputSettings();

    /* GPS */
    loadGPSSettings();

    /* Logfile */
    loadLogfileSettings();
}

void ReceiverSettingsDlg::hideEvent(QHideEvent*)
{
    // Rig tab - TODO
    // input tab
    widgetDRMInput->save(Settings);
    widgetAMInput->save(Settings);
    widgetFMInput->save(Settings);

    // DRM tab
    saveDRMSettings();

    // output tab
    saveOutputSettings();

    // GPS Tab
    saveGPSSettings();

    // Log Tab
    saveLogfileSettings();
}

/* when the dialog closes save the contents of any controls which don't have
 * their own slot handlers
 */

// = GPS Tab ==============================================================

void ReceiverSettingsDlg::loadGPSSettings()
{
    string host = Settings.Get("GPS", "host", string("localhost"));
    LineEditGPSHost->setText(host.c_str());

    int port = Settings.Get("GPS", "port", 2947);
    LineEditGPSPort->setText(QString().number(port));

    CheckBoxUseGPS->setChecked(Settings.Get("GPS", "usegpsd", false));
    CheckBoxDisplayGPS->setChecked(Settings.Get("GPS", "showgps", false));

    double latitude = Settings.Get("GPS", "latitude", 100.0);
    double longitude = Settings.Get("GPS", "longitude", 0.0);

    if(latitude<=90.0)
    {
	    GPSData.SetPositionAvailable(true);
	    GPSData.SetGPSSource(CGPSData::GPS_SOURCE_MANUAL_ENTRY);
	    GPSData.SetLatLongDegrees(latitude, longitude);
    }
    else
    {
	    latitude = 0.0;
	    GPSData.SetPositionAvailable(false);
    }

    setLatLngDlg(latitude, longitude);

    saveGPSSettings(); // in case were not in ini file and defaults used
}

void ReceiverSettingsDlg::saveGPSSettings()
{
    Settings.Put("GPS", "host", LineEditGPSHost->text().toStdString());
    Settings.Put("GPS", "port", LineEditGPSPort->text().toInt());
    Settings.Put("GPS", "usegpsd", CheckBoxUseGPS->isChecked());
    Settings.Put("GPS", "showgps", CheckBoxDisplayGPS->isChecked());

    double latitude, longitude;
    GPSData.GetLatLongDegrees(latitude, longitude);
    Settings.Put("GPS", "latitude", latitude);
    Settings.Put("GPS", "longitude", longitude);
}

void ReceiverSettingsDlg::OnCheckBoxUseGPS()
{
    saveGPSSettings();
    emit StartStopGPS(CheckBoxUseGPS->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxDisplayGPS()
{
    saveGPSSettings();
    emit ShowHideGPS(CheckBoxDisplayGPS->isChecked());
}

void ReceiverSettingsDlg::OnLineEditLatDegChanged(const QString&)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnLineEditLatMinChanged(const QString&)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnComboBoxNSHighlighted(int)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnLineEditLngDegChanged(const QString&)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnLineEditLngMinChanged(const QString&)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnComboBoxEWHighlighted(int)
{
	SetLatLng();
}

void ReceiverSettingsDlg::SetLatLng()
{
    double latitude, longitude;

    longitude = (LineEditLngDegrees->text().toDouble()
			    + LineEditLngMinutes->text().toDouble()/60.0
			    )*((ComboBoxEW->currentText()=="E")?1:-1);

    latitude = (LineEditLatDegrees->text().toDouble()
			    + LineEditLatMinutes->text().toDouble()/60.0
			    )*((ComboBoxNS->currentText()=="N")?1:-1);

    GPSData.SetPositionAvailable(true);
    GPSData.SetLatLongDegrees(latitude, longitude);
}

void ReceiverSettingsDlg::setLatLngDlg(double latitude, double longitude)
{
	QString sVal, sDir;
	if(latitude<0.0)
	{
		latitude = 0.0 - latitude;
		ComboBoxNS->setCurrentIndex(1);
	}
	else
	{
		ComboBoxNS->setCurrentIndex(0);
	}
	LineEditLatDegrees->setText(QString::number(int(latitude)));
	LineEditLatMinutes->setText(QString::number(60.0*(latitude-int(latitude))));
	if(longitude<0.0)
	{
		longitude = 0.0 - longitude;
		ComboBoxEW->setCurrentIndex(1);
	}
	else
	{
		ComboBoxEW->setCurrentIndex(0);
	}
	LineEditLngDegrees->setText(QString::number(int(longitude)));
	LineEditLngMinutes->setText(QString::number(60.0*(longitude-int(longitude))));
}

// = DRM Tab ==============================================================
void ReceiverSettingsDlg::loadDRMSettings()
{
    bgTimeInterp->button(Settings.Get("Input-DRM", "timeinterpolation", 0))->setChecked(true);
    bgFreqInterp->button(Settings.Get("Input-DRM", "frequencyinterpolation", 0))->setChecked(true);
    bgTiSync->button(Settings.Get("Input-DRM", "tracking", 0))->setChecked(true);
	SliderNoOfIterations->setValue(Settings.Get("Input-DRM", "mlciter", 1));
    CheckBoxModiMetric->setChecked(Settings.Get("Input-DRM", "modmetric", 0)==1);
}

void ReceiverSettingsDlg::saveDRMSettings()
{
    Settings.Put("Input-DRM", "timeinterpolation", bgTimeInterp->checkedId());
    Settings.Put("Input-DRM", "frequencyinterpolation", bgFreqInterp->checkedId());
    Settings.Put("Input-DRM", "tracking", bgTiSync->checkedId());
	Settings.Put("Input-DRM", "mlciter", SliderNoOfIterations->value());
    Settings.Put("Input-DRM", "modmetric", CheckBoxModiMetric->isChecked()?1:0);
}

void ReceiverSettingsDlg::OnSelTimeInterp(int iId)
{
    Settings.Put("Input-DRM", "timeinterpolation", iId);
}

void ReceiverSettingsDlg::OnSelFrequencyInterp(int iId)
{
    Settings.Put("Input-DRM", "frequencyinterpolation", iId);
}

void ReceiverSettingsDlg::OnSelTiSync(int iId)
{
    Settings.Put("Input-DRM", "tracking", iId);
}

void ReceiverSettingsDlg::OnSliderIterChange(int value)
{
     Settings.Put("Input-DRM", "mlciter", value);
}

void ReceiverSettingsDlg::OnCheckModiMetric()
{
    Settings.Put("Input-DRM", "modmetric", CheckBoxModiMetric->isChecked());
}

void ReceiverSettingsDlg::OnButtonDRMApply()
{
    widgetDRMInput->save(Settings);
    EModulationType modn = EModulationType(Settings.Get("Receiver", "modulation", int(NONE)));
    if(modn==DRM)
	emit ReConfigureReceiver();
}

void ReceiverSettingsDlg::OnButtonAMApply()
{
    widgetAMInput->save(Settings);
    EModulationType modn = EModulationType(Settings.Get("Receiver", "modulation", int(NONE)));
    if(modn==AM)
	emit ReConfigureReceiver();
}

void ReceiverSettingsDlg::OnButtonFMApply()
{
    widgetFMInput->save(Settings);
    EModulationType modn = EModulationType(Settings.Get("Receiver", "modulation", int(NONE)));
    if(modn==WBFM)
	emit ReConfigureReceiver();
}

/*
.Get("FrontEnd", "smetercorrectiontype", 0));
.Get("FrontEnd", "smeterbandwidth", 0.0);
.Get("FrontEnd", "defaultmeasurementbandwidth", 0);
.Get("FrontEnd", "automeasurementbandwidth", true);
.Get("FrontEnd", "calfactordrm", 0.0);
.Get("FrontEnd", "calfactoram", 0.0);
.Get("FrontEnd", "ifcentrefrequency", SOUNDCRD_SAMPLE_RATE / 4);
*/
// Logging Tab
void ReceiverSettingsDlg::loadLogfileSettings()
{
    /* Start log file flag */
    CheckBoxWriteLog->setChecked(Settings.Get("Logfile", "enablelog", false));

    /* log file flag for storing signal strength in long log */
    CheckBoxLogSigStr->setChecked(Settings.Get("Logfile", "enablerxl", false));

    /* log file flag for storing lat/long in long log */
    CheckBoxLogLatLng->setChecked(Settings.Get("Logfile", "enablepositiondata", false));

    /* logging delay value */
    int iLogDelay = Settings.Get("Logfile", "delay", 0);
    SliderLogStartDelay->setValue(iLogDelay);

    saveLogfileSettings(); // establish defaults in ini file
}

void ReceiverSettingsDlg::saveLogfileSettings()
{
    Settings.Put("Logfile", "enablelog", CheckBoxWriteLog->isChecked());
    Settings.Put("Logfile", "enablerxl", CheckBoxLogSigStr->isChecked());
    Settings.Put("Logfile", "enablepositiondata", CheckBoxLogLatLng->isChecked());
    Settings.Put("Logfile", "delay", SliderLogStartDelay->value());
}

void ReceiverSettingsDlg::OnCheckWriteLog()
{
    emit StartStopLog(CheckBoxWriteLog->isChecked());
    Settings.Put("Logfile", "enablelog", CheckBoxWriteLog->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxLogLatLng()
{
    emit LogPosition(CheckBoxLogLatLng->isChecked());
    Settings.Put("Logfile", "enablepositiondata", CheckBoxLogLatLng->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxLogSigStr()
{
    emit LogSigStr(CheckBoxLogSigStr->isChecked());
    Settings.Put("Logfile", "enablerxl", CheckBoxLogSigStr->isChecked());
}

void ReceiverSettingsDlg::OnSliderLogStartDelayChange(int value)
{
    emit SetLogStartDelay(value);
    Settings.Put("Logfile", "delay", value);
}

void ReceiverSettingsDlg::OnCheckEnableSMeterToggled(bool on)
{
    if(loading)
	    return;
    //CRig* rig = Receiver.GetCurrentRig();
   // if(rig)
//	    rig->SetEnableSMeter(on);
}

void
ReceiverSettingsDlg::OnRigTypeSelected(const QModelIndex& m)
{
    QVariant var = m.data(Qt::UserRole);
    if(var.isValid()==false)
	return;
    rig_model_t model = var.toInt();
}

void
ReceiverSettingsDlg::OnRigSelected(const QModelIndex& index)
{
    QVariant var = index.data(Qt::UserRole);
    //int r = var.value();
    if(false)
    {
    	QMessageBox::information(this, tr("Hamlib"), tr("Rig not created error"), QMessageBox::Ok);
    	return;
    }
    pushButtonConfigureRig->setEnabled(true);
}

void
ReceiverSettingsDlg::OnButtonAddRig()
{
    rig_model_t model = treeViewRigTypes->currentIndex().data(Qt::UserRole).toInt();

    bool parms=false;
    if(parms)
    {
	//const RigTypesModel::parms& p = pp->second;
	/*
	for(map<string,int>::const_iterator i=p.levels.begin(); i!=p.levels.end(); i++)
	{
	    r->setLevel(rig_parse_level(i->first.c_str()), i->second);
	}
	if(p.mode_for_drm!=RIG_MODE_NONE)
	{
	    r->SetModeForDRM(p.mode_for_drm, p.width_for_drm);
	}
	r->SetFrequencyOffset(p.offset);
	*/
	// TODO set these in settings
    }
    //else
	//r->SetFrequencyOffset(0);
    Rigs.add(model);
    //treeViewRigs->setCurrentIndex(Rigs.rowCount()-1);
}

void
ReceiverSettingsDlg::OnButtonRemoveRig()
{
    Rigs.remove(treeViewRigs->currentIndex().internalId());
}

void ReceiverSettingsDlg::OnButtonConfigureRig()
{
    stringstream sec;
    sec << "Rig-" << treeViewRigs->currentIndex().internalId();
    //QMessageBox::information(this, tr("Configure Rig"), tr("Not done"), QMessageBox::Ok);
    RigConfigurationDialog* dlg = new RigConfigurationDialog(Settings, sec.str(), this);
    dlg->show();
}

void ReceiverSettingsDlg::loadOutputSettings()
{
    /* Output - IQ */
    CheckBoxRecordIQ->setChecked(Settings.Get("Receiver", "writeiq", false));

    /* Output - Audio */
    string audiofile = Settings.Get("Receiver", "writewav", string(""));
    lineEditAudioFile->setText(audiofile.c_str());
    CheckBoxRecordAudio->setChecked(audiofile!="");
    CheckBoxReverb->setChecked(Settings.Get("Receiver", "reverb", true));
    CheckBoxMuteAudio->setChecked(Settings.Get("Receiver", "mute", false));
    switch(EInChanSel(Settings.Get("Receiver", "outchansel", CS_MIX_CHAN)))
    {
	case CS_LEFT_CHAN:
	    radioButtonLeft->setChecked(true);
	    break;
	case CS_RIGHT_CHAN:
	    radioButtonRight->setChecked(true);
	    break;
	case CS_MIX_CHAN:
	    radioButtonMix->setChecked(true);
	    break;
	default:
	    break;
    }
    int dev = Settings.Get("Receiver", "snddevout", int(-1));
    if(dev == -1)
    {
	treeViewAudio->setCurrentIndex(treeViewAudio->rootIndex()); // invalidate
    }
    else
    {
	treeViewAudio->setCurrentIndex((treeViewAudio->rootIndex()).child(dev,0));
    }

    /* Output - RSCI */

}

void ReceiverSettingsDlg::saveOutputSettings()
{
    /* Output - IQ */
    Settings.Put("Receiver", "writeiq", CheckBoxRecordIQ->isChecked());

    /* Output - Audio */
    Settings.Put("Receiver", "mute", CheckBoxMuteAudio->isChecked());
    Settings.Put("Receiver", "reverb", CheckBoxReverb->isChecked());
    if(radioButtonLeft->isChecked())
	Settings.Put("Receiver", "outchansel", CS_LEFT_CHAN);
    if(radioButtonRight->isChecked())
	Settings.Put("Receiver", "outchansel", CS_RIGHT_CHAN);
    if(radioButtonMix->isChecked())
	Settings.Put("Receiver", "outchansel", CS_MIX_CHAN);

    if(CheckBoxRecordAudio->isChecked())
    {
	Settings.Put("Receiver", "writewav", lineEditAudioFile->text().toStdString());
    }
    else
    {
	Settings.Put("Receiver", "writewav", string(""));
    }
    int dev = (treeViewAudio->currentIndex()).data(Qt::UserRole).toInt();
    Settings.Put("Receiver", "snddevout", dev);

    /* Output - RSCI */

}

void ReceiverSettingsDlg::OnCheckBoxMuteAudio()
{
    Settings.Put("Receiver", "mute", CheckBoxMuteAudio->isChecked());
}

void ReceiverSettingsDlg::OnCheckSaveAudioWav()
{
    //OnSaveAudio(this, CheckBoxRecordAudio, Receiver);
}

void ReceiverSettingsDlg::OnButtonChooseAudioFile()
{
    QString fileName = QFileDialog::getSaveFileName(
	    this,
	    tr("Open File"),
	     ".",
	     tr("Audio (*.wav *.flac *.mp3)")
     );
    if(fileName!="")
	lineEditAudioFile->setText(fileName);
}

void ReceiverSettingsDlg::OnCheckBoxReverb()
{
    Settings.Put("Receiver", "reverb", CheckBoxReverb->isChecked());
}

void ReceiverSettingsDlg::OnAudioSelected(const QModelIndex& m)
{
    QVariant var = m.data(Qt::UserRole);
    if(var.isValid()==false)
	return;
    Settings.Put("Receiver", "snddevout", var.toInt());
}

void ReceiverSettingsDlg::AddWhatsThisHelp()
{
	/* GPS */
	const QString strGPS =
		tr("<b>Receiver coordinates:</b> Are used on "
		"Live Schedule Dialog to show a little green cube on the left"
		" of the target column if the receiver coordinates (latitude and longitude)"
		" are inside the target area of this transmission.<br>"
		"Receiver coordinates are also saved into the Log file.");

    LineEditLatDegrees->setWhatsThis( strGPS);
    LineEditLatMinutes->setWhatsThis( strGPS);
    LineEditLngDegrees->setWhatsThis( strGPS);
    LineEditLngMinutes->setWhatsThis( strGPS);

	/* MLC, Number of Iterations */
	const QString strNumOfIterations =
		tr("<b>MLC, Number of Iterations:</b> In DRM, a "
		"multilevel channel coder is used. With this code it is possible to "
		"iterate the decoding process in the decoder to improve the decoding "
		"result. The more iterations are used the better the result will be. "
		"But switching to more iterations will increase the CPU load. "
		"Simulations showed that the first iteration (number of "
		"iterations = 1) gives the most improvement (approx. 1.5 dB at a "
		"BER of 10-4 on a Gaussian channel, Mode A, 10 kHz bandwidth). The "
		"improvement of the second iteration will be as small as 0.3 dB."
		"<br>The recommended number of iterations given in the DRM "
		"standard is one iteration (number of iterations = 1).");

	SliderNoOfIterations->setWhatsThis( strNumOfIterations);

	/* Log File */
	CheckBoxWriteLog->setWhatsThis(
		tr("<b>Log File:</b> Checking this box brings the "
		"Dream software to write a log file about the current reception. "
		"Every minute the average SNR, number of correct decoded FAC and "
		"number of correct decoded MSC blocks are logged including some "
		"additional information, e.g. the station label and bit-rate. The "
		"log mechanism works only for audio services using AAC source coding. "
		"<br>The log file will be "
		"written in the directory were the Dream application was started and "
		"the name of this file is always DreamLog.txt"));

	/* Wiener */
	const QString strWienerChanEst =
		tr("<b>Channel Estimation Settings:</b> With these "
		"settings, the channel estimation method in time and frequency "
		"direction can be selected. The default values use the most powerful "
		"algorithms. For more detailed information about the estimation "
		"algorithms there are a lot of papers and books available.<br>"
		"<b>Wiener:</b> Wiener interpolation method "
		"uses estimation of the statistics of the channel to design an optimal "
		"filter for noise reduction.");

	RadioButtonFreqWiener->setWhatsThis( strWienerChanEst);
	RadioButtonTiWiener->setWhatsThis( strWienerChanEst);

	/* Linear */
	const QString strLinearChanEst =
		tr("<b>Channel Estimation Settings:</b> With these "
		"settings, the channel estimation method in time and frequency "
		"direction can be selected. The default values use the most powerful "
		"algorithms. For more detailed information about the estimation "
		"algorithms there are a lot of papers and books available.<br>"
		"<b>Linear:</b> Simple linear interpolation "
		"method to get the channel estimate. The real and imaginary parts "
		"of the estimated channel at the pilot positions are linearly "
		"interpolated. This algorithm causes the lowest CPU load but "
		"performs much worse than the Wiener interpolation at low SNRs.");

	RadioButtonFreqLinear->setWhatsThis( strLinearChanEst);
	RadioButtonTiLinear->setWhatsThis( strLinearChanEst);

	/* DFT Zero Pad */
	RadioButtonFreqDFT->setWhatsThis(
		tr("<b>Channel Estimation Settings:</b> With these "
		"settings, the channel estimation method in time and frequency "
		"direction can be selected. The default values use the most powerful "
		"algorithms. For more detailed information about the estimation "
		"algorithms there are a lot of papers and books available.<br>"
		"<b>DFT Zero Pad:</b> Channel estimation method "
		"for the frequency direction using Discrete Fourier Transformation "
		"(DFT) to transform the channel estimation at the pilot positions to "
		"the time domain. There, a zero padding is applied to get a higher "
		"resolution in the frequency domain -> estimates at the data cells. "
		"This algorithm is very speed efficient but has problems at the edges "
		"of the OFDM spectrum due to the leakage effect."));

	/* Guard Energy */
	RadioButtonTiSyncEnergy->setWhatsThis(
		tr("<b>Guard Energy:</b> Time synchronization "
		"tracking algorithm utilizes the estimation of the impulse response. "
		"This method tries to maximize the energy in the guard-interval to set "
		"the correct timing."));

	/* First Peak */
	RadioButtonTiSyncFirstPeak->setWhatsThis(
		tr("<b>First Peak:</b> This algorithms searches for "
		"the first peak in the estimated impulse response and moves this peak "
		"to the beginning of the guard-interval (timing tracking algorithm)."));

	/* Interferer Rejection */
	const QString strInterfRej =
		tr("<b>Interferer Rejection:</b> There are two "
		"algorithms available to reject interferers:<ul>"
		"<li><b>Bandpass Filter (BP-Filter):</b>"
		" The bandpass filter is designed to have the same bandwidth as "
		"the DRM signal. If, e.g., a strong signal is close to the border "
		"of the actual DRM signal, under some conditions this signal will "
		"produce interference in the useful bandwidth of the DRM signal "
		"although it is not on the same frequency as the DRM signal. "
		"The reason for that behaviour lies in the way the OFDM "
		"demodulation is done. Since OFDM demodulation is a block-wise "
		"operation, a windowing has to be applied (which is rectangular "
		"in case of OFDM). As a result, the spectrum of a signal is "
		"convoluted with a Sinc function in the frequency domain. If a "
		"sinusoidal signal close to the border of the DRM signal is "
		"considered, its spectrum will not be a distinct peak but a "
		"shifted Sinc function. So its spectrum is broadened caused by "
		"the windowing. Thus, it will spread in the DRM spectrum and "
		"act as an in-band interferer.<br>"
		"There is a special case if the sinusoidal signal is in a "
		"distance of a multiple of the carrier spacing of the DRM signal. "
		"Since the Sinc function has zeros at certain positions it happens "
		"that in this case the zeros are exactly at the sub-carrier "
		"frequencies of the DRM signal. In this case, no interference takes "
		"place. If the sinusoidal signal is in a distance of a multiple of "
		"the carrier spacing plus half of the carrier spacing away from the "
		"DRM signal, the interference reaches its maximum.<br>"
		"As a result, if only one DRM signal is present in the 20 kHz "
		"bandwidth, bandpass filtering has no effect. Also,  if the "
		"interferer is far away from the DRM signal, filtering will not "
		"give much improvement since the squared magnitude of the spectrum "
		"of the Sinc function is approx -15 dB down at 1 1/2 carrier "
		"spacing (approx 70 Hz with DRM mode B) and goes down to approx "
		"-30 dB at 10 times the carrier spacing plus 1 / 2 of the carrier "
		"spacing (approx 525 Hz with DRM mode B). The bandpass filter must "
		"have very sharp edges otherwise the gain in performance will be "
		"very small.</li>"
		"<li><b>Modified Metrics:</b> Based on the "
		"information from the SNR versus sub-carrier estimation, the metrics "
		"for the Viterbi decoder can be modified so that sub-carriers with "
		"high noise are attenuated and do not contribute too much to the "
		"decoding result. That can improve reception under bad conditions but "
		"may worsen the reception in situations where a lot of fading happens "
		"and no interferer are present since the SNR estimation may be "
		"not correct.</li></ul>");

	CheckBoxModiMetric->setWhatsThis( strInterfRej);
}
