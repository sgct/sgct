/*******************************************************************************

Copyright Datapath Ltd. 2014.

File:    dgcmedia.h

Purpose: Definitions for video media types.

History:
         06 JUN 14    SB   Created.
         09 JUN 14    JL   Added h.264 video.

*******************************************************************************/

#ifndef _DGCMEDIA_H
#define _DGCMEDIA_H

/******************************************************************************/
/*                             S A M P L E  B U F F E R                       */
/******************************************************************************/

#define DGCMEMORYBUFFERFLAG_MAPPED     0
#define DGCMEMORYBUFFERFLAG_DIRECTGMA  1
#define DGCMEMORYBUFFERFLAG_GPUDIRECT  2

// Memory Buffer Descriptor
typedef struct tagDGCMEMORYBUFFER
{
   uint32_t Size;                // Size of this structure in bytes.
   uint32_t Flags;               // Flags.
   void     *PBuffer;            // Pointer to the start of the buffer.
   uint32_t Length;              // Buffer Length in bytes.
   uint32_t Pitch;               // Buffer Pitch in bytes.
   uint32_t OffsetX;             // Start offset in pixels.
   uint32_t OffsetY;             // Start offset in lines.
   uint32_t ActualLength;        // Amount of actual data within the buffer in bytes.
}  DGCMEMORYBUFFER, *PDGCMEMORYBUFFER;

typedef enum tagDGCBUFFERHEADERTYPE
{
   DGCBUFFERHEADERTYPE_UNKNOWN,
   DGCBUFFERHEADERTYPE_MEMORY,
}  DGCBUFFERHEADERTYPE, *PDGCBUFFERHEADERTYPE;

typedef struct tagDGCMEMORYBUFFERHEADER
{
   uint32_t             Size;                // Size of this structure in bytes.
   uint32_t             Flags;               // Flags.
   uint64_t             StartTime;           // Captured data start time.
   uint64_t             EndTime;             // Captured data end time.
   uint32_t             NumberOfPlanes;      // Number of Planes in the buffer.
   DGCMEMORYBUFFER      Planes[3];           // Array of Planes.
}  DGCMEMORYBUFFERHEADER, *PDGCMEMORYBUFFERHEADER;

/******************************************************************************/
/*                                  V I D E O                                 */
/******************************************************************************/

// Video Sample
typedef struct tagDGCVIDEOHEADER
{
   uint32_t                Size;          // Size of this structure in bytes.
   uint32_t                Flags;         // Flags.
   int32_t                 Width;         // Image Width in pixels.
   int32_t                 Height;        // Image Height in lines.
   int32_t                 FrameRate;     // Frames per second (mHz).
}  DGCVIDEOHEADER, *PDGCVIDEOHEADER;

/******************************************************************************/
/*                                M E D I A                                   */
/******************************************************************************/

typedef enum tagDGCMEDIASAMPLETYPE
{
   DGCMEDIASAMPLETYPE_UNKNOWN = 0,
   DGCMEDIASAMPLETYPE_RAW_VIDEO = 1,
   DGCMEDIASAMPLETYPE_ENC_VIDEO = 2,
}  DGCMEDIASAMPLETYPE, *PDGCMEDIASAMPLETYPE;

typedef enum tagDGCMEDIASAMPLESUBTYPE
{
   DGCMEDIASAMPLESUBTYPE_UNKNOWN = 0,     // UNKNOWN

   // Uncompressed Video Types
   DGCRAWVIDEOSAMPLESUBTYPE_UNKNOWN = 0,  // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_RGB555 = 1,   // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_RGB565 = 2,   // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_RGB888 = 3,   // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_YYYY = 4,     // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_RGB24 = 5,    // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_YUY2 = 6,     // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_YVYU = 7,     // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_UYVY = 8,     // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_NV12 = 9,     // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_YV12 = 10,    // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_I420 = 11,    // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_Y42B = 12,    // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_Y444 = 13,    // DGCVIDEOHEADER

   DGCRAWVIDEOSAMPLESUBTYPE_RGB10 = 14,    // DGCVIDEOHEADER
   DGCRAWVIDEOSAMPLESUBTYPE_Y410 = 15,    // DGCVIDEOHEADER

   DGCENCVIDEOSAMPLESUBTYPE_UNKNOWN = 0,  // DGCVIDEOHEADER
   DGCENCVIDEOSAMPLESUBTYPE_H264 = 1001,  // DGCVIDEOHEADER

}  DGCMEDIASAMPLESUBTYPE, *PDGCMEDIASAMPLESUBTYPE;

typedef enum tagDGCENCPROFILE
{
   DGCENC_PROFILE_UNKNOWN = 0,

   DGCENC_H264_PROFILE_UNKNOWN = 0,
   DGCENC_H264_PROFILE_BASELINE = 1,      // Baseline profile
   DGCENC_H264_PROFILE_MAIN = 2,          // Main profile
   DGCENC_H264_PROFILE_EXTENDED = 3,      // Extended profile
   DGCENC_H264_PROFILE_HIGH = 4,          // High profile

} DGCENCPROFILE;

typedef enum tagDGCENCLEVEL
{
   DGCENC_LEVEL_UNKNOWN = 0,

   DGCENC_H264_LEVEL_UNKNOWN = 0,
   DGCENC_H264_LEVEL_1 = 1,               // Level 1
   DGCENC_H264_LEVEL_1B = 2,              // Level 1b
   DGCENC_H264_LEVEL_1_1 = 3,             // Level 1.1 
   DGCENC_H264_LEVEL_1_2 = 4,             // Level 1.2 
   DGCENC_H264_LEVEL_1_3 = 5,             // Level 1.3 
   DGCENC_H264_LEVEL_2 = 6,               // Level 2
   DGCENC_H264_LEVEL_2_1 = 7,             // Level 2.1 
   DGCENC_H264_LEVEL_2_2 = 8,             // Level 2.2 
   DGCENC_H264_LEVEL_3 = 9,               // Level 3
   DGCENC_H264_LEVEL_3_1 = 10,            // Level 3.1
   DGCENC_H264_LEVEL_3_2 = 11,            // Level 3.2
   DGCENC_H264_LEVEL_4 = 12,              // Level 4
   DGCENC_H264_LEVEL_4_1 = 13,            // Level 4.1
   DGCENC_H264_LEVEL_4_2 = 14,            // Level 4.2
   DGCENC_H264_LEVEL_5 = 15,              // Level 5
   DGCENC_H264_LEVEL_5_1 = 16,            // Level 5.1

} DGCENCLEVEL;

typedef struct tagDGCMEDIASAMPLE
{
   uint32_t                Size;             // Size of this structure in bytes.
   DGCMEDIASAMPLETYPE      MajorType;        // Major Type of media sample.
   DGCMEDIASAMPLESUBTYPE   SubType;          // Sub Type of media sample.
   void                   *PFormatHeader;    // Pointer to media sample header.
   DGCBUFFERHEADERTYPE     BufferHeaderType; // Type of media sample buffer.
   void                   *PBufferHeader;    // Pointer to the media sample buffer.
}  DGCMEDIASAMPLE, *PDGCMEDIASAMPLE;

/******************************************************************************/

#endif // _DGCMEDIA_H

/******************************************************************************/
