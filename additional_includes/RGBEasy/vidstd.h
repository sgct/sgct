/*******************************************************************************

Copyright Datapath Ltd. 2002, 2008.

File:    vidstd.h

Purpose:
         Platform neutral definitions for video standards.

History:
         18 JUL 02   SB    Created.
         11 JUN 07   SB    Added HDTV and RGB standards for Mosaic-HQ8.
         13 JUL 07   SB    Split off HDTV to HDTV and SDTV standards.
         17 AUG 07   SB    Added IsInterlaced definition.
         08 JAN 07   SB    Added 1280x720x50Hz RGB mode.
         09 SEP 08   SB    Fixed IsInterlaced macro.

*******************************************************************************/

#define VIDSTD_MAJOR_MASK        0x000F
#define VIDSTD_MINOR_MASK        0x0F00

#define VIDSTD_PAL               0x0000
#define VIDSTD_NTSC              0x0001
#define VIDSTD_SECAM             0x0002
#define VIDSTD_SDTV              0x0003
#define VIDSTD_HDTV              0x0004
#define VIDSTD_RGB               0x0005

#define VIDSTD_PAL_I             0x0000
#define VIDSTD_PAL_B             0x0100
#define VIDSTD_PAL_D             0x0200
#define VIDSTD_PAL_G             0x0300
#define VIDSTD_PAL_H             0x0400
#define VIDSTD_PAL_M             0x0500
#define VIDSTD_PAL_N             0x0600
#define VIDSTD_PAL_NC            0x0700
#define VIDSTD_PAL_4_43_60       0x0800

#define VIDSTD_NTSC_M            0x0000
#define VIDSTD_NTSC_J            0x0100
#define VIDSTD_NTSC_N            0x0200
#define VIDSTD_NTSC_4_43_50      0x0300
#define VIDSTD_NTSC_4_43_60      0x0400

#define VIDSTD_SECAM_B           0x0000
#define VIDSTD_SECAM_D           0x0100
#define VIDSTD_SECAM_G           0x0200
#define VIDSTD_SECAM_K           0x0300
#define VIDSTD_SECAM_L           0x0400
#define VIDSTD_SECAM_LD          0x0500

#define VIDSTD_SDTV_480I         0x0000
#define VIDSTD_SDTV_480P         0x0100
#define VIDSTD_SDTV_576I         0x0200
#define VIDSTD_SDTV_576P         0x0300

#define VIDSTD_HDTV_720P50       0x0000
#define VIDSTD_HDTV_720P60       0x0100
#define VIDSTD_HDTV_1080I50      0x0200
#define VIDSTD_HDTV_1080I60      0x0300

#define VIDSTD_RGB_1280x1024x60       0x0000
#define VIDSTD_RGB_1024x768x60        0x0100
#define VIDSTD_RGB_800x600x60         0x0200
#define VIDSTD_RGB_640x480x60         0x0300
#define VIDSTD_RGB_1280x720x60        0x0400

#define VIDEOSTANDARD_PAL_I         ( VIDSTD_PAL | VIDSTD_PAL_I )
#define VIDEOSTANDARD_PAL_B         ( VIDSTD_PAL | VIDSTD_PAL_B )
#define VIDEOSTANDARD_PAL_D         ( VIDSTD_PAL | VIDSTD_PAL_D )
#define VIDEOSTANDARD_PAL_G         ( VIDSTD_PAL | VIDSTD_PAL_G )
#define VIDEOSTANDARD_PAL_H         ( VIDSTD_PAL | VIDSTD_PAL_H )
#define VIDEOSTANDARD_PAL_M         ( VIDSTD_PAL | VIDSTD_PAL_M )
#define VIDEOSTANDARD_PAL_N         ( VIDSTD_PAL | VIDSTD_PAL_N )
#define VIDEOSTANDARD_PAL_NC        ( VIDSTD_PAL | VIDSTD_PAL_NC )
#define VIDEOSTANDARD_PAL_4_43_60   ( VIDSTD_PAL | VIDSTD_PAL_4_43_60 )

#define VIDEOSTANDARD_NTSC_M        ( VIDSTD_NTSC | VIDSTD_NTSC_M )
#define VIDEOSTANDARD_NTSC_J        ( VIDSTD_NTSC | VIDSTD_NTSC_J )
#define VIDEOSTANDARD_NTSC_N        ( VIDSTD_NTSC | VIDSTD_NTSC_N )
#define VIDEOSTANDARD_NTSC_4_43_50  ( VIDSTD_NTSC | VIDSTD_NTSC_4_43_50 )
#define VIDEOSTANDARD_NTSC_4_43_60  ( VIDSTD_NTSC | VIDSTD_NTSC_4_43_60 )

#define VIDEOSTANDARD_SECAM_B       ( VIDSTD_SECAM | VIDSTD_SECAM_B )
#define VIDEOSTANDARD_SECAM_D       ( VIDSTD_SECAM | VIDSTD_SECAM_D )
#define VIDEOSTANDARD_SECAM_G       ( VIDSTD_SECAM | VIDSTD_SECAM_G )
#define VIDEOSTANDARD_SECAM_K       ( VIDSTD_SECAM | VIDSTD_SECAM_K )
#define VIDEOSTANDARD_SECAM_L       ( VIDSTD_SECAM | VIDSTD_SECAM_L )
#define VIDEOSTANDARD_SECAM_LD      ( VIDSTD_SECAM | VIDSTD_SECAM_LD )

