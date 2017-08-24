/*******************************************************************************

Copyright Datapath Ltd. 2011, 2012.

Purpose: Function declarations for a simple Audio interface.

History:
         12 OCT 12    RL   Created.

*******************************************************************************/

#define RGBAPI __stdcall

#ifndef API
   #ifdef __cplusplus
      #define API(type,modifier,name,args) \
         extern "C" __declspec(dllimport) type modifier name args;
   #else
      #define API(type,modifier,name,args) \
         __declspec(dllimport) type modifier name args;
   #endif
#endif

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioIsAudioSupported, (
   unsigned long  input,
   signed long    *pBIsSupported ))
/*
 * Summary: Determine if an input is audio capable.
 *
 * Args:    input IN
 *             Specifies the input to query.
 *
 *          pBIsSupported IN/OUT
 *             If TRUE, the input supports audio.
 *
 * Return:  RGBAudioIsAudioSupported either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioIsHDMISupported, (
   unsigned long  input,
   signed long    *pBIsSupported ))
/*
 * Summary: Determine HDMI audio capablility.
 *
 * Args:    input IN
 *             Specifies the input to query.
 *
 *          pBIsSupported IN/OUT
 *             If TRUE, the input supports the capability.
 *
 * Return:  RGBAudioIsHDMISupported either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioIsSDISupported, (
   unsigned long  input,
   signed long    *pBIsSupported ))
/*
 * Summary: Determine SDI audio capablility.
 *
 * Args:    input IN
 *             Specifies the input to query.
 *
 *          pBIsSupported IN/OUT
 *             If TRUE, the input supports the capability.
 *
 * Return:  RGBAudioIsSDISupported either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioIsAnalogueSupported, (
   unsigned long  input,
   signed long    *pBIsSupported ))
/*
 * Summary: Determine analogie audio capablility.
 *
 * Args:    input IN
 *             Specifies the input to query.
 *
 *          pBIsSupported IN/OUT
 *             If TRUE, the input supports the capability
 *
 * Return:  RGBAudioIsAnalogueSupported either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioGetCapabilitiesCount, (
   unsigned long input,
   unsigned long *pCount ))
/*
 * Summary: Get the count for available format capabilities including SampleRate.
 *
 * Args:    input IN
 *              Specifies the input to query.
 *
 *          pCount IN Out
 *             Count of supported capabilities.
 *
 * Return:  AudioGetCapabilitiesCount either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioGetCapabilities, (
   unsigned long  input,
   unsigned long  index,
   PAUDIOCAPS     pCaps ))
/*
 * Summary: Get the index specific capability parameters.
 *          Use RGBAudioGetCapabilitiesCount to return the index total.
 *
 * Args:    input IN
 *             Specifies the input to query.
 *
 *          index IN
 *             Index member for capability.
 *
 *          pCaps IN OUT
 *             Pointer to the structure to fill with the format information. 
 *             The size field must be initialised before this function is 
 *             called.
 *
 * Return:  RGBAudioGetCapabilities either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioSetCapabilities, (
   unsigned long  input,
   unsigned long  index ))
/*
 * Summary: Set the index specific capability parameters.
 *          Use RGBAudioGetCapabilitiesCount to return the index total.
 *
 * Args:    input IN
 *              Specifies the input to set.
 *
 *          index IN
 *              Index member for capability returned from 
 *              RGBAudioGetCapabilities.
 *
 * Return:  RGBAudioSetCapabilities either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioOpenInput, (
   PAUDIOCAPTUREDFN     pNotifyFn,
   ULONG_PTR            pNotifyArg,
   unsigned long        input,
   PHAUDIO              phAudio ))
/*
 * Summary: Opens an Audio capture on the specified input.
 *
 * Args:    
 *          pNotifyFn IN
 *             Points to a user declared callback function, see Audio.H
 *             for further details.
 *
 *          pNotifyArg IN
 *             User defined argument that can be passed into the pNotifyFn
 *             by the notify thread.
 *          
 *          input IN
 *             Specifies the input to open. The input must be a value in the
 *             range 0 to ( numberOfInputs - 1 ). The number of inputs can be
 *             obtained by calling RGBGetNumberOfInputs.
 *
 *          phAudio IN/OUT
 *             Pointer to a variable that receives the handle that identifies
 *             the Audio capture.
 *
 * Return:  RGBAudioOpenInput either returns 0 if successful or an appropriate error
 *          value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioCloseInput, (
   HAUDIO  hAudio ))
/*
 * Summary: Closes an Audio capture.
 *
 * Args:    hAudio IN
 *             The Audio capture handle to be closed.
 *             hAudio is not valid after this call and must not be used again.
 *
 * Return:  RGBAudioCloseInput either returns 0 if successful or an appropriate error
 *          value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBAudioChainOutputBuffer, (
   HAUDIO         hAudio,
   unsigned long  ulNumberBytes,
   unsigned long  ulBufferPitch,
   PVOID          lpBuffer ))
/*
 * Summary : This function adds a user managed buffer into the driver. Multiple
 *           buffers can be used with additional calls to this function. When a
 *           filled buffer is returned to through the FrameCaptured callback it
 *           must be reinserted with another call to this function for it to be
 *           refilled.
 *
 * Args:    hAudio IN
 *             The Audio capture handle.
 *
 *          ulNumberBytes IN
 *             Size of the buffer in bytes. 
  *
 *          ulBufferPitch IN
 *             Buffer pitch in bytes. 
 *
 *          lpBuffer IN/OUT
 *             The buffer to be filled. 
 *
 * Return:  RGBAudioChainOutputBuffer either returns 0 if successful or an 
 *          appropriate error value.
 */

 /******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBAudioLoadOutputBuffer, (
   HAUDIO         hAudio,
   unsigned long  ulNumberBytes,
   unsigned long  ulBufferPitch,
   unsigned long  *pBytesWritten,
   PVOID          lpBuffer ))
/*
 * Summary : This function fills a user managed buffer with avalable data in 
 *           the driver. Zero is inserted as data if no audio is available.
 *
 * Args:    hAudio IN
 *             The Audio capture handle.
 *
 *          ulNumberBytes IN
 *             Size of the buffer in bytes. 
  *
 *          ulBufferPitch IN
 *             Buffer pitch in bytes. 
 *
 *          pBytesWritten IN/OUT
 *             For example, if lpBuffer is large this number is the limited byte 
 *             count available in the drivers cyclic buffer for reading.
 *
 *          lpBuffer IN/OUT
 *             The buffer to be filled. 
 *
 * Return:  RGBAudioLoadOutputBuffer either returns 0 if successful or an 
 *          appropriate error value.
 */

 /******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBAudioReleaseOutputBuffers, (
   HAUDIO   hAudio ))
/*
 * Summary : This function removes a user managed buffer in the driver. Multiple
 *           buffers lodged within the driver are removed. The buffer does not 
 *           return via the FrameCaptured callback.
 *
 * Args:    hAudio IN
 *             The Audio capture handle.
 *
 * Return:  RGBAudioReleaseOutputBuffers either returns 0 if successful or an 
 *          appropriate error value.
 */

 /******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioSetState, (
   HAUDIO            hAudio,
   AUDIOCAPTURESTATE state))
/*
 * Summary: Sets the current Audio capture state.
 *
 * Purpose: Sets the current capture state listed within AudioEasy.H.
 *          
 * Args:    hAudio IN
 *             The Audio capture handle.
 *
 *          state IN
 *             Requested current state.
 *
 *
 * Return:  RGBAudioSetState either returns 0 if successful or an
 *          appropriate error value.
 */

 /******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioGetState, (
   HAUDIO               hAudio,
   PAUDIOCAPTURESTATE   pState))
/*
 * Summary: Gets the current Audio capture state.
 *
 * Purpose: Determine the current capture state listed within AudioEasy.H.
 *          
 * Args:    hAudio IN
 *             The Audio capture handle.
 *
 *          pState OUT
 *             Pointer to the current state.
 *
 *
 * Return:  RGBAudioGetState either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/
/*                                DIGITAL                                     */
/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioSetDigitalChannelPair, (
   unsigned long uInput,
   unsigned long channel ) )

