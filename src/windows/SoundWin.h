/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See Sound.cpp
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

#ifndef _SOUNDWIN_H
#define _SOUNDWIN_H

#include <windows.h>
#include <mmsystem.h>

#include "sound.h"


/* Definitions ****************************************************************/
#define	NUM_IN_OUT_CHANNELS		2		/* Stereo recording (but we only
use one channel for recording) */
#define	BITS_PER_SAMPLE			16		/* Use all bits of the D/A-converter */
#define BYTES_PER_SAMPLE		2		/* Number of bytes per sample */



/* Classes ********************************************************************/
class CSoundInWin : public CSoundIn
{
    public:
    CSoundInWin();
virtual ~CSoundInWin();

void		Init(int iNewBufferSize, _BOOLEAN bNewBlocking = TRUE);
_BOOLEAN	Read(CVector<short>& psData);
virtual void		SetDev(int iNewDev);
void		Close();
protected:
void		OpenInDevice();
void		PrepareInBuffer(int iBufNum);
void		AddInBuffer();

WAVEFORMATEX	sWaveFormatEx;
BOOLEAN			bChangDev;

/* Wave in */
WAVEINCAPS		m_WaveInDevCaps;
HWAVEIN			m_WaveIn;
HANDLE			m_WaveInEvent;
WAVEHDR			m_WaveInHeader[NUM_SOUND_BUFFERS_IN];
int				iBufferSizeIn;
int				iWhichBufferIn;
short*			psSoundcardBuffer[NUM_SOUND_BUFFERS_IN];
_BOOLEAN		bBlocking;

};

class CSoundOutWin : public CSoundOut
{
    public:
    CSoundOutWin();
virtual ~CSoundOutWin();

void		Init(int iNewBufferSize, _BOOLEAN bNewBlocking = FALSE);
_BOOLEAN	Write(CVector<short>& psData);
virtual void		SetDev(int iNewDev);
void		Close();

protected:
void		OpenOutDevice();
void		PrepareOutBuffer(int iBufNum);
void		AddOutBuffer(int iBufNum);
void		GetDoneBuffer(int& iCntPrepBuf, int& iIndexDoneBuf);

WAVEFORMATEX	sWaveFormatEx;
int				iCurDev;
BOOLEAN			bChangDev;

/* Wave out */
WAVEOUTCAPS		m_WaveOutDevCaps;
int				iBufferSizeOut;
HWAVEOUT		m_WaveOut;
short*			psPlaybackBuffer[NUM_SOUND_BUFFERS_OUT];
WAVEHDR			m_WaveOutHeader[NUM_SOUND_BUFFERS_OUT];
HANDLE			m_WaveOutEvent;
_BOOLEAN		bBlocking;
};


#endif
