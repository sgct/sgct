/*******************************************************************************

Copyright Datapath Ltd. 2012, 2013.

File:    AUDIOCAP.H

Purpose: Declarations for a shared mode audio capture.

History:
         17 NOV 12    RL   Created.
         13 MAR 13    RL   Added LaunchAdvancedDefaultSpeakerPropertiesDlg.

*******************************************************************************/

/* The audio input handle returned by OpenSharedAudio. */
typedef ULONG_PTR HAUDIOINPUT, *PHAUDIOINPUT;

HRESULT
OpenSharedAudio (
   ULONG        input,
   PHAUDIOINPUT pHandle );

HRESULT
CloseSharedAudio (
   HAUDIOINPUT handle );

HRESULT
GetAudioFormatIndex (
   HAUDIOINPUT handle,
   ULONG       *pIndex );

VOID 
LaunchAdvancedDefaultSpeakerPropertiesDlg ( );

/******************************************************************************/