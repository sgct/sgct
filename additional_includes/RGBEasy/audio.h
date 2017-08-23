/*******************************************************************************

Copyright Datapath Ltd. 2011.

File:    audio.h

Purpose: Definitions for a simple audio interface.


History:
         10 OCT 11    RL   Created.

*******************************************************************************/

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <pshpack1.h>

/******************************************************************************/

/* The calling convention of the AUDIO SDK callback functions. */

#define AUDIOCBKAPI __stdcall

/******************************************************************************/

/* The Audio load handle. */
typedef ULONG_PTR HAUDIODLL, *PHAUDIODLL;

/* The Audio capture handle. */
typedef ULONG_PTR HAUDIO, *PHAUDIO;

/******************************************************************************/

typedef enum _AUDIOCAPTURESTATE
{
   STOP,
   ACQUIRE,
}  AUDIOCAPTURESTATE, *PAUDIOCAPTURESTATE;

/******************************************************************************/

typedef enum _AUDIOCAPTURESOURCE
{
   AUDIOCAPTURESOURCE_SDI = 1,
   AUDIOCAPTURESOURCE_HDMI,
   AUDIOCAPTURESOURCE_ANALOGUE
}  AUDIOCAPTURESOURCE, *PAUDIOCAPTURESOURCE;

/******************************************************************************/

typedef enum _AUDIOCAPTURECHANNELS
{
   AUDIOCAPTURECHANNELS_STEREO = 2,
}  AUDIOCAPTURECHANNELS, *PAUDIOCAPTURECHANNELS;

/******************************************************************************/

typedef enum _AUDIOCAPTURESAMPLESPERSEC
{
   AUDIOCAPTURESAMPLESPERSEC_32000 = 32000,
   AUDIOCAPTURESAMPLESPERSEC_44100 = 44100,
   AUDIOCAPTURESAMPLESPERSEC_48000 = 48000,
   AUDIOCAPTURESAMPLESPERSEC_96000 = 96000,
}  AUDIOCAPTURESAMPLESPERSEC, *PAUDIOCAPTURESAMPLESPERSEC;

/******************************************************************************/

typedef enum _AUDIOCAPTURESAMPLEDEPTH
{
   AUDIOCAPTURESAMPLEDEPTH_16BPS = 16,
   AUDIOCAPTURESAMPLEDEPTH_24PS = 24,
}  AUDIOCAPTURESAMPLEDEPTH, *PAUDIOCAPTURESAMPLEDEPTH;

/******************************************************************************/

typedef struct tagAudioCaps
{
   ULONG                      Size;

   AUDIOCAPTURECHANNELS       Channels;
   AUDIOCAPTURESAMPLESPERSEC  SamplesPerSec;
   AUDIOCAPTURESAMPLEDEPTH    SampleDepth;

} AUDIOCAPS, *PAUDIOCAPS;

/******************************************************************************/

typedef struct tagAudioData
{
   ULONG       Size;

   ULONGLONG   StartTime;     /* Captured data start time. */
   ULONGLONG   EndTime;       /* Captured data end time. */
   
   PVOID       PBuffer;       /* Pointer to the PCM bits. */

} AUDIODATA, *PAUDIODATA;

typedef void (AUDIOCBKAPI AUDIOCAPTUREDFN) (
   HAUDIO               hAudio,        /* Handle to the hAudio Capture. */
   PAUDIODATA           pAudioData,    /* Further data information. */
   ULONG_PTR            pUserData );   /* Application defined context. */
typedef AUDIOCAPTUREDFN *PAUDIOCAPTUREDFN;

/******************************************************************************/

#include <poppack.h>

/******************************************************************************/

#endif //_AUDIO_H_

/******************************************************************************/
