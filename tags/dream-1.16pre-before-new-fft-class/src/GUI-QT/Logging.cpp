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

/* Implementation *************************************************************/
CLogging::CLogging(CParameter& Parameters) : QObject(),
    TimerLogFileLong(), TimerLogFileShort(), TimerLogFileStart(),
    shortLog(Parameters), longLog(Parameters),
    enabled(false)
{
#if QT_VERSION >= 0x040000
	TimerLogFileStart.setSingleShot(true);
#endif
    connect(&TimerLogFileLong, SIGNAL(timeout()),
            this, SLOT(OnTimerLogFileLong()));
    connect(&TimerLogFileShort, SIGNAL(timeout()),
            this, SLOT(OnTimerLogFileShort()));
    connect(&TimerLogFileStart, SIGNAL(timeout()),
            this, SLOT(start()));
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

    /* Activate log file start if necessary. */
    if (enabled)
    {
        /* One shot timer */
	int iLogDelay = Settings.Get("Logfile", "delay", 0);
#if QT_VERSION < 0x040000
        TimerLogFileStart.start(iLogDelay * 1000 /* ms */, true);
#else
        TimerLogFileStart.start(iLogDelay * 1000 /* ms */);
#endif
	// initialise ini file if never set
        Settings.Put("Logfile", "delay", iLogDelay);
    }
}

void CLogging::SaveSettings(CSettings& Settings)
{
    Settings.Put("Logfile", "enablerxl", shortLog.GetRxlEnabled());
    Settings.Put("Logfile", "enablepositiondata", shortLog.GetPositionEnabled());
    Settings.Put("Logfile", "enablelog", enabled);
}

void CLogging::OnTimerLogFileShort()
{
    /* Write new parameters in log file (short version) */
    shortLog.Update();
}

void CLogging::OnTimerLogFileLong()
{
    /* Write new parameters in log file (long version) */
    longLog.Update();
}

void CLogging::start()
{
    enabled = true;
    /* Start logging (if not already done) */
    if(!longLog.GetLoggingActivated())
    {
        /* Activate log file timer for long and short log file */
        TimerLogFileShort.start(60000); /* Every minute (i.e. 60000 ms) */
        TimerLogFileLong.start(1000); /* Every second */

        /* Open log file */
        shortLog.Start("DreamLog.txt");
        longLog.Start("DreamLogLong.csv");
    }
    if(longLog.GetRxlEnabled())
    {
        emit subscribeRig();
    }
}

void CLogging::stop()
{
    enabled = false;
    TimerLogFileStart.stop();
    TimerLogFileShort.stop();
    TimerLogFileLong.stop();
    shortLog.Stop();
    longLog.Stop();
    if(longLog.GetRxlEnabled())
    {
        emit unsubscribeRig();
    }
}
