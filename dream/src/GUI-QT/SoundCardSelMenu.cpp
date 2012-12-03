/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2012
 *
 * Author(s):
 *      Julian Cable, David Flamand
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
#include "SoundCardSelMenu.h"


static CHANSEL InputChannelTable[] =
{
    { "Left Channel",  CReceiveData::CS_LEFT_CHAN   },
    { "Right Channel", CReceiveData::CS_RIGHT_CHAN  },
    { "L + R",         CReceiveData::CS_MIX_CHAN    },
    { "L - R",         CReceiveData::CS_SUB_CHAN    },
    { "I/Q Pos",       CReceiveData::CS_IQ_POS      },
    { "I/Q Neg",       CReceiveData::CS_IQ_NEG      },
    { "I/Q Pos Zero",  CReceiveData::CS_IQ_POS_ZERO },
    { "I/Q Neg Zero",  CReceiveData::CS_IQ_NEG_ZERO },
    { NULL, 0 } /* end of list */
};

static CHANSEL OutputChannelTable[] =
{
    { "Both Channels",              CWriteData::CS_BOTH_BOTH   },
    { "Left -> Left, Right Muted",  CWriteData::CS_LEFT_LEFT   },
    { "Right -> Right, Left Muted", CWriteData::CS_RIGHT_RIGHT },
    { "L + R -> Left, Right Muted", CWriteData::CS_LEFT_MIX    },
    { "L + R -> Right, Left Muted", CWriteData::CS_RIGHT_MIX   },
    { NULL, 0 } /* end of list */
};


CSoundCardSelMenu::CSoundCardSelMenu(
    CDRMReceiver* DRMReceiver, CSelectionInterface* pNSIn, CSelectionInterface* pNSOut, QWidget* parent) :
    QMenu(parent), DRMReceiver(DRMReceiver), pSoundInIF(pNSIn), pSoundOutIF(pNSOut)
{
    setTitle(tr("Sound Card"));
    if (DRMReceiver != NULL)
    {   /* Receiver */
        CParameter& Parameters = *DRMReceiver->GetParameters();
        Parameters.Lock();
            QMenu* input_menu = addMenu(tr("Signal Input"));
            QMenu* output_menu = addMenu(tr("Audio Output"));
            connect(InitChannel(input_menu, tr("Channel"), (int)DRMReceiver->GetReceiveData()->GetInChanSel(), InputChannelTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInChannel(QAction*)));
            connect(InitChannel(output_menu, tr("Channel"), (int)DRMReceiver->GetWriteData()->GetOutChanSel(), OutputChannelTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutChannel(QAction*)));
            connect(InitDevice(input_menu, tr("Device"), pSoundInIF), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInDevice(QAction*)));
            connect(InitDevice(output_menu, tr("Device"), pSoundOutIF), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutDevice(QAction*)));
        Parameters.Unlock();
    }
    else
    {   /* Transmitter */
        connect(InitDevice(this, tr("Input Device"), pSoundInIF), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInDevice(QAction*)));
        connect(InitDevice(this, tr("Output Device"), pSoundOutIF), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutDevice(QAction*)));
    }
}

void CSoundCardSelMenu::OnSoundInChannel(QAction* action)
{
    if (DRMReceiver != NULL)
    {
        CParameter& Parameters = *DRMReceiver->GetParameters();
        Parameters.Lock();
            CReceiveData& ReceiveData = *DRMReceiver->GetReceiveData();
            CReceiveData::EInChanSel eInChanSel = CReceiveData::EInChanSel(action->data().toInt());
            ReceiveData.SetInChanSel(eInChanSel);
        Parameters.Unlock();
    }
}

void CSoundCardSelMenu::OnSoundOutChannel(QAction* action)
{
    if (DRMReceiver != NULL)
    {
        CParameter& Parameters = *DRMReceiver->GetParameters();
        Parameters.Lock();
            CWriteData& WriteData = *DRMReceiver->GetWriteData();
            CWriteData::EOutChanSel eOutChanSel = CWriteData::EOutChanSel(action->data().toInt());
            WriteData.SetOutChanSel(eOutChanSel);
        Parameters.Unlock();
    }
}

void CSoundCardSelMenu::OnSoundInDevice(QAction* action)
{
    pSoundInIF->SetDev(action->data().toString().toLocal8Bit().data());
}

void CSoundCardSelMenu::OnSoundOutDevice(QAction* action)
{
    pSoundOutIF->SetDev(action->data().toString().toLocal8Bit().data());
}

QMenu* CSoundCardSelMenu::InitDevice(QMenu* parent, const QString& text, CSelectionInterface* intf)
{
    QMenu* menu = parent->addMenu(text);
    QActionGroup* group = new QActionGroup(this);
    vector<string> names;
    vector<string> descriptions;
    intf->Enumerate(names, descriptions);
    int iNumSoundDev = names.size();
    int iNumDescriptions = descriptions.size(); /* descriptions are optional */
    string sDefaultDev = intf->GetDev();
    for (int i = 0; i < iNumSoundDev; i++)
    {
		QString name(QString::fromLocal8Bit(names[i].c_str()));
		QString desc(i < iNumDescriptions ? QString::fromLocal8Bit(descriptions[i].c_str()) : QString());
        QAction* m = menu->addAction(name.isEmpty() ? tr("[default]") : name + (desc.isEmpty() ? desc : " [" + desc + "]"));
        m->setData(name);
        m->setCheckable(true);
        if (names[i] == sDefaultDev)
        {
//            menu->setDefaultAction(m); // TODO
            m->setChecked(true);
    	}
        group->addAction(m);
    }
    return menu;
}

QMenu* CSoundCardSelMenu::InitChannel(QMenu* parent, const QString& text, int iCurrentChanSel, CHANSEL* ChanSel)
{
    QMenu* menu = parent->addMenu(text);
    QActionGroup* group = new QActionGroup(parent);
    for (int i = 0; ChanSel[i].Name; i++)
    {
        QAction* m = menu->addAction(tr(ChanSel[i].Name));
        int iChanSel = ChanSel[i].iChanSel;
        m->setData(iChanSel);
        m->setCheckable(true);
        if (iChanSel == iCurrentChanSel)
            m->setChecked(true);
        group->addAction(m);
    }
    return menu;
}


