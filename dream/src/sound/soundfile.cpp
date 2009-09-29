/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable, Volker Fischer
 *
 * Description:
 *	writing to files that looks like writing to a sound card
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

#include "soundfile.h"
#include "../GlobalDefinitions.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

/* Implementation *************************************************************/

CSoundFileIn::CSoundFileIn(): CSoundInInterface(), pFile(NULL),
		strInFileName(), eFmt(fmt_other),
		iFileSampleRate(0), iFileChannels(1), pacer(NULL)
{
}

CSoundFileIn::~CSoundFileIn()
{
	Close();
}

void
CSoundFileIn::Enumerate(vector<string>& c) const
{
	c.clear();
	c.push_back(strInFileName);
}

void
CSoundFileIn::SetFileName(const string& strFileName)
{
	strInFileName = strFileName;
	string ext;
	size_t p = strInFileName.rfind('.');
	if(p != string::npos)
		ext = strInFileName.substr(p+1);
	if(ext == "txt") eFmt = fmt_txt;
	if(ext == "TXT") eFmt = fmt_txt;
	if(ext.substr(0,2) == "iq") eFmt = fmt_raw_stereo;
	if(ext.substr(0,2) == "IQ") eFmt = fmt_raw_stereo;
	if(ext.substr(0,2) == "if") eFmt = fmt_raw_stereo;
	if(ext.substr(0,2) == "IF") eFmt = fmt_raw_stereo;
	if(ext == "pcm") eFmt = fmt_raw_mono;
	if(ext == "PCM") eFmt = fmt_raw_mono;
	switch(eFmt)
	{
	case fmt_raw_stereo:
		iFileChannels = 2;
		if(ext.length() == 4)
			iFileSampleRate = 1000*atoi(ext.substr(2).c_str());
		else
			iFileSampleRate = 48000; /* not SOUNDCRD_SAMPLE_RATE */
		break;
	default:
		iFileChannels = 1;
		iFileSampleRate = 48000; /* not SOUNDCRD_SAMPLE_RATE */
	}
}

void
CSoundFileIn::Init(int iNewBufferSize, bool bNewBlocking, int iChannels)
{
	if(iChannels != 2)
		throw CGenErr("only stereo supported at the moment for file input, sorry.");

	if (pFile != NULL)
		return;

	/* Check previously a file was being used */
	Close();

	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(SF_INFO));
	switch(eFmt)
	{
	case fmt_txt:
		pFile = fopen(strInFileName.c_str(), "r");
		break;
	case fmt_raw_mono:
	case fmt_raw_stereo:
		sfinfo.samplerate = iFileSampleRate;
		sfinfo.channels = iFileChannels;
		sfinfo.format = SF_FORMAT_RAW|SF_FORMAT_PCM_16|SF_ENDIAN_LITTLE;
		pFile = (FILE*)sf_open(strInFileName.c_str(), SFM_READ, &sfinfo);
		if(pFile==NULL)
			throw CGenErr(sf_strerror(0));
		break;
	case fmt_other:
		pFile = (FILE*)sf_open(strInFileName.c_str(), SFM_READ, &sfinfo);
		if (pFile != NULL)
		{
			iFileChannels = sfinfo.channels;
			iFileSampleRate = sfinfo.samplerate;
			int oversample_factor = SOUNDCRD_SAMPLE_RATE / iFileSampleRate;
			/* we can only cope with integer submultiples */
			if(SOUNDCRD_SAMPLE_RATE != oversample_factor*iFileSampleRate)
				throw CGenErr("unsupported sample rate in input file");
		}
		break;
	}
	/* Check for error */
	if (pFile == NULL)
	{
		throw CGenErr("The file " + strInFileName + " could not be openned");
	}

	if(bNewBlocking)
	{
		double interval = double(iNewBufferSize/2) / double(SOUNDCRD_SAMPLE_RATE);
		pacer = new CPacer(uint64_t(1e9*interval));
	}
}

