/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer
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

#include <qmenubar.h>
#include <qlabel.h>
#include <qaction.h>
#include <qmessagebox.h>
#include <qdir.h>
#include <qfile.h>
#if QT_VERSION < 0x040000
# include <qregexp.h>
#endif
#ifdef _WIN32
# include <winsock2.h>
#endif
#include "DialogUtil.h"
#if QT_VERSION < 0x040000
# include <qwhatsthis.h>
# include <qtextview.h>
#else
# include <QCoreApplication>
# include <QWhatsThis>
# define CHECK_PTR(x) Q_CHECK_PTR(x)
#endif
#include "../Version.h"
#ifdef USE_ALSA
# include <alsa/version.h>
#endif
#ifdef USE_OSS
# include <sys/soundcard.h>
#endif
#ifdef USE_PORTAUDIO
# include <portaudio.h>
#endif
#ifdef USE_PULSEAUDIO
# include <pulse/version.h>
#endif
#ifdef HAVE_LIBSNDFILE
# include <sndfile.h>
#endif
#ifdef HAVE_LIBPCAP
# include <pcap.h>
#endif
#ifdef HAVE_LIBWIRETAP
# include <wtap.h>
#endif
#include "Rig.h"

#ifdef HAVE_LIBFREEIMAGE
# include <FreeImage.h> /* to extract the library version */
#endif

#include <qwt_global.h> /* to extract the library version */

#ifdef USE_FAAD2_LIBRARY
# include <neaacdec.h>
#else
# include "../sourcedecoders/neaacdec_dll.h"
#endif

/* fftw 3.3.2 doesn't export the symbol fftw_version
 * for windows in libfftw3-3.def
 * You can add it regenerate the lib file and it's supposed to work,
 * but for now the version string is disabled for windows. */
#ifndef _WIN32
# ifdef HAVE_FFTW3_H
#  include <fftw3.h> /* to extract the library version */
# else
#  include <fftw.h> /* to extract the library version */
# endif
#endif


QString VersionString(QWidget* parent)
{
    QString strVersionText;
    strVersionText = "<center><b>" + parent->tr("Dream, Version ");
    strVersionText += QString("%1.%2%3")
        .arg(dream_version_major)
        .arg(dream_version_minor)
        .arg(dream_version_build);
    strVersionText += "</b><br> " + parent->tr("Open-Source Software Implementation of "
                                       "a DRM-Receiver") + "<br>";
    strVersionText += parent->tr("Under the GNU General Public License (GPL)") +
                      "</center>";
    return strVersionText;
#ifdef _MSC_VER
	parent; // warning C4100: 'parent' : unreferenced formal parameter !!!
#endif
}

