/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
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

#include "Logging.h"
#include "../util/Settings.h"

#define SHORT_LOG_FILENAME "DreamLog.txt"
#define LONG_LOG_FILENAME "DreamLogLong.csv"


/* Implementation *************************************************************/
CLogging::CLogging(CParameter& Parameters) :
    shortLog(Parameters), longLog(Parameters),
    enabled(false), iLogDelay(0), iLogCount(0)
{
//#if QT_VERSION >= 0x040000
//	TimerLogFileStart.setSingleShot(true);
//#endif
    connect(&TimerLogFile, SIGNAL(timeout()), this, SLOT(OnTimerLogFile()));
//    connect(&TimerLogFileStart, SIGNAL(timeout()), this, SLOT(start()));
}

void CLogging::LoadSettings(CSettings& Settings)
{
    /* log file flag for storing signal strength in long log */
    _BOOLEAN logrxl = Settings.Get("Logfile", "enablerxl", FALSE);
    shortLog.SetRxlEnabled(logrxl);
    longLog.SetRxlEnabled(logrxl);

    /* log file flag for storing lat/long in long log */
    bool enablepositiondata = Settings.Get("Logfile", "enablepositiondata", false);
    shortLog.SetPositionEnabled(enablepositiondata);
    longLog.SetPositionEnabled(enablepositiondata);

    enabled = Settings.Get("Logfile", "enablelog", false);
    iLogDelay = Settings.Get("Logfile", "delay", 0);
    SaveSettings(Settings);
}
#if 0
void CLogging::reStart()
{
    /* Activate log file start if necessary. */
    if (enabled)
    {
        /* One shot timer */
#if QT_VERSION < 0x040000
        TimerLogFileStart.start(iLogDelay * 1000 /* ms */, true);
#else
        TimerLogFileStart.start(iLogDelay * 1000 /* ms */);
#endif
    }
}
#endif
void CLogging::SaveSettings(CSettings& Settings)
{
    Settings.Put("Logfile", "enablerxl", shortLog.GetRxlEnabled());
    Settings.Put("Logfile", "enablepositiondata", shortLog.GetPositionEnabled());
    Settings.Put("Logfile", "enablelog", enabled);
    Settings.Put("Logfile", "delay", iLogDelay);
}

void CLogging::OnTimerLogFile()
{
    if (shortLog.restartNeeded())
    {
//printf("shortLog.restartNeeded()\n");
        shortLog.Stop();
        shortLog.Start(SHORT_LOG_FILENAME);
//        stop();
//        enabled = true;
//        reStart();
    }
//    else
//    {
        iLogCount++;
        if(iLogCount == 60)
        {
            iLogCount = 0;
            shortLog.Update();
        }
        longLog.Update();
//    }
}

void CLogging::start()
{
//printf("CLogging::start()\n");
	iLogCount = 0;
    enabled = true;
    /* Start logging (if not already done) */
    if(!longLog.GetLoggingActivated())
    {
        TimerLogFile.start(1000); /* Every second */

		/* Latch new param */
		shortLog.restartNeeded();
        /* Open log file */
        shortLog.Start(SHORT_LOG_FILENAME);
        longLog.Start(LONG_LOG_FILENAME);
    }
    if(longLog.GetRxlEnabled())
    {
        emit subscribeRig();
    }
}

void CLogging::stop()
{
//printf("CLogging::stop()\n");
    enabled = false;
//    TimerLogFileStart.stop();
    TimerLogFile.stop();
    shortLog.Stop();
    longLog.Stop();
    if(longLog.GetRxlEnabled())
    {
        emit unsubscribeRig();
    }
}
