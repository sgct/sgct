/*******************************************************************************

Copyright Datapath Ltd. 2014, 2015

File:    dgcmediahelper.h

Purpose: Helper functionality for Audio and Video media types.

History:
         09 JUN 14    JL   Created.

*******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

typedef void* (*PDGCALLOCFN) (uintptr_t size);
typedef void  (*PDGCFREEFN ) (void *pData);

uint32_t SubTypeGetBitDepth( DGCMEDIASAMPLESUBTYPE subType );
uint32_t SubTypeGetNumPlanes( DGCMEDIASAMPLESUBTYPE subType );

// For use with DGCRAWVIDEOSAMPLESUBTYPE_* values
uint8_t SubTypeGetPlaneSize( DGCMEDIASAMPLESUBTYPE subType,
                             uint16_t plane,
                             uint32_t width,
                             uint32_t height,
                             uint32_t *pSize );

uint8_t MediaSampleSetAllocFunctionPair( PDGCALLOCFN allocFn,
                                         PDGCFREEFN  freeFn );

PDGCMEDIASAMPLE MediaSampleAllocate( DGCMEDIASAMPLETYPE     type,
                                     DGCMEDIASAMPLESUBTYPE  subType );

uint8_t MediaSampleAllocateFormatHeader( PDGCMEDIASAMPLE pMediaSample );

uint8_t MediaSampleAllocateBufferHeader( PDGCMEDIASAMPLE       pMediaSample,
                                         DGCBUFFERHEADERTYPE   bufferType );

uint8_t MediaSampleInitialisePlane( PDGCMEDIASAMPLE       pMediaSample,
                                    uint16_t              plane,
                                    uint32_t              bufferSize );

uint8_t MediaSampleInitialisePlaneWH( PDGCMEDIASAMPLE       pMediaSample,
                                      uint16_t              plane,
                                      uint32_t              width,
                                      uint32_t              height );

void MediaSampleFree( PDGCMEDIASAMPLE pMediaSample );

/******************************************************************************/
#ifdef __cplusplus
}
#endif
/******************************************************************************/