/* Implementation *************************************************************/
/* About dialog ------------------------------------------------------------- */
CAboutDlg::CAboutDlg(QWidget* parent, const char* name, bool modal, Qt::WFlags f):
    CAboutDlgBase(parent, name, modal, f)
{
#ifdef HAVE_LIBSNDFILE
    char  sfversion [128] ;
    sf_command (NULL, SFC_GET_LIB_VERSION, sfversion, sizeof (sfversion)) ;
#endif
    QString strCredits = 
        "<p>" /* General description of Dream software */
        "<big><b>Dream</b> " + tr("is a software implementation of a Digital "
                                  "Radio Mondiale (DRM) receiver. With Dream, DRM broadcasts can be received "
                                  "with a modified analog receiver (SW, MW, LW) and a PC with a sound card.")
        + "</big></p><br>"
        "<p><font face=\"" FONT_COURIER "\">" /* GPL header text */
        "This program is free software; you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation; either version 2 of the License, or "
        "(at your option) any later version.<br>This program is distributed in "
        "the hope that it will be useful, but WITHOUT ANY WARRANTY; without "
        "even the implied warranty of MERCHANTABILITY or FITNESS FOR A "
        "PARTICULAR PURPOSE. See the GNU General Public License for more "
        "details.<br>You should have received a copy of the GNU General Public "
        "License along with his program; if not, write to the Free Software "
        "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 "
        "USA"
        "</font></p><br>" /* Our warning text */
        "<p><font color=\"#ff0000\" face=\"" FONT_COURIER "\">" +
        tr("Although this software is going to be "
           "distributed as free software under the terms of the GPL this does not "
           "mean that its use is free of rights of others. The use may infringe "
           "third party IP and thus may not be legal in some countries.") +
        "</font></p><br>"
        "<p>" /* Libraries used by this compilation of Dream */
        "<b>" + tr("This compilation of Dream uses the following libraries:") +
        "</b></p>"
        "<ul>"
#ifndef _WIN32
        "<li><b>FFTW</b> (" + QString(fftw_version) + ") <i>http://www.fftw.org</i></li>"
#else
        "<li><b>FFTW</b> <i>http://www.fftw.org</i></li>"
#endif
#if USE_FAAD2_LIBRARY
        "<li><b>FAAD2</b> (" + QString(FAAD2_VERSION) + ") <i>AAC/HE-AAC/HE-AACv2/DRM decoder "
        "(c) Ahead Software, www.nero.com (http://faac.sf.net)</i></li>"
#endif
#ifdef USE_FAAC_LIBRARY
        "<li><b>FAAC</b> <i>http://faac.sourceforge.net</i></li>"
#endif
#ifdef USE_QT_GUI /* QWT */
        "<li><b>Qt</b> (" + QString(QT_VERSION_STR) + ") <i>http://qt-project.org</i></li>"
        "<li><b>QWT</b> (" + QString(QWT_VERSION_STR) + ") <i>Dream is based in part on the work of the Qwt "
        "project (http://qwt.sf.net).</i></li>"
#endif
#ifdef HAVE_LIBHAMLIB
        "<li><b>Hamlib</b> (" + QString(hamlib_version) + ") <i>http://hamlib.sourceforge.net</i></li>"
#endif
        "<li><b>FhG IIS Journaline Decoder</b> <i>Features NewsService "
        "Journaline(R) decoder technology by Fraunhofer IIS, Erlangen, "
        "Germany. For more information visit http://www.iis.fraunhofer.de/en/bf/db/pro.html</i></li>"
#ifdef HAVE_LIBFREEIMAGE
        "<li><b>FreeImage</b> (" + QString(FreeImage_GetVersion()) + ") <i>This software uses the FreeImage open source "
        "image library. See http://freeimage.sourceforge.net for details. "
        "FreeImage is used under the GNU GPL.</i></li>"
#endif
#ifdef HAVE_LIBPCAP
        "<li><b>LIBPCAP</b> (" + QString(pcap_lib_version()) + ") <i>http://www.tcpdump.org/ "
        "This product includes software developed by the Computer Systems "
        "Engineering Group at Lawrence Berkeley Laboratory.</i></li>"
#endif
#ifdef HAVE_LIBSNDFILE
        "<li><b>LIBSNDFILE</b> (" + QString(sfversion) + ") <i>http://www.mega-nerd.com/libsndfile</i></li>"
#endif
#ifdef USE_OSS
        "<li><b>OSS</b> (" + QString("Open Sound System version %1").arg(SOUND_VERSION, 0, 16) + ")</li>"
#endif
#ifdef USE_ALSA
        "<li><b>ALSA</b> (" + QString(SND_LIB_VERSION_STR) + ") <i>http://www.alsa-project.org</i></li>"
#endif
#ifdef USE_PORTAUDIO
        "<li><b>PortAudio</b> (" + QString(Pa_GetVersionText()) + ") <i>http://www.portaudio.com</i></li>"
#endif
#ifdef USE_PULSEAUDIO
        "<li><b>PulseAudio</b> (" + QString(pa_get_headers_version()) + ") <i>http://www.pulseaudio.org</i></li>"
#endif
#ifdef USE_JACK
        "<li><b>libjack</b> (The Jack Audio Connection Kit) <i>http://www.jackaudio.org</i></li>"
#endif
        "</ul><br><br><hr/><br><br>"
        "<center><b>HISTORY</b></center><br>"
        "The Dream software development was started at <i>Darmstadt University "
        "of Technology</i> at the Institute of Communication Technology by <i>Volker "
        "Fischer</i> and <i>Alexander Kurpiers</i> in 2001-2005. "
        "The core digital signal processing and most of the GUI were the "
        "result of this development.<br>In 2005, <i>Andrew Murphy</i> of the <i>British "
        "Broadcasting Corporation</i> added code for an "
        "AMSS demodulator. <i>Oliver Haffenden</i> and <i>Julian Cable</i> (also <i>BBC</i>) rewrote "
        "the MDI interface and added RSCI support."
        " Many other GUI improvements were implemented by <i>Andrea Russo and David Flamand</i>."
        "<br>Right now the code is mainly maintained by <i>David Flamand and Julian Cable</i>."
        " Quality Assurance and user testing is provided by <i>Simone St&ouml;ppler.</i>"
        "<br><br><br>"
        "<center><b>CREDITS</b></center><br>"
        +tr("We want to thank all the contributors to the Dream software (in "
            "alphabetical order):")+"<br><br>"
        "<b>Developers</b>"
        "<center>"
        "<p>Bakker, Menno</p>"
        "<p>Cable, Julian</p>"
        "<p>Cesco</p>"
        "<p>Diniz, Rafael</p>"
        "<p>Fillod, Stephane</p>"
        "<p>Fischer, Volker</p>"
        "<p>Fine, Mark J.</p>"
        "<p>Flamand, David</p>"
        "<p>Haffenden, Oliver</p>"
        "<p>Kurpiers, Alexander</p>"
        "<p>Manninen, Tomi</p>"
        "<p>Moore, Josh</p>"
        "<p>Murphy, Andrew</p>"
        "<p>Pascutto, Gian C.</p>"
        "<p>Peca, Marek</p>"
        "<p>Richard, Doyle</p>"
        "<p>Russo, Andrea</p>"
        "<p>Turnbull, Robert</p>"
        "</center>"
        "<p>If your name should be in the above list and its missing, please let us know.</p>"
        "<br><b>Parts of Dream are based on code by</b>"
        "<center>"
        "<p>Karn, Phil (www.ka9q.net)</p>"
        "<p>Ptolemy Project (http://ptolemy.eecs.berkeley.edu)</p>"
        "<p>Tavernini, Lucio (http://tavernini.com/home.html)</p>"
        "<p>The Math Forum (http://mathforum.org)</p>"
        "<p>The Synthesis ToolKit in C++ (STK) "
        "(http://ccrma.stanford.edu/software/stk)</p>"
        "</center>"
        "<br><b>Supporters</b>"
        "<center>"
        "<p>Amorim, Roberto Jos&eacute; de</p>"
        "<p>Kainka, Burkhard</p>"
        "<p>Keil, Jens</p>"
        "<p>Kilian, Gerd</p>"
        "<p>Kn&uuml;tter, Carsten</p>"
        "<p>Ramisch, Roland</p>"
        "<p>Schall, Norbert</p>"
        "<p>Schill, Dietmar</p>"
        "<p>Schneider, Klaus</p>"
        "<p>St&ouml;ppler, Simone</p>"
        "<p>Varlamov, Oleg</p>"
        "<p>Wade, Graham</p>"
        "</center><br>";

    /* Add link to text */
    Linkify(strCredits);

    /* Set the text for the about dialog html text control */
    TextViewCredits->setText(strCredits);

    /* Set version number in about dialog */
    TextLabelVersion->setText(VersionString(this));

    /* Set author names in about dialog */
    TextLabelAuthorNames->setText("Volker Fischer, Alexander Kurpiers, Andrea Russo\nJulian Cable, Andrew Murphy, Oliver Haffenden");

    /* Set copyright year in about dialog */
    TextLabelCopyright->setText("Copyright (C) 2001 - 2012");
}

