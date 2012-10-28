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
#endif
#if QT_VERSION >= 0x040000
# include <QCoreApplication>
# include <QTranslator>
#endif

class CRx: public QThread
{
public:
	CRx(CDRMReceiver& nRx):rx(nRx){}
	void run();
private:
	CDRMReceiver& rx;
};

void
CRx::run()
{
#ifdef _WIN32
    /* it doesn't matter what the GUI does, we want to be normal! */
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
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

int isTransmitter(const char *argv0)
{
#ifdef EXECUTABLE_NAME
	/* Give the possibility to launch directly dream transmitter
	   with a symbolic link to the executable, a 't' need to be 
	   appended to the symbolic link name */
# define _xstr(s) _str(s)
# define _str(s) #s
# ifndef _WIN32
	const int pathseparator = '/';
# else
	const int pathseparator = '\\';
# endif
	const char *str = strrchr(argv0, pathseparator);
	return !strcmp(str ? str+1 : argv0, _xstr(EXECUTABLE_NAME) "t");
#else
	return 0;
#endif
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

	const int transmitter = isTransmitter(argv[0]);

	try
	{

#ifdef _WIN32
		/* works for both transmit and receive. GUI is low, working is normal.
		 * the working thread does not need to know what the setting is.
		 */
			if (Settings.Get("GUI", "processpriority", TRUE))
			{
				/* Set priority class for this application */
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

				/* Low priority for GUI thread */
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
				Settings.Put("GUI", "processpriority", TRUE);
			}
			else
				Settings.Put("GUI", "processpriority", FALSE);
#endif

		string mode = Settings.Get("command", "mode", string("receive"));
		if (!transmitter && mode == "receive")
		{
			CDRMReceiver DRMReceiver;

			/* First, initialize the working thread. This should be done in an extra
			   routine since we cannot 100% assume that the working thread is
			   ready before the GUI thread */

			CRig rig(DRMReceiver.GetParameters());
			rig.LoadSettings(Settings); // must be before DRMReceiver for G313
			DRMReceiver.LoadSettings(Settings);

			DRMReceiver.SetReceiverMode(ERecMode(Settings.Get("Receiver", "mode", int(0))));

#ifdef HAVE_LIBHAMLIB
			DRMReceiver.SetRig(&rig);

			if(DRMReceiver.GetDownstreamRSCIOutEnabled())
			{
				rig.subscribe();
			}
#endif
			FDRMDialog MainDlg(DRMReceiver, Settings, rig);

			/* Start working thread */
			CRx rx(DRMReceiver);
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
		else if(transmitter || mode == "transmit")
		{
			TransmDialog MainDlg(Settings);

			/* Set main window */
#if QT_VERSION < 0x040000
			app.setMainWidget(&MainDlg);
#endif

			/* Show dialog */
			MainDlg.show();
			app.exec();
		}
		else
		{
			QMessageBox::information(0, "Dream", Settings.UsageArguments(argv).c_str());
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
		const int transmitter = isTransmitter(argv[0]);

		CSettings Settings;
		Settings.Load(argc, argv);
		string mode = Settings.Get("command", "mode", string("receive"));
		if (!transmitter && mode == "receive")
		{
			CDRMSimulation DRMSimulation;
			CDRMReceiver DRMReceiver;

			DRMSimulation.SimScript();
			DRMReceiver.LoadSettings(Settings);
			DRMReceiver.SetReceiverMode(ERecMode(Settings.Get("Receiver", "mode", int(0))));

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
		else if(transmitter || mode == "transmit")
		{
			CDRMTransmitter DRMTransmitter;
//TODO			DRMTransmitter.LoadSettings(Settings);
			DRMTransmitter.Start();
		}
		else
		{
			cerr << Settings.UsageArguments(argv) << endl;
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
	printf("\nDebug error! For more information see test/DebugError.dat\n");
	exit(1);
}
