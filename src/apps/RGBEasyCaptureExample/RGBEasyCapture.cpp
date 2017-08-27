/*******************************************************************************

Copyright Datapath Ltd. 2008, 2014.

Based on SAMPLE5.C

Purpose: VisionRGB-X example program that shows how to display an RGB window
using AMD DirectGMA or NVIDIA GPUDirect technology.

History:
04 SEP 12    OM   Created.
13 AUG 13    OM   Added check for no signal or out of range signal.
11 OCT 13    OM   Corrected bug, passing wrong parameter for NVIDIA.
08 APR 14    OM   Added greyscale option.
10 OCT 14    OM   Fixed NVIDIA bug and tidied up code.
03 JUN 15    OM   DMA to the GPU should not be done if buffer is returned to capture driver.

*******************************************************************************/

/*******************************************************************************

Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h

History:
23 AUG 17	  ES   Added Ganging for 2x1 and rendered side-by-side to top-bottom
				   Made changes to work with SGCT, and OpenGL fixes.
			       Removed threading for now.

*******************************************************************************/

#include "RGBEasyCapture.hpp"
#include <sgct.h>

#include <rgb.h>
#include <rgbapi.h>
#include <rgberror.h>

#define RGB_UNKNOWN  0
#define RGB_565      1
#define RGB_24       2
#define RGB_888      3

#define FRGB8        8
#define FRGB565      16
#define FRGB24       24
#define FRGB888      32

#define NUM_BUFFERS  2
#define ONE_SECOND   1000 * 1000 * 1000L
#define NO_BUFFER    0xFF


/* Static Constants ***********************************************************/

static const TCHAR
RGBWindowClass[] = { TEXT("RGBSampleWndClass") },
Caption[] = { TEXT("RGB Sample 5 Dual/Ganging") };

static struct
{
	COLORREF Mask[3];
}  ColourMasks[] =
{
	{ 0x00000000, 0x00000000, 0x00000000, },  /* Unknown */
	{ 0x0000f800, 0x000007e0, 0x0000001f, },  /* RGB565 */
	{ 0x00ff0000, 0x0000ff00, 0x000000ff, },  /* RGB24 */
	{ 0x00ff0000, 0x0000ff00, 0x000000ff, },  /* RGB888 */
};

#ifdef _DEBUG
typedef struct _TRACK
{
	/*
	Valid buffer State variables:

	A = Buffer filled and awaiting OpenGL render call in this application
	C = Buffer chained within RGBEasy awiting a DMA
	R = Buffer filled and in OpenGL blocking render call
	*/
	TCHAR                State;
} TRACK, *PTRACK;
#endif

/* Global Variables ***********************************************************/
typedef struct
{
	HINSTANCE            HInstance;
	HRGBDLL              HRGBDLL;
	HANDLE               HOGLSetupEvent;
	HANDLE               HMutex;
	int                  XPos;
	int                  YPos;
	unsigned long 		 Width;
	unsigned long    	 Height;
	unsigned long        RefreshRate;
	SIGNALTYPE           SignalType;
	unsigned long        Input;
	unsigned int         CaptureFormat;
	GLenum		         ColourFormat;
	GLenum		         ByteFormat;
	PIXELFORMAT          RgbFormat;
	unsigned int         FormatSize;
	unsigned int         BufferIndex;
	unsigned int         *PDataBuffer;
	GLuint               OGLBuffer[NUM_BUFFERS];
	GLuint               OGLTexture[NUM_BUFFERS];
	int                  Viewport[4];
	LPBITMAPINFO         PBitmapInfo[NUM_BUFFERS];
	HRGB                 HRGB;
	BOOL                 BChainBuffer;
	GRAPHICSHARDWARE     Hardware;
	HANDLE               HCapturedBuffer;
	unsigned long        Error;
#ifdef _DEBUG
	TRACK                BufferTrack[NUM_BUFFERS];
	signed long          NumDroppedBuffers;
	signed long          NumRepeatedBuffers;
#endif
} GLOBAL;

