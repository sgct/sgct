/*******************************************************************************

Copyright Datapath Ltd. 2007, 2015.

File:    rgb.h

Purpose: Definitions for a simple RGB interface.


History:
         19 DEC 07    SB   Created.
         08 FEB 08   MJE   Added capture card #defines so they can be 
                           conditionally parsed by the preprocessor.
         28 MAR 08   MJE   Added drawing callback functions.
         03 APR 08   MJE   Added RGBMODEINFO.
         25 JUN 08    SB   Added colour balance to the RGBVALUECHANGEDINFO
                           structure. Changed RGBDRAWNOSIGNALFN and
                           RGBDRAWINVALIDSIGNALFN to bring them in line with
                           the other callback functions.
         02 JUN 09    SB   Added support for Vision-SD8.
         24 JUN 09    SB   Added YUY2 PIXELFORMAT.
         28 AUG 09    SB   Added RGB_ERROR_UNSUPPORTED.
         02 OCT 09    SB   Moved DEINTERLACE enumeration.
         11 FEB 10    SB   Added SIGNALTYPE enumeration.
         10 JUN 10   MJE   Added OSD functionality. Changed callback user data
                           to ULONG_PTR to simplify future 64 bit port.
         11 JUN 10   MJE   Removed OSD_ORIGIN.
         08 SEP 10    RL   Added RGBINPUTINFO.
         28 OCT 10    TM   Added ROTATIONANGLE enumeration and rotation defs.
         02 NOV 10    SB   Changed HRGBDLL and HRGB handles to ULONG_PTRs.
                           Added RGBDetectInput.
         04 JUL 11    DJ   Added VideoStandard member to RGBMODECHANGEDINFO
         25 JUL 11    SB   Added SDI and DLDVI to SIGNALTYPE enumeration.
                           Changed RGBMODECHANGEDINFO and RGBMODEINFO to
                           support SDI.
                           Added Equalisation to RGBVALUECHANGEDINFO.
         28 JUL 11    RL   RGBFRAMECAPTUREDFNEX and RGBFRAMEDATA for obtaining
                           hardware capture timestamps.
         03 AUG 11    RL   Changed RGBVALUECHANGEDINFO structure members to 
                           match type definitions of there eqivalent atomic 
                           function argument.
         17 JAN 12    RL   Added DeviceName to RGBINPUTINFO.
         24 JAN 12    SB   Fixed incorrect declarations of RGBINPUTINFOA and 
                           RGBINPUTINFOW.
         06 FEB 12   MJE   Added owner drawn callback function definition for
                           OSD.
         25 APR 12    JW   Changed HRGBOSD type to ULONG_PTR.
         25 APR 12    SB   Added RGBChainOutputBufferEx.
         03 OCT 12    SB   Added RGBSetSignalDetectionMethod,
                           RGBSetSignalDetectionMethod and
                           RGBIsSignalDetectionMethodSupported.
         14 NOV 12    OM   Added uniform interface to support NVidia GPUDirect
                           and AMD DirectGMA.
         14 MAR 13    JE   Added RGBSetColourDomain() and RGBGetColourDomain()
                           support.
         18 JUL 13    OM   Added OSD bitmap functionality.
         09 AUG 13    JL   Added RGB_COLOURDOMAINDETECT_AUTO.
         29 MAY 14    SB   Added NV12 capture format.
         10 JUN 14    SB   Added dgctypes.h and dgcmedia.h include file to
                           support the DGCMEDIASAMPLE
         17 JUN 14    SB   Added RGBGetLiveStream and RGBSetLiveStream.
         14 JUL 14    JL   Added functions for controlling encoder parameters.
         26 OCT 15    JL   Added RGBEnableInputSnapshot,
                           RGBIsInputSnapshotEnabled and RGBInputGetStatus.
         16 JUN 16    JL   Added support for ganging.
         29 MAR 17    SB   Added RGBInputIsHDMISupported,
                           RGBInputIsDisplayPortSupported and
                           RGBInputGetDeviceType.

*******************************************************************************/

#ifndef _RGB_H_
#define _RGB_H_

