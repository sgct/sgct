/*******************************************************************************

Copyright Datapath Ltd. 2009, 2017.

Purpose: Function declarations for a simple RGB interface.

History:
         19 DEC 07    SB   Created.
         01 FEB 08    SB   Added Value Changed callback.
         12 FEB 08    SB   Added colour balance functions.
         27 FEB 08    SB   Added RGBGetHorPositionDefault,
                           RGBGetHorScaleDefault, RGBGetCaptureWidthDefault,
                           RGBGetVerPositionDefault, RGBGetCaptureHeightDefault.
         28 MAR 08   MJE   Added RGBSetDrawNoSignalFn and 
                           RGBSetDrawInvalidSignalFn.
         03 APR 08   MJE   Added RGBGetModeInfo.
         13 AUG 08    SB   Added RGBTestCropping.
         09 DEC 08   MJE   Added RGBSetOutputSize, RGBGetOutputSize,
                           RGBSetDownScaling and RGBGetDownScaling.
         12 MAR 09   MJE   Added RGBSetOutputBuffers.
         01 APR 09   MJE   Replaced RGBSetOutputBuffers with RGBUseOutputBuffers
                           and RGBChainOutputBuffer.
         02 OCT 09    SB   Added RGBSetDeinterlace, RGBGetDeinterlace,
                           RGBIsYUVSupported, RGBIsDeinterlaceSupported,
                           RGBGetVideoStandard, RGBSetVideoStandard,
                           RGBTestVideoStandard, RGBGetSaturationMinimum,
                           RGBGetSaturationMaximum, RGBGetSaturationDefault,
                           RGBGetSaturation, RGBSetSaturation, RGBGetHueMinimum,
                           RGBGetHueMaximum, RGBGetHueDefault,RGBGetHue,
                           RGBSetHue.
         11 FEB 10    SB   Added RGBGetInputSignalType, RGBInputIsVGASupported,
                           RGBInputIsDVISupported, RGBInputIsComponentSupported,
                           RGBInputIsCompositeSupported,
                           RGBInputIsSVideoSupported.
         23 FEB 10    SB   Corrected definition for RGBSetDeinterlace and
                           RGBGetDeinterlace.
         02 MAR 10    SB   Added RGBGetCroppingOverscan.
         10 JUN 10   MJE   Added OSD functions. Changed user data passed through
                           callbacks to be a ULONG_PTR.
         11 JUN 10   MJE   Add ASCII versions of the OSD functions.
         08 SEP 10    RL   Added RGBGetInputInfo.
         02 NOV 10    SB   Added RGBLoadEx and RGBDetectInput.
         10 DEC 10    SB   Added RGBSetRotation, RGBGetRotation and
                           RGBIsRotationSupported.
         30 JUN 11    TM   Added descriptions to RGBSetRotation, RGBGetRotation
                           and RGBIsRotationSupported.
         25 JUL 11    SB   Added RGBInputIsSDISupported and
                           RGBInputIsDualLinkDVISupported.
                           Added Equalisation Functions.
         28 JUL 11    RL   Added RGBSetFrameCapturedFnEx.
         17 JAN 12    RL   Added the FriendlyName to RGBGetInputInfo requiring 
                           RGBGetInputInfoA and RGBGetInputInfoW calls.
         24 JAN 12    SB   Fixed incorrect declarations of RGBGetInputInfoA and 
                           RGBGetInputInfoW.
         06 FEB 12   MJE   Added RGBSetOSDOwnerDrawnFn, RGBSetOSDArea, and
                           RGBGetOSDArea. 
         25 APR 12    SB   Added RGBChainOutputBufferEx.
         06 JUN 12    SB   Added RGBSetNoSignalBackground and
                           RGBSetInvalidSignalBackground.
         20 JUN 12    JL   Added RGBAddModeToModeStore and
                           RGBDeleteModeFromModeStore.
         11 JUL 12    JL   Added RGBIsEDIDSupported, RGBGetEDID, RGBSetEDID,
                           RGBIsEDIDEnabled, and RGBResetEDID.
         03 OCT 12    SB   Added RGBSetSignalDetectionMethod,
                           RGBSetSignalDetectionMethod and
                           RGBIsSignalDetectionMethodSupported.
         14 NOV 12    OM   Added uniform interface to support NVidia GPUDirect
                           and AMD DirectGMA.
         14 MAR 13    JE   Added RGBSetColourDomain() and RGBGetColourDomain().
         08 MAY 13    JL   Added RGBGetColourDomainDefault.
         18 JUL 13    OM   Added OSD bitmap functionality.
         10 JUN 14    SB   Added RGBSetMediaSampleCapturedFn and
                           RGBChainMediaSample to support DGCMEDIASAMPLE
                           buffers.
         17 JUN 14    SB   Added RGBGetLiveStream and RGBSetLiveStream.
         09 JUL 14    SB   Added RGBInputIsMediaSampleTypeSupported.
         14 JUL 14    JL   Added functions for controlling encoder parameters.
         24 NOV 14    SB   Added RGBInputGetMaximumPixelClock.
         05 FEB 15    SB   Added RGBSetLinkRate, RGBGetLinkRate and 
                           RGBInputIsLinkRateSupported.
         26 OCT 15    JL   Added RGBEnableInputSnapshot,
                           RGBIsInputSnapshotEnabled and RGBInputGetStatus.
         16 JUN 16    JL   Added support for ganging.
         04 JAN 17    JL   Added support for enabling/disabling of HDCP
         29 MAR 17    SB   Added RGBInputIsHDMISupported,
                           RGBInputIsDisplayPortSupported and
                           RGBInputGetDeviceType.

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
RGBLoad, (
   PHRGBDLL phRGBDLL ))
/*
 * Summary: Loads the error of the RGB driver. This should be the first
 *          function called by an application to determine if RGB is available.
 *
 * Args:    phRGBDLL IN/OUT
 *             Pointer to a variable that receives a handle to the RGB driver.
 *
 * Return:  RGBLoad either returns 0 if the driver was loaded successfully or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBLoadEx, (
   PHRGBDLL    phRGBDLL,
   CAPTURECARD captureCard ))
/*
 * Summary: Loads the error of the RGB driver. This should be the first
 *          function called by an application to determine if RGB is available.
 *
 * Args:    phRGBDLL IN/OUT
 *             Pointer to a variable that receives a handle to the RGB driver.
 *          captureCard IN/OUT
 *             Specifies which capture driver to load.
 *
 * Return:  RGBLoad either returns 0 if the driver was loaded successfully or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBFree, (
   HRGBDLL hRGBDLL ))
/*
 * Summary: Unloads and free's the resources used by the RGB driver. This should
 *          be the last function called by an application following a successful
 *          RGBLoad.
 *
 * Args:    hRGBDLL IN
 *             The RGB handle returned by RGBLoad.
 *
 * Return:  RGBFree either returns 0 if the driver was loaded
 *          successfully or an appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCaptureCard, (
   PCAPTURECARD   pCaptureCard ))
/*
 * Summary: Returns the load error of the RGB driver.
 *
 * Args:    None
 *
 * Return:  RGBGetLoadError either returns 0 if the driver was loaded
 *          successfully or an appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetNumberOfInputs, (
   unsigned long  *pNumberOfInputs ))
/*
 * Summary: Returns the number of RGB inputs.
 *
 * Args:    pNumberOfInputs IN/OUT
 *             Pointer to a variable that receives the number of inputs if the
 *             call is successful.
 *
 * Return:  RGBGetNumberOfInputs either returns 0 if successful or an
 *          appropriate error value.
 */

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBGetInputInfo   RGBGetInputInfoW
#else
#define RGBGetInputInfo   RGBGetInputInfoA
#endif

API ( unsigned long, RGBAPI,
RGBGetInputInfoA, (
   unsigned long   uInput,
   PRGBINPUTINFOA  pInputInfo ))

API ( unsigned long, RGBAPI,
RGBGetInputInfoW, (
   unsigned long   uInput,
   PRGBINPUTINFOW  pInputInfo ))
/*
 * Summary: Returns information including version numbers and identifiers 
 *          for a specified input.
 *
 * Args:    uInput IN
 *             Specifies the input to query.
 *
 *          pInputInfo IN OUT
 *             Pointer to the structure to fill with the input information. 
 *             The size field must be initialised before this function is 
 *             called.
 *
 * Return:  RGBGetInputInfo either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBOpenInput, (
   unsigned long  uInput,
   PHRGB          pHRGB ))
/*
 * Summary: Opens an RGB capture on the specified input.
 *
 * Args:    uInput IN
 *             Specifies the input to open. The input must be a value in the
 *             range 0 to ( numberOfInputs - 1 ). The number of inputs can be
 *             obtained by calling RGBGetNumberOfInputs.
 *
 *          pHRGB IN/OUT
 *             Pointer to a variable that receives the handle that identifies
 *             the RGB capture.
 *
 * Return:  RGBOpenInput either returns 0 if successful or an appropriate error
 *          value.
 */

API ( unsigned long, RGBAPI,
RGBCloseInput, (
   HRGB  hRGB ))
/*
 * Summary: Closes an RGB capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle to be closed.
 *             hRGB is not valid after this call and must not be used again.
 *
 * Return:  RGBCloseInput either returns 0 if successful or an appropriate error
 *          value.
 */

API ( unsigned long, RGBAPI,
RGBCloseInputs, (
   PHRGB pHRGBArray,
   ULONG uInputs ))
/*
 * Summary: Closes all the RGB captures specified.
 *
 * Purpose:
 *          This closes all the specified RGB captures. If no error is returned
 *          all the RGB captures will have finished by the time this function 
 *          returns. 
 *          
 * Args:    pHRGBArray IN OUT
 *             An array of RGB captures handles to be closed.
 *             Captures which are successfully closed are set to NULL in the
 *             returning array. If an error is returned there maybe captures
 *             left open. 
 *
 *          uInputs IN
 *             The number of RGB capture handles to be closed.
 *
 * Return:  RGBCloseInputs either returns 0 if successful or an appropriate
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBDetectInput, (
   HRGB           hRGB ))
/*
 * Summary: Detects the video mode of the specfied RGB capture.
 *
 * Purpose:
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *
 * Return:  RGBGetInput either returns 0 if successful or an appropriate
 *          error value.
 */

API ( unsigned long, RGBAPI,
RGBSetInput, (
   HRGB           hRGB,
   unsigned long  uInput ))
/*
 * Summary: Sets the input of the specified RGB capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          uInput IN
 *             Specifies the new input to open. The input must be a value in the
 *             range 0 to ( numberOfInputs - 1 ). The number of inputs can be
 *             obtained by calling RGBGetNumberOfInputs.
 *
 *
 * Return:  RGBSetInput either returns 0 if successful or an appropriate
 *          error value.
 */

API ( unsigned long, RGBAPI,
RGBGetInput, (
   HRGB           hRGB,
   unsigned long  *pUInput ))