static GLOBAL Global =
{
	NULL,                      /* HInstance */
	0,                         /* HRGBDLL */
	NULL,                      /* HOGLSetupEvent */
	NULL,                      /* HMutex */
	CW_USEDEFAULT,             /* XPos */
	CW_USEDEFAULT,             /* YPos */
	800,                       /* Width */
	600,                       /* Height */
	0,                         /* RefreshRate */
	RGB_SIGNALTYPE_NOSIGNAL,   /* SignalType */
	0,                         /* Input */
	FRGB24,                    /* CaptureFormat possible values: FRGB8, FRGB565, FRGB24, FRGB888 */
	0,                         /* ColourFormat */
	0,                         /* ByteFormat */
	(PIXELFORMAT)0,            /* RgbFormat */
	0,                         /* FormatSize */
	NO_BUFFER,                 /* BufferIndex */
	NULL,                      /* PDataBuffer */
	{ 0 },                     /* OGLBuffer */
	{ 0 },                     /* OGLTexture */
	{ 0 },                     /* Viewport */
	{ NULL },                  /* PBitmapInfo */
	0,                         /* HRGB */
	TRUE,                      /* BChainBuffer */
	GPU_AMD,                   /* Hardware */
	NULL,                      /* HCapturedBuffer */
	0,                         /* Error */
#ifdef _DEBUG
	{ 0 },                     /* BufferTrack */
	0,                         /* NumDroppedBuffers */
	0,                         /* NumRepeatedBuffers */
#endif
};

static int currentBufferIndex = 0;
static GLsync currentFence = 0;

unsigned long
DoGanging(unsigned long input)
{
	unsigned long error;
	long bSupported;

	error = RGBInputIsGangingSupported(input, &bSupported);

	if ((error == 0) && bSupported)
	{
		// attempt to support a 4-input card, and a 2-input card
		RGBGANG_TYPE type[] = { RGBGANG_TYPE_2x2, RGBGANG_TYPE_2x1 };
		long i;

		for (i = 0; i < _countof(type); i++)
		{
			error = RGBInputIsGangingTypeSupported(input, type[i], &bSupported);

			if ((error == 0) && bSupported)
			{
				break;
			}
		}

		if (error == 0)
		{
			RGBGANG_TYPE current;

			error = RGBInputGetGangingType(input, &current);

			if ((error == 0) && (current != type[i]))
			{
				error = RGBInputSetGangingType(input, type[i]);
			}
		}
	}

	return error;
}

/******************************************************************************/

/******************************************************************************/

void
CreateBitmapInformation(
	BITMAPINFO  *pBitmapInfo,
	int         width,
	int         height,
	int         bitCount)
{
	pBitmapInfo->bmiHeader.biWidth = width;
	pBitmapInfo->bmiHeader.biHeight = -height;
	pBitmapInfo->bmiHeader.biBitCount = bitCount;
	pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBitmapInfo->bmiHeader.biPlanes = 1;
	pBitmapInfo->bmiHeader.biSizeImage = 0;
	pBitmapInfo->bmiHeader.biXPelsPerMeter = 3000;
	pBitmapInfo->bmiHeader.biYPelsPerMeter = 3000;
	pBitmapInfo->bmiHeader.biClrUsed = 0;
	pBitmapInfo->bmiHeader.biClrImportant = 0;
	pBitmapInfo->bmiHeader.biSizeImage = width * height * bitCount / 8;

	switch (bitCount)
	{
	case 8:
	{
		int i;
		for (i = 0; i < 256; i++)
		{
			pBitmapInfo->bmiColors[i].rgbRed = i;
			pBitmapInfo->bmiColors[i].rgbGreen = i;
			pBitmapInfo->bmiColors[i].rgbBlue = i;
			pBitmapInfo->bmiColors[i].rgbReserved = 0;
		}

		Global.FormatSize = 1;
		Global.ColourFormat = GL_LUMINANCE;
		Global.ByteFormat = GL_UNSIGNED_BYTE;
		Global.RgbFormat = RGB_PIXELFORMAT_GREY;

		pBitmapInfo->bmiHeader.biCompression = BI_RGB;
		break;
	}

	case 16:
	{
		memcpy(&pBitmapInfo->bmiColors, &ColourMasks[RGB_565],
			sizeof(ColourMasks[RGB_565]));
		Global.FormatSize = 2;
		Global.ColourFormat = GL_RGB;
		Global.ByteFormat = GL_UNSIGNED_SHORT_5_6_5;
		Global.RgbFormat = RGB_PIXELFORMAT_565;

		pBitmapInfo->bmiHeader.biCompression = BI_BITFIELDS;
		break;
	}

	case 24:
	{
		memcpy(&pBitmapInfo->bmiColors, &ColourMasks[RGB_24],
			sizeof(ColourMasks[RGB_24]));
		Global.FormatSize = 3;
		Global.ColourFormat = GL_BGR;
		Global.ByteFormat = GL_UNSIGNED_BYTE;
		Global.RgbFormat = RGB_PIXELFORMAT_RGB24;

		pBitmapInfo->bmiHeader.biCompression = BI_RGB;
		break;
	}

	case 32:
	{
		memcpy(&pBitmapInfo->bmiColors, &ColourMasks[RGB_888],
			sizeof(ColourMasks[RGB_888]));
		Global.FormatSize = 4;
		Global.ColourFormat = GL_BGRA;
		Global.ByteFormat = GL_UNSIGNED_BYTE;
		Global.RgbFormat = RGB_PIXELFORMAT_888;

		pBitmapInfo->bmiHeader.biCompression = BI_BITFIELDS;
		break;
	}
	}
}