#include <dgctypes.h>
#include <dgcmedia.h>

#include <pshpack1.h>

/******************************************************************************/

/* The calling convention of the RGB SDK callback functions. */

#define RGBCBKAPI __stdcall

/******************************************************************************/

/* The RGB load handle. */
typedef ULONG_PTR HRGBDLL, *PHRGBDLL;

/* The RGB capture handle. */
typedef ULONG_PTR HRGB, *PHRGB;

/******************************************************************************/

#define CAPTURECARD_DGC103    0
#define CAPTURECARD_DGC133    1

/* The CAPTURESTATE enumerated type contains a list of the capture cards
 * returned by RGBGetCaptureCard. */
typedef enum _CAPTURECARD
{
   RGB_CAPTURECARD_DGC103     = CAPTURECARD_DGC103,
   RGB_CAPTURECARD_DGC133     = CAPTURECARD_DGC133,
}  CAPTURECARD, *PCAPTURECARD;

/******************************************************************************/

typedef enum _RGBDEVICETYPE
{
   DEVICETYPE133 = 133,
   DEVICETYPE139 = 139,
   DEVICETYPE144 = 144,
   DEVICETYPE150 = 150,
   DEVICETYPE151 = 151,
   DEVICETYPE139S = 1139,
   DEVICETYPE150S = 1150,
   DEVICETYPE151S = 1151,
   DEVICETYPE153 = 153,
   DEVICETYPE154 = 154,
   DEVICETYPE159 = 159,
   DEVICETYPE161 = 161,
   DEVICETYPE165 = 165,
   DEVICETYPE167 = 167,
   DEVICETYPE168 = 168,
   DEVICETYPE179 = 179,
   DEVICETYPE182 = 182,
   DEVICETYPE184 = 184,
   DEVICETYPE200 = 200,
   DEVICETYPE204 = 204,
   DEVICETYPE205 = 205,
}  RGBDEVICETYPE, *PRGBDEVICETYPE;

/******************************************************************************/

/* The CAPTURESTATE enumerated type contains a list of the capture states
 * returned by RGBGetCaptureState. */

typedef enum _CAPTURESTATE
{
   RGB_STATE_CAPTURING        = 0,
   RGB_STATE_NOSIGNAL         = 1,
   RGB_STATE_INVALIDSIGNAL    = 2,
   RGB_STATE_PAUSED           = 3,
   RGB_STATE_ERROR            = 4,
}  CAPTURESTATE, *PCAPTURESTATE;

/******************************************************************************/

/* The PIXELFORMAT enumerated type contains a list of possible pixel formats.
 * Please not that a capture card may only support a subset of these pixel
 * formats. */

/******************************************************************************/

typedef enum _PIXELGAMUT
{
   RGB_PIXELGAMUT_709 = 0,
   RGB_PIXELGAMUT_601 = 1,
   RGB_PIXELGAMUT_2020 = 2,
}  PIXELGAMUT, *PPIXELGAMUT;

typedef enum _PIXELRANGE
{
   RGB_PIXELRANGE_FULL = 0,
   RGB_PIXELRANGE_LIMITED = 1,
}  PIXELRANGE, *PPIXELRANGE;

typedef enum _PIXELFORMAT
{
   RGB_PIXELFORMAT_AUTO = 0,
   RGB_PIXELFORMAT_555  = 1,
   RGB_PIXELFORMAT_565  = 2,
   RGB_PIXELFORMAT_888  = 3,
   RGB_PIXELFORMAT_GREY = 4,
   RGB_PIXELFORMAT_RGB24 = 5,
   RGB_PIXELFORMAT_YUY2 = 6,
   RGB_PIXELFORMAT_YVYU = 7,
   RGB_PIXELFORMAT_UYVY = 8,
   RGB_PIXELFORMAT_NV12 = 9,
   RGB_PIXELFORMAT_YV12 = 10,
   RGB_PIXELFORMAT_RGB10 = 11,
   RGB_PIXELFORMAT_Y410 = 12,
}  PIXELFORMAT, *PPIXELFORMAT;

/******************************************************************************/

