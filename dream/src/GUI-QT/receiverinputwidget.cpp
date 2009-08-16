/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description:  Widget to choose sound card, rig or RCSI
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

#include "receiverinputwidget.h"
#include "../util/Settings.h"
#include "ReceiverSettingsDlg.h"

ReceiverInputWidget::ReceiverInputWidget(QWidget *parent) :
    QWidget(parent),
    bglr(new QButtonGroup(this)),
    bgriq(new QButtonGroup(this)),
    bglrm(new QButtonGroup(this)),
    bgiq(new QButtonGroup(this))
{
    setupUi(this);
    bglr->addButton(radioButtonRSCI, 0);
    bglr->addButton(radioButtonLocal, 1);
    bgriq->addButton(radioButtonReal, 0);
    bgriq->addButton(radioButtonIQ, 1);
    bglrm->addButton(radioButtonLeft, 0);
    bglrm->addButton(radioButtonRight, 1);
    bglrm->addButton(radioButtonMix, 2);
    bgiq->addButton(radioButtonIQPos, 0);
    bgiq->addButton(radioButtonIQNeg, 1);

    connect(bglr, SIGNAL(buttonClicked(int)), SLOT(OnRadioRealRigRSCI(int)));
    connect(bgriq, SIGNAL(buttonClicked(int)), SLOT(OnRadioRealIQ(int)));
}

ReceiverInputWidget::~ReceiverInputWidget()
{
}

void ReceiverInputWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        retranslateUi(this);
        break;
    default:
        break;
    }
}

void ReceiverInputWidget::OnRadioRealRigRSCI(int i)
{
    stackedWidgetLR->setCurrentIndex(i);
}

void ReceiverInputWidget::OnRadioRealIQ(int i)
{
    stackedWidgetip->setCurrentIndex(i);
}

void ReceiverInputWidget::load(const CSettings& settings)
{
    string sec = accessibleName().toStdString();
    string str = settings.Get(sec, "rsiin_port", string(""));
    if(str=="")
    {
	radioButtonLocal->setChecked(true);
	stackedWidgetLR->setCurrentIndex(1);
        int rig = settings.Get(sec, "rig", int(0));
        for(int i=0; i<comboBoxRig->count(); i++)
        {
	    QVariant var = comboBoxRig->itemData(i);
	    RigData r = var.value<RigData>();
	    if(r.id==rig)
	    {
	        comboBoxRig->setCurrentIndex(i);
	        break;
	    }
        }
        comboBoxSound->setCurrentIndex(settings.Get(sec, "soundcard", int(0)));
        int riq = settings.Get(sec, "mode", int(0));
        stackedWidgetip->setCurrentIndex(riq);
        QAbstractButton* button = bgriq->button(riq);
        if(button) button->setChecked(true);
        button = bglrm->button(settings.Get(sec, "channels", int(0)));
        if(button) button->setChecked(true);
        button = bgiq->button(settings.Get(sec, "sign", int(0)));
        if(button) button->setChecked(true);
            CheckBoxFlipSpec->setChecked(settings.Get(sec, "flipspectrum", int(0)));
    }
    else
    {
	radioButtonRSCI->setChecked(true);
	stackedWidgetLR->setCurrentIndex(0);
	lineEditsport->setText(str.c_str());
	lineEditGroup->setText(settings.Get(sec, "rsiin_group", string("")).c_str());
	lineEditHost->setText(settings.Get(sec, "rciout_host", string("")).c_str());
	lineEditcport->setText(settings.Get(sec, "rciout_port", string("")).c_str());
    }
}

void ReceiverInputWidget::save(CSettings& settings) const
{
    string sec = accessibleName().toStdString();
    if(radioButtonRSCI->isChecked())
    {
	settings.Put(sec, "rsiin_group", lineEditGroup->text().toStdString());
	settings.Put(sec, "rsiin_port", lineEditsport->text().toStdString());
	settings.Put(sec, "rciout_host", lineEditHost->text().toStdString());
	settings.Put(sec, "rciout_port", lineEditcport->text().toStdString());
    }
    else
    {
	int i = comboBoxRig->currentIndex();
	QVariant var =comboBoxRig->itemData(i);
	RigData r = var.value<RigData>();
	settings.Put(sec, "rig", r.id);
	settings.Put(sec, "soundcard", comboBoxSound->currentIndex());
	settings.Put(sec, "mode", stackedWidgetip->currentIndex());
	settings.Put(sec, "channels", bglrm->checkedId());
	settings.Put(sec, "sign", bgiq->checkedId());
	settings.Put(sec, "flipspectrum", CheckBoxFlipSpec->isChecked());
    }
}
