/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Decription:
 * Read a file at the correct rate
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

#include "audiofilein.h"
#ifdef _WIN32
# include <windows.h>
#endif
#include <iostream>
#ifdef HAVE_LIBSNDFILE
# include <sndfile.h>
#endif
#include <cstdlib>
#include <cstring>

CAudioFileIn::CAudioFileIn(): CSoundInInterface(), pFileReceiver(NULL),
        strInFileName(), eFmt(fmt_other),
        iFileSampleRate(0), iFileChannels(1), pacer(NULL),buffer(NULL), iBufferSize(0)
{
}

CAudioFileIn::~CAudioFileIn()
{
	if(buffer!=NULL)
		delete[] buffer;
    Close();
}

void
CAudioFileIn::SetFileName(const string& strFileName)
{
    strInFileName = strFileName;
    string ext;
    size_t p = strInFileName.rfind('.');
    if (p != string::npos)
        ext = strInFileName.substr(p+1);
	eFmt = fmt_other;
    if (ext == "txt") eFmt = fmt_txt;
    if (ext == "TXT") eFmt = fmt_txt;
    if (ext.substr(0,2) == "iq") eFmt = fmt_raw_stereo;
    if (ext.substr(0,2) == "IQ") eFmt = fmt_raw_stereo;
    if (ext.substr(0,2) == "if") eFmt = fmt_raw_stereo;
    if (ext.substr(0,2) == "IF") eFmt = fmt_raw_stereo;
    if (ext == "pcm") eFmt = fmt_raw_mono;
    if (ext == "PCM") eFmt = fmt_raw_mono;
    switch (eFmt)
    {
    case fmt_raw_stereo:
        iFileChannels = 2;
        if (ext.length() == 4)
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
CAudioFileIn::Init(int iNewBufferSize, _BOOLEAN bNewBlocking)
{
    if (pFileReceiver != NULL)
        return;

    /* Check previously a file was being used */
    Close();

	if(buffer!=NULL)
	{
		delete[] buffer;
		buffer=NULL;
	}

	iBufferSize = iNewBufferSize;
	buffer = new short[iBufferSize];

#ifdef HAVE_LIBSNDFILE
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    switch (eFmt)
    {
    case fmt_txt:
        pFileReceiver = fopen(strInFileName.c_str(), "r");
        break;
    case fmt_raw_mono:
    case fmt_raw_stereo:
        sfinfo.samplerate = iFileSampleRate;
        sfinfo.channels = iFileChannels;
        sfinfo.format = SF_FORMAT_RAW|SF_FORMAT_PCM_16|SF_ENDIAN_LITTLE;
        pFileReceiver = (FILE*)sf_open(strInFileName.c_str(), SFM_READ, &sfinfo);
        if (pFileReceiver==NULL)
            throw CGenErr(sf_strerror(0));
        break;
    case fmt_other:
        pFileReceiver = (FILE*)sf_open(strInFileName.c_str(), SFM_READ, &sfinfo);
        if (pFileReceiver != NULL)
        {
            iFileChannels = sfinfo.channels;
            iFileSampleRate = sfinfo.samplerate;
            int oversample_factor = SOUNDCRD_SAMPLE_RATE / iFileSampleRate;
            /* we can only cope with inter submultiples */
            if (SOUNDCRD_SAMPLE_RATE != oversample_factor*iFileSampleRate)
                throw CGenErr("unsupported sample rate in input file");
        }
        break;
    }
#else
    if (eFmt==fmt_txt)
        pFileReceiver = fopen(strInFileName.c_str(), "r");
    else
        pFileReceiver = fopen(strInFileName.c_str(), "rb");
#endif
    /* Check for error */
    if (pFileReceiver == NULL)
    {
        throw CGenErr("The file " + strInFileName + " could not be openned");
    }

    if (bNewBlocking)
    {
        double interval = double(iNewBufferSize/2) / double(SOUNDCRD_SAMPLE_RATE);
        pacer = new CPacer(uint64_t(1e9*interval));
    }
}

_BOOLEAN
CAudioFileIn::Read(CVector<short>& psData)
{
    int iFrames = psData.Size()/2;
    int i;
    if (pacer) pacer->wait();

    if (pFileReceiver == NULL)
    {
        return TRUE;
    }

    if (eFmt==fmt_txt)
    {
        for (i = 0; i < iFrames; i++)
        {
            float tIn;
            if (fscanf(pFileReceiver, "%e\n", &tIn) == EOF)
            {
                /* If end-of-file is reached, stop simulation */
                return FALSE;
            }
            psData[2*i] = (short)tIn;
            psData[2*i+1] = (short)tIn;
        }
        return FALSE;
    }
#ifdef HAVE_LIBSNDFILE
	sf_count_t c = sf_readf_short((SNDFILE*)pFileReceiver, buffer, iFrames);
	if (c!=iFrames)
	{
		/* rewind */
		sf_seek((SNDFILE*)pFileReceiver, 0, SEEK_SET);
		c = sf_readf_short((SNDFILE*)pFileReceiver, buffer, iFrames);
	}
#else
    if (fread(buffer, sizeof(short), size_t(iFileChannels*iFrames), pFileReceiver) == size_t(0))
    {
        rewind(pFileReceiver);
        fread(buffer, sizeof(short), size_t(iFileChannels*iFrames), pFileReceiver);
    }
#endif
    int oversample_factor = SOUNDCRD_SAMPLE_RATE / iFileSampleRate;
    if (iFileChannels==2)
    {
        for (i = 0; i < iFrames/oversample_factor; i++)
        {
            for (int j = 0; j < oversample_factor; j++)
            {
                psData[2*i+j] = buffer[2*i];
                psData[2*i+1+j] = buffer[2*i+1];
            }
        }
    }
    else
    {
        for (i = 0; i < iFrames/oversample_factor; i++)
        {
            for (int j = 0; j < oversample_factor; j++)
            {
                psData[2*i+j] = buffer[i];
                psData[2*i+1+j] = buffer[i];
            }
        }
    }
    return FALSE;
}

void
CAudioFileIn::Close()
{
    /* Close file (if opened) */
    if (pFileReceiver != NULL)
    {
#ifdef HAVE_LIBSNDFILE
        if (eFmt==fmt_txt)
            fclose(pFileReceiver);
        else
            sf_close((SNDFILE*)pFileReceiver);
#else
        fclose(pFileReceiver);
#endif
        pFileReceiver = NULL;
    }
    if (pacer)
        delete pacer;
    pacer = NULL;
}