/* The DEINTERLACE enumerated type contains a list of possible de-interlace
   options for interlaced capture sources. Please note that the capture card
   in conjunction with the display card may only support a subset of these
   de-interlace options. */

typedef enum _DEINTERLACE
{
   RGB_DEINTERLACE_WEAVE = 0,
   RGB_DEINTERLACE_BOB = 1,
   RGB_DEINTERLACE_FIELD_0 = 2,
   RGB_DEINTERLACE_FIELD_1 = 3,
}  DEINTERLACE, *PDEINTERLACE;

/******************************************************************************/

/* Frame or Field (default is frame) */
#define RGB_FRAME_FLAG_FIELD_0     0x0000L
#define RGB_FRAME_FLAG_FIELD_1     0x0001L

/******************************************************************************/

/* The SIGNALTYPE enumerated type contains a list of possible signal types
   returned by RGBGetInputSignalType. */

typedef enum _SIGNALTYPE
{
   RGB_SIGNALTYPE_NOSIGNAL = 0,
   RGB_SIGNALTYPE_VGA = 1,
   RGB_SIGNALTYPE_DVI = 2,
   RGB_SIGNALTYPE_YPRPB = 3,
   RGB_SIGNALTYPE_COMPOSITE = 4,
   RGB_SIGNALTYPE_SVIDEO = 5,
   RGB_SIGNALTYPE_OUTOFRANGE = 6,
   RGB_SIGNALTYPE_SDI = 7,
   RGB_SIGNALTYPE_DLDVI = 8,
   RGB_SIGNALTYPE_DISPLAYPORT = 9,
}  SIGNALTYPE, *PSIGNALTYPE;

typedef enum _BUFFERTYPE
{
   RGB_BUFFERTYPE_MAPPED = 0,
   RGB_BUFFERTYPE_DIRECTGMA = 1,
   RGB_BUFFERTYPE_GPUDIRECT = 2,
}  BUFFERTYPE, *PBUFFERTYPE;

/******************************************************************************/

/* The RGBFRAMECAPTUREDFN function is an application-defined callback function
 * used with RGBSetFrameCapturedFn. The RGB SDK calls this function when a frame
 * of RGB data has been captured.
 *
 * The pBitmapInfo and pBitmapBits pointers will be NULL when DirectDMA has been
 * enabled. This is because the RGB data is not available as it has been
 * transfered by DMA directly from the RGB capture card to the Display card. */

typedef void (RGBCBKAPI RGBFRAMECAPTUREDFN) (
   HWND                 hWnd,          /* Handle to the RGB Window. */
   HRGB                 hRGB,          /* Handle to the RGB Capture. */
   LPBITMAPINFOHEADER   pBitmapInfo,   /* Pointer to a BITMAPINFOHEADER structure. */
   void                 *pBitmapBits,  /* Pointer to the bitmap bits. */
   ULONG_PTR            userData );    /* Application defined context. */
typedef RGBFRAMECAPTUREDFN *PRGBFRAMECAPTUREDFN;

typedef struct tagRGBFrameData
{
   unsigned long        Size;

   LPBITMAPINFOHEADER   PBitmapInfo;   /* Pointer to a BITMAPINFOHEADER structure. */
   void                 *PBitmapBits;  /* Pointer to the bitmap bits. */
   unsigned long        FrameFlags;    /* Frame flags, RGB_FRAME_FLAG_. */
   ULONGLONG            TimeStamp;     /* Captured frame time stamp. */
   
} RGBFRAMEDATA, *PRGBFRAMEDATA;

typedef void (RGBCBKAPI RGBFRAMECAPTUREDFNEX) (
   HWND                 hWnd,          /* Handle to the RGB Window. */
   HRGB                 hRGB,          /* Handle to the RGB Capture. */
   PRGBFRAMEDATA        pFrameData,    /* Further frame information. */
   ULONG_PTR            userData );    /* Application defined context. */
typedef RGBFRAMECAPTUREDFNEX *PRGBFRAMECAPTUREDFNEX;