/*
 * Summary: Gets the current input of the specfied RGB capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          uInput IN/OUT
 *             Pointer to a variable that receives the current of input.
 *
 *
 * Return:  RGBGetInput either returns 0 if successful or an appropriate
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetWindow, (
   HRGB  hRGB,
   HWND  hWnd ))
/*
 * Summary: Sets the window in which specified RGB capture is to be displayed.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          hWnd IN
 *             The window handle.
 *
 * Return:  RGBSetWindow either returns 0 if successful or an appropriate
 *          error value.
 */

API ( unsigned long, RGBAPI,
RGBGetWindow, (
   HRGB  hRGB,
   HWND  *pHWnd ))
/*
 * Summary: Returns the window handle of the window in which the specified RGB
 *          capture is being displayed.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          hWnd IN/OUT
 *             Pointer to a variable that receives the window handle.
 *
 * Return:  RGBGetWindow either returns 0 if successful or an appropriate
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBResetCapture, (
   HRGB hRGB ))

/*
 * Summary: Resets the capture parameters to their intial detected values.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *
 * Return:  RGBResetCapture either returns 0 if successful or an appropriate
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetHorPositionMinimum, (
   HRGB        hRGB,
   signed long *pHorPosition ))
/*
 * Summary: Returns the minimum value that can be set for the number of pixels
 *          between the horizontal sync and the beginning of capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHorPosition IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the number of pixels between the horizontal sync and the
 *             beginning of capture.
 *
 * Return:  RGBGetHorPositionMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetHorPositionMaximum, (
   HRGB        hRGB,
   signed long *pHorPosition ))
/*
 * Summary: Returns the maximum value that can be set for the number of pixels
 *          between the horizontal sync and the beginning of capture.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHorPosition IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the number of pixels between the horizontal sync and the
 *             beginning of capture.
 *
 * Return:  RGBGetHorPositionMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetHorPositionDefault, (
   HRGB        hRGB,
   signed long *pHorPosition ))
/*
 * Summary: Returns the default value for the number of pixels between the
 *          horizontal sync and the beginning of capture.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHorPosition IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             number of pixels between the horizontal sync and the
 *             beginning of capture.
 *
 * Return:  RGBGetHorPositionDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetHorPosition, (
   HRGB        hRGB,
   signed long *pHorPosition ))
/*
 * Summary: Returns the currently set value for the number of pixels between the
 *          horizontal sync and the beginning of capture.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHorPosition IN/OUT
 *             Pointer to a variable that receives the currently set value for
 *             the number of pixels between the horizontal sync and the
 *             beginning of capture.
 *
 * Return:  RGBGetHorPosition either returns 0 if successful or an appropriate
 *          error value.
 */

API ( unsigned long, RGBAPI,
RGBSetHorPosition, (
   HRGB        hRGB,
   signed long horPosition ))
/*
 * Summary: Sets the number of pixels between the horizontal sync and the
 *          beginning of capture.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          horPosition IN
 *             The number of pixels between the horizontal sync and the
 *             beginning of capture value to set.
 *
 * Return:  RGBSetHorPosition  either returns 0 if successful or an appropriate
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetHorScaleMinimum, (
   HRGB           hRGB,
   unsigned long  *pHorScale ))
/*
 * Summary: Returns the minimum value that can be set for the total number of
 *          pixels on a line.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHorPosition IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the total number of pixels on a line.
 *
 * Return:  RGBGetHorScaleMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetHorScaleMaximum, (
   HRGB           hRGB,
   unsigned long  *pHorScale ))
/*
 * Summary: Returns the maximum value that can be set for the total number of
 *          pixels on a line.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHorPosition IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the total number of pixels on a line.
 *
 * Return:  RGBGetHorScaleMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetHorScaleDefault, (
   HRGB           hRGB,
   unsigned long  *pHorScale ))
/*
 * Summary: Returns the default value for the total number of pixels on a line.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHorPosition IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             total number of pixels on a line.
 *
 * Return:  RGBGetHorScaleDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetHorScale, (
   HRGB           hRGB,
   unsigned long  *pHorScale ))
/*
 * Summary: Returns the currently set value for the total number of pixels on a
 *          line.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHorPosition IN/OUT
 *             Pointer to a variable that receives the currently set value for
 *             the total number of pixels on a line.
 *
 * Return:  RGBGetHorScale either returns 0 if successful or an appropriate
 *          error value.
 */

API ( unsigned long, RGBAPI,
RGBSetHorScale, (
   HRGB           hRGB,
   unsigned long  horScale ))
/*
 * Summary: Sets the total number of pixels on a line.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          horScale IN
 *             The total number of pixels on a line value to set.
 *
 * Return:  RGBSetHorScale either returns 0 if successful or an appropriate
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetCaptureWidthMinimum, (
   HRGB           hRGB,
   unsigned long  *pWidth ))
/*
 * Summary: Returns the minimum value that can be set for the number of pixels
 *          on each line that are to be captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pWidth IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the number of pixels on each line that are to be
 *             captured.
 *
 * Return:  RGBGetCaptureWidthMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCaptureWidthMaximum, (
   HRGB           hRGB,
   unsigned long  *pWidth ))
/*
 * Summary: Returns the maximum value that can be set for the number of pixels
 *          on each line that are to be captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pWidth IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the number of pixels on each line that are to be
 *             captured.
 *
 * Return:  RGBGetCaptureWidthMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCaptureWidthDefault, (
   HRGB           hRGB,
   unsigned long  *pWidth ))
/*
 * Summary: Returns the default value for the number of pixels on each line that
 *          are to be captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pWidth IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             number of pixels on each line that are to be
 *             captured.
 *
 * Return:  RGBGetCaptureWidthDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCaptureWidth, (
   HRGB           hRGB,
   unsigned long  *pWidth ))
/*
 * Summary: Returns the currently set value for the number of pixels on each
 *          line that are to be captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pWidth IN/OUT
 *             Pointer to a variable that receives the currently set value for
 *             the number of pixels on each line that are to be captured.
 *
 * Return:  RGBGetCaptureWidth either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBTestCaptureWidth, (
   HRGB           hRGB,
   unsigned long  width ))
/*
 * Summary: The RGB capture cards require the number of pixels on each line
 *          to be aligned to certain natural boundaries. This alignment value
 *          may not be the same for the different RGB capture cards. This
 *          function tests whether a value is correct for the RGB capture card
 *          in use.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          width IN
 *             The total number of pixels on a line value to test.
 *
 * Return:  RGBTestCaptureWidth either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetCaptureWidth, (
   HRGB           hRGB,
   unsigned long  width ))
/*
 * Summary: Sets the number of pixels on each line that are to be captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          width IN
 *             The number of pixels on each line that are to be captured value
 *             to set.
 *
 * Return:  RGBSetCaptureWidth either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetVerPositionMinimum, (
   HRGB        hRGB,
   signed long *pVerPosition ))
/*
 * Summary: Returns the minimum value that can be set for the number of lines
 *          between the vertical sync and the beginning of capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pVerPosition IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the number of lines between the vertical sync and the
 *             beginning of capture.
 *
 * Return:  RGBGetVerPositionMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetVerPositionMaximum, (
   HRGB        hRGB,
   signed long *pVerPosition ))
/*
 * Summary: Returns the maximum value that can be set for the number of lines
 *          between the vertical sync and the beginning of capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pVerPosition IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the number of lines between the vertical sync and the
 *             beginning of capture.
 *
 * Return:  RGBGetVerPositionMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetVerPositionDefault, (
   HRGB        hRGB,
   signed long *pVerPosition ))
/*
 * Summary: Returns the default value for the number of lines between the
 *          vertical sync and the beginning of capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pVerPosition IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             number of lines between the vertical sync and the
 *             beginning of capture.
 *
 * Return:  RGBGetVerPositionDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetVerPosition, (
   HRGB        hRGB,
   signed long *pVerPosition ))
/*
 * Summary: Returns the currently set value for the number of lines between the
 *          vertical sync and the beginning of capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pVerPosition IN/OUT
 *             Pointer to a variable that receives the currently set value for
 *             the number of lines between the vertical sync and the beginning
 *             of capture.
 *
 * Return:  RGBGetVerPosition either returns 0 if successful or an
 *          appropriate error value.
 */
 
API ( unsigned long, RGBAPI,
RGBSetVerPosition, (
   HRGB        hRGB,
   signed long verPosition ))
/*
 * Summary: Sets the number of lines between the vertical sync and the beginning
 *          of capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          verPosition IN
 *             The number of lines between the vertical sync and the beginning
 *             of capture value to set.
 *
 * Return:  RGBSetVerPosition either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetCaptureHeightMinimum, (
   HRGB           hRGB,
   unsigned long  *pHeight ))
/*
 * Summary: Returns the minimum value that can be set for the number of lines
 *          that are to be captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHeight IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the number of lines that are to be captured.
 *
 * Return:  RGBGetCaptureHeightMinimum either returns 0 if successful or an
 *          appropriate error value.
 */
 
API ( unsigned long, RGBAPI,
RGBGetCaptureHeightMaximum, (
   HRGB           hRGB,
   unsigned long  *pHeight ))
/*
 * Summary: Returns the maximum value that can be set for the number of lines
 *          that are to be captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHeight IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the number of lines that are to be captured.
 *
 * Return:  RGBGetCaptureHeightMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCaptureHeightDefault, (
   HRGB           hRGB,
   unsigned long  *pHeight ))
/*
 * Summary: Returns the default value for the number of lines that are to be
 *          captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHeight IN/OUT
 *             Pointer to a variable that receives the default for the number of
 *             lines that are to be captured.
 *
 * Return:  RGBGetCaptureHeightDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCaptureHeight, (
   HRGB           hRGB,
   unsigned long  *pHeight ))
/*
 * Summary: Returns the currently set value for the number of lines that are to
 *          be captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pHeight IN/OUT
 *             Pointer to a variable that receives the currently set value for
 *             the number of lines that are to be captured.
 *
 * Return:  RGBGetCaptureHeight either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetCaptureHeight, (
   HRGB           hRGB,
   unsigned long  height ))
/*
 * Summary: Sets the number of lines that are to be captured.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          height IN
 *             The number of lines that are to be captured value to set.
 *
 * Return:  RGBSetCaptureHeight either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetBrightnessMinimum, (
   HRGB        hRGB,
   signed long *pBrightness ))
/*
 * Summary: Returns the minimum value that can be set for brightness.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBrightness IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the brightness.
 *
 * Return:  RGBGetBrightnessMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetBrightnessMaximum, (
   HRGB        hRGB,
   signed long *pBrightness ))
/*
 * Summary: Returns the maximum value that can be set for brightness.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBrightness IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the brightness.
 *
 * Return:  RGBGetBrightnessMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetBrightnessDefault, (
   HRGB        hRGB,
   signed long *pBrightness ))
/*
 * Summary: Returns the default value for brightness.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBrightness IN/OUT
 *             Pointer to a variable that receives the default value for
 *             brightness.
 *
 * Return:  RGBGetBrightnessDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetBrightness, (
   HRGB        hRGB,
   signed long *pBrightness ))
/*
 * Summary: Returns the currently set value for brightness.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBrightness IN/OUT
 *             Pointer to a variable that receives the currently set value for
 *             brightness.
 *
 * Return:  RGBGetBrightness either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetBrightness, (
   HRGB        hRGB,
   signed long brightness ))
/*
 * Summary: Sets the brightness.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBrightness IN/OUT
 *             The brightness value to set.
 *
 * Return:  RGBSetBrightness either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetContrastMinimum, (
   HRGB        hRGB,
   signed long *pContrast ))
/*
 * Summary: Returns the minimum value that can be set for contrast.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pContrast IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the contrast.
 *
 * Return:  RGBGetContrastMinimum either returns 0 if successful or an
 *          appropriate error value.
 */
 
