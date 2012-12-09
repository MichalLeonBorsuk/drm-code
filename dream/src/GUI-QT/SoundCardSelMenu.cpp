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
#include "../Parameter.h"
#include "../DRMSignalIO.h"
#include "../DataIO.h"
#include "../DrmReceiver.h"
#include "../DrmTransmitter.h"
#include "SoundCardSelMenu.h"
#include "DialogUtil.h"
#include <QFileDialog>


static const CHANSEL InputChannelTable[] =
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

static const CHANSEL OutputChannelTable[] =
{
    { "Both Channels",              CWriteData::CS_BOTH_BOTH   },
    { "Left -> Left, Right Muted",  CWriteData::CS_LEFT_LEFT   },
    { "Right -> Right, Left Muted", CWriteData::CS_RIGHT_RIGHT },
    { "L + R -> Left, Right Muted", CWriteData::CS_LEFT_MIX    },
    { "L + R -> Right, Left Muted", CWriteData::CS_RIGHT_MIX   },
    { NULL, 0 } /* end of list */
};

static const int AudioSampleRateTable[] =
{
    8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 96000, 192000, 0
};

static const int SignalSampleRateTable[] =
{
    -24000, -48000, -96000, -192000, 0
};


/* Implementation *************************************************************/

/* CSoundCardSelMenu **********************************************************/

CSoundCardSelMenu::CSoundCardSelMenu(CDRMTransceiver& DRMTransceiver,
    CFileMenu* pFileMenu, QWidget* parent) : QMenu(parent),
    DRMTransceiver(DRMTransceiver), Parameters(*DRMTransceiver.GetParameters()),
    menuSigInput(NULL), menuSigDevice(NULL), menuSigSampleRate(NULL),
    bReceiver(DRMTransceiver.IsReceiver())
{
    setTitle(tr("Sound Card"));
    if (bReceiver)
    {   /* Receiver */
        Parameters.Lock();
            menuSigInput = addMenu(tr("Signal Input"));
            QMenu* menuAudOutput = addMenu(tr("Audio Output"));
            connect(InitChannel(menuSigInput, tr("Channel"), (int)((CDRMReceiver&)DRMTransceiver).GetReceiveData()->GetInChanSel(), InputChannelTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInChannel(QAction*)));
            connect(InitChannel(menuAudOutput, tr("Channel"), (int)((CDRMReceiver&)DRMTransceiver).GetWriteData()->GetOutChanSel(), OutputChannelTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutChannel(QAction*)));
            menuSigDevice = InitDevice(NULL, menuSigInput, tr("Device"), true);
            connect(menuSigDevice, SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInDevice(QAction*)));
            connect(InitDevice(NULL, menuAudOutput, tr("Device"), false), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutDevice(QAction*)));
            menuSigSampleRate = InitSampleRate(menuSigInput, tr("Sample Rate"), Parameters.GetSigSampleRate(), SignalSampleRateTable);
            connect(menuSigSampleRate, SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
            connect(InitSampleRate(menuAudOutput, tr("Sample Rate"), Parameters.GetAudSampleRate(), AudioSampleRateTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
        Parameters.Unlock();
        if (pFileMenu != NULL)
            connect(pFileMenu, SIGNAL(soundFileChanged(CDRMReceiver::ESFStatus)), this, SLOT(OnSoundFileChanged(CDRMReceiver::ESFStatus)));
    }
    else
    {   /* Transmitter */
        connect(InitDevice(NULL, this, tr("Input Device"), true), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInDevice(QAction*)));
        connect(InitDevice(NULL, this, tr("Output Device"), false), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutDevice(QAction*)));
    }
}

void CSoundCardSelMenu::OnSoundInDevice(QAction* action)
{
    Parameters.Lock();
        CSelectionInterface* pSoundInIF = DRMTransceiver.GetSoundInInterface();
        pSoundInIF->SetDev(action->data().toString().toLocal8Bit().data());
    Parameters.Unlock();
}

void CSoundCardSelMenu::OnSoundOutDevice(QAction* action)
{
    Parameters.Lock();
        CSelectionInterface* pSoundOutIF = DRMTransceiver.GetSoundOutInterface();
        pSoundOutIF->SetDev(action->data().toString().toLocal8Bit().data());
    Parameters.Unlock();
}

