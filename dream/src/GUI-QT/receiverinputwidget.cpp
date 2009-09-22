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
    bgrsf(new QButtonGroup(this)),
    bgfriq(new QButtonGroup(this)),
    bgflrm(new QButtonGroup(this)),
    bgfiq(new QButtonGroup(this)),
    bgsriq(new QButtonGroup(this)),
    bgslrm(new QButtonGroup(this)),
    bgsiq(new QButtonGroup(this)),
    bgrh(new QButtonGroup(this))
{
    setupUi(this);
    bgrsf->addButton(radioButtonSoundCard, stackedWidgetStatus->indexOf(pageSound));
    bgrsf->addButton(radioButtonFile, stackedWidgetStatus->indexOf(pageFile));
    bgrsf->addButton(radioButtonRSCI, stackedWidgetStatus->indexOf(pageRSCIStatus));
    bgfriq->addButton(radioButtonRealFile, stackedWidgetFile->indexOf(pagerealFile));
    bgfriq->addButton(radioButtonIQFile, stackedWidgetFile->indexOf(pageiqFile));
    bgslrm->addButton(radioButtonLeftFile, 0);
    bgslrm->addButton(radioButtonRightFile, 1);
    bgslrm->addButton(radioButtonMixFile, 2);
    bgfiq->addButton(radioButtonIQPosFile, 0);
    bgfiq->addButton(radioButtonIQNegFile, 1);
    bgsriq->addButton(radioButtonRealCard, stackedWidgetCard->indexOf(pagereal));
    bgsriq->addButton(radioButtonIQCard, stackedWidgetCard->indexOf(pageiq));
    bgslrm->addButton(radioButtonLeftCard, 0);
    bgslrm->addButton(radioButtonRightCard, 1);
    bgslrm->addButton(radioButtonMixCard, 2);
    bgsiq->addButton(radioButtonIQPosCard, 0);
    bgsiq->addButton(radioButtonIQNegCard, 1);
    bgrh->addButton(radioButtonRSCIControl, 1);
    bgrh->addButton(radioButtonHamlib, 0);

    connect(bgrsf, SIGNAL(buttonClicked(int)), stackedWidgetStatus, SLOT(setCurrentIndex(int)));
    connect(bgfriq, SIGNAL(buttonClicked(int)), stackedWidgetFile, SLOT(setCurrentIndex(int)));
    connect(bgsriq, SIGNAL(buttonClicked(int)), stackedWidgetCard, SLOT(setCurrentIndex(int)));
    connect(bgrh, SIGNAL(buttonClicked(int)), stackedWidgetControl, SLOT(setCurrentIndex(int)));

    const QString s = tr("<b>Flip Input Spectrum:</b> Checking this box "
	    "will flip or invert the input spectrum. This is necessary if the "
	    "mixer in the front-end uses the lower side band.");
    CheckBoxFlipSpecFile->setWhatsThis(s);
    CheckBoxFlipSpecCard->setWhatsThis(s);
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

void ReceiverInputWidget::load(CSettings& settings)
{
    string sec = accessibleName().toStdString();
    // input can either be RSCI or local
    // if RSCI it can be a mode specific RSCI spec (in the section)
    // or a global RSCI spec in the command section
    // an RSCI spec can be either a network port or a file (.rsM, .rsA ...)
    // If local it can be a rig+soundcard or an iq file or a wav file
    // this can either be mode specific or global
    // first look for global specific settings then specific ones.
    // global settings will be turned into mode specific settings ???

    // Input (Status)
    string str = settings.Get("command", "rsiin", string(""));
    if(str!="")
    {
	size_t p = str.rfind('.');
	if (p == string::npos)
	{
	    settings.Put(sec, "rsiin", str);
	}
	else
	{
	    settings.Put(sec, "file", str);
	}
    }
    str = settings.Get("command", "fileio", string(""));
    if(str!="")
    {
	settings.Put(sec, "file", str);
    }
    int dev = settings.Get("Receiver", "snddevin", -1);
    if(dev!=-1)
    {
	settings.Put(sec, "soundcard", dev);
    }
    str = settings.Get(sec, "rsiin", string(""));
    if(str!="")
    {
	string addr, port;
	size_t p = str.find(':');
	if(p==string::npos)
	{
	    port = str;
	}
	else
	{
	    addr = str.substr(0, p);
	    port = str.substr(p+1);
	}
	radioButtonRSCI->setChecked(true);
	lineEditStatusAddress->setText(addr.c_str());
	lineEditStatusPort->setText(port.c_str());
    }
    str = settings.Get(sec, "file", string(""));
    if(str!="")
    {
	radioButtonFile->setChecked(true);
	lineEditFile->setText(str.c_str());
	int riq = settings.Get(sec, "mode", int(0));
	QAbstractButton* button = bgfriq->button(riq);
	if(button) button->setChecked(true);
	button = bgflrm->button(settings.Get(sec, "channels", int(0)));
	if(button) button->setChecked(true);
	button = bgfiq->button(settings.Get(sec, "sign", int(0)));
	if(button) button->setChecked(true);
	CheckBoxFlipSpecFile->setChecked(settings.Get(sec, "flipspectrum", int(0)));
    }
    dev = settings.Get(sec, "soundcard", -1);
    if(dev!=-1)
    {
	radioButtonSoundCard->setChecked(true);
	comboBoxSoundCard->setCurrentIndex(dev);
	int riq = settings.Get(sec, "mode", int(0));
	stackedWidgetCard->setCurrentIndex(riq);
	QAbstractButton* button = bgsriq->button(riq);
	if(button) button->setChecked(true);
	button = bgslrm->button(settings.Get(sec, "channels", stackedWidgetFile->indexOf(pagereal)));
	if(button) button->setChecked(true);
	button = bgsiq->button(settings.Get(sec, "sign", int(0)));
	if(button) button->setChecked(true);
	CheckBoxFlipSpecCard->setChecked(settings.Get(sec, "flipspectrum", int(0)));
    }

    // Control
    str = settings.Get("command", "rciout", string(""));
    if(str!="")
    {
	settings.Put(sec, "rciout", str);
    }
    dev = settings.Get("command", "hamlib-model", -1);
    if(dev!=-1)
    {
	settings.Put(sec, "rig", str);
    }
    str = settings.Get(sec, "rciout", string(""));
    if(str!="")
    {
	string addr, port;
	size_t p = str.find(':');
	if(p==string::npos)
	{
	    addr = "127.0.0.1"; // assume local
	    port = str;
	}
	else
	{
	    addr = str.substr(0, p);
	    port = str.substr(p+1);
	}
	radioButtonRSCIControl->setChecked(true);
	lineEditControlAddress->setText(addr.c_str());
	lineEditControlPort->setText(port.c_str());
    }

    int rig = settings.Get(sec, "rig", -1);
    if(rig!=-1)
    {
	radioButtonHamlib->setChecked(true);
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
    }
    checkBoxBPFFile->setChecked(settings.Get(sec, "BPFfile", false));
    checkBoxBPFSound->setChecked(settings.Get(sec, "BPFsound", false));
}

void ReceiverInputWidget::save(CSettings& settings) const
{
    string sec = accessibleName().toStdString();
    // Status
    if(radioButtonRSCI->isChecked())
    {
	settings.Put(sec, "rsiin_group", lineEditStatusAddress->text().toStdString());
	settings.Put(sec, "rsiin_port", lineEditStatusPort->text().toStdString());
	settings.Put(sec, "rciout_host", lineEditControlAddress->text().toStdString());
	settings.Put(sec, "rciout_port", lineEditControlPort->text().toStdString());
    }
    if(radioButtonFile->isChecked())
    {
	settings.Put(sec, "file", lineEditFile->text().toStdString());
	settings.Put(sec, "mode", stackedWidgetFile->currentIndex());
	settings.Put(sec, "channels", bgflrm->checkedId());
	settings.Put(sec, "sign", bgfiq->checkedId());
	settings.Put(sec, "flipspectrum", CheckBoxFlipSpecFile->isChecked());
    }
    if(radioButtonSoundCard->isChecked())
    {
	settings.Put(sec, "soundcard", comboBoxSoundCard->currentIndex());
	settings.Put(sec, "mode", stackedWidgetCard->currentIndex());
	settings.Put(sec, "channels", bgslrm->checkedId());
	settings.Put(sec, "sign", bgsiq->checkedId());
	settings.Put(sec, "flipspectrum", CheckBoxFlipSpecCard->isChecked());
    }
    // Control
    if(radioButtonRSCIControl->isChecked())
    {
    }
    if(radioButtonHamlib->isChecked())
    {
	int i = comboBoxRig->currentIndex();
	QVariant var =comboBoxRig->itemData(i);
	RigData r = var.value<RigData>();
	settings.Put(sec, "rig", r.id);
    }
    settings.Put(sec, "BPFfile", checkBoxBPFFile->isChecked());
    settings.Put(sec, "BPFsound", checkBoxBPFSound->isChecked());
}
