/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):  Julian Cable, Ollie Haffenden, Andrew Murphy
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
/* general audio library wrapper */

#ifndef __AU_CHANNEL_H
#define __AU_CHANNEL_H


#ifdef __cplusplus
extern "C" {
#endif
  
  typedef void* hAudioChannel;

  typedef struct {
    char date[20];
    char versionNo[20];
  } AuChanLibInfo;

  typedef struct {
    int   card;       /* alsa only */
    int   device;     /* alsa only */
    float level;
    int   inputSource;
    int   outputDrain;
  } AuChanInfoRT;

  typedef struct {
    int   bitrate;
    int   ID;
    int   flags;
    int   blockSize;
    int   framesPerBlock;
    int   codecDelay;
  } AuChanInfoMP3;

  typedef struct {
    int   bitrate;
    int   ID;
    int   flags;
    int   profile;
    int   channelConfiguration;
    int   nPrograms;
    int   codecDelay;
  } AuChanInfoAAC;

  typedef union {
    AuChanInfoRT  rt;
    AuChanInfoMP3 mp3;
    AuChanInfoAAC aac;
  } typeSpecInfo;

  typedef struct {
    int    valid;
    int    bitsPerSample;
    int    sampleRate;
    int    nChannels;
    long   nSamples;
    int    isLittleEndian;
    double fpScaleFactor;    /**< for scaling floating point values */
    typeSpecInfo typeInfo;
  } AuChanInfo;

  /* error codes */

  typedef enum {
    AU_CHAN_OK=0,
    /* for open:*/
    AU_CHAN_FILE_INEXISTENT,
    AU_CHAN_OUT_OF_MEMORY,
    AU_CHAN_UNSUPPORTED_SETTINGS,
    AU_CHAN_BAD_SETTINGS_COMBINATION,
    AU_CHAN_INFO_INVALID,
    AU_CHAN_OPEN_FAILED,

    /* for read/write/seek */
    AU_CHAN_READ_ERROR,
    AU_CHAN_WRITE_ERROR,
    AU_CHANNEL_READ_EXHAUSTED,
    AU_CHAN_SEEK_ERROR,
    
    AU_CHAN_UNIMPLEMENTED_FEATURE,
    AU_CHAN_GENERAL_ERROR
  } AuChanError;
  
  typedef enum { 
    AU_CHAN_READ,
    AU_CHAN_READ_STDIN,
    AU_CHAN_WRITE, 
    AU_CHAN_WRITE_STDOUT
  } AuChanMode;
  
  typedef enum {
    TYPE_AUTODETECT = 0,
    TYPE_AIFF,
    TYPE_AIFC,
    TYPE_RIFF,
    TYPE_SND,
    TYPE_PCM,
    TYPE_DVD,     /**< DVD raw data format (6 channels, 24 bits) */
    TYPE_MP3,     /**< MPEG-LAYER3 bitstream data in RIFF */
    TYPE_AAC,     /**< MPEG AAC bitstream data in RIFF */

    TYPE_VOID,
    TYPE_RT
  } AuChanType;
  
#define OUT_SPEAKER 1
#define OUT_HEADPHONE 2
#define OUT_LINE 4

#define IN_MIC 1
#define IN_LINE 2
#define IN_CD 4

  enum {
    MPEGLAYER3_ID_MPEG = 1,
    MPEGLAYER3_FLAG_PADDING_ISO = 2
  };

  AuChanError AuChannelOpen ( hAudioChannel* audioChannel,
                              const char* filename,
                              AuChanMode mode,
                              AuChanType* type,
                              AuChanInfo* info);
                              
  AuChanError AuChannelSeek ( hAudioChannel audioChannel, int nSamples );
                              
  void AuChannelClose (hAudioChannel audioChannel);

  AuChanError AuChannelWriteLong( hAudioChannel audioChannel,
                                  const long* samples,
                                  int nBytes,
                                  int* written);

  AuChanError AuChannelWriteShort( hAudioChannel audioChannel,
                                   const short* samples,
                                   int nSamples,
                                   int* written);

  AuChanError AuChannelWriteFloat( hAudioChannel audioChannel,
                                   const float* samples,
                                   int nSamples,
                                   int* written);

  AuChanError AuChannelWriteBytes( hAudioChannel audioChannel,
                                   const char* bytes,
                                   int nBytes,
                                   int* written);

  AuChanError AuChannelReadShort( hAudioChannel audioChannel,
                                  short* samples,
                                  int nSamples,
                                  int* written);

  AuChanError AuChannelReadFloat( hAudioChannel audioChannel,
                                  float* samples,
                                  int nSamples,
                                  int* read);

  AuChanError AuChannelReadBytes( hAudioChannel audioChannel,
                                   char* bytes,
                                   int nBytes,
                                   int* read);
  
  AuChanError AuChannelSetLevel( hAudioChannel audioChannel,
                                 int volume);
  
  AuChanError AuChannelParseFormatString(const char* format,
                                         AuChanInfo* info,
                                         AuChanType* type);

  void AuChannelGetLibInfo (AuChanLibInfo* libInfo);
  

#ifdef __cplusplus
} /* extern "C"                                  } */
#endif

#endif /* AU_CHANNEL_H */