void CSoundCardSelMenu::OnSoundInChannel(QAction* action)
{
    if (bReceiver)
    {
        Parameters.Lock();
            CReceiveData& ReceiveData = *((CDRMReceiver&)DRMTransceiver).GetReceiveData();
            CReceiveData::EInChanSel eInChanSel = CReceiveData::EInChanSel(action->data().toInt());
            ReceiveData.SetInChanSel(eInChanSel);
        Parameters.Unlock();
    }
}

void CSoundCardSelMenu::OnSoundOutChannel(QAction* action)
{
    if (bReceiver)
    {
        Parameters.Lock();
            CWriteData& WriteData = *((CDRMReceiver&)DRMTransceiver).GetWriteData();
            CWriteData::EOutChanSel eOutChanSel = CWriteData::EOutChanSel(action->data().toInt());
            WriteData.SetOutChanSel(eOutChanSel);
        Parameters.Unlock();
    }
}

void CSoundCardSelMenu::OnSoundSampleRate(QAction* action)
{
    if (bReceiver)
    {
        const int iSampleRate = action->data().toInt();
        Parameters.Lock();
            if (iSampleRate < 0) Parameters.SetSigSampleRate(-iSampleRate);
            else                 Parameters.SetAudSampleRate(iSampleRate);
        Parameters.Unlock();
        RestartTransceiver(&DRMTransceiver);
    }
}

QMenu* CSoundCardSelMenu::InitDevice(QMenu* self, QMenu* parent, const QString& text, bool bInput)
{
//printf("CSoundCardSelMenu::InitDevice(%i)\n", bInput);
    CSelectionInterface* intf = bInput ? (CSelectionInterface*)DRMTransceiver.GetSoundInInterface() : (CSelectionInterface*)DRMTransceiver.GetSoundOutInterface();
    QMenu* menu = self != NULL ? self : parent->addMenu(text);
    menu->clear();
    QActionGroup* group = NULL;
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
        if (group == NULL)
            group = new QActionGroup(m);
        group->addAction(m);
//printf("CSoundCardSelMenu::InitDevice() %s\n", name.toUtf8().data());
    }
    return menu;
}

QMenu* CSoundCardSelMenu::InitChannel(QMenu* parent, const QString& text, const int iCurrentChanSel, const CHANSEL* ChanSel)
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

QMenu* CSoundCardSelMenu::InitSampleRate(QMenu* parent, const QString& text, const int iCurrentSampleRate, const int* SampleRate)
{
    QMenu* menu = parent->addMenu(text);
    QActionGroup* group = new QActionGroup(parent);
    for (int i = 0; SampleRate[i]; i++)
    {
        const int iSampleRate = SampleRate[i];
        const int iAbsSampleRate = abs(iSampleRate);
        QAction* m = menu->addAction(QString::number(iAbsSampleRate) + tr(" Hz"));
        m->setData(iSampleRate);
        m->setCheckable(true);
        if (iAbsSampleRate == iCurrentSampleRate)
            m->setChecked(true);
        group->addAction(m);
    }
    return menu;
}

void CSoundCardSelMenu::OnSoundFileChanged(CDRMReceiver::ESFStatus eStatus)
{
    const bool bSoundFile = eStatus == CDRMReceiver::SF_SNDFILEIN;
    const bool bRsciMdiIn = eStatus == CDRMReceiver::SF_RSCIMDIIN;

    if (menuSigInput != NULL && bRsciMdiIn == menuSigInput->isEnabled())
        menuSigInput->setEnabled(!bRsciMdiIn);
	
    if (menuSigDevice != NULL && bSoundFile == menuSigDevice->isEnabled())
        menuSigDevice->setEnabled(!bSoundFile);

    if (menuSigSampleRate != NULL && bSoundFile == menuSigSampleRate->isEnabled())
        menuSigSampleRate->setEnabled(!bSoundFile);

    if (eStatus == CDRMReceiver::SF_SNDCARDIN)
    {
        if (bReceiver)
        {
            Parameters.Lock();
                InitDevice(menuSigDevice, menuSigInput, tr("Device"), true);
            Parameters.Unlock();
        }
    }
}

/* CFileMenu ******************************************************************/
// TODO DRMTransmitter