#define VIDEOSTANDARD_SDTV_480I     ( VIDSTD_SDTV | VIDSTD_SDTV_480I )
#define VIDEOSTANDARD_SDTV_480P     ( VIDSTD_SDTV | VIDSTD_SDTV_480P )
#define VIDEOSTANDARD_SDTV_576I     ( VIDSTD_SDTV | VIDSTD_SDTV_576I )
#define VIDEOSTANDARD_SDTV_576P     ( VIDSTD_SDTV | VIDSTD_SDTV_576P )

#define VIDEOSTANDARD_HDTV_720P50   ( VIDSTD_HDTV | VIDSTD_HDTV_720P50 )
#define VIDEOSTANDARD_HDTV_720P60   ( VIDSTD_HDTV | VIDSTD_HDTV_720P60 )
#define VIDEOSTANDARD_HDTV_1080I50  ( VIDSTD_HDTV | VIDSTD_HDTV_1080I50 )
#define VIDEOSTANDARD_HDTV_1080I60  ( VIDSTD_HDTV | VIDSTD_HDTV_1080I60 )

#define VIDEOSTANDARD_RGB_1280x1024x60 ( VIDSTD_RGB | VIDSTD_RGB_1280x1024x60 )
#define VIDEOSTANDARD_RGB_1280x720x60  ( VIDSTD_RGB | VIDSTD_RGB_1280x720x60 )
#define VIDEOSTANDARD_RGB_1024x768x60  ( VIDSTD_RGB | VIDSTD_RGB_1024x768x60 )
#define VIDEOSTANDARD_RGB_800x600x60   ( VIDSTD_RGB | VIDSTD_RGB_800x600x60 )
#define VIDEOSTANDARD_RGB_640x480x60   ( VIDSTD_RGB | VIDSTD_RGB_640x480x60 )

#define Is525Lines(__vidstd)   \
   (( ( ( __vidstd ) == VIDEOSTANDARD_PAL_M ) || \
      ( ( __vidstd ) == VIDEOSTANDARD_PAL_4_43_60 ) || \
      ( ( __vidstd ) == VIDEOSTANDARD_NTSC_M ) ||   \
      ( ( __vidstd ) == VIDEOSTANDARD_NTSC_J ) ||   \
      ( ( __vidstd ) == VIDEOSTANDARD_NTSC_4_43_60 ) ||  \
      ( ( __vidstd ) == VIDEOSTANDARD_SDTV_480I ) ||  \
      ( ( __vidstd ) == VIDEOSTANDARD_SDTV_480P ) ) ? 1: 0 )

#define Is60Hertz(__vidstd)   \
   (( ( ( __vidstd ) == VIDEOSTANDARD_PAL_4_43_60 ) || \
      ( ( __vidstd ) == VIDEOSTANDARD_NTSC_M ) ||   \
      ( ( __vidstd ) == VIDEOSTANDARD_NTSC_N ) ||   \
      ( ( __vidstd ) == VIDEOSTANDARD_NTSC_J ) ||   \
      ( ( __vidstd ) == VIDEOSTANDARD_NTSC_4_43_60 ) ||  \
      ( ( __vidstd ) == VIDEOSTANDARD_SDTV_480I ) ||  \
      ( ( __vidstd ) == VIDEOSTANDARD_SDTV_480P ) ||  \
      ( ( __vidstd ) == VIDEOSTANDARD_HDTV_720P60 ) ||   \
      ( ( __vidstd ) == VIDEOSTANDARD_HDTV_1080I60 ) ||  \
      ( ( __vidstd ) == VIDEOSTANDARD_RGB_1280x1024x60 ) || \
      ( ( __vidstd ) == VIDEOSTANDARD_RGB_1280x720x60 ) || \
      ( ( __vidstd ) == VIDEOSTANDARD_RGB_1024x768x60 ) ||  \
      ( ( __vidstd ) == VIDEOSTANDARD_RGB_800x600x60 ) ||   \
      ( ( __vidstd ) == VIDEOSTANDARD_RGB_640x480x60 ) ) ? 1: 0 )

#define IsInterlaced(__vidstd) \
   ((((( __vidstd ) & VIDSTD_MAJOR_MASK ) == VIDSTD_PAL ) || \
     ((( __vidstd ) & VIDSTD_MAJOR_MASK ) == VIDSTD_NTSC ) ||   \
     ((( __vidstd ) & VIDSTD_MAJOR_MASK ) == VIDSTD_SECAM ) ||   \
      (( __vidstd ) == VIDEOSTANDARD_SDTV_480I ) ||  \
      (( __vidstd ) == VIDEOSTANDARD_SDTV_576I ) ||  \
      (( __vidstd ) == VIDEOSTANDARD_HDTV_1080I50 ) ||   \
      (( __vidstd ) == VIDEOSTANDARD_HDTV_1080I60 )) ? 1: 0 )



/******************************************************************************/

