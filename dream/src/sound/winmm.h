/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See winmm.cpp
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

#if !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
#define AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_

#include "../soundinterface.h"
#include <windows.h>
#include <mmsystem.h>

/* Set this number as high as we have to prebuffer symbols for one MSC block.
   In case of robustness mode D we have 24 symbols */
#define NUM_SOUND_BUFFERS_IN	24		/* Number of sound card buffers */

#define NUM_SOUND_BUFFERS_OUT	3		/* Number of sound card buffers */
#define	NUM_IN_OUT_CHANNELS		2		/* Stereo recording (but we only
										   use one channel for recording) */
#define	BITS_PER_SAMPLE			16		/* Use all bits of the D/A-converter */
#define BYTES_PER_SAMPLE		2		/* Number of bytes per sample */

/* Classes ********************************************************************/
class CSoundIn : public CSoundInInterface
{
public:
	CSoundIn();
	virtual ~CSoundIn();

	virtual void	Init(int iNewBufferSize, bool bNewBlocking=true, int iChannels=2);
	virtual bool	Read(vector<_SAMPLE>& data);
	virtual void	Enumerate(vector<string>&) const;
	virtual int	GetDev() const;
	virtual void	SetDev(int iNewDev);
	virtual void	Close();

protected:
	void		OpenDevice();
	void		PrepareBuffer(int iBufNum);
	void		AddBuffer();

	vector<string>	vecstrDevices;
	int		iCurDev;
	bool		bChangDev;
	int		iBufferSize;
	int		iWhichBuffer;
	bool		bBlocking;
	WAVEFORMATEX	sWaveFormatEx;
	HANDLE		m_WaveEvent;

	/* Wave in */
	WAVEINCAPS	m_WaveInDevCaps;
	HWAVEIN		m_WaveIn;
	WAVEHDR		m_WaveInHeader[NUM_SOUND_BUFFERS_IN];
	_SAMPLE*	psSoundcardBuffer[NUM_SOUND_BUFFERS_IN];

};

class CSoundOut : public CSoundOutInterface
{
public:
	CSoundOut();
	virtual ~CSoundOut();

	virtual void		Init(int iNewBufferSize, bool bNewBlocking=false, int iChannels=2);
	virtual bool		Write(vector<_SAMPLE>& data);
	virtual void		Enumerate(vector<string>&) const;
	virtual int		GetDev() const;
	virtual void		SetDev(int iNewDev);
	virtual void		Close();

protected:
	void		OpenDevice();
	void		PrepareBuffer(int iBufNum);
	void		AddBuffer(int iBufNum);
	void		GetDoneBuffer(int& iCntPrepBuf, int& iIndexDoneBuf);

	vector<string>	vecstrDevices;
	int		iCurDev;
	bool		bChangDev;
	int		iBufferSize;
	int		iWhichBuffer;
	bool		bBlocking;
	WAVEFORMATEX	sWaveFormatEx;
	HANDLE		m_WaveEvent;

	/* Wave out */
	WAVEOUTCAPS	m_WaveOutDevCaps;
	HWAVEOUT	m_WaveOut;
	_SAMPLE*	psPlaybackBuffer[NUM_SOUND_BUFFERS_OUT];
	WAVEHDR		m_WaveOutHeader[NUM_SOUND_BUFFERS_OUT];
};

#endif // !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
