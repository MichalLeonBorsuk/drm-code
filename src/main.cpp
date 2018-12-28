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
# include <windows.h>
#endif
#if defined(__unix__) && !defined(__APPLE__)
# include <csignal>
#endif

#include "GlobalDefinitions.h"
#include "DrmReceiver.h"
#include "DrmTransmitter.h"
#include "DrmSimulation.h"
#include "util/Settings.h"
#include <iostream>

#ifdef QT_CORE_LIB
# ifdef HAVE_LIBHAMLIB
#   include "util-QT/Rig.h"
# endif
# include <QCoreApplication>
# include <QTranslator>
# include <QThread>

class CRx: public QThread
{
public:
	CRx(CDRMReceiver& nRx):rx(nRx)
	{}
	void run();
private:
	CDRMReceiver& rx;
};

void
CRx::run()
{
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
#endif

#ifdef USE_OPENSL
# include <SLES/OpenSLES.h>
SLObjectItf engineObject = nullptr;
#endif

int
main(int argc, char **argv)
{
#ifdef USE_OPENSL
    (void)slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    (void)(*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
#endif
    try
	{
		CSettings Settings;
		Settings.Load(argc, argv);

		string mode = Settings.Get("command", "mode", string());
		if (mode == "receive")
		{
			CDRMSimulation DRMSimulation;
			CDRMReceiver DRMReceiver(&Settings);

			DRMSimulation.SimScript();
			DRMReceiver.LoadSettings();

#ifdef _WIN32
	WSADATA wsaData;
	(void)WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
#ifdef QT_CORE_LIB
			QCoreApplication app(argc, argv);
			/* Start working thread */
			CRx rx(DRMReceiver);
			rx.start();
			return app.exec();
#else
			DRMReceiver.Start();
#endif
		}
		else if (mode == "transmit")
		{
			CDRMTransmitter DRMTransmitter(&Settings);
			DRMTransmitter.LoadSettings();
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