typedef void (RGBCBKAPI RGBMEDIASAMPLECAPTUREDFN) (
   HWND              hWnd,          /* Handle to the RGB Window. */
   HRGB              hRGB,          /* Handle to the RGB Capture. */
   PDGCMEDIASAMPLE   pMediaSample,  /* Pointer to the media sample. */
   ULONG_PTR         userData);     /* Application defined context. */
typedef RGBMEDIASAMPLECAPTUREDFN *PRGBMEDIASAMPLECAPTUREDFN;

/******************************************************************************/

/* The RGBMODECHANGEDFN function is an application-defined callback function
 * used with RGBSetModeChangedFn. The RGB SDK calls this function when a new
 * video mode has been detected. */

typedef enum _ANALOG_INPUT_TYPE
{
   RGB_TYPE_VGA   = 0,
   RGB_TYPE_VIDEO = 1,
}  ANALOG_INPUT_TYPE, *PANALOG_INPUT_TYPE;

typedef enum _DIGITAL_INPUT_TYPE
{
   RGB_TYPE_DVI = 0,
   RGB_TYPE_SDI = 1,
   RGB_TYPE_DISPLAYPORT = 2,
}  DIGITAL_INPUT_TYPE, *PDIGITAL_INPUT_TYPE;

typedef struct
{
   unsigned long  Size;                /* Size of this structure. */
   unsigned long  RefreshRate;         /* The horizontal refresh rate in Hertz. */
   unsigned long  LineRate;            /* The vertical line rate in Hertz. */
   unsigned long  TotalNumberOfLines;  /* The total number of lines. */
   long           BInterlaced;         /* A boolean indicating an interlaced
                                          mode has been detected. */
   long           BDVI;                /* A boolean indicating a digital mode
                                          has been detected. */
   union
   {
      ANALOG_INPUT_TYPE    AnalogType; /* The analog input type connected. */
      DIGITAL_INPUT_TYPE   DigitalType;/* The digital input type connected. */
   };
   unsigned long  VideoStandard;       /* If RGB_TYPE_VIDEO, the video standard */
}  RGBMODECHANGEDINFO, *PRGBMODECHANGEDINFO;

typedef void (RGBCBKAPI RGBMODECHANGEDFN)  (
   HWND                 hWnd,                /* Handle to the RGB Window. */
   HRGB                 hRGB,                /* Handle to the RGB Capture. */
   PRGBMODECHANGEDINFO  pModeChangedInfo,    /* Pointer to a RGBMODECHANGEDINFO
                                                structure. */
   ULONG_PTR            userData );          /* Application defined context. */
typedef RGBMODECHANGEDFN   *PRGBMODECHANGEDFN;

/******************************************************************************/

/* The RGBMODEINFO structure is filled by RGBGetModeInfo. It describes the 
 * current mode being captured. If State is RGB_STATE_CAPTURING then all the 
 * fields are valid. If State is RGB_STATE_NOSIGNAL then RefreshRate, LineRate,
 * TotalNumberOfLines, BInterlaced and BDVI are all invalid. If State is 
 * RGB_STATE_INVALIDSIGNAL RefreshRate and LineRate contain the out of range
 * frequencies, and BDVI indicates whether the signal is DVI. */
typedef struct
{
   unsigned long  Size;                /* Size of this structure. */

   CAPTURESTATE   State;               /* The state of the capture. */

   unsigned long  RefreshRate;         /* The horizontal refresh rate in Hertz. */
   unsigned long  LineRate;            /* The vertical line rate in Hertz. */
   unsigned long  TotalNumberOfLines;  /* The total number of lines. */
   long           BInterlaced;         /* A boolean indicating an interlaced
                                          mode has been detected. */
   long           BDVI;                /* A boolean indicating a digital mode
                                          has been detected. */
   union
   {
      ANALOG_INPUT_TYPE AnalogType;    /* The analog input type connected. */
      DIGITAL_INPUT_TYPE DigitalType;  /* The analog input type connected. */
   };
   unsigned long  VideoStandard;       /* If RGB_TYPE_VIDEO, the video standard */
}  RGBMODEINFO, *PRGBMODEINFO;