/* Help Usage --------------------------------------------------------------- */
CHelpUsage::CHelpUsage(const char* usage, const char* argv0,
    QWidget* parent, const char* name, bool modal, Qt::WFlags f)
    : CAboutDlgBase(parent, name, modal, f)
{
#if QT_VERSION < 0x040000
    SetDialogCaption(this, tr("Dream Command Line Help"));
#else
    setWindowTitle(tr("Dream Command Line Help"));
#endif
    TextLabelVersion->setText(VersionString(this));
    TextLabelAuthorNames->setText("");
    TextLabelCopyright->setText(tr("Command line usage:"));
    QString text(tr(usage));
    text.replace(QRegExp("\\$EXECNAME"), QString::fromUtf8(argv0));
#if QT_VERSION < 0x040000
    TextViewCredits->setText(text);
#else
    TextViewCredits->setFontFamily(FONT_COURIER);
    TextViewCredits->setPlainText(text);
#endif
    show();
}

#if QT_VERSION < 0x040000
/* Help menu ---------------------------------------------------------------- */
CDreamHelpMenu::CDreamHelpMenu(QWidget* parent) : QPopupMenu(parent)
{
    /* Standard help menu consists of about and what's this help */
        insertItem(tr("What's &This"), this, SLOT(OnHelpWhatsThis()), Qt::SHIFT+Qt::Key_F1);
        insertSeparator();
        insertItem(tr("&About..."), parent, SLOT(OnHelpAbout()));
}