API ( unsigned long, RGBAPI,
RGBGetContrastMaximum, (
   HRGB        hRGB,
   signed long *pContrast ))
/*
 * Summary: Returns the maximum value that can be set for contrast.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pContrast IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the contrast.
 *
 * Return:  RGBGetContrastMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetContrastDefault, (
   HRGB        hRGB,
   signed long *pContrast ))
/*
 * Summary: Returns the default value for contrast.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pContrast IN/OUT
 *             Pointer to a variable that receives the default value for
 *             contrast.
 *
 * Return:  RGBGetContrastDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetContrast, (
   HRGB        hRGB,
   signed long *pContrast ))
/*
 * Summary: Returns the currently set value for contrast.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pContrast IN/OUT
 *             Pointer to a variable that receives the currently set value for
 *             contrast.
 *
 * Return:  RGBGetContrast either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetContrast, (
   HRGB        hRGB,
   signed long contrast ))
/*
 * Summary: Sets the contrast.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          contrast IN
 *             The contrast value to set.
 *
 * Return:  RGBSetContrast either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetBlackLevelMinimum, (
   HRGB        hRGB,
   signed long *pBlackLevel ))
/*
 * Summary: Returns the minimum value that can be set for the black level sample
 *          position.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBlackLevel IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the black level sample position.
 *
 * Return:  RGBGetBlackLevelMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetBlackLevelMaximum, (
   HRGB        hRGB,
   signed long *pBlackLevel ))
/*
 * Summary: Returns the maximum value that can be set for the black level sample
 *          position.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBlackLevel IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the black level sample position.
 *
 * Return:  RGBGetBlackLevelMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetBlackLevelDefault, (
   HRGB        hRGB,
   signed long *pBlackLevel ))
/*
 * Summary: Returns the default value for the black level sample position.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBlackLevel IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             black level sample position.
 *
 * Return:  RGBGetBlackLevelDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetBlackLevel, (
   HRGB        hRGB,
   signed long *pBlackLevel ))
/*
 * Summary: Returns the current value for the black level sample position.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBlackLevel IN/OUT
 *             Pointer to a variable that receives the current value for the
 *             black level sample position.
 *
 * Return:  RGBGetBlackLevel either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetBlackLevel, (
   HRGB        hRGB,
   signed long blackLevel ))
/*
 * Summary: Sets the position at which the black level is sampled.
 *
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          blackLevel IN
 *             The position at which the black level is sampled value to set.
 *
 * Return:  RGBSetBlackLevel either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetPhaseMinimum, (
   HRGB        hRGB,
   signed long *pPhase ))
/*
 * Summary: Returns the minimum value that can be set for the phase of the
 *          sampling clock in relation to the pixel clock.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pPhase IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the phase of the sampling clock in relation to the pixel
 *             clock.
 *
 * Return:  RGBGetPhaseMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetPhaseMaximum, (
   HRGB        hRGB,
   signed long *pPhase ))
/*
 * Summary: Returns the maximum value that can be set for the phase of the
 *          sampling clock in relation to the pixel clock.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pPhase IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the phase of the sampling clock in relation to the pixel
 *             clock.
 *
 * Return:  RGBGetPhaseMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetPhaseDefault, (
   HRGB        hRGB,
   signed long *pPhase ))
/*
 * Summary: Returns the default value for the phase of the sampling clock in
 *          relation to the pixel clock.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pPhase IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             phase of the sampling clock in relation to the pixel clock.
 *
 * Return:  RGBGetPhaseDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetPhase, (
   HRGB        hRGB,
   signed long *pPhase ))
/*
 * Summary: Returns the current value for the phase of the sampling clock in
 *          relation to the pixel clock.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pPhase IN/OUT
 *             Pointer to a variable that receives the current value for the
 *             phase of the sampling clock in relation to the pixel clock.
 *
 * Return:  RGBGetPhase either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetPhase, (
   HRGB        hRGB,
   signed long phase ))
/*
 * Summary: Sets the phase of the sampling clock in relation to the pixel clock.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          phase IN
 *             The phase of the sampling clock in relation to the pixel clock
 *             value to set.
 *
 * Return:  RGBGetPhase either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetFrameDroppingMinimum, (
   HRGB           hRGB,
   unsigned long  *pFrameDropping ))
/*
 * Summary: Returns the minimum value that can be set for the number of RGB
 *          frames that will be dropped between captures.
 *
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pFrameDropping IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the number of RGB frames that will be dropped between
 *             captures.
 *
 * Return:  RGBGetFrameDroppingMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetFrameDroppingMaximum, (
   HRGB           hRGB,
   unsigned long  *pFrameDropping ))
/*
 * Summary: Returns the maximum value that can be set for the number of RGB
 *          frames that will be dropped between captures.
 *
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pFrameDropping IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the number of RGB frames that will be dropped between
 *             captures.
 *
 * Return:  RGBGetFrameDroppingMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetFrameDroppingDefault, (
   HRGB           hRGB,
   unsigned long  *pFrameDropping ))
/*
 * Summary: Returns the default value for the number of RGB frames that will be
 *          dropped between captures.
 *
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pFrameDropping IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             number of RGB frames that will be dropped between captures.
 *
 * Return:  RGBGetFrameDroppingDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetFrameDropping, (
   HRGB           hRGB,
   unsigned long  *pFrameDropping ))
/*
 * Summary: Returns the current value for the number of RGB frames that will be
 *          dropped between captures.
 *
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pFrameDropping IN/OUT
 *             Pointer to a variable that receives the current value for the
 *             number of RGB frames that will be dropped between captures.
 *
 * Return:  RGBGetFrameDropping either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetFrameDropping, (
   HRGB           hRGB,
   unsigned long  frameDropping ))
/*
 * Summary: Sets the number of frames of RGB that will be dropped between
 *          captures. For instance, a value of 6 indicates that 6 frames of RGB
 *          will be dropped between captures.
 *
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          frameDropping IN
 *             The number of frames of RGB that will be dropped between captures
 *             value to set.
 *
 * Return:  RGBSetFrameDropping either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetFrameRate, (
   HRGB           hRGB,
   unsigned long  *pFrameRate ))
/*
 * Summary: Returns the current frame rate (in Hertz) of the RGB capture.
 *
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pFrameRate IN/OUT
 *             Pointer to a variable that receives the current frame rate (in
 *             Hertz) of the RGB capture.
 *
 * Return:  RGBGetFrameRate either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBIsCroppingEnabled, (
   HRGB           hRGB,
   unsigned long  *pBEnabled ))
/*
 * Summary: Returns a value that indicates whether cropping is enabled or
 *          disabled.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBEnabled IN/OUT
 *             Pointer to a variable that receives a value that indicates
 *             whether cropping is enabled or disabled.
 *                A value of 0 indicates that cropping is disabled.
 *                A value of 1 indicates that cropping is enabled.
 *
 * Return:  RGBIsCroppingEnabled either returns 0 if successful or an
 *          appropriate error value.
 */
/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBEnableCropping, (
   HRGB           hRGB,
   unsigned long  bEnable ))