/******************************************************************************/

/* The RGBNOSIGNALFN function is an application-defined callback function
 * used with RGBSetNoSignalFn. The RGB SDK calls this function when a video
 * signal cannot be detected. */

typedef void (RGBCBKAPI RGBNOSIGNALFN) (
   HWND           hWnd,                /* Handle to the RGB Window. */
   HRGB           hRGB,                /* Handle to the RGB Capture. */
   ULONG_PTR      userData );          /* Application defined context. */
typedef RGBNOSIGNALFN  *PRGBNOSIGNALFN;

/* The RGBDRAWNOSIGNALFN function is an application defined callback function
 * used with RGBSetDrawNoSignalFn. The RGB SDK calls this function to draw the
 * window when a video signal cannot be detected. */
typedef void (RGBCBKAPI RGBDRAWNOSIGNALFN) (
   HWND           hWnd,
   HRGB           hRGB,
   HDC            hDC,
   ULONG_PTR      userData );
typedef RGBDRAWNOSIGNALFN  *PRGBDRAWNOSIGNALFN;

/******************************************************************************/

/* The RGBINVALIDSIGNALFN function is an application-defined callback function
 * used with RGBSetInvalidSignalFn. The RGB SDK calls this function when a video
 * signal beyond the capabilities of the hardware is detected. */

typedef void (RGBCBKAPI RGBINVALIDSIGNALFN) (
   HWND           hWnd,                /* Handle to the RGB Window. */
   HRGB           hRGB,                /* Handle to the RGB Capture. */
   unsigned long  horClock,
   unsigned long  verClock,
   ULONG_PTR      userData );          /* Application defined context. */
typedef RGBINVALIDSIGNALFN *PRGBINVALIDSIGNALFN;

/* The RGBDRAWINVALIDSIGNALFN function is an application defined callback
 * function used with RGBSetDrawInvalidSignalFn. The RGB SDK calls
 * this function to draw the window when a video signal is invalid. */
typedef void (RGBCBKAPI RGBDRAWINVALIDSIGNALFN) (
   HWND           hWnd,
   HRGB           hRGB,
   HDC            hDC,
   unsigned long  horClock,
   unsigned long  verClock,
   ULONG_PTR      userData );
typedef RGBDRAWINVALIDSIGNALFN   *PRGBDRAWINVALIDSIGNALFN;

/******************************************************************************/

typedef void (RGBCBKAPI RGBERRORFN) (
   HWND           hWnd,                /* Handle to the RGB Window. */
   HRGB           hRGB,                /* Handle to the RGB Capture. */
   unsigned long  error,               /* The error. */
   ULONG_PTR      userData,            /* Application defined context. */
   unsigned long  *pReserved );
typedef RGBERRORFN *PRGBERRORFN;

/******************************************************************************/

/* The RGBVALUECHANGEDFN function is an application-defined callback function
 * used with RGBSetModeValueChangedFn. The RGB SDK calls this function when a
 * value new video mode has been detected. */

typedef struct
{
   long        BChanged;      /* Flag to indicate that value has changed. */
   signed long Value;         /* The new signed value. */
}  SIGNEDVALUE;

typedef struct
{
   long           BChanged;   /* Flag to indicate that value has changed. */
   unsigned long  Value;      /* The new unsigned value. */
}  UNSIGNEDVALUE;