void CDreamHelpMenu::OnHelpWhatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

/* Sound card selection menu ------------------------------------------------ */
# undef DEVICE_STRING
# ifdef _WIN32
#  define DEVICE_STRING(s) s.c_str()
# else
#  define DEVICE_STRING(s) QString::fromUtf8(s.c_str())
# endif

CSoundCardSelMenu::CSoundCardSelMenu(
    CSelectionInterface* pNSIn, CSelectionInterface* pNSOut, QWidget* parent) :
    QPopupMenu(parent), pSoundInIF(pNSIn), pSoundOutIF(pNSOut)
{
    pSoundInMenu = new QPopupMenu(parent);
    CHECK_PTR(pSoundInMenu);
    pSoundOutMenu = new QPopupMenu(parent);
    CHECK_PTR(pSoundOutMenu);
    vector<string> vecDescriptions;
    int i, iDefaultInDev = -1, iDefaultOutDev = -1;

    /* Get sound in device names */
    string sDefaultInDev = pSoundInIF->GetDev();
    pSoundInIF->Enumerate(vecSoundInNames, vecDescriptions);
    iNumSoundInDev = vecSoundInNames.size();
    for (i = 0; i < iNumSoundInDev; i++)
    {
        if (vecSoundInNames[i] == sDefaultInDev)
            iDefaultInDev = i;
        QString name(DEVICE_STRING(vecSoundInNames[i]));
# if defined(_MSC_VER) && (_MSC_VER < 1400)
        if (name.find("blaster", 0, FALSE) >= 0)
            name += " (has problems on some platforms)";
# endif
        if (name == "") name = tr("[default]");
        pSoundInMenu->insertItem(name, this, SLOT(OnSoundInDevice(int)), 0, i);
    }

    /* Get sound out device names */
    string sDefaultOutDev = pSoundOutIF->GetDev();
    pSoundOutIF->Enumerate(vecSoundOutNames, vecDescriptions);
    iNumSoundOutDev = vecSoundOutNames.size();
    for (i = 0; i < iNumSoundOutDev; i++)
    {
        if (vecSoundOutNames[i] == sDefaultOutDev)
            iDefaultOutDev = i;
        QString name(DEVICE_STRING(vecSoundOutNames[i]));
        if (name == "") name = tr("[default]");
        pSoundOutMenu->insertItem(name, this, SLOT(OnSoundOutDevice(int)), 0, i);
    }

    if (iDefaultInDev >= 0)
        pSoundInMenu->setItemChecked(iDefaultInDev, TRUE);
    if (iDefaultOutDev >= 0)
        pSoundOutMenu->setItemChecked(iDefaultOutDev, TRUE);

    insertItem(tr("Sound &In"), pSoundInMenu);
    insertItem(tr("Sound &Out"), pSoundOutMenu);
}

void CSoundCardSelMenu::OnSoundInDevice(int id)
{
    if (id >= 0 && id < int(vecSoundInNames.size()))
        pSoundInIF->SetDev(vecSoundInNames[id]);
    for (int i = 0; i < iNumSoundInDev; i++)
        pSoundInMenu->setItemChecked(i, i == id);
}

void CSoundCardSelMenu::OnSoundOutDevice(int id)
{
    if (id >= 0 && id < int(vecSoundOutNames.size()))
        pSoundOutIF->SetDev(vecSoundOutNames[id]);
    for (int i = 0; i < iNumSoundOutDev; i++)
        pSoundOutMenu->setItemChecked(i, i == id);
}
# undef DEVICE_STRING
#endif

RemoteMenu::RemoteMenu(QWidget* parent, CRig& nrig)
#ifdef HAVE_LIBHAMLIB
    :rigmenus(),specials(),rig(nrig)