/******************************************************************************/

void
FreeBitmapInformation()
{
	int j;

	/* Clear the BitmapInfo. */
	for (j = 0; j < NUM_BUFFERS; j++)
	{
		if (Global.PBitmapInfo[j] != NULL)
		{
			free(Global.PBitmapInfo[j]);
		}
	}
}

void
FreeOpenGLSetup()
{
	/* Delete named textures. */
	glDeleteTextures(NUM_BUFFERS, Global.OGLTexture);

	if (Global.Hardware == GPU_AMD)
	{
		/* Free buffers only needed for AMD hardware. */
		glDeleteBuffers(NUM_BUFFERS, &Global.OGLBuffer[0]);
	}
}

/******************************************************************************/

/******************************************************************************/
/* The RGBEasy frame capture callback function. */

void
RGBCBKAPI FrameCapturedFnEx(
	HWND                 hWnd,
	HRGB                 hRGB,
	PRGBFRAMEDATA        pFrameData,
	ULONG_PTR            pUserData)
{
	unsigned int i;

	for (i = 0; i < NUM_BUFFERS; i++)
	{
		if ((pFrameData->PBitmapBits == (void *)Global.PDataBuffer[i]) &&
			pFrameData->PBitmapInfo && Global.BChainBuffer)
		{
			/* Wait for access to the protected buffer index variable. */
			WaitForSingleObject(Global.HMutex, INFINITE);

#ifdef _DEBUG
			/* Store the state variable. */
			Global.BufferTrack[i].State = TEXT('A');
#endif

			/* A valid buffer doesn't exist. It is stored so the buffer can be
			rendered later. */
			if (Global.BufferIndex == NO_BUFFER)
			{
				if (Global.Hardware == GPU_NVIDIA)
				{
					/* Buffer is copied to the graphics card by DMA operation. */
					RGBDirectGPUNVIDIAOp(hRGB, i, NVIDIA_GPU_COPY);
				}

				Global.BufferIndex = i;
			}
			else
			{
				/* A valid buffer is waiting to be rendered.
				Therefore, the capturing rate is faster than the rendering rate.
				To keep minimum latency this buffer is dropped and returned to
				RGBEasy. */
				RGBChainOutputBufferEx(Global.HRGB,
					Global.PBitmapInfo[Global.BufferIndex],
					(void*)Global.PDataBuffer[Global.BufferIndex],
					RGB_BUFFERTYPE_DIRECTGMA);

				/* Store the index of the latest captured buffer. */
				Global.BufferIndex = i;

#ifdef _DEBUG
				/* Increase counter of dropped buffers. */
				Global.NumDroppedBuffers++;
#endif
			}

			/* Release Mutex. */
			ReleaseMutex(Global.HMutex);

			/* A valid buffer is stored, tell the render thread to process it. */
			SetEvent(Global.HCapturedBuffer);
			break;
		}
	}
}

/******************************************************************************/
/* Setup for the OpenGL textures. */

void
SetupOpenGLTextures()
{
	unsigned int i;

	/* Generate texture name for Global.OGLTexture. */
	glGenTextures(NUM_BUFFERS, Global.OGLTexture);

	for (i = 0; i < NUM_BUFFERS; i++)
	{
		// Bind Global.OGLTexture to the texture container.
		glBindTexture(GL_TEXTURE_2D, Global.OGLTexture[i]);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Global.Width, Global.Height, 0, Global.ColourFormat,
			Global.ByteFormat, NULL);

		// Linear filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Unbind Global.OGLTexture from OpenGL texture container.
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (!sgct::Engine::checkForOGLErrors()) //if error occured
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "RGBEasyCapture : Setup OpenGL Textures failed\n");
		void cleanup();
	}
}