typedef struct
{
   unsigned long  Size;                /* Size of this structure. */
   SIGNEDVALUE    HorPosition;         /* Horizontal Position. */
   UNSIGNEDVALUE  HorScale;            /* Horizontal Scale. */
   SIGNEDVALUE    VerPosition;         /* Vertical Position. */
   UNSIGNEDVALUE  CaptureWidth;        /* Capture Width. */
   UNSIGNEDVALUE  CaptureHeight;       /* Capture Height. */
   SIGNEDVALUE    Brightness;          /* Brightness. */
   SIGNEDVALUE    Contrast;            /* Contrast. */
   SIGNEDVALUE    BlackLevel;          /* Black level. */
   SIGNEDVALUE    Phase;               /* Phase. */
   SIGNEDVALUE    RedGain;             /* RedGain. */
   SIGNEDVALUE    GreenGain;           /* GreenGain. */
   SIGNEDVALUE    BlueGain;            /* BlueGain. */
   SIGNEDVALUE    RedOffset;           /* RedOffset. */
   SIGNEDVALUE    GreenOffset;         /* GreenOffset. */
   SIGNEDVALUE    BlueOffset;          /* BlueOffset. */
   SIGNEDVALUE    Saturation;          /* Saturation. */
   SIGNEDVALUE    Hue;                 /* Hue. */
   UNSIGNEDVALUE  VideoStandard;       /* VideoStandard. */
   UNSIGNEDVALUE  Equalisation;        /* Equalisation. */
   UNSIGNEDVALUE  ColourDomain;        /* Colour Domain. */
   UNSIGNEDVALUE  LinkRate;            /* LinkRate. */
}  RGBVALUECHANGEDINFO, *PRGBVALUECHANGEDINFO;

typedef void (RGBCBKAPI RGBVALUECHANGEDFN)  (
   HWND                 hWnd,                /* Handle to the RGB Window. */
   HRGB                 hRGB,                /* Handle to the RGB Capture. */
   PRGBVALUECHANGEDINFO pValueChangedInfo,   /* Pointer to a RGBVALUECHANGEDINFO
                                                structure. */
   ULONG_PTR            userData );          /* Application defined context. */
typedef RGBVALUECHANGEDFN   *PRGBVALUECHANGEDFN;

/******************************************************************************/

typedef struct tagDriverVer
{
   unsigned long Major;
   unsigned long Minor;
   unsigned long Micro;
   unsigned long Revision;

} RGBDRIVERVER, *PRGBDRIVERVER;

typedef struct tagLocation
{
   unsigned long Bus;
   unsigned long Device;
   unsigned long Function;

} RGBLOCATION, *PRGBLOCATION;

typedef struct tagChassis
{
   unsigned long Index;       /* 0 for host, 1+ for backplanes. */
   unsigned long Slot;        /* Expansion slot number. */

} RGBCHASSIS, *PRGBCHASSIS;

#if ( defined ( _UNICODE ) || defined ( UNICODE ))
#define RGBINPUTINFO   RGBINPUTINFOW
#define PRGBINPUTINFO  PRGBINPUTINFOW

#else
#define RGBINPUTINFO   RGBINPUTINFOA
#define PRGBINPUTINFO  PRGBINPUTINFOA
#endif

typedef struct tagRGBDevInfoA
{
   unsigned long  Size;

   RGBDRIVERVER   Driver;
   RGBLOCATION    Location;
   unsigned long  FirmWare;
   unsigned long  VHDL;
   unsigned long  Identifier[2];
   CHAR           DeviceName[256];
   RGBCHASSIS     Chassis;
   unsigned long  ParentBus;
} RGBINPUTINFOA, *PRGBINPUTINFOA;

typedef struct tagRGBDevInfoW
{
   unsigned long  Size;

   RGBDRIVERVER   Driver;
   RGBLOCATION    Location;
   unsigned long  FirmWare;
   unsigned long  VHDL;
   unsigned long  Identifier[2];
   WCHAR          DeviceName[256];
   RGBCHASSIS     Chassis;
   unsigned long  ParentBus;
} RGBINPUTINFOW, *PRGBINPUTINFOW;

/******************************************************************************/

typedef ULONG_PTR HRGBOSD, *PHRGBOSD;

typedef enum
{
   RGBOSD_TYPE_DISABLED    = 0,
   RGBOSD_TYPE_TEXT        = 1,
   RGBOSD_TYPE_OWNERDRAWN  = 2,
   RGBOSD_TYPE_BITMAP      = 3,

   RGBOSD_NUM_TYPES,
}  RGBOSD_TYPE;
/*****************************************************************************/

/* The RGBOSDDRAWFN function is an application defined callback function
 * for implementing owner drawn OSD. The RGB SDK calls this function to draw the
 * OSD when the window is accelerated. */
