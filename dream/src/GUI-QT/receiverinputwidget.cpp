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
#include <QFileDialog>

ReceiverInputWidget::ReceiverInputWidget(QWidget *parent) :
    QWidget(parent),
    bgrsf(new QButtonGroup(this)),
    bgriq(new QButtonGroup(this)),
    bglrm(new QButtonGroup(this)),
    bgiq(new QButtonGroup(this)),
    bgrh(new QButtonGroup(this))
{
    setupUi(this);
    bgrsf->addButton(radioButtonCard, stackedWidgetCardFileNet->indexOf(pageCard));
    bgrsf->addButton(radioButtonFile, stackedWidgetCardFileNet->indexOf(pageFile));
    bgrsf->addButton(radioButtonRSCI, stackedWidgetCardFileNet->indexOf(pageRSCIStatus));

    bgriq->addButton(radioButtonReal, stackedWidgetRealIQ->indexOf(pageReal));
    bgriq->addButton(radioButtonIQ, stackedWidgetRealIQ->indexOf(pageIQ));

    bglrm->addButton(radioButtonLeft, CS_LEFT_CHAN);
    bglrm->addButton(radioButtonRight, CS_RIGHT_CHAN);
    bglrm->addButton(radioButtonMix, CS_MIX_CHAN);

    bgrh->addButton(radioButtonRSCIControl, 1);
    bgrh->addButton(radioButtonHamlib, 0);

    connect(bgrsf, SIGNAL(buttonClicked(int)), stackedWidgetCardFileNet, SLOT(setCurrentIndex(int)));
    connect(bgriq, SIGNAL(buttonClicked(int)), this, SLOT(onBGriq(int)));
    connect(bgrh, SIGNAL(buttonClicked(int)), stackedWidgetControl, SLOT(setCurrentIndex(int)));
    connect(pushButtonChooseFile, SIGNAL(clicked()), this, SLOT(onChooseFile()));

    const QString s = tr("<b>Flip Input Spectrum:</b> Checking this box "
	    "will flip or invert the input spectrum. This is necessary if the "
	    "mixer in the front-end uses the lower side band.");
    CheckBoxFlipSpec->setWhatsThis(s);
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
    // an RSCI spec can be either a network port or a file (.rsM, .rsA ...)
    // If local it can be a rig+soundcard or an iq file or a wav file
    // this can either be mode specific or global
    // first look for global specific settings then specific ones.
    // global settings will be turned into mode specific settings ???

    string inp = settings.Get(sec, "input", string("card"));
    if(inp == "card")
	radioButtonCard->setChecked(true);
    if(inp == "file")
	radioButtonFile->setChecked(true);
    if(inp == "net")
	radioButtonRSCI->setChecked(true);

    string str = settings.Get(sec, "rsiin", string(""));
    if(str=="")
    {
	lineEditStatusAddress->setText("");
	lineEditStatusPort->setText("");
    }
    else
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
	lineEditStatusAddress->setText(addr.c_str());
	lineEditStatusPort->setText(port.c_str());
    }
    lineEditFile->setText(settings.Get(sec, "file", string("")).c_str());
    if(settings.Get(sec, "iq", true))
	radioButtonIQ->setChecked(true);
    else
	radioButtonReal->setChecked(true);
    EInChanSel inChanSel = EInChanSel(settings.Get(sec, "channels", CS_MIX_CHAN));
    switch(inChanSel)
    {
	case CS_LEFT_CHAN:
	    radioButtonReal->setChecked(true);
	    radioButtonLeft->setChecked(true);
	    break;
	case CS_RIGHT_CHAN:
	    radioButtonReal->setChecked(true);
	    radioButtonRight->setChecked(true);
	    break;
	case CS_MIX_CHAN:
	    radioButtonReal->setChecked(true);
	    radioButtonMix->setChecked(true);
	    break;
	case CS_IQ_POS:
	    radioButtonIQ->setChecked(true);
	    radioButtonIQPos->setChecked(true);
	    checkBoxZIF->setChecked(false);
	    break;
	case CS_IQ_NEG:
	    radioButtonIQ->setChecked(true);
	    radioButtonIQNeg->setChecked(true);
	    checkBoxZIF->setChecked(false);
	    break;
	case CS_IQ_POS_ZERO:
	    radioButtonIQ->setChecked(true);
	    radioButtonIQPos->setChecked(true);
	    checkBoxZIF->setChecked(true);
	    break;
	case CS_IQ_NEG_ZERO:
	    radioButtonIQ->setChecked(true);
	    radioButtonIQNeg->setChecked(true);
	    checkBoxZIF->setChecked(true);
    }
    CheckBoxFlipSpec->setChecked(settings.Get(sec, "flipspectrum", false));
    checkBoxBPF->setChecked(settings.Get(sec, "BPF", false));
    lineEditCalFactor->setText(settings.Get(sec, "calfactor", string("0.0")).c_str());

    int dev = settings.Get(sec, "soundcard", -1);
    if(dev==-1)
    {
	comboBoxCard->clearFocus();
    }
    else
    {
	comboBoxCard->setCurrentIndex(dev);
    }

    // Control

    str = settings.Get(sec, "control", string("Hamlib"));
    if(str=="Hamlib")
	radioButtonHamlib->setChecked(true);
    else
	radioButtonRSCIControl->setChecked(true);

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
}

