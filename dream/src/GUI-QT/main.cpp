/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod
 *
 * Description:
 *
 * 11/10/2004 Stephane Fillod
 *	- QT translation
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

#ifdef _WIN32
# ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
# endif
# define _WIN32_WINNT 0x0400
# include <windows.h>
#endif

#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "../DrmTransmitter.h"
#include "../DrmSimulation.h"
#include "../util/Settings.h"
#include "Rig.h"

#include <iostream>

#include <qthread.h>
#ifdef USE_QT_GUI
# include <qapplication.h>
# include <qmessagebox.h>
# include "fdrmdialog.h"
# include "TransmDlg.h"
# include "DialogUtil.h"
#endif
#if QT_VERSION >= 0x040000
# include <QCoreApplication>
# include <QTranslator>
#endif

class CRx: public QThread
{
public:
	CRx(CDRMReceiver& nRx
#ifdef _WIN32
	, bool bPriorityEnabled
#endif
	):rx(nRx)
#ifdef _WIN32
	, bPriorityEnabled(bPriorityEnabled)
#endif
	{}
	void run();
private:
	CDRMReceiver& rx;
	bool bPriorityEnabled;
};

void
CRx::run()
{
#ifdef _WIN32
    if (bPriorityEnabled)
    {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    }
#endif
    qDebug("Working thread started");
    try
    {
        /* Call receiver main routine */
        rx.Start();
    }
    catch (CGenErr GenErr)
    {
        ErrorMessage(GenErr.strError);
    }
    catch (string strError)
    {
        ErrorMessage(strError);
    }
    qDebug("Working thread complete");
}

#ifdef USE_QT_GUI
/******************************************************************************\
* Using GUI with QT                                                            *
\******************************************************************************/
int
main(int argc, char **argv)
{
	/* create app before running Settings.Load to consume platform/QT parameters */
	QApplication app(argc, argv);

#if defined(__APPLE__)
	/* find plugins on MacOs when deployed in a bundle */
# if QT_VERSION>0x040000
	app.addLibraryPath(app.applicationDirPath()+"../PlugIns");
# else
	app.setLibraryPaths(app.applicationDirPath()+"../PlugIns");
# endif
#endif

	/* Load and install multi-language support (if available) */
	QTranslator translator(0);
	if (translator.load("dreamtr"))
		app.installTranslator(&translator);

	CDRMSimulation DRMSimulation;

	/* Call simulation script. If simulation is activated, application is
	   automatically exit in that routine. If in the script no simulation is
	   activated, this function will immediately return */
	DRMSimulation.SimScript();

	CSettings Settings;
	/* Parse arguments and load settings from init-file */
	Settings.Load(argc, argv);

	try
	{

#ifdef _WIN32
		/* works for both transmit and receive. GUI is low, working is normal.
		 * the working thread does not need to know what the setting is.
		 */
		bool bPriorityEnabled = Settings.Get("GUI", "processpriority", bool(TRUE));
		if (bPriorityEnabled)
		{
			/* Set priority class for this application */
			SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

			/* Normal priority for GUI thread */
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		}
		Settings.Put("GUI", "processpriority", bPriorityEnabled);
#endif

		string mode = Settings.Get("command", "mode", string());
		if (mode == "receive")
		{
			CDRMReceiver DRMReceiver;

			/* First, initialize the working thread. This should be done in an extra
			   routine since we cannot 100% assume that the working thread is
			   ready before the GUI thread */

			CRig rig(DRMReceiver.GetParameters());
			rig.LoadSettings(Settings); // must be before DRMReceiver for G313
			DRMReceiver.LoadSettings(Settings);

#ifdef HAVE_LIBHAMLIB
			DRMReceiver.SetRig(&rig);

			if(DRMReceiver.GetDownstreamRSCIOutEnabled())
			{
				rig.subscribe();
			}
#endif
			FDRMDialog MainDlg(DRMReceiver, Settings, rig
#if QT_VERSION < 0x040000
				, NULL, NULL, FALSE, Qt::WStyle_MinMax
#endif
				);

			/* Start working thread */
#ifdef _WIN32
			CRx rx(DRMReceiver, bPriorityEnabled);
#else
			CRx rx(DRMReceiver);
#endif
			rx.start();

			/* Set main window */
#if QT_VERSION < 0x040000
			app.setMainWidget(&MainDlg);
#endif

			app.exec();

#ifdef HAVE_LIBHAMLIB
			if(DRMReceiver.GetDownstreamRSCIOutEnabled())
			{
				rig.unsubscribe();
			}
			rig.SaveSettings(Settings);
#endif
			DRMReceiver.SaveSettings(Settings);
		}
		else if(mode == "transmit")
		{
			TransmDialog MainDlg(Settings
#if QT_VERSION < 0x040000
				, NULL, NULL, FALSE, Qt::WStyle_MinMax
#endif
				);

#if QT_VERSION < 0x040000
			/* Set main window */
			app.setMainWidget(&MainDlg);
#endif

			/* Show dialog */
			MainDlg.show();
			app.exec();
		}
		else
		{
			CHelpUsage HelpUsage(Settings.UsageArguments(), argv[0]
#if QT_VERSION < 0x040000
			, NULL, NULL, FALSE, Qt::WStyle_MinMax
#endif
			);
#if QT_VERSION < 0x040000
			app.setMainWidget(&HelpUsage);
#endif
			app.exec();
			exit(0);
		}
	}

	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}
	catch(string strError)
	{
		ErrorMessage(strError);
	}
	catch(char *Error)
	{
		ErrorMessage(Error);
	}

	/* Save settings to init-file */
	Settings.Save();

	return 0;
}

