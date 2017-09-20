/*******************************************************************************
Copyright Datapath Ltd. 2014.

File:    rgbh264nal.h

History: 30 OCT 14    RL   Created.
         12 OCT 15    DC   Added further hard-coded URLs.
                           Included GOP Length as an option in URLs.
         
*******************************************************************************/

// Internal buffer count containing multiple NALU's
#define NUM_SAMPLES  3

typedef enum _RGBEasyH264_FRAME_STATE
{
   RGBEasyH264_FRAME_STATE_UNINITIALISED,
   RGBEasyH264_FRAME_STATE_INITIALISED,
   RGBEasyH264_FRAME_STATE_STARTED,
   RGBEasyH264_FRAME_STATE_STOPING,
   RGBEasyH264_FRAME_STATE_STOPPED,
}  RGBEasyH264_FRAME_STATE, *PRGBEasyH264_FRAME_STATE;

typedef struct _StreamInfo {
   uint32_t Width;
   uint32_t Height;
   uint32_t FPS;
} StreamInfo, *PStreamInfo;

typedef struct _H264NAL
{
   struct _H264NAL   *PNext;
   uint8_t           *PStart;
   uint32_t          Size;
   uint64_t          StartTime;
}H264NAL, *PH264NAL;

uint32_t GetInputSignalType (
   uint32_t input,
   uint32_t *pWidth,
   uint32_t *pHeight,
   uint32_t *pFPS );

uint32_t GetSupportedH264Inputs (
   uint32_t **ppInputList,
   uint32_t *plength,
   uint32_t *pH264Count);

BOOL DoesH264NALUnitBeginNewAccessUnit(
   uint8_t *pNalUnit);

class CRGBEasyH264
{
public:

   CRGBEasyH264 ( uint32_t input );
   ~CRGBEasyH264 ( );

   uint32_t RGBEasyH264Start( uint32_t width,
                              uint32_t height,
                              uint32_t frameRate,
                              DGCENCLEVEL level,
                              DGCENCPROFILE profile,
                              uint32_t bitrate,
                              uint32_t keyframeinterval );
   uint32_t RGBEasyH264GetNAL ( uint8_t* pNAL, uint32_t maxSize, uint32_t *pTrunkSize, 
         uint32_t *pCopySize, uint64_t *pTimeStamp );
   uint32_t RGBEasyH264Stop();
private:

   uint32_t RGBEasyH264Init( uint32_t input );
   uint32_t RGBEasyH264UnInit();

   PDGCMEDIASAMPLE CreateMediaSample(uint32_t width, uint32_t height);
   void AddNALToList ( PH264NAL pH264NAL );
   void CheckListRemoveFirstNALFromList( PH264NAL* ppH264NAL );
   void LockedRemoveNALFromList( PH264NAL pH264NALToRemove, PH264NAL pH264NALPrevToToRemove );

   void RGBEasyH264RealEncoderErrorFn(HRGB hRGB, unsigned long error);
   static void RGBCBKAPI RGBEasyH264EncoderErrorFn(   HWND              hWnd,
                                                      HRGB              hRGB,
                                                      unsigned long     error,
                                                      ULONG_PTR         pUserData,
                                                      unsigned long     *pReserved);

   void RGBEasyH264RealEncoderFrameFn(HRGB hRGB,PDGCMEDIASAMPLE pMediaSample);
   static void RGBCBKAPI RGBEasyH264EncoderFrameFn(   HWND              hWnd,
                                                      HRGB              hRGB,
                                                      PDGCMEDIASAMPLE   pMediaSample,
                                                      ULONG_PTR         pUserData);
public:
   
   StreamInfo              m_StreamInfo;

private:
   unsigned char * GetNextNAL(unsigned char * pBuffer, uint32_t *pBufferLen, unsigned char ** pNAL, unsigned long * NALlength);
   uint32_t                m_error;
   uint32_t                m_input;
   RGBEasyH264_FRAME_STATE m_state;
   HINSTANCE               m_hInstance;
   HRGBDLL                 m_hDLL;
   HRGB                    m_hRGB;
   
   PDGCMEDIASAMPLE         m_pMediaSample[NUM_SAMPLES];

   PH264NAL                m_pNAL_List;
   HANDLE                  m_hListMutex;
   HANDLE                  m_AdderMutex;
   uint32_t                m_startCodeOffset;
   uint64_t                m_queuedNALs;
   uint64_t                m_unqueuedNALs;
   uint64_t                m_NAL_ListSize;
};
