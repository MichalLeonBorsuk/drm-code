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
#include "../util-QT/Util.h"

#ifdef HAVE_LIBPCAP
# define PCAP_FILES " *.pcap"
#else
# define PCAP_FILES ""
#endif
#ifdef HAVE_LIBSNDFILE
# define SND_FILES "*.aif* *.au *.flac *.ogg *.rf64 *.snd *.wav"
#else
# define SND_FILES "*.if* *.iq* *.pcm* *.txt"
#endif
#define SND_FILE1 SND_FILES " "
#define SND_FILE2 "Sound Files (" SND_FILES ");;"
#define RSCI_FILES "*.rsA *.rsB *.rsC *.rsD *.rsQ *.rsM" PCAP_FILES
#define RSCI_FILE1 RSCI_FILES " "
#define RSCI_FILE2 "MDI/RSCI Files (" RSCI_FILES ");;"


static const CHANSEL InputChannelTable[] =
{
    { "Left Channel",  CReceiveData::CS_LEFT_CHAN    },
    { "Right Channel", CReceiveData::CS_RIGHT_CHAN   },
    { "L + R",         CReceiveData::CS_MIX_CHAN     },
    { "L - R",         CReceiveData::CS_SUB_CHAN     },
    { "I/Q Pos",       CReceiveData::CS_IQ_POS       },
    { "I/Q Neg",       CReceiveData::CS_IQ_NEG       },
    { "I/Q Pos Zero",  CReceiveData::CS_IQ_POS_ZERO  },
    { "I/Q Neg Zero",  CReceiveData::CS_IQ_NEG_ZERO  },
    { "I/Q Pos Split", CReceiveData::CS_IQ_POS_SPLIT },
    { "I/Q Neg Split", CReceiveData::CS_IQ_NEG_SPLIT },
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
    22050, 24000, 44100, 48000, 96000, 192000, 0
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
    bReceiver(DRMTransceiver.IsReceiver()),
    curInputDev(NULL), curOutputDev(NULL)
{
    setTitle(tr("Sound Card"));
    if (bReceiver)
    {   /* Receiver */
        Parameters.Lock();
            /* Menu Entries */
            menuSigInput = addMenu(tr("Signal Input"));
            QMenu* menuAudOutput = addMenu(tr("Audio Output"));
            /* Device List */
            menuSigDevice = InitDevice(NULL, menuSigInput, tr("Device"), SignalSampleRateTable, true);
            connect(menuSigDevice, SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInDevice(QAction*)));
            connect(InitDevice(NULL, menuAudOutput, tr("Device"), AudioSampleRateTable, false), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutDevice(QAction*)));
            /* Sample Rate List */
            menuSigSampleRate = InitSampleRate(menuSigInput, tr("Sample Rate"), Parameters.GetSoundCardSigSampleRate(), SignalSampleRateTable, true);
            connect(menuSigSampleRate, SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
            QMenu* menuAudSampleRate = InitSampleRate(menuAudOutput, tr("Sample Rate"), Parameters.GetAudSampleRate(), AudioSampleRateTable, false);
            connect(menuAudSampleRate, SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
            /* Channel List */
            connect(InitChannel(menuSigInput, tr("Channel"), (int)((CDRMReceiver&)DRMTransceiver).GetReceiveData()->GetInChanSel(), InputChannelTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInChannel(QAction*)));
            connect(InitChannel(menuAudOutput, tr("Channel"), (int)((CDRMReceiver&)DRMTransceiver).GetWriteData()->GetOutChanSel(), OutputChannelTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutChannel(QAction*)));
            /* Upscale checkbox */
            QAction *actionUpscale = menuSigInput->addAction(tr("2:1 upscale"));
            actionUpscale->setCheckable(true);
            actionUpscale->setChecked(Parameters.GetSigUpscaleRatio() == 2);
            connect(actionUpscale, SIGNAL(toggled(bool)), this, SLOT(OnSoundSignalUpscale(bool)));
        Parameters.Unlock();
        if (pFileMenu != NULL)
            connect(pFileMenu, SIGNAL(soundFileChanged(CDRMReceiver::ESFStatus)), this, SLOT(OnSoundFileChanged(CDRMReceiver::ESFStatus)));
    }
    else
    {   /* Transmitter */
        /* Menu Entries */
        QMenu* menuAudio = addMenu(tr("Audio Input"));
        QMenu* menuSignal = addMenu(tr("Signal Output"));
        /* Device List */
        connect(InitDevice(NULL, menuAudio, tr("Device"), AudioSampleRateTable, true), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInDevice(QAction*)));
        connect(InitDevice(NULL, menuSignal, tr("Device"), SignalSampleRateTable, false), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutDevice(QAction*)));
        /* Sample Rate List */
        connect(InitSampleRate(menuAudio, tr("Sample Rate"), Parameters.GetAudSampleRate(), AudioSampleRateTable, true), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
        connect(InitSampleRate(menuSignal, tr("Sample Rate"), Parameters.GetSigSampleRate(), SignalSampleRateTable, false), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
    }
    /* Update Sample Rate */
    UpdateSampleRate(true);
    UpdateSampleRate(false);
}

void CSoundCardSelMenu::OnSoundInDevice(QAction* action)
{
    Parameters.Lock();
    QString inputName;
    CSelectionInterface* pSoundInIF = DRMTransceiver.GetSoundInInterface();
    curInputDev = AsDeviceProp(action);
    pSoundInIF->SetDev(curInputDev->name);
    Parameters.Unlock();
    UpdateSampleRate(true);
}

void CSoundCardSelMenu::OnSoundOutDevice(QAction* action)
{
    Parameters.Lock();
    CSelectionInterface* pSoundOutIF = DRMTransceiver.GetSoundOutInterface();
    curOutputDev = AsDeviceProp(action);
    pSoundOutIF->SetDev(curOutputDev->name);
    Parameters.Unlock();
    UpdateSampleRate(false);
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
    const int iSampleRate = action->data().toInt();
    Parameters.Lock();
        if (iSampleRate < 0) Parameters.SetNewSigSampleRate(-iSampleRate);
        else                 Parameters.SetNewAudSampleRate(iSampleRate);
    Parameters.Unlock();
    RestartTransceiver(&DRMTransceiver);
    emit sampleRateChanged();
}

void CSoundCardSelMenu::OnSoundSignalUpscale(bool bChecked)
{
    Parameters.Lock();  
        Parameters.SetNewSigSampleRate(Parameters.GetSoundCardSigSampleRate());
        Parameters.SetNewSigUpscaleRatio(bChecked ? 2 : 1);
    Parameters.Unlock();
    RestartTransceiver(&DRMTransceiver);
    emit sampleRateChanged();
}

QMenu* CSoundCardSelMenu::InitDevice(QMenu* self, QMenu* parent, const QString& text, const int* deriredsamplerate, const bool bInput)
{
    QMenu* menu = self != NULL ? self : parent->addMenu(text);
    menu->clear();
    QActionGroup* group = NULL;
    CSelectionInterface* intf = bInput ? (CSelectionInterface*)DRMTransceiver.GetSoundInInterface() : (CSelectionInterface*)DRMTransceiver.GetSoundOutInterface();
    vector<deviceprop>& devs(bInput ? inputDevs : outputDevs);
    intf->Enumerate(devs, deriredsamplerate);
    int iNumSoundDev = devs.size();
    string sDefaultDev = intf->GetDev();
    deviceprop** curDev = bInput ? &curInputDev : &curOutputDev;
    *curDev = NULL;
    for (int i = 0; i < iNumSoundDev; i++)
    {
        QString name(QString::fromLocal8Bit(devs[i].name.c_str()));
        QString desc(QString::fromLocal8Bit(devs[i].desc.c_str()));
        QAction* m = menu->addAction(name == DEFAULT_DEVICE_NAME ? tr("[default]") : name + (desc.isEmpty() ? desc : " [" + desc + "]"));
        m->setData(qVariantFromValue((void*)&devs[i]));
        m->setCheckable(true);
        if (devs[i].name == sDefaultDev)
        {
            m->setChecked(true);
            *curDev = &devs[i];
        }
        if (group == NULL)
            group = new QActionGroup(m);
        group->addAction(m);
//printf("CSoundCardSelMenu::InitDevice() %s\n", devs[i].name.c_str());
    }
    UpdateSampleRate(bInput);
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

QMenu* CSoundCardSelMenu::InitSampleRate(QMenu* parent, const QString& text, const int iCurrentSampleRate, const int* SampleRate, const bool bInput)
{
    QMenu* menu = parent->addMenu(text);
    QActionGroup* group = new QActionGroup(parent);
    vector<QAction*>& actionSampleRate(bInput ? inputSampleRate : outputSampleRate);
    actionSampleRate.clear();
    for (int i = 0; SampleRate[i]; i++)
    {
        const int iSampleRate = SampleRate[i];
        const int iAbsSampleRate = abs(iSampleRate);
        QAction* m = menu->addAction(QString::number(iAbsSampleRate) + tr(" Hz"));
        m->setData(iSampleRate);
        m->setCheckable(true);
//        if (iAbsSampleRate == DEFAULT_SOUNDCRD_SAMPLE_RATE)
//            menu->setDefaultAction(m);
        if (iAbsSampleRate == iCurrentSampleRate)
            m->setChecked(true);
        group->addAction(m);
        actionSampleRate.push_back(m);
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
                InitDevice(menuSigDevice, menuSigInput, tr("Device"), SignalSampleRateTable, true);
            Parameters.Unlock();
        }
    }
}

deviceprop* CSoundCardSelMenu::AsDeviceProp(QAction* action)
{
	return (deviceprop*)action->data().value<void*>();
}

void CSoundCardSelMenu::UpdateSampleRate(const bool bInput)
{
    deviceprop* dev = bInput ? curInputDev : curOutputDev;
    if (dev)
    {
        vector<QAction*>& actionSampleRate(bInput ? inputSampleRate : outputSampleRate);
        for (size_t i = 0; i < actionSampleRate.size(); i++)
        {
            int samplerate = abs(actionSampleRate[i]->data().toInt());
            actionSampleRate[i]->setEnabled(dev->samplerates[samplerate] == true);
        }
    }
}

/* CFileMenu ******************************************************************/
// TODO DRMTransmitter

CFileMenu::CFileMenu(CDRMTransceiver& DRMTransceiver, QMainWindow* parent,
    QMenu* menuInsertBefore)
    : QMenu(parent), DRMTransceiver(DRMTransceiver), bReceiver(DRMTransceiver.IsReceiver())
{
    setTitle(tr("&File"));
    if (bReceiver)
    {
        QString openFile(tr("&Open File..."));
        QString closeFile(tr("&Close File"));
        actionOpenFile = addAction(openFile, this, SLOT(OnOpenFile()), QKeySequence(tr("Alt+O")));
        actionCloseFile = addAction(closeFile, this, SLOT(OnCloseFile()), QKeySequence(tr("Alt+C")));
        addSeparator();
    }
    addAction(tr("&Exit"), parent, SLOT(close()), QKeySequence(tr("Alt+X")));
    parent->menuBar()->insertMenu(menuInsertBefore->menuAction(), this);
}


void CFileMenu::OnOpenFile()
{
#define FILE_FILTER \
	"Supported Files (" \
	SND_FILE1 \
	RSCI_FILE1 \
	");;" \
	SND_FILE2 \
	RSCI_FILE2 \
	"All Files (*)"
    if (bReceiver)
    {
	    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), strLastSoundPath, tr(FILE_FILTER));
	    /* Check if user not hit the cancel button */
	    if (!filename.isEmpty())
	    {
			strLastSoundPath = filename;
		    ((CDRMReceiver&)DRMTransceiver).SetInputFile(string(filename.toLocal8Bit().constData()));
		    RestartTransceiver(&DRMTransceiver);
            UpdateMenu();
	    }
    }
}

void CFileMenu::OnCloseFile()
{
    if (bReceiver)
    {
	    ((CDRMReceiver&)DRMTransceiver).ClearInputFile();
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

        const bool bInputFile = bSoundFile | bRsciMdiIn;
        if (bInputFile != actionCloseFile->isEnabled())
            actionCloseFile->setEnabled(bInputFile);

        emit soundFileChanged(eStatus);
    }
}