#endif
{
#ifndef HAVE_LIBHAMLIB
    (void)nrig;
#endif
#if QT_VERSION < 0x040000
    pRemoteMenu = new QPopupMenu(parent);
    pRemoteMenuOther = new QPopupMenu(parent);
#else
    pRemoteMenu = new QMenu(parent);
    pRemoteMenuOther = new QMenu(parent);
#endif
    CHECK_PTR(pRemoteMenu);

    CHECK_PTR(pRemoteMenuOther);

#ifdef HAVE_LIBHAMLIB

    map<rig_model_t,CHamlib::SDrRigCaps> rigs;

    rig.GetRigList(rigs);

    rigmenus.clear();
    specials.clear();
    /* Add menu entry "none" */
#if QT_VERSION < 0x040000
    pRemoteMenu->insertItem(tr("None"), this, SLOT(OnRemoteMenu(int)), 0, RIG_MODEL_NONE);
    pRemoteMenu->setItemChecked(RIG_MODEL_NONE, TRUE);
#else
    QAction* actionNoRig = pRemoteMenu->addAction(tr("None"), this, SLOT(OnRemoteMenu(int)));
    actionNoRig->setData(RIG_MODEL_NONE);
    actionNoRig->setChecked(true);
#endif
    specials.push_back(RIG_MODEL_NONE);

    rig_model_t currentRig = rig.GetHamlibModelID();

    /* Add menu entries */
    for (map<rig_model_t,CHamlib::SDrRigCaps>::iterator i=rigs.begin(); i!=rigs.end(); i++)
    {
        rig_model_t iModelID = i->first;
        CHamlib::SDrRigCaps& rig = i->second;

        Rigmenu m;
        int backend = RIG_BACKEND_NUM(iModelID);
        map<int, Rigmenu>::iterator k = rigmenus.find(backend);
        if(k == rigmenus.end())
        {
            m.mfr = rig.strManufacturer;
#if QT_VERSION < 0x040000
            m.pMenu = new QPopupMenu(pRemoteMenuOther);
#else
            m.pMenu = new QMenu(pRemoteMenuOther);
#endif
            CHECK_PTR(m.pMenu);
        }
        else
        {
            m = k->second;
        }

        // add all rigs to "other" menu - specials will appear twice but we only check the specials entry
        /* Set menu string. Should look like: [ID] Model (status) */
        QString strMenuText =
            "[" + QString().setNum(iModelID) + "] " + rig.strModelName.c_str()
            + " (" + rig_strstatus(rig.eRigStatus) + ")";
#if QT_VERSION < 0x040000
        m.pMenu->insertItem(strMenuText, this, SLOT(OnRemoteMenu(int)), 0, iModelID);
#else
	QAction* actionRig = m.pMenu->addAction(strMenuText, this, SLOT(OnRemoteMenu(int)));
	actionNoRig->setData(iModelID);
#endif
        rigmenus[backend] = m;

        if (rig.bIsSpecRig || (currentRig == iModelID))
        {
            /* Main rigs */
            /* Set menu string. Should look like: [ID] Manuf. Model */
            QString strMenuText =
                "[" + QString().setNum(iModelID) + "] " +
                rig.strManufacturer.c_str() + " " +
                rig.strModelName.c_str();

#if QT_VERSION < 0x040000
            pRemoteMenu->insertItem(strMenuText, this, SLOT(OnRemoteMenu(int)), 0, iModelID);
#else
	    actionRig = m.pMenu->addAction(strMenuText, this, SLOT(OnRemoteMenu(int)));
	    actionRig->setData(iModelID);
#endif

            /* Check for checking */
            if (currentRig == iModelID)
            {
#if QT_VERSION < 0x040000
                pRemoteMenu->setItemChecked(RIG_MODEL_NONE, FALSE);
                pRemoteMenu->setItemChecked(iModelID, TRUE);
#else
		actionNoRig->setChecked(false);
		actionRig->setChecked(true);
#endif
            }

            specials.push_back(iModelID);
        }
    }

    for (map<int,Rigmenu>::iterator j=rigmenus.begin(); j!=rigmenus.end(); j++)
    {
#if QT_VERSION < 0x040000
        pRemoteMenuOther->insertItem(j->second.mfr.c_str(), j->second.pMenu);
#else
	//pRemoteMenuOther->addAction(j->second.mfr.c_str(), j->second.pMenu);
#endif
    }

    /* Add "other" menu */
#if QT_VERSION < 0x040000
    pRemoteMenu->insertItem(tr("Other"), pRemoteMenuOther, OTHER_MENU_ID);
#else
    //pRemoteMenu->addAction(tr("Other"), pRemoteMenuOther);
#endif

    /* Separator */
#if QT_VERSION < 0x040000
    pRemoteMenu->insertSeparator();
#else
    pRemoteMenu->addSeparator();
#endif

    /* COM port selection --------------------------------------------------- */
    /* Toggle action for com port selection menu entries */
    QActionGroup* agCOMPortSel = new QActionGroup(parent);
    agCOMPortSel->setExclusive(true);
    map<string,string> ports;
    rig.GetPortList(ports);
    string strPort = rig.GetComPort();
    for(map<string,string>::iterator p=ports.begin(); p!=ports.end(); p++)
    {
        QString text = p->second.c_str();
        QString menuText = p->first.c_str();
        QAction* pacMenu = new QAction(agCOMPortSel);
#if QT_VERSION < 0x040000
        pacMenu->setText(text);
        pacMenu->setMenuText(menuText);
        pacMenu->setToggleAction(true);
        if(strPort == p->second)
            pacMenu->setOn(TRUE);
#else
        pacMenu->setData(text);
        pacMenu->setText(menuText);
        pacMenu->setCheckable(true);
        if(strPort == p->second)
            pacMenu->setChecked(true);
#endif
    }

    /* Action group */
    connect(agCOMPortSel, SIGNAL(selected(QAction*)), this, SLOT(OnComPortMenu(QAction*)));
#if QT_VERSION < 0x040000
    agCOMPortSel->addTo(pRemoteMenu);
#else
    pRemoteMenu->addActions(agCOMPortSel->actions());
#endif
    /* Separator */
#if QT_VERSION < 0x040000
    pRemoteMenu->insertSeparator();
#else
    pRemoteMenu->addSeparator();
#endif

    /* Enable special settings for rigs */
#if QT_VERSION < 0x040000
    const int iModRigMenuID = pRemoteMenu->insertItem(
	tr("With DRM Modification"), this, SLOT(OnModRigMenu(int)), 0);
    /* Set check */
    pRemoteMenu->setItemChecked(iModRigMenuID, rig.GetEnableModRigSettings());
#else
    QAction* modRigAction = pRemoteMenu->addAction(
	tr("With DRM Modification"), this, SLOT(OnModRigMenu(int)));
    modRigAction->setChecked(rig.GetEnableModRigSettings());
#endif

#endif
}

