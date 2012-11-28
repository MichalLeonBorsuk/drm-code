/******************************************************************************\
 *
 * Copyright (c) 2012
 *
 * Author(s):
 *	David Flamand
 *
 * Decription:
 *  PulseAudio sound interface with clock drift adjustment (optional)
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

#ifndef PULSEAUDIO_H_INCLUDED
#define PULSEAUDIO_H_INCLUDED


/* Master switch */
//#define ENABLE_CLOCK_DRIFT_ADJ


#include <pulse/pulseaudio.h>
#include "../soundinterface.h"
#if defined(PA_STREAM_VARIABLE_RATE) && defined(ENABLE_CLOCK_DRIFT_ADJ)
# define CLOCK_DRIFT_ADJ_ENABLED
#endif

#ifdef CLOCK_DRIFT_ADJ_ENABLED
# include "../matlib/MatlibSigProToolbox.h"
#endif


typedef struct _pa_stream_notify_cb_userdata_t
{
	void *SoundIO;
	_BOOLEAN	bOverflow;
	_BOOLEAN	bMute;
	_BOOLEAN*	bBufferingError;
} pa_stream_notify_cb_userdata_t;

typedef struct _pa_common
{
	_BOOLEAN		bClockDriftComp;
	int				sample_rate_offset;
} pa_common;


/* Classes ********************************************************************/

class CSoundInPulse : public CSoundInInterface
{
public:
	CSoundInPulse();
	virtual ~CSoundInPulse() {}

	virtual void	Enumerate(vector<string>& choices) { choices = names; }
	virtual void	SetDev(int iNewDevice);
	virtual int		GetDev();

	void			Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
	_BOOLEAN		Read(CVector<_SAMPLE>& psData);
	void			Close();
#ifdef CLOCK_DRIFT_ADJ_ENABLED
	void			SetCommonParamPtr(pa_common *cp_ptr) { cp = cp_ptr; }
#endif

protected:
	void			Init_HW();
	int				Read_HW(void *recbuf, int size);
	void			Close_HW();
	void			SetBufferSize_HW();

	int 			iSampleRate;
	int 			iBufferSize;
	_BOOLEAN		bBlockingRec;

	vector<string>	devices;
	vector<string>	names;
	_BOOLEAN		bChangDev;
	int				iCurrentDevice;

	pa_mainloop		*pa_m;
	pa_context		*pa_c;
	pa_stream		*pa_s;
	size_t			remaining_nbytes;
	const char		*remaining_data;

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	int				record_sample_rate;
	_BOOLEAN		bClockDriftComp;
	pa_common		*cp;
#endif
};

class CSoundOutPulse : public CSoundOutInterface
{
public:
	CSoundOutPulse();
	virtual ~CSoundOutPulse() {}

	virtual void	Enumerate(vector<string>& choices) { choices = names; }
	virtual void	SetDev(int iNewDevice);
	virtual int		GetDev();

	void			Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking);
	_BOOLEAN		Write(CVector<_SAMPLE>& psData);
	void			Close();
#ifdef CLOCK_DRIFT_ADJ_ENABLED
	pa_common *		GetCommonParamPtr() { return &cp; }
	void			EnableClockDriftAdj(_BOOLEAN bEnable) { bNewClockDriftComp = bEnable; }
	_BOOLEAN		IsClockDriftAdjEnabled() { return bNewClockDriftComp; }
#endif

	_BOOLEAN		bPrebuffer;
	_BOOLEAN		bSeek;

protected:
	void			Init_HW();
	int				Write_HW(void *playbuf, int size);
	void			Close_HW();

	int				iSampleRate;
	int				iBufferSize;
	_BOOLEAN		bBlockingPlay;

	_BOOLEAN		bBufferingError;

	vector<string>	devices;
	vector<string>	names;
	_BOOLEAN		bChangDev;
	int				iCurrentDevice;

	pa_mainloop		*pa_m;
	pa_context		*pa_c;
	pa_stream		*pa_s;
	pa_stream_notify_cb_userdata_t pa_stream_notify_cb_userdata_underflow;
	pa_stream_notify_cb_userdata_t pa_stream_notify_cb_userdata_overflow;

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	int				iMaxSampleRateOffset;
	CReal			playback_usec_smoothed;
	int				playback_usec;
	int				target_latency;
	int				filter_stabilized;
	int				wait_prebuffer;
	int				clock;
	CMatlibVector<CReal> B, A, X, Z;
	_BOOLEAN		bNewClockDriftComp;
	pa_common		cp;
#endif
};

#endif
