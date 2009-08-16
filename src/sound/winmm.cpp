/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * Sound card interface for Windows operating systems
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

#include "Sound.h"
#include <iostream>

/* Implementation *************************************************************/
/******************************************************************************\
* Wave in                                                                      *
\******************************************************************************/

CSoundIn::CSoundIn():CSoundInInterface(),m_WaveIn(NULL)
{
	int i;

	/* Get the number of audio capture devices in this computer, check range */
	int iNumDevs = waveInGetNumDevs();

	if (iNumDevs == 0)
	   return;

	/* Init buffer pointer to zero */
	for (i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
	{
		memset(&m_WaveInHeader[i], 0, sizeof(WAVEHDR));
		psSoundcardBuffer[i] = NULL;
	}

	/* Init wave-format structure */
	sWaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
	sWaveFormatEx.nChannels = NUM_IN_OUT_CHANNELS;
	sWaveFormatEx.wBitsPerSample = BITS_PER_SAMPLE;
	sWaveFormatEx.nSamplesPerSec = SOUNDCRD_SAMPLE_RATE;
	sWaveFormatEx.nBlockAlign = sWaveFormatEx.nChannels *
		sWaveFormatEx.wBitsPerSample / 8;
	sWaveFormatEx.nAvgBytesPerSec = sWaveFormatEx.nBlockAlign *
		sWaveFormatEx.nSamplesPerSec;
	sWaveFormatEx.cbSize = 0;

	/* Get info about the devices and store the names */
	for (i = 0; i < iNumDevs; i++)
		if (!waveInGetDevCaps(i, &m_WaveInDevCaps, sizeof(WAVEINCAPS)))
			vecstrDevices.push_back(m_WaveInDevCaps.szPname);
	vecstrDevices.push_back("Primary Capture Device");

	/* We use an event controlled wave-in structure */
	/* Create events */
	m_WaveEvent = CreateEvent(NULL, false, false, NULL);

	/* Set flag to open devices */
	bChangDev = true;


	/* Blocking wave in is default */
	bBlocking = true;

}

CSoundIn::~CSoundIn()
{
	/* Delete allocated memory */
	for (int i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
	{
		if (psSoundcardBuffer[i] != NULL)
			delete[] psSoundcardBuffer[i];
	}

	/* Close the handle for the events */
	if (m_WaveEvent != NULL)
		CloseHandle(m_WaveEvent);
}

bool CSoundIn::Read(vector<_SAMPLE>& data)
{
	int			i;
	bool	bError;

	/* Check if device must be opened or reinitialized */
	if (bChangDev == true)
	{
		OpenDevice();

		/* Reinit sound interface */
		Init(iBufferSize, bBlocking);

		/* Reset flag */
		bChangDev = false;
	}

	/* Wait until data is available */
	if (!(m_WaveInHeader[iWhichBuffer].dwFlags & WHDR_DONE))
	{
		if (bBlocking == true)
			WaitForSingleObject(m_WaveEvent, INFINITE);
		else
			return false;
	}

	/* Check if buffers got lost */
	int iNumInBufDone = 0;
	for (i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
	{
		if (m_WaveInHeader[i].dwFlags & WHDR_DONE)
			iNumInBufDone++;
	}

	/* If the number of done buffers equals the total number of buffers, it is
	   very likely that a buffer got lost -> set error flag */
	if (iNumInBufDone == NUM_SOUND_BUFFERS_IN)
		bError = true;
	else
		bError = false;

	/* Copy data from sound card in output buffer */
	for (i = 0; i < iBufferSize; i++)
		data[i] = psSoundcardBuffer[iWhichBuffer][i];

	/* Add the buffer so that it can be filled with new samples */
	AddBuffer();

	/* In case more than one buffer was ready, reset event */
	ResetEvent(m_WaveEvent);
	return bError;
}

void CSoundIn::AddBuffer()
{
	/* Unprepare old wave-header */
	waveInUnprepareHeader(
		m_WaveIn, &m_WaveInHeader[iWhichBuffer], sizeof(WAVEHDR));

	/* Prepare buffers for sending to sound interface */
	PrepareBuffer(iWhichBuffer);

	/* Send buffer to driver for filling with new data */
	waveInAddBuffer(m_WaveIn, &m_WaveInHeader[iWhichBuffer], sizeof(WAVEHDR));

	/* Toggle buffers */
	iWhichBuffer++;
	if (iWhichBuffer == NUM_SOUND_BUFFERS_IN)
		iWhichBuffer = 0;
}

void CSoundIn::PrepareBuffer(int iBufNum)
{
	/* Set struct entries */
	m_WaveInHeader[iBufNum].lpData = (LPSTR) &psSoundcardBuffer[iBufNum][0];
	m_WaveInHeader[iBufNum].dwBufferLength = iBufferSize * BYTES_PER_SAMPLE;
	m_WaveInHeader[iBufNum].dwFlags = 0;

	/* Prepare wave-header */
	waveInPrepareHeader(m_WaveIn, &m_WaveInHeader[iBufNum], sizeof(WAVEHDR));
}

void CSoundIn::Init(int iNewBufferSize, bool bNewBlocking, int)
{
	/* Check if device must be opened or reinitialized */
	if (bChangDev == true)
	{
		OpenDevice();

		/* Reset flag */
		bChangDev = false;
	}

	/* Set internal parameter */
	iBufferSize = iNewBufferSize;
	bBlocking = bNewBlocking;

	/* Reset interface so that all buffers are returned from the interface */
	waveInReset(m_WaveIn);
	waveInStop(m_WaveIn);

	/* Reset current buffer ID (it is important to do this BEFORE calling
	   "AddInBuffer()" */
	iWhichBuffer = 0;

	/* Create memory for sound card buffer */
	for (int i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
	{
		/* Unprepare old wave-header in case that we "re-initialized" this
		   module. Calling "waveInUnprepareHeader()" with an unprepared
		   buffer (when the module is initialized for the first time) has
		   simply no effect */
		waveInUnprepareHeader(m_WaveIn, &m_WaveInHeader[i], sizeof(WAVEHDR));

		if (psSoundcardBuffer[i] != NULL)
			delete[] psSoundcardBuffer[i];

		psSoundcardBuffer[i] = new _SAMPLE[iBufferSize];


		/* Send all buffers to driver for filling the queue ----------------- */
		/* Prepare buffers before sending them to the sound interface */
		PrepareBuffer(i);

		AddBuffer();
	}

	/* Notify that sound capturing can start now */
	waveInStart(m_WaveIn);

	/* This reset event is very important for initialization, otherwise we will
	   get errors! */
	ResetEvent(m_WaveEvent);
}

void CSoundIn::OpenDevice()
{
	/* Open wave-input and set call-back mechanism to event handle */
	if (m_WaveIn != NULL)
	{
		waveInReset(m_WaveIn);
		waveInClose(m_WaveIn);
	}

	/* Default ? */
	if(iCurDev < 0)
		iCurDev = vecstrDevices.size()-1;

	/* out of range ? */
	if(size_t(iCurDev) >= vecstrDevices.size())
		iCurDev = vecstrDevices.size()-1;

	int mmdev = iCurDev;
	if(size_t(iCurDev) == (vecstrDevices.size()-1))
	    mmdev = WAVE_MAPPER;

	MMRESULT result = waveInOpen(&m_WaveIn, mmdev, &sWaveFormatEx,
		(DWORD_PTR) m_WaveEvent, 0, CALLBACK_EVENT);

	if (result != MMSYSERR_NOERROR)
		throw CGenErr("Sound Interface Start, waveInOpen() failed. This error "
			"usually occurs if another application blocks the sound in.");
}

void CSoundIn::SetDev(int iNewDev)
{
	/* Change only in case new device id is not already active */
	if (iNewDev != iCurDev)
	{
		iCurDev = iNewDev;
		bChangDev = true;
	}
}

void CSoundIn::Enumerate(vector<string>& names) const
{
	names = vecstrDevices;
}

int	CSoundIn::GetDev() const
{
	return iCurDev;
}

void CSoundIn::Close()
{
	int			i;
	MMRESULT	result;

	/* Reset audio driver */
	if (m_WaveIn != NULL)
	{
		result = waveInReset(m_WaveIn);
		if (result != MMSYSERR_NOERROR)
			throw CGenErr("Sound Interface, waveInReset() failed.");
	}

	/* Set event to ensure that thread leaves the waiting function */
	if (m_WaveEvent != NULL)
		SetEvent(m_WaveEvent);

	/* Wait for the thread to terminate */
	Sleep(500);

	/* Unprepare wave-headers */
	if (m_WaveIn != NULL)
	{
		for (i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
		{
			result = waveInUnprepareHeader(
				m_WaveIn, &m_WaveInHeader[i], sizeof(WAVEHDR));

			if (result != MMSYSERR_NOERROR)
				throw CGenErr("Sound Interface, waveInUnprepareHeader()"
					" failed.");
		}

		/* Close the sound in device */
		result = waveInClose(m_WaveIn);
		if (result != MMSYSERR_NOERROR)
			throw CGenErr("Sound Interface, waveInClose() failed.");
	}


	/* Set flag to open devices the next time it is initialized */
	bChangDev = true;
}
/******************************************************************************\
* Wave out                                                                     *
\******************************************************************************/
CSoundOut::CSoundOut():CSoundOutInterface(),m_WaveOut(NULL)
{
	int i;

	/* Get the number of digital audio devices in this computer, check range */
	int iNumDevs = waveOutGetNumDevs();

	if (iNumDevs == 0)
	   return;

	/* Init buffer pointer to zero */
	for (i = 0; i < NUM_SOUND_BUFFERS_OUT; i++)
	{
		memset(&m_WaveOutHeader[i], 0, sizeof(WAVEHDR));
		psPlaybackBuffer[i] = NULL;
	}

	/* Init wave-format structure */
	sWaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
	sWaveFormatEx.nChannels = NUM_IN_OUT_CHANNELS;
	sWaveFormatEx.wBitsPerSample = BITS_PER_SAMPLE;
	sWaveFormatEx.nSamplesPerSec = SOUNDCRD_SAMPLE_RATE;
	sWaveFormatEx.nBlockAlign = sWaveFormatEx.nChannels *
		sWaveFormatEx.wBitsPerSample / 8;
	sWaveFormatEx.nAvgBytesPerSec = sWaveFormatEx.nBlockAlign *
		sWaveFormatEx.nSamplesPerSec;
	sWaveFormatEx.cbSize = 0;

	/* Get info about the devices and store the names */
	for (i = 0; i < iNumDevs; i++)
		if (!waveOutGetDevCaps(i, &m_WaveOutDevCaps, sizeof(WAVEOUTCAPS)))
			vecstrDevices.push_back(m_WaveOutDevCaps.szPname);
	vecstrDevices.push_back("Primary Playback Device");

	/* We use an event controlled wave-out structure */
	/* Create events */
	m_WaveEvent = CreateEvent(NULL, false, false, NULL);

	/* Set flag to open devices */
	bChangDev = true;

	/* TODO does not work well with hot pluggable devices! */
	iCurDev = iNumDevs-1;

	/* Non-blocking wave out is default */
	bBlocking = false;
}

CSoundOut::~CSoundOut()
{
	/* Delete allocated memory */
	for (int i = 0; i < NUM_SOUND_BUFFERS_OUT; i++)
	{
		if (psPlaybackBuffer[i] != NULL)
			delete[] psPlaybackBuffer[i];
	}

	/* Close the handle for the events */
	if (m_WaveEvent != NULL)
		CloseHandle(m_WaveEvent);
}

bool CSoundOut::Write(vector<_SAMPLE>& data)
{
	int			i, j;
	int			iCntPrepBuf;
	int			iIndexDoneBuf;
	bool	bError=false;

	/* Check if device must be opened or reinitialized */
	if (bChangDev == true)
	{
		OpenDevice();

		/* Reinit sound interface */
		Init(iBufferSize, bBlocking);

		/* Reset flag */
		bChangDev = false;
	}

	/* Get number of "done"-buffers and position of one of them */
	GetDoneBuffer(iCntPrepBuf, iIndexDoneBuf);

	/* Now check special cases (Buffer is full or empty) */
	if (iCntPrepBuf == 0)
	{
		if (bBlocking == true)
		{
			/* Blocking wave out routine. Needed for transmitter. Always
			   ensure that the buffer is completely filled to avoid buffer
			   underruns */
			while (iCntPrepBuf == 0)
			{
				WaitForSingleObject(m_WaveEvent, INFINITE);

				GetDoneBuffer(iCntPrepBuf, iIndexDoneBuf);
			}
		}
		else
		{
			/* All buffers are filled, dump new block ----------------------- */
// It would be better to kill half of the buffer blocks to set the start
// back to the middle: TODO
			return true; /* An error occurred */
		}
	}
	else if (iCntPrepBuf == NUM_SOUND_BUFFERS_OUT)
	{
		/* ---------------------------------------------------------------------
		   Buffer is empty -> send as many cleared blocks to the sound-
		   interface until half of the buffer size is reached */
		/* Send half of the buffer size blocks to the sound-interface */
		for (j = 0; j < NUM_SOUND_BUFFERS_OUT / 2; j++)
		{
			/* First, clear these buffers */
			for (i = 0; i < iBufferSize; i++)
				psPlaybackBuffer[j][i] = 0;

			/* Then send them to the interface */
			AddBuffer(j);
		}

		/* Set index for done buffer */
		iIndexDoneBuf = NUM_SOUND_BUFFERS_OUT / 2;

		bError = true;
	}
	else
		bError = false;

	/* Copy stereo data from input in soundcard buffer */
	for (i = 0; i < iBufferSize; i++)
		psPlaybackBuffer[iIndexDoneBuf][i] = data[i];

	/* Now, send the current block */
	AddBuffer(iIndexDoneBuf);

	return bError;
}

void CSoundOut::GetDoneBuffer(int& iCntPrepBuf, int& iIndexDoneBuf)
{
	/* Get number of "done"-buffers and position of one of them */
	iCntPrepBuf = 0;
	for (int i = 0; i < NUM_SOUND_BUFFERS_OUT; i++)
	{
		if (m_WaveOutHeader[i].dwFlags & WHDR_DONE)
		{
			iCntPrepBuf++;
			iIndexDoneBuf = i;
		}
	}
}

void CSoundOut::AddBuffer(int iBufNum)
{
	/* Unprepare old wave-header */
	waveOutUnprepareHeader(
		m_WaveOut, &m_WaveOutHeader[iBufNum], sizeof(WAVEHDR));

	/* Prepare buffers for sending to sound interface */
	PrepareBuffer(iBufNum);

	/* Send buffer to driver for filling with new data */
	waveOutWrite(m_WaveOut, &m_WaveOutHeader[iBufNum], sizeof(WAVEHDR));
}

void CSoundOut::PrepareBuffer(int iBufNum)
{
	/* Set Header data */
	m_WaveOutHeader[iBufNum].lpData = (LPSTR) &psPlaybackBuffer[iBufNum][0];
	m_WaveOutHeader[iBufNum].dwBufferLength = iBufferSize * BYTES_PER_SAMPLE;
	m_WaveOutHeader[iBufNum].dwFlags = 0;

	/* Prepare wave-header */
	waveOutPrepareHeader(m_WaveOut, &m_WaveOutHeader[iBufNum], sizeof(WAVEHDR));
}

void CSoundOut::Init(int iNewBufferSize, bool bNewBlocking, int)
{

	int	i, j;

	/* Check if device must be opened or reinitialized */
	if (bChangDev == true)
	{
		OpenDevice();

		/* Reset flag */
		bChangDev = false;
	}

	/* Set internal parameters */
	iBufferSize = iNewBufferSize;
	bBlocking = bNewBlocking;

	/* Reset interface */
	waveOutReset(m_WaveOut);

	for (j = 0; j < NUM_SOUND_BUFFERS_OUT; j++)
	{
		/* Unprepare old wave-header (in case header was not prepared before,
		   simply nothing happens with this function call */
		waveOutUnprepareHeader(m_WaveOut, &m_WaveOutHeader[j], sizeof(WAVEHDR));

		/* Create memory for playback buffer */
		if (psPlaybackBuffer[j] != NULL)
			delete[] psPlaybackBuffer[j];

		psPlaybackBuffer[j] = new _SAMPLE[iBufferSize];

		/* Clear new buffer */
		for (i = 0; i < iBufferSize; i++)
			psPlaybackBuffer[j][i] = 0;

		/* Prepare buffer for sending to the sound interface */
		PrepareBuffer(j);

		/* Initially, send all buffers to the interface */
		AddBuffer(j);
	}
}

void CSoundOut::OpenDevice()
{

	if (m_WaveOut != NULL)
	{
		waveOutReset(m_WaveOut);
		waveOutClose(m_WaveOut);
	}

	/* Default ? */
	if(iCurDev < 0)
		iCurDev = vecstrDevices.size()-1;

	/* out of range ? */
	if(size_t(iCurDev) >= vecstrDevices.size())
		iCurDev = vecstrDevices.size()-1;

	int mmdev = iCurDev;
	if(size_t(iCurDev) == (vecstrDevices.size()-1))
	    mmdev = WAVE_MAPPER;

	MMRESULT result = waveOutOpen(&m_WaveOut, mmdev, &sWaveFormatEx,
		(DWORD_PTR) m_WaveEvent, 0, CALLBACK_EVENT);

	if (result != MMSYSERR_NOERROR)
		throw CGenErr("Sound Interface Start, waveOutOpen() failed.");
}

void CSoundOut::Enumerate(vector<string>& names) const
{
	names = vecstrDevices;
}

int	CSoundOut::GetDev() const
{
	return iCurDev;
}

void CSoundOut::SetDev(int iNewDev)
{
	/* Change only in case new device id is not already active */
	if (iNewDev != iCurDev)
	{
		iCurDev = iNewDev;
		bChangDev = true;
	}
}


void CSoundOut::Close()
{
	int			i;
	MMRESULT	result;

	/* Reset audio driver */
	if (m_WaveOut != NULL)
	{
		result = waveOutReset(m_WaveOut);
		if (result != MMSYSERR_NOERROR)
			throw CGenErr("Sound Interface, waveOutReset() failed.");
	}

	/* Wait for the thread to terminate */
	Sleep(500);

	if (m_WaveOut != NULL)
	{
		for (i = 0; i < NUM_SOUND_BUFFERS_OUT; i++)
		{
			result = waveOutUnprepareHeader(
				m_WaveOut, &m_WaveOutHeader[i], sizeof(WAVEHDR));

			if (result != MMSYSERR_NOERROR)
				throw CGenErr("Sound Interface, waveOutUnprepareHeader()"
					" failed.");
		}

		/* Close the sound out device */
		result = waveOutClose(m_WaveOut);
		if (result != MMSYSERR_NOERROR)
			throw CGenErr("Sound Interface, waveOutClose() failed.");
	}

	/* Set flag to open devices the next time it is initialized */
	bChangDev = true;
}

