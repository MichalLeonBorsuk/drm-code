/******************************************************************************\
 *
 * Copyright (c) 2013
 *
 * Author(s):
 *  David Flamand
 *
 * Description:
 *  Audio codec base class
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

#include "AudioCodec.h"
#include "null_codec.h"
#include "aac_codec.h"
#include "opus_codec.h"
#ifdef HAVE_LIBFDK_AAC
# include "fdk_aac_codec.h"
#endif

CAudioCodec::~CAudioCodec() {

}

vector<CAudioCodec*>
CAudioCodec::CodecList;

int
CAudioCodec::RefCount = 0;

void
CAudioCodec::InitCodecList()
{
	if (CodecList.size() == 0)
	{
		/* Null codec, MUST be the first */
		CodecList.push_back(new NullCodec);

		/* AAC */
#ifdef HAVE_LIBFDK_AAC
        CodecList.push_back(new FdkAacCodec);
#else
        CodecList.push_back(new AacCodec);
#endif

		/* Opus */
		CodecList.push_back(new OpusCodec);
	}
	RefCount ++;
}

void
CAudioCodec::UnrefCodecList()
{
	RefCount --;
	if (!RefCount)
	{
		while (CodecList.size() != 0)
		{
			delete CodecList.back();
			CodecList.pop_back();
		}
	}
}

CAudioCodec*
CAudioCodec::GetDecoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr)
{
    const int size = int(CodecList.size());
	for (int i = 1; i < size; i++)
        if (CodecList[unsigned(i)]->CanDecode(eAudioCoding))
            return CodecList[unsigned(i)];
	/* Fallback to null codec */
    return bCanReturnNullPtr ? nullptr : CodecList[0]; // ie the null codec
}

CAudioCodec*
CAudioCodec::GetEncoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr)
{
	const int size = CodecList.size();
	for (int i = 1; i < size; i++)
		if (CodecList[i]->CanEncode(eAudioCoding))
			return CodecList[i];
	/* Fallback to null codec */
    return bCanReturnNullPtr ? nullptr : CodecList[0]; // ie the null codec
}

void
CAudioCodec::extractSamples(size_t iNumAudioFrames, size_t iNumHigherProtectedBytes, CVectorEx<_BINARY>& vecInputData, vector< vector<uint8_t> >& audio_frame, vector<uint8_t>& aac_crc_bits)
{
    /* Higher-protected part */
    for (size_t i = 0; i < iNumAudioFrames; i++)
    {
        /* Extract higher protected part bytes (8 bits per byte) */
        for (size_t j = 0; j < iNumHigherProtectedBytes; j++)
            audio_frame[i][j] = _BINARY(vecInputData.Separate(8));

        /* Extract CRC bits (8 bits) */
        aac_crc_bits[i] = _BINARY(vecInputData.Separate(8));
    }

    /* Lower-protected part */
    for (size_t i = 0; i < iNumAudioFrames; i++)
    {
        /* First calculate frame length, derived from higher protected
           part frame length and total size */
        const size_t iNumLowerProtectedBytes =
            audio_frame[i].size() - iNumHigherProtectedBytes;

        /* Extract lower protected part bytes (8 bits per byte) */
        for (size_t j = 0; j < iNumLowerProtectedBytes; j++)
        {
            audio_frame[i][iNumHigherProtectedBytes + j] =
                _BINARY(vecInputData.Separate(8));
        }
    }
}
