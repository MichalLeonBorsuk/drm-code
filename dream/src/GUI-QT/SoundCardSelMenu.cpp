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
            menuSigDevice = InitDevice(NULL, menuSigInput, tr("Device"), true);
            connect(menuSigDevice, SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInDevice(QAction*)));
            connect(InitDevice(NULL, menuAudOutput, tr("Device"), false), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutDevice(QAction*)));
            connect(InitChannel(menuSigInput, tr("Channel"), (int)((CDRMReceiver&)DRMTransceiver).GetReceiveData()->GetInChanSel(), InputChannelTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInChannel(QAction*)));
            connect(InitChannel(menuAudOutput, tr("Channel"), (int)((CDRMReceiver&)DRMTransceiver).GetWriteData()->GetOutChanSel(), OutputChannelTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutChannel(QAction*)));
            menuSigSampleRate = InitSampleRate(menuSigInput, tr("Sample Rate"), Parameters.GetSigSampleRate(), SignalSampleRateTable);
            connect(menuSigSampleRate, SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
            connect(InitSampleRate(menuAudOutput, tr("Sample Rate"), Parameters.GetAudSampleRate(), AudioSampleRateTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
        Parameters.Unlock();
        if (pFileMenu != NULL)
            connect(pFileMenu, SIGNAL(soundFileChanged(CDRMReceiver::ESFStatus)), this, SLOT(OnSoundFileChanged(CDRMReceiver::ESFStatus)));
    }
    else
    {   /* Transmitter */
        QMenu* menuAudio = addMenu(tr("Audio Input"));
        QMenu* menuSignal = addMenu(tr("Signal Output"));
        connect(InitDevice(NULL, menuAudio, tr("Device"), true), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInDevice(QAction*)));
        connect(InitDevice(NULL, menuSignal, tr("Device"), false), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutDevice(QAction*)));
        connect(InitSampleRate(menuAudio, tr("Sample Rate"), Parameters.GetAudSampleRate(), AudioSampleRateTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
        connect(InitSampleRate(menuSignal, tr("Sample Rate"), Parameters.GetSigSampleRate(), SignalSampleRateTable), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundSampleRate(QAction*)));
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
    const int iSampleRate = action->data().toInt();
    Parameters.Lock();
        if (iSampleRate < 0) Parameters.SetNewSigSampleRate(-iSampleRate);
        else                 Parameters.SetNewAudSampleRate(iSampleRate);
    Parameters.Unlock();
    RestartTransceiver(&DRMTransceiver);
    emit sampleRateChanged();
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
//        if (name.isEmpty())
//            menu->setDefaultAction(m);
        if (names[i] == sDefaultDev)
            m->setChecked(true);
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
//        if (iAbsSampleRate == DEFAULT_SOUNDCRD_SAMPLE_RATE)
//            menu->setDefaultAction(m);
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
#ifdef FILE_MENU_UNIFIED_OPEN_FILE
        (void)bSignal;
        QString openFile(tr("&Open File..."));
        QString closeFile(tr("&Close File"));
        actionOpenFile = addAction(openFile, this, SLOT(OnOpenFile()), QKeySequence(tr("Alt+O")));
        actionCloseFile = addAction(closeFile, this, SLOT(OnCloseFile()), QKeySequence(tr("Alt+C")));
#else
        QString openFile(tr(bSignal ? "&Open Signal File..." : "&Open Audio File..."));
        QString closeFile(tr(bSignal ? "&Close Signal File" : "&Close Audio File"));
        actionOpenSignalFile = addAction(openFile, this, SLOT(OnOpenSignalFile()), QKeySequence(tr("Alt+O")));
        actionCloseSignalFile = addAction(closeFile, this, SLOT(OnCloseSignalFile()), QKeySequence(tr("Alt+C")));
        addSeparator();
        actionOpenRsciFile = addAction(tr("Open MDI/RSCI File..."), this, SLOT(OnOpenRsciFile())/*, QKeySequence(tr("Alt+O"))*/);
        actionCloseRsciFile = addAction(tr("Close MDI/RSCI File"), this, SLOT(OnCloseRsciFile())/*, QKeySequence(tr("Alt+C"))*/);
#endif
        addSeparator();
    }
    addAction(tr("&Exit"), parent, SLOT(close()), QKeySequence(tr("Alt+X")));
    parent->menuBar()->insertMenu(menuInsertBefore->menuAction(), this);
}


#ifdef FILE_MENU_UNIFIED_OPEN_FILE

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
		    ((CDRMReceiver&)DRMTransceiver).SetInputFile(string(filename.toLocal8Bit().data()));
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

#else

void CFileMenu::OnOpenSignalFile()
{
#define AUDIO_FILE_FILTER "Sound Files (" SND_FILES ");;All Files (*)"
    if (bReceiver)
    {
        QString filename = QFileDialog::getOpenFileName(this, tr("Open Sound File"),
            strLastSoundPath, tr(AUDIO_FILE_FILTER));
        /* Check if user not hit the cancel button */
        if (!filename.isEmpty())
        {
            strLastSoundPath = filename;
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
#define RSCI_FILE_FILTER "MDI/RSCI Files (" RSCI_FILES ");;All Files (*)"
    if (bReceiver)
    {
        QString filename = QFileDialog::getOpenFileName(this, tr("Open MDI/RSCI File"),
            strLastRsciPath, tr(RSCI_FILE_FILTER));
        /* Check if user not hit the cancel button */
        if (!filename.isEmpty())
        {
			strLastRsciPath = filename;
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

#endif


void CFileMenu::UpdateMenu()
{
    if (bReceiver)
    {
        CDRMReceiver::ESFStatus eStatus = ((CDRMReceiver&)DRMTransceiver).GetInputStatus();
        const bool bSoundFile = eStatus == CDRMReceiver::SF_SNDFILEIN;
        const bool bRsciMdiIn = eStatus == CDRMReceiver::SF_RSCIMDIIN;

#ifdef FILE_MENU_UNIFIED_OPEN_FILE
        const bool bInputFile = bSoundFile | bRsciMdiIn;
        if (bInputFile != actionCloseFile->isEnabled())
            actionCloseFile->setEnabled(bInputFile);
#else
        if (bRsciMdiIn == actionOpenSignalFile->isEnabled())
            actionOpenSignalFile->setEnabled(!bRsciMdiIn);

        if (bSoundFile != actionCloseSignalFile->isEnabled())
            actionCloseSignalFile->setEnabled(bSoundFile);

        if (bRsciMdiIn != actionCloseRsciFile->isEnabled())
            actionCloseRsciFile->setEnabled(bRsciMdiIn);
#endif

        emit soundFileChanged(eStatus);
    }
}