void ReceiverInputWidget::save(CSettings& settings) const
{
    string sec = accessibleName().toStdString();
    // Status
    if(radioButtonRSCI->isChecked())
	settings.Put(sec, "input", "net");
    if(radioButtonFile->isChecked())
	settings.Put(sec, "input", "file");
    if(radioButtonCard->isChecked())
	settings.Put(sec, "input", "card");

    if(radioButtonReal->isChecked())
    {
	settings.Put(sec, "channels", bglrm->checkedId());
    }
    else
    {
	if(radioButtonIQPos->isChecked())
	{
	    if(checkBoxZIF->isChecked())
	    {
		settings.Put(sec, "channels", CS_IQ_POS_ZERO);
	    }
	    else
	    {
		settings.Put(sec, "channels", CS_IQ_POS);
	    }
	}
	else
	{
	    if(checkBoxZIF->isChecked())
	    {
		settings.Put(sec, "channels", CS_IQ_NEG_ZERO);
	    }
	    else
	    {
		settings.Put(sec, "channels", CS_IQ_NEG);
	    }
	}
    }
    settings.Put(sec, "rsiin_group", lineEditStatusAddress->text().toStdString());
    settings.Put(sec, "rsiin_port", lineEditStatusPort->text().toStdString());
    settings.Put(sec, "file", lineEditFile->text().toStdString());
    settings.Put(sec, "flipspectrum", CheckBoxFlipSpec->isChecked());
    settings.Put(sec, "soundcard", comboBoxCard->currentIndex());
    settings.Put(sec, "BPF", checkBoxBPF->isChecked());
    settings.Put(sec, "calfactor", lineEditCalFactor->text().toStdString());

    // Control
    if(radioButtonRSCIControl->isChecked())
    {
	settings.Put(sec, "control", string("RSCI"));
    }
    else
    {
	settings.Put(sec, "control", string("Hamlib"));
    }
    settings.Put(sec, "rciout_host", lineEditControlAddress->text().toStdString());
    settings.Put(sec, "rciout_port", lineEditControlPort->text().toStdString());
    int i = comboBoxRig->currentIndex();
    QVariant var =comboBoxRig->itemData(i);
    RigData r = var.value<RigData>();
    settings.Put(sec, "rig", r.id);
}

void ReceiverInputWidget::onChooseFile()
{
    QString fileName = QFileDialog::getOpenFileName(
	    this,
	    tr("Open File"),
	     ".",
	     tr("Audio (*.wav *.flac);;RSCI (*.rs* *.pcap);;IQ(*.iq*) (*.IQ*)")
     );
    if(fileName!="")
	lineEditFile->setText(fileName);
}

void ReceiverInputWidget::onBGriq(int id)
{
}