typedef void (RGBCBKAPI RGBOSDDRAWFN) (
   HWND           hWnd,
   HRGB           hOSD,
   HDC            hDC,
   ULONG_PTR      userData );
   
typedef RGBOSDDRAWFN  *PRGBOSDDRAWFN;

/*****************************************************************************/

 /* The ROTATIONANGLE enumerated type contains a list of rotation angles.
    Values in this enumeration must correspond exactly to those in 
    D3D_ROTATION in d3dapi.h */

/******************************************************************************/

typedef enum _ROTATIONANGLE
{
   RGB_ROTATIONANGLE_0   = 0,
   RGB_ROTATIONANGLE_90  = 1,
   RGB_ROTATIONANGLE_180 = 2,
   RGB_ROTATIONANGLE_270 = 3,   
   RGB_MAXANGLES         = 4,

}  ROTATIONANGLE, *PROTATIONANGLE;

/******************************************************************************/

#define MINIMUM_ROTATED_WIDTH  213
#define MINIMUM_ROTATED_HEIGHT 160

/******************************************************************************/

#define RGBOSD_HOR_LEFT    0x0000
#define RGBOSD_HOR_CENTRE  0x0001
#define RGBOSD_HOR_RIGHT   0x0002

#define RGBOSD_VER_TOP     0x0000
#define RGBOSD_VER_CENTRE  0x0001
#define RGBOSD_VER_BOTTOM  0x0002

/******************************************************************************/

 /* DirectGPU definitions */

/******************************************************************************/
typedef enum _GRAPHICSHARDWARE
{
   GPU_AMD = 1,
   GPU_NVIDIA = 2,
}  GRAPHICSHARDWARE, *PGRAPHICSHARDWARE;

typedef struct
{
   unsigned int         Size;
   unsigned int         **Buffer;
   unsigned long        Width;
   unsigned long        Height;
   unsigned int         OglByteFormat;
   unsigned int         OglColourFormat;
   unsigned int         FormatSize;
   unsigned int         *OglObject;
   unsigned int         NumBuffers;
   GRAPHICSHARDWARE     GpuBrand;
   unsigned int         BufferSize;
} GPUTRANSFERDESCRIPTOR, *PGPUTRANSFERDESCRIPTOR;

typedef enum _NVIDIAOP
{
   NVIDIA_GPU_COPY = 0,
   NVIDIA_GPU_WAIT = 1,
   NVIDIA_GPU_END = 2,
}  NVIDIAOP, *PNVIDIAOP;

/******************************************************************************/

typedef enum _SIGNALDETECT
{
   RGB_SIGNALDETECT_AUTOMATIC = 0,
   RGB_SIGNALDETECT_ANALOG = 1,
   RGB_SIGNALDETECT_DVI = 2,
}  SIGNALDETECT, *PSIGNALDETECT;

/******************************************************************************/

typedef enum _COLOURDOMAINDETECT
{
   RGB_COLOURDOMAINDETECT_RGB709_FULL = 0,
   RGB_COLOURDOMAINDETECT_YUV709_FULL = 1,
   RGB_COLOURDOMAINDETECT_YUV601_FULL = 2,
   RGB_COLOURDOMAINDETECT_YUV709_STUDIO = 3,
   RGB_COLOURDOMAINDETECT_YUV601_STUDIO = 4,
   RGB_COLOURDOMAINDETECT_AUTO = 5,
   RGB_COLOURDOMAINDETECT_RGB709_STUDIO = 6,
   RGB_COLOURDOMAINDETECT_YUV2020_FULL = 7,
   RGB_COLOURDOMAINDETECT_YUV2020_STUDIO = 8,
   RGB_COLOURDOMAINDETECT_RGB601_FULL = 9,
   RGB_COLOURDOMAINDETECT_RGB601_STUDIO = 10,
   RGB_COLOURDOMAINDETECT_RGB2020_FULL = 11,
   RGB_COLOURDOMAINDETECT_RGB2020_STUDIO = 12,
   RGB_COLOURDOMAINDETECT_LAST = 13,

   // Backwards compatibility
   RGB_COLOURDOMAINDETECT_RGB = RGB_COLOURDOMAINDETECT_RGB709_FULL,
   RGB_COLOURDOMAINDETECT_HDYPrPb = RGB_COLOURDOMAINDETECT_YUV709_FULL,
   RGB_COLOURDOMAINDETECT_YPrPb = RGB_COLOURDOMAINDETECT_YUV601_FULL,
   RGB_COLOURDOMAINDETECT_HDYCrCb = RGB_COLOURDOMAINDETECT_YUV709_STUDIO,
   RGB_COLOURDOMAINDETECT_YCrCb = RGB_COLOURDOMAINDETECT_YUV601_STUDIO,
} COLOURDOMAINDETECT, *PCOLOURDOMAINDETECT;

