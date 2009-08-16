/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable, Volker Fischer
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

#include "Loghelper.h"

/* Implementation *************************************************************/
Loghelper::Loghelper(ReceiverInterface& NDRMR, CSettings& NSettings):
	DRMReceiver(NDRMR), Settings(NSettings),
	TimerLogFileLong(), TimerLogFileShort(), TimerLogFileStart(),
	shortLog(*NDRMR.GetParameters()), longLog(*NDRMR.GetParameters()),
	iLogDelay(0)
{
	/* Timers */
	connect(&TimerLogFileLong, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFileLong()));
	connect(&TimerLogFileShort, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFileShort()));
	connect(&TimerLogFileStart, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFileStart()));

	/* Logging (get directly from settings since Receiver Settings may not send to us at the right time
	 * on startup)
	 */
	iLogDelay = Settings.Get("Logfile", "delay", 0);
	LogSigStr(Settings.Get("Logfile", "enablerxl", false));
	LogPosition(Settings.Get("Logfile", "enablepositiondata", false));
	EnableLog(Settings.Get("Logfile", "enablelog", false));
}

Loghelper::~Loghelper()
{
	if(longLog.GetLoggingActivated())
		shortLog.Stop();
	if(longLog.GetLoggingActivated())
		longLog.Stop();
}

void Loghelper::OnTimerLogFileStart()
{
	/* Start logging (if not already done) */
	if(!longLog.GetLoggingActivated())
	{
		TimerLogFileLong.start(1000); /* Every second */
		longLog.SetLogFrequency(DRMReceiver.GetFrequency());
		longLog.Start("DreamLogLong.csv");
	}
	if(!shortLog.GetLoggingActivated())
	{
		TimerLogFileShort.start(60000); /* Every minute (i.e. 60000 ms) */
		shortLog.SetLogFrequency(DRMReceiver.GetFrequency());
		shortLog.Start("DreamLog.txt");
	}
}

void Loghelper::OnTimerLogFileShort()
{
	/* Write new parameters in log file (short version) */
	shortLog.Update();
}

void Loghelper::OnTimerLogFileLong()
{
	/* Write new parameters in log file (long version) */
	longLog.SetLogFrequency(DRMReceiver.GetFrequency());
	longLog.Update();

	/* this make sure re-tunings appear in the short log, even if they happen
	 * more frequently than the short log timer
	 */
	shortLog.SetLogFrequency(DRMReceiver.GetFrequency());
}

void Loghelper::EnableLog(bool b)
{
	if(b)
	{
		TimerLogFileStart.setSingleShot(true);
		TimerLogFileStart.start(1000*iLogDelay);
	}
	else
	{
		TimerLogFileStart.stop();
		TimerLogFileShort.stop();
		TimerLogFileLong.stop();
		shortLog.Stop();
		longLog.Stop();
	}
}

void Loghelper::LogStartDel(long iValue)
{
	iLogDelay = iValue;
}

void Loghelper::LogPosition(bool b)
{
	shortLog.SetPositionEnabled(b);
	longLog.SetPositionEnabled(b);
}

void Loghelper::LogSigStr(bool b)
{
	shortLog.SetRxlEnabled(b);
	longLog.SetRxlEnabled(b);
}