/*
 * Summary: Enables or disables cropping using the cropping rectangle specified
 *          RGBSetCropping.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          bEnable IN
 *             A value of 0 disables cropping.
 *             A value of 1 enables cropping.
 *
 * Return:  RGBIsCroppingEnabled either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCroppingMinimum, (
   HRGB           hRGB,
   signed long    *pTop,
   signed long    *pLeft,
   unsigned long  *pWidth,
   unsigned long  *pHeight ))
/*
 * Summary: Returns the minimum cropping rectangle that can be specified.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pTop IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the top (Y) start coordinate of the cropping rectangle.
 *
 *          pLeft IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the left (X) start coordinate of the cropping rectangle.
 *
 *          pWidth IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the width of the cropping rectangle.
 *
 *          pHeight IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for the height of the cropping rectangle.
 *
 * Return:  RGBGetCroppingMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCroppingMaximum, (
   HRGB           hRGB,
   signed long    *pTop,
   signed long    *pLeft,
   unsigned long  *pWidth,
   unsigned long  *pHeight ))
/*
 * Summary: Returns the maximum cropping rectangle that can be specified.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pTop IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the top (Y) start coordinate of the cropping rectangle.
 *
 *          pLeft IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the left (X) start coordinate of the cropping rectangle.
 *
 *          pWidth IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the width of the cropping rectangle.
 *
 *          pHeight IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for the height of the cropping rectangle.
 *
 * Return:  RGBGetCroppingMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCroppingDefault, (
   HRGB           hRGB,
   signed long    *pTop,
   signed long    *pLeft,
   unsigned long  *pWidth,
   unsigned long  *pHeight ))
/*
 * Summary: Returns the default cropping rectangle.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pTop IN/OUT
 *             Pointer to a variable that receives the default value for the top
 *             (Y) start coordinate of the cropping rectangle.
 *
 *          pLeft IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             left (X) start coordinate of the cropping rectangle.
 *
 *          pWidth IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             width of the cropping rectangle.
 *
 *          pHeight IN/OUT
 *             Pointer to a variable that receives the default value for the
 *             height of the cropping rectangle.
 *
 * Return:  RGBGetCroppingDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCroppingOverscan, (
   HRGB           hRGB,
   signed long    *pTop,
   signed long    *pLeft,
   unsigned long  *pWidth,
   unsigned long  *pHeight ))
/*
 * Summary: Returns the overscan cropping rectangle for video windows only.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pTop IN/OUT
 *             Pointer to a variable that receives the overscan value for the top
 *             (Y) start coordinate of the cropping rectangle.
 *
 *          pLeft IN/OUT
 *             Pointer to a variable that receives the overscan value for the
 *             left (X) start coordinate of the cropping rectangle.
 *
 *          pWidth IN/OUT
 *             Pointer to a variable that receives the overscan value for the
 *             width of the cropping rectangle.
 *
 *          pHeight IN/OUT
 *             Pointer to a variable that receives the overscan value for the
 *             height of the cropping rectangle.
 *
 * Return:  RGBGetCroppingOverscan either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCropping, (
   HRGB           hRGB,
   signed long    *pTop,
   signed long    *pLeft,
   unsigned long  *pWidth,
   unsigned long  *pHeight ))
/*
 * Summary: Returns the current cropping rectangle.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pTop IN/OUT
 *             Pointer to a variable that receives the current value for the top
 *             (Y) start coordinate of the cropping rectangle.
 *
 *          pLeft IN/OUT
 *             Pointer to a variable that receives the current value for the
 *             left (X) start coordinate of the cropping rectangle.
 *
 *          pWidth IN/OUT
 *             Pointer to a variable that receives the current value for the
 *             width of the cropping rectangle.
 *
 *          pHeight IN/OUT
 *             Pointer to a variable that receives the current value for the
 *             height of the cropping rectangle.
 *
 * Return:  RGBGetCropping either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetCropping, (
   HRGB           hRGB,
   signed long    top,
   signed long    left,
   unsigned long  width,
   unsigned long  height ))
/*
 * Summary: Sets the cropping rectangle.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          top IN
 *             The top (Y) start coordinate value of the cropping rectangle to
 *             set.
 *
 *          left IN
 *             The left (X) start coordinate value of the cropping rectangle to
 *             set.
 *
 *          width IN
 *             The width value of the cropping rectangle to set.
 *
 *          height IN
 *             The height value of the cropping rectangle to set.
 *
 * Return:  RGBSetCropping either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBTestCropping, (
   HRGB           hRGB,
   signed long    top,
   signed long    left,
   unsigned long  width,
   unsigned long  height ))
/*
 * Summary: The RGB capture cards require that cropped windows are aligned to
 *          certain natural boundaries. The alignment may not be the same for
 *          the different RGB capture cards. This function tests whether the
 *          cropping values are correct for the RGB capture card in use.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          top IN
 *             The top (Y) start coordinate value of the cropping rectangle to
 *             test.
 *
 *          left IN
 *             The left (X) start coordinate value of the cropping rectangle to
 *             test.
 *
 *          width IN
 *             The width value of the cropping rectangle to test.
 *
 *          height IN
 *             The height value of the cropping rectangle to test.
 *
 * Return:  RGBTestCropping either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBPauseCapture, (
   HRGB  hRGB ))
/*
 * Summary: Pauses the specified RGB capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBPauseCapture either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBResumeCapture, (
   HRGB  hRGB ))
/*
 * Summary: Resumes a paused RGB capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBResumeCapture either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetCaptureState, (
   HRGB           hRGB,
   PCAPTURESTATE  pState ))
/*
 * Summary: Gets the current capture state of the specified RGB capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pState IN/OUT
 *             Pointer to a variable that receives the current capture state.
 *             The capture state returned will be one of the RGB_STATE_XXX
 *             values defined in RGB.H.
 *
 *
 * Return:  RGBGetCaptureState either returns 0 if successful or an appropriate
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetMessageDelay, (
   HRGB           hRGB,
   signed long    bShowMessages,
   unsigned long  delay ))
/*
 * Summary: Enables or disables the displaying of and the delay (in seconds)
 *          before displaying the No Signal, Invalid Signal or Error text within
 *          the window.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          bShowMessages IN
 *             A value of 0 disables the displaying of the text.
 *             A value of 1 enables the displaying of the text.
 *
 *          delay IN
 *             The delay, in seconds, to set before the text is displayed.
 *
 * Return:  RGBSetMessageDelay either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetMessageDelay, (
   HRGB           hRGB,
   signed long    *pBShowMessages,
   unsigned long  *pDelay ))
/*
 * Summary: Returns the current values that indicate whether the No Signal,
 *          Invalid Signal or Error text is displayed within the window and the
 *          delay in seconds before displaying the text.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBShowMessages IN/OUT
 *             Pointer to a variable that receives a value that indicates
 *             whether the text is displayed.
 *                A value of 0 indicates that the text will not be displayed.
 *                A value of 1 indicates that the text will be displayed.
 *
 *          pDelay IN/OUT
 *             Pointer to a variable that receives the value of the delay (in
 *             seconds) before displaying the text.
 *
 * Return:  RGBGetMessageDelay either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetPixelFormat, (
   HRGB           hRGB,
   PPIXELFORMAT   pPixelFormat ))
/*
 * Summary: Returns the current pixel format of the RGB capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pPixelFormat IN/OUT
 *             Pointer to a variable that receives the value of the current
 *             pixel format. The pixel format returned will be one of the
 *             RGB_PIXELFORMAT_XXX values defined in RGB.H.
 *
 * Return:  RGBGetPixelFormat either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetPixelFormat, (
   HRGB        hRGB,
   PIXELFORMAT pixelFormat ))
/*
 * Summary: Sets the pixel format of the RGB capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pixelFormat IN
 *             The pixel format to set. The pixel set must be one of the
 *             RGB_PIXELFORMAT_XXX values defined in RGB.H.
 *
 * Return:  RGBSetPixelFormat either returns 0 if successful or an
 *          appropriate error value.
 */

 /******************************************************************************/

API(unsigned long, RGBAPI,
RGBGetPixelGamut, (
   HRGB        hRGB,
   PPIXELGAMUT pPixelGamut))
/*
 * Summary: Returns the current pixel gamut of the capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pPixelGamut IN/OUT
 *             Pointer to a variable that receives the value of the current
 *             pixel gamut. The pixel gamut returned will be one of the
 *             RGB_PIXELGAMUT_XXX values defined in RGB.H.
 * 
 * Return:  RGBGetPixelGamut either returns 0 if successful or an
 *          appropriate error value.
 */

API(unsigned long, RGBAPI,
RGBSetPixelGamut, (
   HRGB        hRGB,
   PIXELGAMUT  pixelGamut))
/*
 * Summary: Sets the pixel gamut of the RGB capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pixelGamut IN
 *             The pixel gamut to set. The pixel set must be one of the
 *             RGB_PIXELGAMUT_XXX values defined in RGB.H.
 *
 * Return:  RGBSetPixelGamut either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API(unsigned long, RGBAPI,
RGBGetPixelRange, (
   HRGB        hRGB,
   PPIXELRANGE pPixelRange))
/*
* Summary: Returns the current pixel range of the RGB capture.
*
* Args:    hRGB IN
*             The RGB capture handle.
*
*          pPixelRange IN/OUT
*             Pointer to a variable that receives the value of the current
*             pixel range. The pixel range returned will be one of the
*             RGB_PIXELRANGE_XXX values defined in RGB.H.
*
* Return:  RGBGetPixelRange either returns 0 if successful or an
*          appropriate error value.
*/

API(unsigned long, RGBAPI,
RGBSetPixelRange, (
   HRGB       hRGB,
   PIXELRANGE pixelRange))
/*
* Summary: Sets the pixel range of the RGB capture.
*
* Args:    hRGB IN
*             The RGB capture handle.
*
*          pixelRange IN
*             The pixel range to set. The pixel set must be one of the
*             RGB_PIXELRANGE_XXX values defined in RGB.H.
*
* Return:  RGBSetPixelRange either returns 0 if successful or an
*          appropriate error value.
*/

/******************************************************************************/

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBSaveCurrentFrame   RGBSaveCurrentFrameW
#else
#define RGBSaveCurrentFrame   RGBSaveCurrentFrameA
#endif

API ( unsigned long, RGBAPI,
RGBSaveCurrentFrameA, (
   HRGB        hRGB,
   const char  *pFileName ))

API ( unsigned long, RGBAPI,
RGBSaveCurrentFrameW, (
   HRGB           hRGB,
   const wchar_t  *pFileName ))
/*
 * Summary: Saves a paused RGB capture to a windows bitmap file.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pFileName IN
 *             The full path to the file in which to save RGB data,
 *             e.g. C:\MyCaptures\Capture1.bmp
 *             If the file does not exist it will be created.
 *             If the file already exists it will be overwritten.
 *
 * Return:  RGBSaveCurrentFrame either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBIsDirectDMASupported, (
   signed long *pBIsSupported ))
/*
 * Summary: Returns a value that indicates whether the RGB data can transfered
 *          directly from the RGB capture card to the display device using DMA.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBIsSupported IN/OUT
 *             Pointer to a variable that receives a value that indicates
 *             whether the RGB data can transfered directly from the RGB capture
 *             card to the display device using DMA.
 *                A value of 0 indicates that direct DMA is not supported.
 *                A value of 1 indicates that direct DMA is supported.
 *
 * Return:  RGBIsDirectDMASupported either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetDMADirect, (
   HRGB  hRGB,
   signed long bDMADirect ))
/*
 * Summary: Enables or disables direct DMA of the RGB data from the RGB capture
 *          card to a supported display device.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          bDMADirect IN
 *             A value of 0 disables direct DMA.
 *             A value of 1 enables direct DMA.
 *
 * Return:  RGBSetDMADirect either returns 0 if successful or an appropriate
 *          error value.
 */

API ( unsigned long, RGBAPI,
RGBGetDMADirect, (
   HRGB  hRGB,
   signed long *pBDMADirect ))
/*
 * Summary: Returns a value indicating wether the specified RGB capture is using
 *          direct DMA to transfer RGB data from RGB capture card to the display
 *          device.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          bDMADirect IN
 *             Pointer to a variable that receives a value that indicates
 *             whether the RGB data is being transfered using direct DMA.
 *                A value of 0 indicates that direct DMA is not being used.
 *                A value of 1 indicates that direct DMA is being used.
 *
 * Return:  RGBGetDMADirect either returns 0 if successful or an appropriate
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetDownScaling, (
   HRGB  hRGB,
   long  bFastScaling ))

/*
 * Summary: Changes the scaling used on the RGB data when drawing on a window. 
 *
 *          If bFastScaling is true, and the window is smaller than the capture,
 *          the downscaling is done in hardware on the capture card before DMA. 
 *          If bFastScaling is false the DMA is 1:1 and the scaling is handled
 *          by the graphics system.
 *
 *          When the capture has been set to DMA to system memory through the 
 *          RGBSetDMADirect call then bFastScaling additionally enables a 
 *          slower, but high quality, drawing algorithm.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          bFastScaling IN
 *             A value of 0 disables scaling on the capture card.
 *             A value of 1 enables scaling on the capture card.
 *
 * Return:  RGBGetDMADirect either returns 0 if successful or an appropriate
 *          error value.
 */


API ( unsigned long, RGBAPI,
RGBGetDownScaling, (
   HRGB  hRGB,
   long  *pBFastScaling ))
/*
 * Summary: Returns a value indicating whether the specified RGB capture is 
 *          scaling the captured data before DMA across the the PCI bus.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          bDMADirect IN
 *             Pointer to a variable that receives a value that indicates
 *             whether the RGB data is being scaled on the capture card.
 *                A value of 0 indicates that scaling on the capture card is
 *                diasbled.
 *                A value of 1 indicates that scaling on the capture card is
 *                enabled.
 *
 * Return:  RGBGetDMADirect either returns 0 if successful or an appropriate
 *          error value.
 */


/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetFrameCapturedFn, (
   HRGB                 hRGB,
   PRGBFRAMECAPTUREDFN  pFrameCapturedFn,
   ULONG_PTR            userData ))