/******************************************************************************/
/* Setup for the comunication with graphics card. */

unsigned long
SetupDirectGPU()
{
	unsigned int          j;
	GPUTRANSFERDESCRIPTOR gpuTransfer;
	unsigned long         error = 0;

	/* Fill in structure to define parameters for the
	AMD DirectGMA functionality. */
	gpuTransfer.Size = sizeof(GPUTRANSFERDESCRIPTOR);
	gpuTransfer.Buffer = &Global.PDataBuffer;
	gpuTransfer.Width = Global.Width;
	gpuTransfer.Height = Global.Height;
	gpuTransfer.OglByteFormat = (unsigned int)Global.ByteFormat;
	gpuTransfer.OglColourFormat = (unsigned int)Global.ColourFormat;
	gpuTransfer.FormatSize = Global.FormatSize;
	gpuTransfer.NumBuffers = NUM_BUFFERS;
	gpuTransfer.BufferSize = 0;

	/* Try to initialize RGBEasy for AMD GPU (DirectGMA) communication. */
	Global.Hardware = GPU_AMD;
	gpuTransfer.GpuBrand = Global.Hardware;

	/* Only needed for AMD hardware. */
	glGenBuffers(NUM_BUFFERS, Global.OGLBuffer);
	/* Note: for AMD the buffer object names are passed. */
	gpuTransfer.OglObject = &Global.OGLBuffer[0];

	if (!sgct::Engine::checkForOGLErrors()) //if error occured
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "RGBEasyCapture : Setup Direct GPU Transfer failed\n");
		void cleanup();
	}

	error = RGBDirectGPUInit(Global.HRGB, &gpuTransfer);

	if (error)
	{
		/* Free buffers only needed for AMD hardware. */
		glDeleteBuffers(NUM_BUFFERS, &Global.OGLBuffer[0]);

		/* Try to initialize RGBEasy to NVIDIA GPU (GPUDirect) communication. */
		Global.Hardware = GPU_NVIDIA;
		gpuTransfer.GpuBrand = Global.Hardware;

		/* Note: for NVIDIA the texture names are passed. */
		gpuTransfer.OglObject = &Global.OGLTexture[0];

		error = RGBDirectGPUInit(Global.HRGB, &gpuTransfer);
	}

	if (error == 0)
	{
		/*if (!sgct::Engine::checkForOGLErrors()) //if error occured
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "RGBEasyCapture : Init Direct GPU Transfer failed\n");
			void cleanup();
		}*/

		/* The size of the buffer allocated is stored in
		the field bufferSize. The size of the buffer is re-assigned to the
		bitmap header for any changes in pitch. */
		for (j = 0; j < NUM_BUFFERS; j++)
		{
			Global.PBitmapInfo[j]->bmiHeader.biSizeImage = gpuTransfer.BufferSize;
		}
	}

	return error;
}

/******************************************************************************/
/* Setup capture. */