void RemoteMenu::OnModRigMenu(int iID)
{
#if QT_VERSION < 0x040000
# ifdef HAVE_LIBHAMLIB
    if (pRemoteMenu->isItemChecked(iID))
    {
        pRemoteMenu->setItemChecked(iID, FALSE);
        rig.SetEnableModRigSettings(FALSE);
    }
    else
    {
        pRemoteMenu->setItemChecked(iID, TRUE);
        rig.SetEnableModRigSettings(TRUE);
    }
# else
    (void)iID;
# endif
#else
    // TODO QT4
    (void)iID;
#endif
}

void RemoteMenu::OnRemoteMenu(int iID)
{
#if QT_VERSION < 0x040000
# ifdef HAVE_LIBHAMLIB
    // if an "others" rig was selected add it to the specials list
    for (map<int,Rigmenu>::iterator i=rigmenus.begin(); i!=rigmenus.end(); i++)
    {
        QPopupMenu* pMenu = i->second.pMenu;
        for(size_t j=0; j<pMenu->count(); j++)
        {
            int mID = pMenu->idAt(j);
            if(mID==iID)
            {
                // if necessary add it to the specials menus
                if(pRemoteMenu->indexOf(mID)==-1)
                {
                    map<rig_model_t,CHamlib::SDrRigCaps> rigs;
                    rig.GetRigList(rigs);
                    QString strMenuText =
                        "[" + QString().setNum(mID) + "] " +
                        i->second.mfr.c_str() + " " +
                        rigs[mID].strModelName.c_str();
                    int pos = pRemoteMenu->indexOf(OTHER_MENU_ID);
                    pRemoteMenu->insertItem(strMenuText, this, SLOT(OnRemoteMenu(int)), 0, mID, pos);
                    specials.push_back(mID);
                }
            }
        }
    }


    /* Take care of check */
    // do this after others menu in case we added something
    for(size_t j=0; j<specials.size(); j++)
    {
        pRemoteMenu->setItemChecked(specials[j], specials[j]==iID);
    }
    // disable com port - if rig has changed
    //if(iID != Hamlib.GetHamlibModelID())
    //	agComPortSel->setChecked(false);

    /* Set ID */
    rig.SetHamlibModelID(iID);
    double r;
    if(rig.GetSMeter(r) == CHamlib::SS_VALID)
    {
        emit SMeterAvailable();
    }
# else
	(void)iID;
# endif
#else
    // TODO QT4
    (void)iID;
#endif
}