/*
 * Summary: Sets and unsets a Frame Captured callback function.
 *
 * Purpose: When a frame of RGB data has been captured the RGB SDK uses a
 *          default handler to draw the RGB frame to the window and set up the
 *          next RGB capture. RGBSetFrameCapturedFn allows an application to
 *          specify a callback function which will be executed instead of the
 *          default handler. From within this callback an application can run
 *          the default handler by calling RGBDrawFrame.
 *          Please see RGB.H for more information regarding the Frame Captured
 *          callback function in particular its use when direct DMA is being
 *          used for the RGB capture.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pFrameCapturedFn IN
 *             Pointer to an application defined Frame Captured callback
 *             function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the Frame Captured
 *             function.
 *
 *
 * Return:  RGBSetFrameCapturedFn either returns 0 if successful or an
 *          appropriate error value.
 */

 API ( unsigned long, RGBAPI,
RGBSetFrameCapturedFnEx, (
   HRGB                  hRGB,
   PRGBFRAMECAPTUREDFNEX pFrameCapturedFnEx,
   ULONG_PTR             userData ))
/*
 * Summary: Sets and unsets a Frame Captured callback function.
 *
 * Purpose: When a frame of RGB data has been captured the RGB SDK uses a
 *          default handler to draw the RGB frame to the window and set up the
 *          next RGB capture. RGBSetFrameCapturedFnEx allows an application to
 *          specify a callback function which will be executed instead of the
 *          default handler. From within this callback an application can run
 *          the default handler by calling RGBDrawFrame. In addition to 
 *          RGBSetFrameCapturedFn, RGBSetFrameCapturedFnEx provides the user
 *          with a pointer to a RGBFRAMEDATA structure defined in RGB.H.
 *          The RGBFRAMEDATA structure contains additional information about the 
 *          frame inculding time stamp, frame count and buffer pointer.
 *          Please see RGB.H for more information regarding the Frame Captured
 *          callback function in particular its use when direct DMA is being
 *          used for the RGB capture.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pFrameCapturedFn IN
 *             Pointer to an application defined Frame Captured callback
 *             function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the Frame Captured
 *             function.
 *
 *
 * Return:  RGBSetFrameCapturedFn either returns 0 if successful or an
 *          appropriate error value.
 */

API(unsigned long, RGBAPI,
RGBSetMediaSampleCapturedFn, (
   HRGB                       hRGB,
   PRGBMEDIASAMPLECAPTUREDFN  pMediaSampleCapturedFn,
   ULONG_PTR                  userData))
 /*
 * Summary: Sets and unsets a Video Sample Captured callback function.
 *
 * Purpose: When a video sample has been captured the RGB SDK uses a
 *          default handler to draw the RGB frame to the window and set up the
 *          next RGB capture. RGBSetVideoSampleCapturedFn allows an application to
 *          specify a callback function which will be executed instead of the
 *          default handler. From within this callback an application can run
 *          the default handler by calling RGBDrawFrame.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pVideoSampleCapturedFnEx IN
 *             Pointer to an application defined Video Sample Captured callback
 *             function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the Video Sample Captured
 *             function.
 *
 *
 * Return:  RGBSetFrameCapturedFn either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBDrawFrame, (
   HRGB  hRGB ))
/*
 * Summary: Calls the default Frame Captured handler. Can only be used from
 *          within an application defined Frame Captured callback function.
 *
 * Purpose: The SDK's default Frame Captured function draws the RGB data within
 *          the window and initiates the next RGB capture. An application that
 *          has implemented a Frame Captured callback function can call
 *          RGBDrawFrame to run the default handler.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBDrawFrame either returns 0 if successful or an appropriate error
 *          value.
 */

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBSaveBitmap         RGBSaveBitmapW
#else
#define RGBSaveBitmap         RGBSaveBitmapA
#endif

API ( unsigned long, RGBAPI,
RGBSaveBitmapA, (
   HRGB                 hRGB,
   LPBITMAPINFOHEADER   pBitmapInfo,
   PVOID                pBitmapBits,
   const char           *pFileName ))

API ( unsigned long, RGBAPI,
RGBSaveBitmapW, (
   HRGB                 hRGB,
   LPBITMAPINFOHEADER   pBitmapInfo,
   PVOID                pBitmapBits,
   const wchar_t        *pFileName ))

/*
 * Summary: Saves the RGB capture to a windows bitmap file.
 *
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBDrawFrame either returns 0 if successful or an appropriate error
 *          value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetModeChangedFn, (
   HRGB              hRGB,
   PRGBMODECHANGEDFN pModeChangedFn,
   ULONG_PTR         userData ))
/*
 * Summary: Sets and unsets a Mode Changed callback function.
 *
 * Purpose: The SDK's default Mode Changed function initialses the new video
 *          mode with default values for capture height, capture width, phase,
 *          black level, etc. RGBSetModeChangedFn allows an application to
 *          specify a callback function which will be executed after the default
 *          values have been initialised but prior to them being sent to the
 *          RGB capture card. This allows an application to override the default
 *          values with application defined ones.
 *          Please see RGB.H for more information regarding the Mode Changed
 *          callback function.
 * 
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pModeChangedFn IN
 *             Pointer to an application defined Mode Changed callback function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the Frame Captured
 *             callback function.
 *
 * Return:  RGBSetModeChangedFn either returns 0 if successful or an appropriate
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetNoSignalFn, (
   HRGB           hRGB,
   PRGBNOSIGNALFN pNoSignalFn,
   ULONG_PTR      userData ))
/*
 * Summary: Sets and unsets a No Signal callback function.
 *
 * Purpose: When the specified RGB capture becomes No Signal the RGB SDK uses a
 *          default handler to re-detect the video source and to display the
 *          "No Signal" message. RGBSetNoSignalFn allows an application to
 *          specify a callback function which will be executed instead of the
 *          default handler. From within this callback an application can run
 *          the default handler by calling RGBNoSignal.
 *          Please see RGB.H for more information regarding the No Signal
 *          callback function.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pNoSignalFn IN
 *             Pointer to an application defined No Signal callback function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the No Signal callback
 *             function.
 *
 *
 * Return:  RGBSetNoSignalFn either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBNoSignal, (
   HRGB  hRGB ))
/*
 * Summary: Calls the default No Signal handler. Can only be used from within
 *          an application defined No Signal callback function.
 *
 * Purpose: The SDK's default No Signal handler sets up the capture to re-detect
 *          the video source and displays the "No Signal" text within the
 *          window. An application that has implemented a No Signal callback
 *          function can call RGBNoSignal to run the default handler.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBNoSignal either returns 0 if successful or an appropriate error
 *          value.
 */

API ( unsigned long, RGBAPI,
RGBSetDrawNoSignalFn, (
    HRGB                hRGB,
    PRGBDRAWNOSIGNALFN  pDrawFn,
    ULONG_PTR           userData ))
/*
 * Summary: Sets and unsets a No Signal drawing callback function.
 *
 * Purpose: When there is No Signal the SDK's default painting function will 
 *          draw a "No Signal" message in the window. This function can be 
 *          used register a custom callback to override the drawing of the 
 *          message. The callback is called upon receipt of a WM_PAINT message.
 *
 * Args:    hRGB  IN
 *             The RGB capture handle.
 *
 *          pDrawFn IN
 *             Pointer to an application defined drawing callback function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the drawing callback
 *             function.
 *
 * Return:  RGBSetDrawNoSignalFn either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetInvalidSignalFn, (
   HRGB                 hRGB,
   PRGBINVALIDSIGNALFN  pInvalidSignalFn,
   ULONG_PTR            userData ))
/*
 * Summary: Sets and unsets an Invalid Signal callback function that is called
 *          when the signal detected is beyond the capabilities of the hardware.
 *
 * Purpose: When the specified RGB capture becomes Invalid Signal the RGB SDK
 *          uses a default handler to re-detect the video source and to display
 *          the "Invalid Signal" text. RGBSetInvalidSignalFn allows an
 *          application to specify a callback function which will be executed
 *          instead of the default handler. From within this callback an
 *          application can run the default handler by calling RGBInvalidSignal.
 *          Please see RGB.H for more information regarding the Invalid Signal
 *          callback function.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pInvalidSignalFn IN
 *             Pointer to an application defined Invalid Signal callback
 *             function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the Invalid Signal
 *             callback function.
 *
 *
 * Return:  RGBSetInvalidSignalFn either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBInvalidSignal, (
   HRGB           hRGB,
   unsigned long  horClock,
   unsigned long  verClock ))
/*
 * Summary: Calls the default Invalid Signal handler. Can only be used from
 *          within an application defined Invalid Signal callback function.
 *
 * Purpose: The SDK's default Invalid Signal handler sets up the capture to
 *          re-detect the video source and displays the "Invalid Signal" text
 *          within the window. An application that has implemented an Invalid
 *          Signal callback function can call RGBInvalidSignal to run the
 *          default handler.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBInvalidSignal either returns 0 if successful or an appropriate
 *          error value.
 */

API ( unsigned long, RGBAPI,
RGBSetDrawInvalidSignalFn, ( 
   HRGB                    hRGB,
   PRGBDRAWINVALIDSIGNALFN pDrawFn,
   ULONG_PTR               userData ))