/******************************************************************************/

typedef enum _LIVESTREAM{
   LIVESTREAM_OFF   = 0,
   LIVESTREAM_1     = 1,
} LIVESTREAM, *PLIVESTREAM;

typedef enum _LIVESTREAM_SUPPORT
{
   RGB_LIVESTREAM_UNSUPPORTED = 0,
   RGB_LIVESTREAM_SELECTABLE = 1,
   RGB_LIVESTREAM_ALWAYS_ON = 2,
} LIVESTREAM_SUPPORT, *PLIVESTREAM_SUPPORT;

/******************************************************************************/

typedef enum _LINKRATE {
   RGB_LINKRATE_RBR = 0,
   RGB_LINKRATE_HBR = 1,
   RGB_LINKRATE_HBR2 = 2,
} LINKRATE, *PLINKRATE;

/******************************************************************************/

typedef enum _RGBINPUTSTATUS
{
   RGBINPUT_NOERROR = 0,

   // The input is not present in the system.
   RGBINPUT_INVALID = 1,

   // The input is not included in the snapshot.
   RGBINPUT_UNAVAILABLE = 2,

} RGBINPUTSTATUS, *PRGBINPUTSTATUS;

/******************************************************************************/

typedef enum _RGBGANG_TYPE
{
   RGBGANG_TYPE_DISABLED = 0,
   RGBGANG_TYPE_2x1      = 4,
   RGBGANG_TYPE_1x2      = 5,
   RGBGANG_TYPE_3x1      = 6,
   RGBGANG_TYPE_1x3      = 7,
   RGBGANG_TYPE_4x1      = 2,
   RGBGANG_TYPE_1x4      = 3,
   RGBGANG_TYPE_2x2      = 1,
} RGBGANG_TYPE, *PRGBGANG_TYPE;

/******************************************************************************/

#define RGB_ERROR_BASE                             0x01160000

#define RGB_ERROR_UNKNOWN                          ( RGB_ERROR_BASE + 0x0000 )
#define RGB_ERROR_OUTOFRANGE                       ( RGB_ERROR_BASE + 0x0001 )
#define RGB_ERROR_INVALIDINPUT                     ( RGB_ERROR_BASE + 0x0002 )
#define RGB_ERROR_WINDOWINUSE                      ( RGB_ERROR_BASE + 0x0003 )
#define RGB_ERROR_OSDATTACHED                      ( RGB_ERROR_BASE + 0x0004 )
#define RGB_ERROR_DETECTVIDEOMODE                  ( RGB_ERROR_BASE + 0x0005 )
#define RGB_ERROR_INVALIDBUFFER                    ( RGB_ERROR_BASE + 0x0006 )
#define RGB_ERROR_INVALIDHANDLE                    ( RGB_ERROR_BASE + 0x0007 )
#define RGB_ERROR_UNSUPPORTED                      ( RGB_ERROR_BASE + 0x0008 )
#define RGB_ERROR_INVALIDOSD                       ( RGB_ERROR_BASE + 0x0009 )
#define RGB_ERROR_INVALIDDATA                      ( RGB_ERROR_BASE + 0x000a )
#define RGB_ERROR_AUDIOINPUT                       ( RGB_ERROR_BASE + 0x000b )

/******************************************************************************/

#include <poppack.h>

/******************************************************************************/

#endif //_RGB_H_

/******************************************************************************/