void RemoteMenu::OnComPortMenu(QAction* action)
{
#ifdef HAVE_LIBHAMLIB
# if QT_VERSION < 0x040000
    rig.SetComPort(action->text().utf8().data());
# else
    rig.SetComPort(action->text().toUtf8().data());
# endif
#else
    (void)action;
#endif
}

/* Ensure that the given filename is secure */
QString VerifyFilename(QString filename)
{
    filename.replace(QRegExp("/"), "_"); /* replace unix-like path separator with '_' */
#ifdef _WIN32
    filename.replace(QRegExp("\\\\"), "_"); /* replace windows path separator with '_' */
    filename.replace(QRegExp(":"), "_"); /* replace ':' with '_' */
#endif
    return filename;
}

/* Ensure that the given html path is secure */
QString VerifyHtmlPath(QString path)
{
    if (path == "..")
        return "_";
#ifdef _WIN32
    path.replace(QRegExp("\\\\"), "_"); /* replace windows path separator with '_' */
    path.replace(QRegExp(":"), "_"); /* replace ':' with '_' */
#endif
    path.replace(QRegExp("^\\.\\./"), "_/"); /* replace '../' at the beginning with '_/' */
    path.replace(QRegExp("/\\.\\.$"), "/_"); /* replace '/..' at the end with '/_' */
    path.replace(QRegExp("/\\.\\./"), "/_/"); /* replace '/../' with '/_/' */
    return path;
}

#if QT_VERSION >= 0x040000
/* Accept both absolute and relative url, but only return the path component.
   Invalid characters in path are percent-encoded (e.g. space = %20) */
QString UrlEncodePath(QString url)
{
    /* Get path component */
    QString path(QUrl(url, QUrl::TolerantMode).path());
    /* Prepend with '/' if none present */
    if (path.size() == 0 || path.at(0) != QChar('/'))
        path.insert(0, QChar('/'));
    /* Replace multiple '/' by single '/' */
    path.replace(QRegExp("/{1,}"), "/");
    /* Replace all occurrence of '/./' with '/' */
    while (path.indexOf("/./") != -1)
        path.replace(QRegExp("/\\./"), "/");
    /* The Actual percent encoding */
# if QT_VERSION < 0x040400
    path = QString(QUrl(path, QUrl::TolerantMode).toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority));
# else
    path = QString(QUrl(path, QUrl::TolerantMode).encodedPath());
# endif
    return path;
}

/* Determine if the given url is a directory */
bool IsUrlDirectory(QString url)
{
    return url.endsWith(QChar('/'));
}
#endif

void InitSMeter(QWidget* parent, QwtThermo* sMeter)
{
    sMeter->setRange(S_METER_THERMO_MIN, S_METER_THERMO_MAX);
#if QT_VERSION < 0x040000
    sMeter->setOrientation(QwtThermo::Horizontal, QwtThermo::Top);
//#else
//    sMeter->setOrientation(Qt::Horizontal, QwtThermo::TopScale); // Set via ui file
#endif
    sMeter->setAlarmLevel(S_METER_THERMO_ALARM);
    sMeter->setAlarmLevel(-12.5);
    sMeter->setScale(S_METER_THERMO_MIN, S_METER_THERMO_MAX, 10.0);
    sMeter->setAlarmEnabled(TRUE);
    sMeter->setValue(S_METER_THERMO_MIN);
#if QWT_VERSION < 0x060000
    (void)parent;
    sMeter->setAlarmColor(QColor(255, 0, 0));
    sMeter->setFillColor(QColor(0, 190, 0));
#else
    QPalette newPalette = parent->palette();
    newPalette.setColor(QPalette::Base, newPalette.color(QPalette::Window));
    newPalette.setColor(QPalette::ButtonText, QColor(0, 190, 0));
    newPalette.setColor(QPalette::Highlight,  QColor(255, 0, 0));
    sMeter->setPalette(newPalette);
#endif
}