unsigned long
StartCapture()
{
	unsigned long  error = 0;
	unsigned int   j;
	signed long    liveStream;
	LIVESTREAM     val = LIVESTREAM_1;

	/* Check for LiveStream support. */
	error = RGBInputIsLiveStreamSupported(Global.Input, &liveStream);

	if (error == 0)
	{
		if (liveStream)
		{
			/* Enable LiveStream. */
			RGBSetLiveStream(Global.HRGB, val);
		}
	}

	/* Maximise the capture rate. */
	error = RGBSetFrameDropping(Global.HRGB, 0);
	if (error == 0)
	{
		/* Set Capture format. */
		error = RGBSetPixelFormat(Global.HRGB, Global.RgbFormat);
		if (error == 0)
		{
			/* Set the Frame Captured callback function. */
			error = RGBSetFrameCapturedFnEx(Global.HRGB, FrameCapturedFnEx, 0);
			if (error == 0)
			{
				error = RGBSetOutputSize(Global.HRGB, Global.Width, Global.Height);
				if (error != 0)
				{
					MessageBox(NULL, TEXT("Couldn't set the output size."),
						TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
				}
				else
				{
					/* Pass buffers to RGBEasy.*/
					for (j = 0; j < NUM_BUFFERS; j++)
					{
						error = RGBChainOutputBufferEx(Global.HRGB,
							Global.PBitmapInfo[j], (void*)Global.PDataBuffer[j],
							RGB_BUFFERTYPE_DIRECTGMA);

						if (error != 0)
						{
							MessageBox(NULL, TEXT("Couldn't set output buffer."),
								TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
						}
#ifdef _DEBUG
						/* Buffer is in RGBEasy. */
						Global.BufferTrack[j].State = TEXT('C');
#endif
					}

					error = RGBUseOutputBuffers(Global.HRGB, TRUE);
					if (error == 0)
					{
						Global.BChainBuffer = TRUE;
					}

					error = RGBStartCapture(Global.HRGB);
					if (error != 0)
					{
						MessageBox(NULL, TEXT("Couldn't start capture."),
							TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
					}
				}
			}
		}
	}

	return error;
}

/******************************************************************************/
/* Stops capture. */

void
StopCapture()
{
	if (Global.HRGBDLL)
	{
		RGBStopCapture(Global.HRGB);
		RGBSetFrameCapturedFn(Global.HRGB, NULL, 0);
		RGBDirectGPUClose(Global.HRGB);
	}
}

/******************************************************************************/
/* Close RGBEasy. */

void
CloseRGBEasy()
{
	if (Global.HRGBDLL)
	{
		RGBCloseInput(Global.HRGB);
		RGBFree(Global.HRGBDLL);
	}
}

RGBEasyCapture::RGBEasyCapture()
{
	mCaptureHost = "";
	mCaptureGanging = false;
}

RGBEasyCapture::~RGBEasyCapture()
{
}

bool RGBEasyCapture::initialize()
{
	Global.Error = RGBLoad(&Global.HRGBDLL);
	if (Global.Error)
	{
		/*TCHAR buffer[MAX_PATH];

		StringCchPrintf(buffer, MAX_PATH,
			TEXT("Error returned from RGBLoad: 0x%08x"), Global.Error);
		MessageBox(NULL, buffer, Caption, MB_OK | MB_ICONERROR);
		return Global.Error;*/
	}

	/* Open RGB input. */
	Global.Error = RGBOpenInput(Global.Input, &Global.HRGB);

	if (Global.Error == 0)
	{
		if(mCaptureGanging)
			DoGanging(Global.Input);

		Global.Error = RGBGetInputSignalType(Global.Input, &Global.SignalType,
			&Global.Width, &Global.Height, &Global.RefreshRate);

		if ((Global.SignalType == RGB_SIGNALTYPE_NOSIGNAL) ||
			(Global.SignalType == RGB_SIGNALTYPE_OUTOFRANGE))
		{
			Global.Width = 800;
			Global.Height = 600;
		}

		if (Global.Error != 0)
		{
			MessageBox(NULL, TEXT("Couln't recognize input signal type."),
				TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
			CloseRGBEasy();
		}
		else
		{
			/* Create event used to signal a completed operation. */
			Global.HCapturedBuffer = CreateEvent(NULL, FALSE, FALSE, NULL);

			/* Create mutex for shared data/ */
			Global.HMutex = CreateMutex(NULL, FALSE, NULL);

			/* Create event used to signal a OpenGL setup and initialization completed. */
			Global.HOGLSetupEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

			return true;
		}
	}
	else
	{
		RGBFree(Global.HRGBDLL);
	}

	return false;
}

void RGBEasyCapture::deinitialize()
{
	StopCapture();
	CloseRGBEasy();

	CloseHandle(Global.HOGLSetupEvent);
	CloseHandle(Global.HMutex);
	CloseHandle(Global.HCapturedBuffer);
}

bool RGBEasyCapture::initializeGL()
{
	int            j;
	unsigned int   index;
	/* Create Bitmap Information for capture. */
	for (j = 0; j < NUM_BUFFERS; j++)
	{
		if (Global.CaptureFormat != 8)
		{
			Global.PBitmapInfo[j] = (LPBITMAPINFO)malloc(
				sizeof(BITMAPINFOHEADER) + (3 * sizeof(DWORD)));
		}
		else
		{
			Global.PBitmapInfo[j] = (LPBITMAPINFO)malloc(
				sizeof(BITMAPINFOHEADER) + (256 * sizeof(DWORD)));
		}

		CreateBitmapInformation(Global.PBitmapInfo[j], Global.Width,
			Global.Height, Global.CaptureFormat);
	}

	SetupOpenGLTextures();

	Global.Error = SetupDirectGPU();

	/* Allow WinMain to continue. */
	SetEvent(Global.HOGLSetupEvent);

	if (Global.Error == 0)
	{
		return true;
	}

	return false;
}

void RGBEasyCapture::deinitializeGL()
{
	FreeBitmapInformation();
	FreeOpenGLSetup();
}

void RGBEasyCapture::runCapture()
{
	/* Wait for the OpenGL initialisation to finish. */
	if (WaitForSingleObject(Global.HOGLSetupEvent, INFINITE)
		== WAIT_OBJECT_0)
	{
		/* Setup and start capture. */
		StartCapture();
	}
}

bool RGBEasyCapture::prepareForRendering() {
	/* Wait for access to the protected buffer index variable. */
	WaitForSingleObject(Global.HMutex, INFINITE);
	/* Take a local copy of Global.BufferIndex so buffer mutex can be
	released. */
	currentBufferIndex = Global.BufferIndex;
	/* Set the index as no buffer ready for any new index. */
	Global.BufferIndex = NO_BUFFER;
	/* Release mutex to the protected buffer index variable. */
	ReleaseMutex(Global.HMutex);
	/* If a valid buffer index exists, render it. If it does not, the
	previous capture buffer will be rastered again from graphics card
	memory. This may happen when the render rate is faster
	than the capture rate. */
	if (currentBufferIndex != NO_BUFFER)
	{
#ifdef _DEBUG
		/* Buffer is about to be rendered. */
		Global.BufferTrack[currentBufferIndex].State = TEXT('R');
#endif
		if (Global.Hardware == GPU_NVIDIA)
		{
			RGBDirectGPUNVIDIAOp(Global.HRGB, currentBufferIndex, NVIDIA_GPU_WAIT);
		}

		/* Clear screen and depth buffer. */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* Reset the current matrix. */
		glLoadIdentity();

		glBindTexture(GL_TEXTURE_2D, Global.OGLTexture[currentBufferIndex]);

		if (Global.Hardware == GPU_AMD)
		{
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, Global.OGLBuffer[currentBufferIndex]);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Global.Width, Global.Height, Global.ColourFormat,
				Global.ByteFormat, NULL);
		}

		/* Insert Sync object to check for completion. */
		currentFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

		return true;
	}
#ifdef _DEBUG
	else
	{
		Global.NumRepeatedBuffers++;
	}
#endif

	return false;
}

void RGBEasyCapture::renderingCompleted() {
	if (currentBufferIndex != NO_BUFFER)
	{
		glBindTexture(GL_TEXTURE_2D, 0);

		if (glIsSync(currentFence))
		{
			glClientWaitSync(currentFence, GL_SYNC_FLUSH_COMMANDS_BIT, ONE_SECOND);
			glDeleteSync(currentFence);
		}

		if (Global.Hardware == GPU_NVIDIA)
		{
			RGBDirectGPUNVIDIAOp(Global.HRGB, currentBufferIndex, NVIDIA_GPU_END);
		}

		/* Now that the buffer has been rendered give it back to RGBEasy. */
		Global.Error = RGBChainOutputBufferEx(Global.HRGB,
			Global.PBitmapInfo[currentBufferIndex], (void*)Global.PDataBuffer[currentBufferIndex],
			RGB_BUFFERTYPE_DIRECTGMA);
#ifdef _DEBUG
		/* Buffer is in RGBEasy. */
		Global.BufferTrack[currentBufferIndex].State = TEXT('C');
#endif
		/* Reset the event as rendering has finished. */
		ResetEvent(Global.HCapturedBuffer);
	}

	WaitForSingleObject(Global.HCapturedBuffer, INFINITE);
}

std::string RGBEasyCapture::getCaptureHost() const
{
	return mCaptureHost;
}

void RGBEasyCapture::setCaptureHost(std::string hostAdress)
{
    mCaptureHost = hostAdress;
}

void RGBEasyCapture::setCaptureInput(int input)
{
	Global.Input = input;
}

void RGBEasyCapture::setCaptureGanging(bool doGanging) {
	mCaptureGanging = true;
}

unsigned long  RGBEasyCapture::getWidth() {
	return Global.Width;
}

unsigned long RGBEasyCapture::getHeight() {
	return Global.Height;
}

bool RGBEasyCapture::getGanging() {
	return mCaptureGanging;
}