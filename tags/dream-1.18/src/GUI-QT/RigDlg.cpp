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

#include "RigDlg.h"
#include "DialogUtil.h"
#include <QTreeWidgetItem>

/* Implementation *************************************************************/

RigDlg::RigDlg(
    CRig& nrig,
    QWidget* parent, Qt::WindowFlags f) :
    QDialog(parent, f), Ui_RigDlg(),
    rig(nrig),rigmap()
{

    map<rig_model_t,CHamlib::SDrRigCaps> r;

    setupUi(this);
    rig.GetRigList(r);
    modified->setEnabled(false);
    //rigTypes->setColumnCount(2);
    rigTypes->setSortingEnabled(false);
    QTreeWidgetItem* none = new QTreeWidgetItem(rigTypes);
	none->setText(0, "None");
	none->setData(0, Qt::UserRole, RIG_MODEL_NONE);
    for(map<rig_model_t,CHamlib::SDrRigCaps>::const_iterator i=r.begin(); i!=r.end(); i++)
    {
		rig_model_t model_num = i->first;
        CHamlib::SDrRigCaps rc =  i->second;
        QTreeWidgetItem* mfr, *model;
        if(rc.strManufacturer=="" || rc.strModelName=="")
        {
            continue;
        }
        QList<QTreeWidgetItem *> l = rigTypes->findItems(rc.strManufacturer.c_str(), Qt::MatchFixedString);
        if(l.size()==0)
        {
            mfr = new QTreeWidgetItem(rigTypes);
            mfr->setText(0,rc.strManufacturer.c_str());
			mfr->setFlags(mfr->flags() & ~Qt::ItemIsSelectable);
        }
        else
        {
            mfr = l.first();
        }
        model = new QTreeWidgetItem(mfr);
        model->setText(0,rc.strModelName.c_str());
		model->setData(0, Qt::UserRole, model_num);
		rigmap[model_num] = rc.strModelName;
    }
    rigTypes->setSortingEnabled(false);
    rigTypes->sortItems(9, Qt::AscendingOrder);

   	InitSMeter(this, sMeter);
}

RigDlg::~RigDlg()
{
}

void RigDlg::showEvent(QShowEvent*)
{
    map<string,string> ports;
    rig.GetPortList(ports);
    comboBoxPort->clear();
	string port = rig.GetComPort();
	int index=0;
    for(map<string,string>::const_iterator i=ports.begin(); i!=ports.end(); i++)
    {
		comboBoxPort->addItem(i->first.c_str(), i->second.c_str());
		if(i->second.compare(port)==0)
			index = comboBoxPort->count();
    }
	comboBoxPort->setCurrentIndex(index);

    prev_rig_model = rig.GetHamlibModelID();
    if (prev_rig_model == RIG_MODEL_NONE)
    {
        rigTypes->setCurrentItem(rigTypes->topLevelItem(0)); /* None */
    }
    else
    {
        map<rig_model_t,string>::const_iterator m = rigmap.find(prev_rig_model);
        if(m!=rigmap.end())
        {
            QString name(m->second.c_str());
            QList<QTreeWidgetItem *> l = rigTypes->findItems(name, Qt::MatchExactly | Qt::MatchRecursive);
            if(l.size()>0) {
                rigTypes->setCurrentItem(l.front());
                selectedRigType->setText(name);
            }
        }
    }

    prev_port = rig.GetComPort();
    comboBoxPort->setCurrentIndex(comboBoxPort->findText(prev_port.c_str()));

	connect(&rig, SIGNAL(sigstr(double)), this, SLOT(onSigstr(double)));
}

void RigDlg::hideEvent(QHideEvent*)
{
	disconnect(&rig, SIGNAL(sigstr(double)), this, SLOT(onSigstr(double)));
}

void
RigDlg::on_rigTypes_itemSelectionChanged()
{
	QList<QTreeWidgetItem*> l = rigTypes->selectedItems();
	if(l.count()==1) {
		const QTreeWidgetItem* item = l.first();
		selectedRigType->setText(item->text(0));
		rig.SetHamlibModelID(item->data(0, Qt::UserRole).toInt());
	}
}

void
RigDlg::on_modified_stateChanged(int state)
{
	rig.SetEnableModRigSettings(state?false:true);
}

void
RigDlg::on_testRig_clicked()
{
	rig.SetComPort(comboBoxPort->itemData(comboBoxPort->currentIndex()).toString().toStdString());
	rig.SetHamlibModelID(rigTypes->currentItem()->data(0, Qt::UserRole).toInt());
	rig.subscribe();
}

void
RigDlg::on_buttonBox_accepted()
{
	rig.SetComPort(comboBoxPort->itemData(comboBoxPort->currentIndex()).toString().toStdString());
	rig.SetHamlibModelID(rigTypes->currentItem()->data(0, Qt::UserRole).toInt());
	rig.unsubscribe();
	close();
}

void
RigDlg::on_buttonBox_rejected()
{
	rig.SetComPort(prev_port);
	rig.SetHamlibModelID(prev_rig_model);
	rig.unsubscribe();
	close();
}

void
RigDlg::on_comboBoxPort_currentIndexChanged(int index)
{
	rig.SetComPort(comboBoxPort->itemData(index).toString().toStdString());
}

void
RigDlg::onSigstr(double r)
{
	sMeter->setValue(r);
}