/* Convert all www. or http:// or email to real
   clickable link, for use with QLabel and such.
   Code by David Flamand */
void Linkify(QString& text)
{
#if QT_VERSION >= 0x040000
    int i, j, posWWW=-2, posHTTP=-2, posMAIL=-2, posBegin, posEnd, size;
    size = text.size();
    for (i = 0; i < size;)
    {
        if (posWWW != -1 && posWWW < i)
            posWWW  = text.indexOf("www.", i, Qt::CaseInsensitive);
        if (posHTTP != -1 && posHTTP < i)
            posHTTP = text.indexOf("http://", i, Qt::CaseInsensitive);
#if QT_VERSION >= 0x040500
        if (posMAIL != -1 && posMAIL < i)
            posMAIL = text.indexOf(QRegExp("\\b[0-9a-z._-]+@[0-9a-z.-]+\\.[a-z]{2,4}\\b", Qt::CaseInsensitive), i);
#else
        posMAIL = -1;
#endif
        if (posMAIL>=0 && (posMAIL<=posWWW || posWWW<0) && (posMAIL<posHTTP || posHTTP<0))
            posBegin = posMAIL;
        else if (posWWW>=0 && (posWWW<posHTTP || posHTTP<0))
            posBegin = posWWW;
        else
            posBegin = posHTTP;
        if (posBegin >= 0)
        {
            posEnd = size;
            for (j = posBegin; j < size; j++)
            {
                int chr = text[j].unicode();
                if (!((chr=='@' && posBegin==posMAIL) ||
                      chr=='.' || chr=='/' ||
                      chr=='~' || chr=='-' ||
                      chr=='_' || chr==':' ||
                     (chr>='a' && chr<='z') ||
                     (chr>='A' && chr<='Z') ||
                     (chr>='0' && chr<='9')))
                {
                    posEnd = j;
                    break;
                }
            }
            const int rawLinkSize = posEnd-posBegin;
            QStringRef rawLink(&text, posBegin, rawLinkSize);
            QString newLink;
            if (posBegin == posMAIL)
                newLink = "<a href=\"mailto:%1\">%1</a>";
            else if (posBegin == posWWW)
                newLink = "<a href=\"http://%1\">%1</a>";
            else /* posBegin == posHTTP */
                newLink = "<a href=\"%1\">%1</a>";
            newLink = newLink.arg(rawLink.toString());
            const int newLinkSize = newLink.size();
            text.replace(posBegin, rawLinkSize, newLink);
            const int diff = newLinkSize - rawLinkSize;
            i = posEnd + diff;
            size += diff;
            if (posWWW >= 0)
                posWWW += diff;
            if (posHTTP >= 0)
                posHTTP += diff;
            if (posMAIL >= 0)
                posMAIL += diff;
        }
        else
            break;
    }
#else
	(void)text;
#endif
}

void CreateDirectories(const QString& strFilename)
{
    /*
    	This function is for creating a complete directory structure to a given
    	file name. If there is a pathname like this:
    	/html/files/images/myimage.gif
    	this function create all the folders into MOTCache:
    	/html
    	/html/files
    	/html/files/images
    	QFileInfo only creates a file if the pathname is valid. If not all folders
    	are created, QFileInfo will not save the file. There was no QT function
    	or a hint the QT mailing list found in which does the job of this function.
    */
    for (int i = 0;; i++)
    {
#if QT_VERSION < 0x040000
        i = strFilename.find(QChar('/'), i);
#else
        i = strFilename.indexOf(QChar('/'), i);
#endif
        if (i < 0)
            break;
        const QString strDirName = strFilename.left(i);
        if (!strDirName.isEmpty() && !QFileInfo(strDirName).exists())
            QDir().mkdir(strDirName);
    }
}

void RestartReceiver(CDRMReceiver *DRMReceiver)
{
#if QT_VERSION >= 0x040000
    if (DRMReceiver != NULL)
    {
        QMutex sleep;
        CParameter& Parameters = *DRMReceiver->GetParameters();
        DRMReceiver->Restart();
        while (Parameters.eRunState == CParameter::RESTART)
        {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            sleep.lock(); /* TODO find a better way to sleep on Qt */
            sleep.tryLock(10);
            sleep.unlock();
        }
    }
#else
	(void)DRMReceiver;
#endif
}