API ( unsigned long, RGBAPI,
RGBAudioGetDigitalChannelPair, (
   unsigned long uInput,
   unsigned long *pChannel ) )

API ( unsigned long, RGBAPI,
RGBAudioSetDigitalMute, (
   unsigned long uInput,
   unsigned long mute ) )

API ( unsigned long, RGBAPI,
RGBAudioGetDigitalMute, (
   unsigned long uInput,
   unsigned long *pMute ) )

/******************************************************************************/
/*                                BALANCED                                    */
/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioSetAnalogueBalancedGain, (
   unsigned long uInput,
   signed long  gain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueBalancedGain, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueBalancedGainMinimum, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueBalancedGainMaximum, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueBalancedGainDefault, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueBalancedGainScale, (
   unsigned long uInput,
   unsigned long *pScale ) )

API ( unsigned long, RGBAPI,
RGBAudioSetAnalogueBalancedGainBoost, (
   unsigned long uInput,
   unsigned long boost ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueBalancedGainBoost, (
   unsigned long uInput,
   unsigned long *pBoost ) )

API ( unsigned long, RGBAPI,
RGBAudioSetAnalogueBalancedMute, (
   unsigned long uInput,
   unsigned long mute ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueBalancedMute, (
   unsigned long uInput,
   unsigned long *pMute ) )


/******************************************************************************/
/*                               UNBALANCED                                   */
/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioSetAnalogueUnbalancedGain, (
   unsigned long uInput,
   signed long gain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueUnbalancedGain, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueUnbalancedGainMinimum, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueUnbalancedGainMaximum, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueUnbalancedGainDefault, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueUnbalancedGainScale, (
   unsigned long uInput,
   unsigned long *pScale ) )

API ( unsigned long, RGBAPI,
RGBAudioSetAnalogueUnbalancedMute, (
   unsigned long uInput,
   unsigned long mute ) )

API ( unsigned long, RGBAPI,
RGBAudioGetAnalogueUnbalancedMute, (
   unsigned long uInput,
   unsigned long *pMute ) )



/******************************************************************************/
/*                                   ADC                                      */
/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioSetADCGain, (
   unsigned long uInput,
   signed long  gain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetADCGain, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetADCGainMinimum, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetADCGainMaximum, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetADCGainDefault, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetADCGainScale, (
   unsigned long uInput,
   unsigned long *pScale ) )
   
API ( unsigned long, RGBAPI,
RGBAudioSetADCMute, (
   unsigned long uInput,
   unsigned long mute ) )

API ( unsigned long, RGBAPI,
RGBAudioGetADCMute, (
   unsigned long uInput,
   unsigned long *pMute ) )

/******************************************************************************/
/*                                Line Out                                    */
/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAudioIsLineOutSupported, (
   unsigned long uInput,
   signed long  *pBIsSupported ) )

API ( unsigned long, RGBAPI,
RGBAudioSetLineOutSource, (
   unsigned long uInput,
   AUDIOCAPTURESOURCE source ) )

API ( unsigned long, RGBAPI,
RGBAudioGetLineOutSource, (
   unsigned long uInput,
   AUDIOCAPTURESOURCE *pSource ) )

API ( unsigned long, RGBAPI,
RGBAudioSetLineOutGain, (
   unsigned long uInput,
   signed long  gain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetLineOutGain, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetLineOutGainMinimum, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetLineOutGainMaximum, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetLineOutGainDefault, (
   unsigned long uInput,
   signed long *pGain ) )

API ( unsigned long, RGBAPI,
RGBAudioGetLineOutGainScale, (
   unsigned long uInput,
   unsigned long *pScale ) )

API ( unsigned long, RGBAPI,
RGBAudioSetLineOutMute, (
   unsigned long uInput,
   unsigned long mute ) )

API ( unsigned long, RGBAPI,
RGBAudioGetLineOutMute, (
   unsigned long uInput,
   unsigned long *pMute ) )

API ( unsigned long, RGBAPI,
RGBAudioSaveConfigurationToRegistry, (
   unsigned long input ) )

API ( unsigned long, RGBAPI,
RGBAudioDeleteRegistryConfiguration, (
   unsigned long input ) )

#ifdef API
#undef API
#endif

/******************************************************************************/
