/******************************************************************************\
 *
 * Copyright (c) 2019
 *
 * Author(s):
 *  Julian Cable
 *
 * Description:
 *  adapt fdk decoder to Dream
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

#ifndef FDK_AAC_CODEC_H_
#define FDK_AAC_CODEC_H_

#include "../Parameter.h"
#include <fdk-aac/aacdecoder_lib.h>
#include <fdk-aac/aacenc_lib.h>

enum EDecError { DECODER_ERROR_OK, DECODER_ERROR_CRC, DECODER_ERROR_CORRUPTED, DECODER_ERROR_UNKNOWN };

class FdkAacCodec
{
public:
    FdkAacCodec();
    virtual ~FdkAacCodec();
    virtual std::string DecGetVersion();
    virtual bool DecOpen(const CAudioParam& AudioParam, int& iAudioSampleRate);
    virtual EDecError Decode(const vector<uint8_t>& audio_frame, uint8_t aac_crc_bits, CVector<_REAL>& left,  CVector<_REAL>& right);
    virtual void DecClose();
protected:
    HANDLE_AACDECODER hDecoder;
    int16_t decode_buf[13840];
};

#endif // FDK_AAC_CODEC_H_