/* Implementation of global functions *****************************************/

void
ErrorMessage(string strErrorString)
{
	/* Workaround for the QT problem */
	string strError = "The following error occured:\n";
	strError += strErrorString.c_str();
	strError += "\n\nThe application will exit now.";

#ifdef _WIN32
	MessageBoxA(NULL, strError.c_str(), "Dream",
			   MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);
#else
	perror(strError.c_str());
#endif

/*
// Does not work correctly. If it is called by a different thread, the
// application hangs! FIXME
	QMessageBox::critical(0, "Dream",
		QString("The following error occured:<br><b>") +
		QString(strErrorString.c_str()) +
		"</b><br><br>The application will exit now.");
*/
	exit(1);
}
#else /* USE_QT_GUI */
/******************************************************************************\
* No GUI                                                                       *
\******************************************************************************/

int
main(int argc, char **argv)
{
	try
	{
		CSettings Settings;
		Settings.Load(argc, argv);

		string mode = Settings.Get("command", "mode", string());
		if (mode == "receive")
		{
			CDRMSimulation DRMSimulation;
			CDRMReceiver DRMReceiver;

			DRMSimulation.SimScript();
			DRMReceiver.LoadSettings(Settings);

#if QT_VERSION >= 0x040000
			QCoreApplication a(argc, argv);
			/* Start working thread */
			CRx rx(DRMReceiver);
			rx.start();
			return a.exec();
#else
			DRMReceiver.Start();
#endif

		}
		else if(mode == "transmit")
		{
			CDRMTransmitter DRMTransmitter;
			DRMTransmitter.LoadSettings(Settings);
			DRMTransmitter.Start();
		}
		else
		{
			string usage(Settings.UsageArguments());
			for (;;)
			{
				size_t pos = usage.find("$EXECNAME");
				if (pos == string::npos) break;
				usage.replace(pos, sizeof("$EXECNAME")-1, argv[0]);
			}
			cerr << usage << endl << endl;
			exit(0);
		}
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}

	return 0;
}

void
ErrorMessage(string strErrorString)
{
	perror(strErrorString.c_str());
}
#endif /* USE_QT_GUI */

void
DebugError(const char *pchErDescr, const char *pchPar1Descr,
		   const double dPar1, const char *pchPar2Descr, const double dPar2)
{
	FILE *pFile = fopen("test/DebugError.dat", "a");
	fprintf(pFile, "%s", pchErDescr);
	fprintf(pFile, " ### ");
	fprintf(pFile, "%s", pchPar1Descr);
	fprintf(pFile, ": ");
	fprintf(pFile, "%e ### ", dPar1);
	fprintf(pFile, "%s", pchPar2Descr);
	fprintf(pFile, ": ");
	fprintf(pFile, "%e\n", dPar2);
	fclose(pFile);
	fprintf(stderr, "\nDebug error! For more information see test/DebugError.dat\n");
	exit(1);
}