/*
 * Summary: Sets and unsets a Invalid Signal drawing callback function.
 *
 * Purpose: When there is and Invalid Signal the SDK's default painting 
 *          function will draw a message in the window showing the sync rates. 
 *          This function can be used register a custom callback to override
 *          the drawing of the message. The callback is called upon receipt
 *          of a WM_PAINT message.
 *
 * Args:    hRGB  IN
 *             The RGB capture handle.
 *
 *          pDrawFn IN
 *             Pointer to an application defined drawing callback function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the drawing callback
 *             function.
 *
 * Return:  RGBSetDrawInvalidSignalFn either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetErrorFn, (
   HRGB           hRGB,
   PRGBERRORFN    pErrorFn,
   ULONG_PTR      userData ))
/*
 * Summary: Sets and unsets an Error callback function that is called when the
 *          SDK detects a non-recoverable error.
 *
 * Purpose: RGBSetErrorFn allows an application to specify a callback function
 *          which will be executed when a non-recoverable error has occured
 *          within the RGB SDK.
 *          It is the applications responsibility to close the RGB capture using
 *          RGBCloseInput when the Error callback function is executed.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pErrorFn IN
 *             Pointer to an application defined Error callback function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the Error callback
 *             function.
 *
 *
 * Return:  RGBSetErrorFn either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetValueChangedFn, (
   HRGB                 hRGB,
   PRGBVALUECHANGEDFN   pValueChangedFn,
   ULONG_PTR            userData ))
/*
 * Summary: Sets and unsets a Value Changed function that is called only for
 *          DGC133 / VisionRGB-X capture cards when another capture on the same
 *          input as this capture has changed one of the following values:
 *          
 *             Horizontal Position.
 *             Horizontal Scale.
 *             Vertical Position.
 *             Capture Width.
 *             Capture Height.
 *             Brightness.
 *             Contrast.
 *             Black level.
 *             Phase.
 *          
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pValueChangedFn IN
 *             Pointer to an application defined Value Changed callback function.
 *             If this value is NULL, the default handler is restored.
 *
 *          userData IN
 *             Application-determined context to pass to the Value Changed
 *             callback function.
 *
 *
 * Return:  RGBSetValueChangedFn either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBStartCapture, (
   HRGB  hRGB ))
/*
 * Summary: Starts the RGB capture for RGB applications not using RGBSetWindow.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBStartCapture either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBStopCapture, (
   HRGB  hRGB ))
/*
 * Summary: Stops the RGB capture for RGB applications not using RGBSetWindow.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBStopCapture either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetColourBalanceMinimum, (
   HRGB        hRGB,
   signed long *pBrightnessRed,
   signed long *pBrightnessGreen,
   signed long *pBrightnessBlue,
   signed long *pContrastRed,
   signed long *pContrastGreen,
   signed long *pContrastBlue ))
/*
 * Summary: Returns the minimum values that can be set for the colour balance.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBrightnessRed IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for red brightness.
 *
 *          pBrightnessGreen IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for green brightness.
 *
 *          pBrightnessBlue IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for blue brightness.
 *
 *          pContrastRed IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for red contrast.
 *
 *          pContrastGreen IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for green contrast.
 *
 *          pContrastBlue IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for blue contrast.
 *
 * Return:  RGBGetColourBalanceMinimum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetColourBalanceMaximum, (
   HRGB        hRGB,
   signed long *pBrightnessRed,
   signed long *pBrightnessGreen,
   signed long *pBrightnessBlue,
   signed long *pContrastRed,
   signed long *pContrastGreen,
   signed long *pContrastBlue ))
/*
 * Summary: Returns the maximum values that can be set for the colour balance.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBrightnessRed IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for red brightness.
 *
 *          pBrightnessGreen IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for green brightness.
 *
 *          pBrightnessBlue IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for blue brightness.
 *
 *          pContrastRed IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for red contrast.
 *
 *          pContrastGreen IN/OUT
 *             Pointer to a variable that receives the maximum value that can be
 *             set for green contrast.
 *
 *          pContrastBlue IN/OUT
 *             Pointer to a variable that receives the minimum value that can be
 *             set for blue contrast.
 *
 * Return:  RGBGetColourBalanceMaximum either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetColourBalanceDefault, (
   HRGB        hRGB,
   signed long *pBrightnessRed,
   signed long *pBrightnessGreen,
   signed long *pBrightnessBlue,
   signed long *pContrastRed,
   signed long *pContrastGreen,
   signed long *pContrastBlue ))
/*
 * Summary: Returns the default values that can be set for the colour balance.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBrightnessRed IN/OUT
 *             Pointer to a variable that receives the default value that can be
 *             set for red brightness.
 *
 *          pBrightnessGreen IN/OUT
 *             Pointer to a variable that receives the default value that can be
 *             set for green brightness.
 *
 *          pBrightnessBlue IN/OUT
 *             Pointer to a variable that receives the default value that can be
 *             set for blue brightness.
 *
 *          pContrastRed IN/OUT
 *             Pointer to a variable that receives the default value that can be
 *             set for red contrast.
 *
 *          pContrastGreen IN/OUT
 *             Pointer to a variable that receives the default value that can be
 *             set for green contrast.
 *
 *          pContrastBlue IN/OUT
 *             Pointer to a variable that receives the default value that can be
 *             set for blue contrast.
 *
 * Return:  RGBGetColourBalanceDefault either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBGetColourBalance, (
   HRGB        hRGB,
   signed long *pBrightnessRed,
   signed long *pBrightnessGreen,
   signed long *pBrightnessBlue,
   signed long *pContrastRed,
   signed long *pContrastGreen,
   signed long *pContrastBlue ))
/*
 * Summary: Returns the current values for the colour balance .
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pBrightnessRed IN/OUT
 *             Pointer to a variable that receives the current value of the red
 *             brightness.
 *
 *          pBrightnessGreen IN/OUT
 *             Pointer to a variable that receives the current value of the
 *             green brightness.
 *
 *          pBrightnessBlue IN/OUT
 *             Pointer to a variable that receives the current value of the blue
 *             brightness.
 *
 *          pContrastRed IN/OUT
 *             Pointer to a variable that receives the current value of the red
 *             contrast.
 *
 *          pContrastGreen IN/OUT
 *             Pointer to a variable that receives the current value of the
 *             green contrast.
 *
 *          pContrastBlue IN/OUT
 *             Pointer to a variable that receives the current value of the blue
 *             contrast.
 *
 * Return:  RGBGetColourBalance either returns 0 if successful or an
 *          appropriate error value.
 */
   
API ( unsigned long, RGBAPI,
RGBSetColourBalance, (
   HRGB        hRGB,
   signed long brightnessRed,
   signed long brightnessGreen,
   signed long brightnessBlue,
   signed long contrastRed,
   signed long contrastGreen,
   signed long contrastBlue ))
/*
 * Summary: Sets the colour balance .
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          brightnessRed IN
 *             The red brightness value to set.
 *
 *          brightnessGreen IN
 *             The green brightness value to set.
 *
 *          brightnessBlue IN
 *             The blue brightness value to set.
 *
 *          contrastRed IN
 *             The red contrast value to set.
 *
 *          contrastGreen IN
 *             The green contrast value to set.
 *
 *          contrastBlue IN
 *             The blue contrast value to set.
 *
 * Return:  RGBSetColourBalance either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetModeInfo, ( 
   HRGB           hRGB,
   PRGBMODEINFO   pModeInfo ) )
/*
 * Summary: Function that returns information about the current mode.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pModeInfo IN OUT
 *             Pointer to the structure to fill with the mode information. 
 *             The size field must be intialised before this function is 
 *             called.
 *
 * Return:  RGBGetModeInfo either returns 0 if successful or an appropriate 
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBSetOutputSize, (
   HRGB           hRGB,
   unsigned long  uWidth,
   unsigned long  uHeight ) )
/*
 * Summary : Function which sets the size of the capture for RGB applications 
 *           not using RGBSetWindow. The output size is set to 1:1 with the
 *           incoming source upon a mode change.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          uWidth IN
 *             The width of the buffer to capture.
 *          
 *          uHeight IN
 *             The height of the buffer to capture.
 *
 * Return:  RGBSetOutputSize either returns 0 if successful or an appropriate 
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetOutputSize, (
   HRGB           hRGB,
   unsigned long  *pWidth,
   unsigned long  *pHeight ) )
/*
 * Summary : Function which gets the size of the capture for RGB applications 
 *           not using RGBSetWindow.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pWidth OUT
 *             The width of the buffer captured.
 *          
 *          pHeight OUT
 *             The height of the buffer captured.
 *
 * Return:  RGBGetOutputSize either returns 0 if successful or an appropriate 
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBUseOutputBuffers, (
   HRGB  hRGB,
   BOOL  BEnableBuffers ) )
/*
 * Summary : Enables or disables the capturing into the users own buffers.
 *           
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          bEnableBuffers
 *             If true the capture is into a user managed buffer. If false the
 *             RGBEASY environment manages the allocation of capture buffers.
 *
 * Return:  RGBUseOutputBuffers either returns 0 if successful or an appropriate 
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBChainOutputBuffer, (
   HRGB           hRGB,
   LPBITMAPINFO   lpBitmapInfo,
   PVOID          lpBuffer ) )
/*
 * Summary : This function adds a user managed buffer into which the capture can
 *           be placed when RGBUseOutputBuffers has been set to enabled. Multiple
 *           buffers can be used with additional calls to this function. When a
 *           filled buffer is returned to through the FrameCaptured callback it
 *           must be reinserted with another call to this function for it to be
 *           refilled.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          lpBitmapInfo IN
 *             The bitmap info describing the buffer to be filled. 
 *
 *          lpBuffer IN
 *             The buffer to be filled. 
 *
 * Return:  RGBChainOutputBuffer either returns 0 if successful or an appropriate 
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBChainOutputBufferEx, (
   HRGB           hRGB,
   LPBITMAPINFO   lpBitmapInfo,
   PVOID          lpBuffer,
   BUFFERTYPE     bufferType ) )
/*
 * Summary : This function adds a user managed buffer into which the capture can
 *           be placed when RGBUseOutputBuffers has been set to enabled. Multiple
 *           buffers can be used with additional calls to this function. When a
 *           filled buffer is returned to through the FrameCaptured callback it
 *           must be reinserted with another call to this function for it to be
 *           refilled.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          lpBitmapInfo IN
 *             The bitmap info describing the buffer to be filled. 
 *
 *          lpBuffer IN
 *             The buffer to be filled. 
 *
 *          bufferType IN
 *             The type of buffer to be changed.
 *
 * Return:  RGBChainOutputBuffer either returns 0 if successful or an appropriate 
 *          error value.
 */

/******************************************************************************/

API(unsigned long, RGBAPI,
RGBChainMediaSample, (
   HRGB              hRGB,
   PDGCMEDIASAMPLE   pMediaSample))
/*
* Summary : This function adds a user managed video sample into which the capture
*           can be placed when RGBUseOutputBuffers has been set to enabled.
*           Multiple buffers can be used with additional calls to this function.
*           When a video sample is returned to through the callback function it
*           must be reinserted with another call to this function for it to be
*           refilled.
*
* Args:    hRGB IN
*             The RGB capture handle.
*
*          lpBuffer IN
*             The video sample to be filled.
*
* Return:  RGBChainVideoSample either returns 0 if successful or an appropriate
*          error value.
*/

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBDirectGPUInit, (
   HRGB                       hRGB,
   PGPUTRANSFERDESCRIPTOR     pGpuDesc ))
/*
 * Summary : This function initializes a given graphics API for direct memory
             transfer between a capture card and the graphics card.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pGpuDesc IN/OUT
 *             A pointer to a structure describing the characteristics of the 
 *             transfer, such widht, weight, colour format, byte format,
 *             number of buffers etc.
 *             For more details see GPUTRANSFERDESCRIPTOR structure.
 *
 * Return:  RGBDirectGPUInit either returns 0 if successful or an appropriate 
 *          error value. Also sets pointers to the buffers in the 
 *          GPUTRANSFERDESCRIPTOR structure
 */

API ( unsigned long, RGBAPI,
RGBDirectGPUClose, (
   HRGB     hRGB ))
/*
 * Summary : 
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBDirectGPUClose either returns 0 if successful or an appropriate 
 *          error value.
 */

API ( unsigned long, RGBAPI,
RGBDirectGPUNVIDIAOp, (
   HRGB                       hRGB,
   unsigned int               index,
   NVIDIAOP                   op ))
/*
 * Summary : 
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 * Return:  RGBDirectGPUNVIDIAOp either returns 0 if successful or an appropriate 
 *          error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBSetDeinterlace, (
   HRGB        hRGB,
   DEINTERLACE value ))
/*
 * Summary : This function sets the de-interlacing method used on interlaced 
             video sources. The capture can either be single field, weave or bob
             interlaced.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *          
 *          value IN
 *             The de-interlace method to use.
 *
 * Return: RGBSetDeinterlace returns either 0 if successful or an appropriate 
 *         error value.
 */

API ( unsigned long, RGBAPI, 
RGBGetDeinterlace, (
   HRGB         hRGB,
   PDEINTERLACE pValue ))
