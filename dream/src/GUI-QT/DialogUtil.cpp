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

#include <QMenuBar>
#include <QLabel>
#include <QAction>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QWhatsThis>
#ifdef _WIN32
# include <winsock2.h>
#endif
#include "../Version.h"
#include "../util-QT/Util.h"
#include "DialogUtil.h"
#ifdef HAVE_LIBHAMLIB
# include "../util-QT/Rig.h"
#endif

/* to extract the library version */
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
#ifdef HAVE_LIBFREEIMAGE
# include <FreeImage.h>
#endif
#ifdef QT_GUI_LIB
# include <qwt_global.h>
#endif
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
#  include <fftw3.h> 
# else
#  include <fftw.h>
# endif
#endif


QString VersionString(QWidget* parent)
{
    QString strVersionText;
    strVersionText =
        "<center><b>" + parent->tr("Dream, Version ");
    strVersionText += QString("%1.%2%3")
        .arg(dream_version_major)
        .arg(dream_version_minor)
        .arg(dream_version_build);
    strVersionText +=
        "</b><br> " + parent->tr("Open-Source Software Implementation of "
                                 "a DRM-Receiver") +
        "<br>";
    strVersionText += parent->tr("Under the GNU General Public License (GPL)") +
        "</center>";
    return strVersionText;
#ifdef _MSC_VER /* MSVC 2008 */
    parent; // warning C4100: 'parent' : unreferenced formal parameter
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
#ifdef QT_CORE_LIB
        "<li><b>Qt</b> (" + QString(QT_VERSION_STR) + ") <i>http://qt-project.org</i></li>"
#endif
#ifdef QT_GUI_LIB
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
        "<center><b>"+tr("CREDITS")+"</b></center><br>"
        +tr("We want to thank all the contributors to the Dream software (in "
            "alphabetical order):")+"<br><br>"
        "<b>"+tr("Developers")+"</b>"
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
        "<p>"+tr("If your name should be in the above list and its missing, please let us know.")+"</p>"
        "<br><b>"+tr("Parts of Dream are based on code by")+"</b>"
        "<center>"
        "<p>Karn, Phil (www.ka9q.net)</p>"
        "<p>Ptolemy Project (http://ptolemy.eecs.berkeley.edu)</p>"
        "<p>Tavernini, Lucio (http://tavernini.com/home.html)</p>"
        "<p>The Math Forum (http://mathforum.org)</p>"
        "<p>The Synthesis ToolKit in C++ (STK) "
        "(http://ccrma.stanford.edu/software/stk)</p>"
        "</center>"
        "<br><b>"+tr("Supporters")+"</b>"
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
    setWindowTitle(tr("Dream Command Line Help"));
    TextLabelVersion->setText(VersionString(this));
    TextLabelAuthorNames->setText("");
    TextLabelCopyright->setText(tr("Command line usage:"));
    QString text(tr(usage));
    text.replace(QRegExp("\\$EXECNAME"), QString::fromUtf8(argv0));
    TextViewCredits->setFontFamily(FONT_COURIER);
    TextViewCredits->setPlainText(text);
    show();
}

RemoteMenu::RemoteMenu(QWidget* parent, CRig& nrig)
#ifdef HAVE_LIBHAMLIB
    :rigmenus(),specials(),rig(nrig)
#endif
{
#ifndef HAVE_LIBHAMLIB
    (void)nrig;
#endif
    pRemoteMenu = new QMenu(parent);
    pRemoteMenuOther = new QMenu(parent);
    Q_CHECK_PTR(pRemoteMenu);

    Q_CHECK_PTR(pRemoteMenuOther);

#ifdef HAVE_LIBHAMLIB

    map<rig_model_t,CHamlib::SDrRigCaps> rigs;

    rig.GetRigList(rigs);

    rigmenus.clear();
    specials.clear();
    /* Add menu entry "none" */
    QAction* actionNoRig = pRemoteMenu->addAction(tr("None"), this, SLOT(OnRemoteMenu(int)));
    actionNoRig->setData(RIG_MODEL_NONE);
    actionNoRig->setChecked(true);
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
            m.pMenu = new QMenu(pRemoteMenuOther);
            Q_CHECK_PTR(m.pMenu);
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
	QAction* actionRig = m.pMenu->addAction(strMenuText, this, SLOT(OnRemoteMenu(int)));
	actionNoRig->setData(iModelID);
        rigmenus[backend] = m;

        if (rig.bIsSpecRig || (currentRig == iModelID))
        {
            /* Main rigs */
            /* Set menu string. Should look like: [ID] Manuf. Model */
            QString strMenuText =
                "[" + QString().setNum(iModelID) + "] " +
                rig.strManufacturer.c_str() + " " +
                rig.strModelName.c_str();

	    actionRig = m.pMenu->addAction(strMenuText, this, SLOT(OnRemoteMenu(int)));
	    actionRig->setData(iModelID);

            /* Check for checking */
            if (currentRig == iModelID)
            {
		actionNoRig->setChecked(false);
		actionRig->setChecked(true);
            }

            specials.push_back(iModelID);
        }
    }

    for (map<int,Rigmenu>::iterator j=rigmenus.begin(); j!=rigmenus.end(); j++)
    {
	//pRemoteMenuOther->addAction(j->second.mfr.c_str(), j->second.pMenu);
    }

    /* Add "other" menu */
    //pRemoteMenu->addAction(tr("Other"), pRemoteMenuOther);

    /* Separator */
    pRemoteMenu->addSeparator();

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
        pacMenu->setData(text);
        pacMenu->setText(menuText);
        pacMenu->setCheckable(true);
        if(strPort == p->second)
            pacMenu->setChecked(true);
    }

    /* Action group */
    connect(agCOMPortSel, SIGNAL(selected(QAction*)), this, SLOT(OnComPortMenu(QAction*)));
    pRemoteMenu->addActions(agCOMPortSel->actions());
    /* Separator */
    pRemoteMenu->addSeparator();

    /* Enable special settings for rigs */
    QAction* modRigAction = pRemoteMenu->addAction(
	tr("With DRM Modification"), this, SLOT(OnModRigMenu(int)));
    modRigAction->setChecked(rig.GetEnableModRigSettings());

#endif
}

void RemoteMenu::OnModRigMenu(int iID)
{
    // TODO QT4
    (void)iID;
}

void RemoteMenu::OnRemoteMenu(int iID)
{
    // TODO QT4
    (void)iID;
}

void RemoteMenu::OnComPortMenu(QAction* action)
{
#ifdef HAVE_LIBHAMLIB
    rig.SetComPort(action->text().toUtf8().constData());
#else
    (void)action;
#endif
}

void InitSMeter(QWidget* parent, QwtThermo* sMeter)
{
    sMeter->setRange(S_METER_THERMO_MIN, S_METER_THERMO_MAX);
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