CFileMenu::CFileMenu(CDRMTransceiver& DRMTransceiver, QMainWindow* parent,
    QMenu* menuInsertBefore, bool bSignal)
    : QMenu(parent), DRMTransceiver(DRMTransceiver), bReceiver(DRMTransceiver.IsReceiver())
{
    setTitle(tr("&File"));
    if (bReceiver)
    {
        QString openFile(tr(bSignal ? "&Open Signal File..." : "&Open Audio File..."));
        QString closeFile(tr(bSignal ? "&Close Signal File" : "&Close Audio File"));
        actionOpenSignalFile = addAction(openFile, this, SLOT(OnOpenSignalFile()), QKeySequence(tr("Alt+O")));
        actionCloseSignalFile = addAction(closeFile, this, SLOT(OnCloseSignalFile()), QKeySequence(tr("Alt+C")));
        addSeparator();
        actionOpenRsciFile = addAction(tr("Open RSCI/MDI File..."), this, SLOT(OnOpenRsciFile())/*, QKeySequence(tr("Alt+O"))*/);
        actionCloseRsciFile = addAction(tr("Close RSCI/MDI File"), this, SLOT(OnCloseRsciFile())/*, QKeySequence(tr("Alt+C"))*/);
        addSeparator();
    }
    addAction(tr("&Exit"), parent, SLOT(close()), QKeySequence(tr("Alt+X")));
    parent->menuBar()->insertMenu(menuInsertBefore->menuAction(), this);
}

void CFileMenu::OnOpenSignalFile()
{
#ifdef HAVE_LIBSNDFILE
# define AUDIO_FILE_FILTER "Sound Files (*.aif* *.au *.flac *.ogg *.rf64 *.snd *.wav);;All Files (*)"
#else
# define AUDIO_FILE_FILTER "Sound Files (*.if* *.iq* *.pcm* *.txt);;All Files (*)"
#endif
    if (bReceiver)
    {
	    QString filename = QFileDialog::getOpenFileName(this, tr("Open Sound File"), NULL, tr(AUDIO_FILE_FILTER));
	    /* Check if user not hit the cancel button */
	    if (!filename.isEmpty())
	    {
		    // TODO implement a queue for more that one file!
		    ((CDRMReceiver&)DRMTransceiver).SetSoundFile(string(filename.toLocal8Bit().data()));
		    RestartTransceiver(&DRMTransceiver);
            UpdateMenu();
	    }
    }
}

void CFileMenu::OnCloseSignalFile()
{
    if (bReceiver)
    {
	    ((CDRMReceiver&)DRMTransceiver).ClearSoundFile();
	    RestartTransceiver(&DRMTransceiver);
        UpdateMenu();
    }
}

void CFileMenu::OnOpenRsciFile()
{
    if (bReceiver)
    {
        QString filename = QFileDialog::getOpenFileName(this, tr("Open RSCI/MDI File"), NULL, tr("RSCI/MDI Files (*.rs*);;All Files (*)"));
        /* Check if user not hit the cancel button */
        if (!filename.isEmpty())
        {
            // TODO implement a queue for more that one file!
            ((CDRMReceiver&)DRMTransceiver).SetRsciInput(string(filename.toLocal8Bit().data()));
            RestartTransceiver(&DRMTransceiver);
            UpdateMenu();
        }
    }
}

void CFileMenu::OnCloseRsciFile()
{
    if (bReceiver)
    {
        ((CDRMReceiver&)DRMTransceiver).ClearRsciInput();
        RestartTransceiver(&DRMTransceiver);
        UpdateMenu();
    }
}

void CFileMenu::UpdateMenu()
{
    if (bReceiver)
    {
        CDRMReceiver::ESFStatus eStatus = ((CDRMReceiver&)DRMTransceiver).GetInputStatus();
        const bool bSoundFile = eStatus == CDRMReceiver::SF_SNDFILEIN;
        const bool bRsciMdiIn = eStatus == CDRMReceiver::SF_RSCIMDIIN;

        if (bRsciMdiIn == actionOpenSignalFile->isEnabled())
            actionOpenSignalFile->setEnabled(!bRsciMdiIn);

        if (bSoundFile != actionCloseSignalFile->isEnabled())
            actionCloseSignalFile->setEnabled(bSoundFile);

        if (bRsciMdiIn != actionCloseRsciFile->isEnabled())
            actionCloseRsciFile->setEnabled(bRsciMdiIn);

        emit soundFileChanged(eStatus);
    }
}