/*
 * Summary : This function gets the de-interlacing method used by the capture on
 *           interlaced sources.
 *  
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pValue OUT
 *             The de-interlace method in use.
 *
 * Return: RGBGetDeinterlace returns either 0 if successful or an appropriate 
 *         error value.
 *
 */

API ( unsigned long, RGBAPI, 
RGBIsYUVSupported, (
   signed long *pBIsSupported ))
/*
 * Summary : This function queries whether YUV display is supported on the 
 *           Datapath graphics display card.
 *
 * Args:    pBIsSupported OUT
 *             Boolean value which is true if the graphics card supports YUV 
 *             output.
 *
 * Return:  RGBIsYUVSupported returns 0.
 *
 */

API ( unsigned long, RGBAPI, 
RGBIsDeinterlaceSupported, (
   signed long *pBIsSupported ))
/*
 * Summary : This function queries whether an interlaced signal can be 
 *           de-interlaced by the hardware.
 *
 * Args:    pBIsSupported OUT
 *             Boolean value which is true if the hardware can de-interlace an
 *             interlaced signal.  
 *
 * Return:  
 *
 */

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetVideoStandard, (
   HRGB           hRGB,
   unsigned long *pStandard ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBSetVideoStandard, (
   HRGB           hRGB,
   unsigned long  standard ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBTestVideoStandard, (
   HRGB           hRGB,
   unsigned long  standard ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetSaturationMinimum, (
   HRGB        hRGB,
   signed long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetSaturationMaximum, (
   HRGB     hRGB,
   signed long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetSaturationDefault, (
   HRGB        hRGB,
   signed long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetSaturation, (
   HRGB        hRGB,
   signed long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBSetSaturation, (
   HRGB        hRGB,
   signed long value ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetHueMinimum, (
   HRGB     hRGB,
   signed long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetHueMaximum, (
   HRGB     hRGB,
   signed long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetHueDefault, (
   HRGB     hRGB,
   signed long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetHue, (
   HRGB     hRGB,
   signed long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBSetHue, (
   HRGB     hRGB,
   signed long value ) )

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBGetInputSignalType, (
   unsigned long input,
   PSIGNALTYPE   pSignalType,
   unsigned long *pCaptureWidth,
   unsigned long *pCaptureHeight,
   unsigned long *pRefreshRate ) )

API ( unsigned long, RGBAPI,
RGBInputIsVGASupported, (
   unsigned long input,
   signed long *pBIsSupported ) )

API ( unsigned long, RGBAPI,
RGBInputIsDVISupported, (
   unsigned long input,
   signed long *pBIsSupported ) )

API ( unsigned long, RGBAPI,
RGBInputIsComponentSupported, (
   unsigned long input,
   signed long *pBIsSupported ) )

API ( unsigned long, RGBAPI,
RGBInputIsCompositeSupported, (
   unsigned long input,
   signed long *pBIsSupported ) )

API ( unsigned long, RGBAPI,
RGBInputIsSVideoSupported, (
   unsigned long input,
   signed long *pBIsSupported ) )

API ( unsigned long, RGBAPI,
RGBInputIsSDISupported, (
   unsigned long input,
   signed long *pBIsSupported ) )

API ( unsigned long, RGBAPI,
RGBInputIsDualLinkDVISupported, (
   unsigned long input,
   signed long *pBIsSupported ) )

API ( unsigned long, RGBAPI,
RGBInputIsHDMISupported, (
   unsigned long input,
   signed long *pBIsSupported ) )
      
API(unsigned long, RGBAPI,
RGBInputIsDisplayPortSupported, (
   unsigned long input,
   signed long *pBIsSupported))

API(unsigned long, RGBAPI,
RGBInputGetMaximumPixelClock, (
   unsigned long input,
   unsigned long *pMaximumPixelClock))

/******************************************************************************/

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBSetNoSignalText   RGBSetNoSignalTextW
#else
#define RGBSetNoSignalText   RGBSetNoSignalTextA
#endif

API ( unsigned long, RGBAPI,
RGBSetNoSignalTextA, (
   const char  *pNoSignalTxt ))

API ( unsigned long, RGBAPI,
RGBSetNoSignalTextW, (
   const wchar_t  *pNoSignalTxt ))
/*
 * Summary: Sets the Global no signal display text drawn within a window.
 *
 * Args:    pNoSignalTxt IN
 *             Text to display.
 *
 * Return:  RGBSetNoSignalText either returns 0 if successful or an
 *          appropriate error value.
 */

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetNoSignalBackground, (
   COLORREF cBackground ))

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBSetInvalidSignalText   RGBSetInvalidSignalTextW
#else
#define RGBSetInvalidSignalText   RGBSetInvalidSignalTextA
#endif

API ( unsigned long, RGBAPI,
RGBSetInvalidSignalTextA, (
   const char  *pInvalidSignalTxt ))


API ( unsigned long, RGBAPI,
RGBSetInvalidSignalTextW, (
   const wchar_t  *pInvalidSignalTxt ))
/*
 * Summary: Sets the Global invalid signal display text drawn within a window.
 *
 * Args:    pInvalidSignalTxt IN
 *             Text to display.
 *
 * Return:  RGBSetInvalidSignalText either returns 0 if successful or an
 *          appropriate error value.
 */

API ( unsigned long, RGBAPI,
RGBSetInvalidSignalBackground, (
   COLORREF cBackground ))

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBCreateOSD, (
   PHRGBOSD pHOSD ))

API ( unsigned long, RGBAPI,
RGBDeleteOSD, (
   HRGBOSD  hOSD ))

API ( unsigned long, RGBAPI,
RGBAttachOSD, (
   HRGB     hRGB,
   HRGBOSD  hOSD ))

API ( unsigned long, RGBAPI,
RGBDetachOSD, (
   HRGB     hRGB,
   HRGBOSD  hOSD ))

API ( unsigned long, RGBAPI,
RGBGetFirstOSD, ( 
   HRGB     hRGB,
   PHRGBOSD pHOSD ))

API ( unsigned long, RGBAPI,
RGBGetNextOSD, ( 
   HRGB     hRGB,
   HRGBOSD  hOSD,
   PHRGBOSD pHOSD ))

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetOSDType, (
   HRGBOSD     hOSD,
   RGBOSD_TYPE type ))

API ( unsigned long, RGBAPI,
RGBGetOSDType, (
   HRGBOSD     hOSD,
   RGBOSD_TYPE *pType ))

API ( unsigned long, RGBAPI,
RGBSetOSDScaling, (
   HRGBOSD  hOSD,
   BOOL     bFixedSize ))

API ( unsigned long, RGBAPI,
RGBGetOSDScaling, (
   HRGBOSD  hOSD,
   BOOL     *pBFixedSize ))

API ( unsigned long, RGBAPI,
RGBSetOSDBackground, (
   HRGBOSD     hOSD,
   COLORREF    cBackground,
   BOOL        bTransparent ))

API ( unsigned long, RGBAPI,
RGBGetOSDBackground, (
   HRGBOSD     hOSD,
   COLORREF    *pCBackground,
   BOOL        *pBTransparent ))

/******************************************************************************/

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBSetOSDText   RGBSetOSDTextW
#else
#define RGBSetOSDText   RGBSetOSDTextA
#endif

API ( unsigned long, RGBAPI,
RGBSetOSDTextA, (
   HRGBOSD     hOSD,
   const char  *lpString ))

API ( unsigned long, RGBAPI,
RGBSetOSDTextW, (
   HRGBOSD        hOSD,
   const wchar_t  *lpString ))

API ( unsigned long, RGBAPI,
RGBGetOSDTextLength, (
   HRGBOSD        hOSD,
   unsigned long  *pNChars ))

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBGetOSDText   RGBGetOSDTextW
#else
#define RGBGetOSDText   RGBGetOSDTextA
#endif

API ( unsigned long, RGBAPI,
RGBGetOSDTextA, (
   HRGBOSD        hOSD,
   char           *lpString,
   unsigned long  *pNChars ))

API ( unsigned long, RGBAPI,
RGBGetOSDTextW, (
   HRGBOSD        hOSD,
   wchar_t        *lpString,
   unsigned long  *pNChars ))

API ( unsigned long, RGBAPI,
RGBSetOSDWrapping, (
   HRGBOSD  hOSD,
   BOOL     bWrapText ))

API ( unsigned long, RGBAPI,
RGBGetOSDWrapping, (
   HRGBOSD  hOSD,
   BOOL     *pBWrapText ))

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBSetOSDFont   RGBSetOSDFontW
#else
#define RGBSetOSDFont   RGBSetOSDFontA
#endif

API ( unsigned long, RGBAPI,
RGBSetOSDFontA, (
   HRGBOSD     hOSD,
   PLOGFONTA   pFont,
   COLORREF    cForeground ))

API ( unsigned long, RGBAPI,
RGBSetOSDFontW, (
   HRGBOSD     hOSD,
   PLOGFONTW   pFont,
   COLORREF    cForeground ))

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBGetOSDFont   RGBGetOSDFontW
#else
#define RGBGetOSDFont   RGBGetOSDFontA
#endif

API ( unsigned long, RGBAPI,
RGBGetOSDFontA, (
   HRGBOSD     hOSD,
   PLOGFONTA   pFont,
   COLORREF    *pCForeground ))

API ( unsigned long, RGBAPI,
RGBGetOSDFontW, (
   HRGBOSD     hOSD,
   PLOGFONTW   pFont,
   COLORREF    *pCForeground ))

API ( unsigned long, RGBAPI,
RGBSetOSDMargins, (
   HRGBOSD  hOSD,
   long     top,
   long     left,
   long     bottom,
   long     right ))

API ( unsigned long, RGBAPI,
RGBGetOSDMargins, (
   HRGBOSD  hOSD,
   long     *pTop,
   long     *pLeft,
   long     *pBottom,
   long     *pRight ))

API ( unsigned long, RGBAPI,
RGBSetOSDAlignment, (
   HRGBOSD  hOSD,
   ULONG    uHorizontal,
   ULONG    uVertical ))

API ( unsigned long, RGBAPI,
RGBGetOSDAlignment, (
   HRGBOSD  hOSD,
   PULONG   pUHorizontal,
   PULONG   pUVertical ))

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetOSDOwnerDrawnFn, (
   HRGBOSD        hOSD,
   PRGBOSDDRAWFN  pDrawFn,
   ULONG_PTR      userData ))

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetOSDArea, (
   HRGBOSD        hOSD,
   long           top,
   long           left,
   unsigned long  width,
   unsigned long  height ))

API ( unsigned long, RGBAPI,
RGBGetOSDArea, (
   HRGBOSD        hOSD,
   long           *pTop,
   long           *pLeft,
   unsigned long  *pWidth,
   unsigned long  *pHeight ))

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBSetOSDBitmapFilename  RGBSetOSDBitmapFilenameW
#else
#define RGBSetOSDBitmapFilename  RGBSetOSDBitmapFilenameA
#endif

API ( unsigned long, RGBAPI,
RGBSetOSDBitmapFilenameW, (
   HRGBOSD        hOSD,
   const wchar_t  *lpFilename ))

API ( unsigned long, RGBAPI,
RGBSetOSDBitmapFilenameA, (
   HRGBOSD        hOSD,
   const char    *lpFilename ))

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBIsOSDAccelerated, (
   unsigned long  *pBIsSupported ))

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetKeyColour, ( 
   HRGB     hRGB,
   COLORREF cKeyColour ))

API ( unsigned long, RGBAPI,
RGBGetKeyColour, ( 
   HRGB     hRGB,
   COLORREF *pKeyColour ))

   
/******************************************************************************/
   
API ( unsigned long, RGBAPI,
RGBSetRotation, (
   HRGB          hRGB,
   ROTATIONANGLE RotationAngle ))
/*
 * Summary: Sets the angle of rotation for the capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          ROTATIONANGLE IN
 *             A member of the ROTATIONANGLE enumeration defined in RGB.H.
 *
 * Return:  RGBSetRotation either returns 0 if successful or an
 *          appropriate error value.
 */
/******************************************************************************/
   
API ( unsigned long, RGBAPI,
RGBGetRotation, (
   HRGB           hRGB,
   PROTATIONANGLE pRotationAngle ))
/*
 * Summary: Gets the angle of rotation for the capture.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          PROTATIONANGLE IN/OUT
 *             Pointer to a variable that receives a value indicating the
 *             current angle of rotation (see ROTATIONANGLE in RGB.H).
 *
 * Return:  RGBGetRotation either returns 0 if successful or an
 *          appropriate error value.
 */
/******************************************************************************/
   
API ( unsigned long, RGBAPI,
RGBIsRotationSupported, (
   HRGB  hRGB,
   PBOOL pSupported ))
/*
 * Summary: Returns a value that indicates whether rotation is supported
 *          for this capture. Rotation is only supported for captures using
 *          RGBSetWindow.
 *
 * Args:    hRGB IN
 *             The RGB capture handle.
 *
 *          pSupported IN/OUT
 *             Pointer to a variable that receives a value that indicates
 *             whether rotation is supported or not.
 *
 * Return:  RGBIsRotationSupported either returns 0 if successful or an
 *          appropriate error value.
 */
/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetEqualisationMinimum, (
   HRGB        hRGB,
   unsigned long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetEqualisationMaximum, (
   HRGB     hRGB,
   unsigned long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetEqualisationDefault, (
   HRGB        hRGB,
   unsigned long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBGetEqualisation, (
   HRGB        hRGB,
   unsigned long *pValue ) )

/******************************************************************************/

API ( unsigned long, RGBAPI, 
RGBSetEqualisation, (
   HRGB        hRGB,
   unsigned long value ) )

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBInputIsEqualisationSupported, (
   unsigned long  input,
   signed long    *pSupported ))

/******************************************************************************/

API(unsigned long, RGBAPI,
RGBSetLinkRate, (
   HRGB     hRGB,
   LINKRATE value))

/******************************************************************************/

API(unsigned long, RGBAPI,
RGBGetLinkRate, (
   HRGB      hRGB,
   PLINKRATE pValue))

/******************************************************************************/

API(unsigned long, RGBAPI,
RGBInputIsLinkRateSupported, (
   unsigned long  input,
   signed long    *pSupported))

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBAddModeToModeStore, (
   HRGB hRGB,
   BOOL bPrivate ) )

API ( unsigned long, RGBAPI,
RGBDeleteModeFromModeStore, (
   HRGB hRGB,
   BOOL bPrivate ) )

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBIsEDIDSupported, (
  signed long *pBIsSupported ))

API ( unsigned long, RGBAPI,
RGBGetEDID, (
   unsigned long  input,
   char*          pEDID,
   unsigned long  *pnChars ))

API ( unsigned long, RGBAPI,
RGBSetEDID, (
   unsigned long  input,
   char*          pEDID,
   unsigned long  nChars ))

API ( unsigned long, RGBAPI,
RGBIsEDIDEnabled, (
   unsigned long  input,
   unsigned long* pbEnabled ))

API ( unsigned long, RGBAPI,
RGBResetEDID, (
   unsigned long  input ))

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetSignalDetectionMethod, (
   unsigned long  input,
   SIGNALDETECT   method ))

API ( unsigned long, RGBAPI,
RGBGetSignalDetectionMethod, (
   unsigned long  input,
   PSIGNALDETECT  pMethod ))

API ( unsigned long, RGBAPI,
RGBIsSignalDetectionMethodSupported, (
   unsigned long  input,
   signed long    *pBIsSupported ))

/******************************************************************************/

API ( unsigned long, RGBAPI,
RGBSetColourDomain, (
   HRGB                 hRGB,
   COLOURDOMAINDETECT   value ))

API ( unsigned long, RGBAPI,
RGBGetColourDomain, (
   HRGB                 hRGB,
   PCOLOURDOMAINDETECT  pValue ))

API ( unsigned long, RGBAPI,
RGBGetColourDomainDefault, (
   HRGB                 hRGB,
   PCOLOURDOMAINDETECT  pValue ))
   
API ( unsigned long, RGBAPI,
RGBGetColourDomainDetect, (
   HRGB                 hRGB,
   PCOLOURDOMAINDETECT  pValue ))

/******************************************************************************/

API(unsigned long, RGBAPI,
RGBInputIsLiveStreamSupported, (
   unsigned long input,
   signed long *pBIsSupported))

API(unsigned long, RGBAPI,
RGBGetLiveStream, (
   HRGB        hRGB,
   PLIVESTREAM pValue))

API(unsigned long, RGBAPI,
RGBSetLiveStream, (
   HRGB        hRGB,
   LIVESTREAM  value))

/******************************************************************************/

API(unsigned long, RGBAPI,
RGBInputIsMediaSampleTypeSupported, (
   unsigned long           input,
   DGCMEDIASAMPLETYPE      majorType,
   DGCMEDIASAMPLESUBTYPE   subType,
   signed long             *pBIsSupported))

/******************************************************************************/

API( unsigned long, RGBAPI,
RGBGetCaptureRate, (
   HRGB           hRGB,
   unsigned long  *pRate) )

API( unsigned long, RGBAPI,
RGBSetCaptureRate, (
   HRGB           hRGB,
   unsigned long  rate) )

/******************************************************************************/

API( unsigned long, RGBAPI,
RGBGetEncodeBitRateMinimum, (
   HRGB           hRGB,
   unsigned long  *pBitRate) )

API( unsigned long, RGBAPI,
RGBGetEncodeBitRateMaximum, (
   HRGB           hRGB,
   unsigned long  *pBitRate) )

API( unsigned long, RGBAPI,
RGBGetEncodeBitRateDefault, (
   HRGB           hRGB,
   unsigned long  *pBitRate) )

API( unsigned long, RGBAPI,
RGBGetEncodeBitRate, (
   HRGB           hRGB,
   unsigned long  *pBitRate) )

API( unsigned long, RGBAPI,
RGBSetEncodeBitRate, (
   HRGB           hRGB,
   unsigned long  bitRate) )

/******************************************************************************/

API( unsigned long, RGBAPI,
RGBGetEncodeKeyFrameIntervalMinimum, (
   HRGB           hRGB,
   unsigned long  *pInterval) )

API( unsigned long, RGBAPI,
RGBGetEncodeKeyFrameIntervalMaximum, (
   HRGB           hRGB,
   unsigned long  *pInterval) )

API( unsigned long, RGBAPI,
RGBGetEncodeKeyFrameIntervalDefault, (
   HRGB           hRGB,
   unsigned long  *pInterval) )

API( unsigned long, RGBAPI,
RGBGetEncodeKeyFrameInterval, (
   HRGB           hRGB,
   unsigned long  *pInterval) )

API( unsigned long, RGBAPI,
RGBSetEncodeKeyFrameInterval, (
   HRGB           hRGB,
   unsigned long  interval) )

/******************************************************************************/

API( unsigned long, RGBAPI,
RGBGetEncodeProfileDefault, (
   HRGB           hRGB,
   DGCENCPROFILE  *pProfile) )

API( unsigned long, RGBAPI,
RGBGetEncodeProfile, (
   HRGB           hRGB,
   DGCENCPROFILE  *pProfile) )

API( unsigned long, RGBAPI,
RGBIsEncodeProfileSupported, (
   HRGB           hRGB,
   DGCENCPROFILE  profile,
   signed long    *pBIsSupported) )

API( unsigned long, RGBAPI,
RGBSetEncodeProfile, (
   HRGB           hRGB,
   DGCENCPROFILE  profile) )

/******************************************************************************/

API( unsigned long, RGBAPI,
RGBGetEncodeLevelDefault, (
   HRGB           hRGB,
   DGCENCLEVEL    *pLevel) )

API( unsigned long, RGBAPI,
RGBGetEncodeLevel, (
   HRGB           hRGB,
   DGCENCLEVEL    *pLevel) )

API( unsigned long, RGBAPI,
RGBIsEncodeLevelSupported, (
   HRGB           hRGB,
   DGCENCLEVEL    level,
   signed long    *pBIsSupported) )

API( unsigned long, RGBAPI,
RGBSetEncodeLevel, (
   HRGB           hRGB,
   DGCENCLEVEL    level) )

/******************************************************************************/

API( unsigned long, RGBAPI,
RGBEnableInputSnapshot, (
   signed long bEnable ) )

API( unsigned long, RGBAPI,
RGBIsInputSnapshotEnabled, (
   signed long *pBEnabled ) )

API( unsigned long, RGBAPI,
RGBInputGetStatus, (
   unsigned long input,
   RGBINPUTSTATUS *pStatus ) )

/******************************************************************************/

API( unsigned long, RGBAPI,
RGBInputIsGangingSupported, (
   unsigned long input,
   signed long *pBIsSupported ) )

API( unsigned long, RGBAPI,
RGBInputGetGangingType, (
   unsigned long input,
   RGBGANG_TYPE *pType ) )

API( unsigned long, RGBAPI,
RGBInputSetGangingType, (
   unsigned long input,
   RGBGANG_TYPE type ) )

API( unsigned long, RGBAPI,
RGBInputIsGangingTypeSupported, (
   unsigned long input,
   RGBGANG_TYPE type,
   signed long *pBIsSupported ) )

/******************************************************************************/

API( unsigned long, RGBAPI,
RGBIsHDCPSupported, (
   signed long *pBIsSupported ) )

API( unsigned long, RGBAPI,
RGBInputIsHDCPSupported, (
   unsigned long input,
   signed long *pBIsSupported ) )

API( unsigned long, RGBAPI,
RGBInputSetHDCPEnabled, (
   unsigned long input,
   signed long bEnabled ) )

API( unsigned long, RGBAPI,
RGBInputGetHDCPEnabled, (
   unsigned long input,
   signed long *pBEnabled ) )

API( unsigned long, RGBAPI,
RGBInputIsHDCPProtected, (
   unsigned long input,
   signed long *pBProtected ) )

/******************************************************************************/

API(unsigned long, RGBAPI,
RGBInputGetDeviceType, (
   unsigned long input,
   RGBDEVICETYPE *pDeviceType))

/******************************************************************************/


#ifdef API
#undef API
#endif

/******************************************************************************/