bool
CSoundFileIn::Read(vector<_SAMPLE>& data)
{
	int iFrames = data.size()/2;
	int i;
	if(pacer) pacer->wait();

	if (pFile == NULL)
	{
		return true;
	}

	if(eFmt==fmt_txt)
	{
		for (i = 0; i < iFrames; i++)
		{
			float tIn;
			if (fscanf(pFile , "%e\n", &tIn) == EOF)
			{
				/* If end-of-file is reached, stop simulation */
				return false;
			}
			data[2*i] = _SAMPLE(tIn);
			data[2*i+1] = _SAMPLE(tIn);
		}
		return false;
	}
	short *buffer = new short[iFileChannels*iFrames];
	sf_count_t c = sf_readf_short((SNDFILE*)pFile, buffer, iFrames);
	if(c!=iFrames)
	{
		/* rewind */
		sf_seek((SNDFILE*)pFile, 0, SEEK_SET);
		c = sf_readf_short((SNDFILE*)pFile, buffer, iFrames);
	}
	int oversample_factor = SOUNDCRD_SAMPLE_RATE / iFileSampleRate;
	if(iFileChannels==2)
	{
		for (i = 0; i < iFrames/oversample_factor; i++)
		{
			for (int j = 0; j < oversample_factor; j++)
			{
				data[2*(i+j)] = _SAMPLE(buffer[2*i]);
				data[2*(i+j)+1] = _SAMPLE(buffer[2*i+1]);
			}
		}
	}
	else
	{
		for (i = 0; i < iFrames/oversample_factor; i++)
		{
			for (int j = 0; j < oversample_factor; j++)
			{
				data[2*(i+j)] = _SAMPLE(buffer[i]);
				data[2*(i+j)+1] = _SAMPLE(buffer[i]);
			}
		}
	}
	delete[] buffer;

	return false;
}

void
CSoundFileIn::Close()
{
	/* Close file (if opened) */
	if (pFile != NULL)
	{
		if(eFmt==fmt_txt)
			fclose(pFile);
		else
			sf_close((SNDFILE*)pFile);
		pFile = NULL;
	}
	if(pacer)
		delete pacer;
	pacer = NULL;
}
struct CWaveHdr
{
	/* Wave header struct */
	char cMainChunk[4]; /* "RIFF" */
	uint32_t length; /* Length of file */
	char cChunkType[4]; /* "WAVE" */
	char cSubChunk[4]; /* "fmt " */
	uint32_t cLength; /* Length of cSubChunk (always 16 bytes) */
	uint16_t iFormatTag; /* waveform code: PCM */
	uint16_t iChannels; /* Number of channels */
	uint32_t iSamplesPerSec; /* Sample-rate */
	uint32_t iAvgBytesPerSec;
	uint16_t iBlockAlign; /* Bytes per sample */
	uint16_t iBitsPerSample;
	char cDataChunk[4]; /* "data" */
	uint32_t iDataLength; /* Length of data */
};

sf_count_t  sf_writef(SNDFILE *sndfile, short *ptr, sf_count_t frames)
{
	return sf_writef_short(sndfile, ptr, frames);
}

sf_count_t  sf_writef(SNDFILE *sndfile, float *ptr, sf_count_t frames)
{
	return sf_writef_float(sndfile, ptr, frames);
}

sf_count_t  sf_writef(SNDFILE *sndfile, double *ptr, sf_count_t frames)
{
	return sf_writef_double(sndfile, ptr, frames);
}

CSoundFileOut::CSoundFileOut()
{
}

CSoundFileOut::~CSoundFileOut()
{
	Close();
}

void
CSoundFileOut::SetDev(int iNewDevice)
{
	if (dev != iNewDevice)
	{
		dev = iNewDevice;
		device_changed = true;
	}
}

void
CSoundFileOut::SetDev(const string& s)
{
	for(size_t i=0; i<files.size(); i++)
	{
		if(files[i] == s)
		{
			SetDev(i);
			return;
		}
	}
	files.push_back(s);
	SetDev(files.size()-1);
}

void
CSoundFileOut::Init(int, bool, int iChannels)
{
	string s = files[dev];
	string ext;
	size_t p = s.rfind('.');
	if (p != string::npos)
		ext = s.substr(p + 1);
	channels = iChannels;
	SF_INFO sfinfo;
	sfinfo.samplerate = 48000;
	sfinfo.channels = channels;
	if(ext=="wav")
	{
		sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	}
	else if(ext=="txt")
	{
		sfinfo.format = SF_FORMAT_MAT4 | SF_FORMAT_PCM_16;
	}
	else if(ext=="raw")
	{
		sfinfo.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16;
	}
	pFile = sf_open(s.c_str(), SFM_WRITE, &sfinfo);
}

void
CSoundFileOut::Close()
{
	if(pFile==NULL)
		return;
	sf_close(pFile);
	pFile = NULL;
}

bool
CSoundFileOut::Write(vector<_SAMPLE>& data)
{
	if (pFile == NULL)
		return true;
	(void)sf_writef(pFile, (short int*)&data[0], data.size()/channels);
	return false;
}
