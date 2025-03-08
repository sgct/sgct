/* =========================================================================

  Program:   Multiple Projector Library
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2013 Scalable Display Technologies, Inc.
  All Rights Reserved
  The source code contained herein is confidential and is considered a
  trade secret of Scalable Display Technologies, Inc

===================================================================auto== */

#ifndef _EasyBlendSDK_H_
#define _EasyBlendSDK_H_

#if  !defined(_EASYBLENDSDK_LINUX) && !defined(_EASYBLENDSDK_STATIC)
#ifdef MESHSDK_EXPORTS
#  define EasyBlendSDK_API extern "C" __declspec(dllexport)
#else
#  define EasyBlendSDK_API extern "C" __declspec(dllimport)
#endif /* ifdef MESHSDK_EXPORTS */
#else
#  define EasyBlendSDK_API extern "C"
#endif /* ifndef _EASYBLENDSDK_LINUX */

typedef unsigned long      EasyBlendSDKUINT32;
typedef EasyBlendSDKUINT32 EasyBlendSDKError;
typedef EasyBlendSDKUINT32 EasyBlendSDKMapping;
typedef EasyBlendSDKUINT32 EasyBlendSDKSampling;
typedef EasyBlendSDKUINT32 EasyBlendSDKProjection;
typedef unsigned int       EasyBlendSDKGLBuffer;
typedef unsigned int       EasyBlendSDKGLTexture;
typedef struct _ClientMesh    EasyBlendSDK_ClientMesh;
#define EasyBlendSDK_PROJECTION_Perspective  0x01
#define EasyBlendSDK_PROJECTION_Orthographic 0x02
#define EasyBlendSDK_SDKMODE_ClientData 0x01
#define EasyBlendSDK_SDKMODE_OpenGL     0x02

typedef struct {
    double  XOffset;
    double  YOffset;
    double  ZOffset;
    double  ViewAngleA;
    double  ViewAngleB;
    double  ViewAngleC;
    double  LeftAngle;
    double  RightAngle;
    double  TopAngle;
    double  BottomAngle;
} EasyBlendSDK_Frustum;

typedef struct {
    EasyBlendSDKMapping    Mapping;
    EasyBlendSDKSampling   Sampling;
    EasyBlendSDKProjection Projection;
    EasyBlendSDK_Frustum   Frustum;
    float AnisValue;
    float Bottom;
    float Top;
    float Left;
    float Right;
    float Version;
    unsigned long Xres;
    unsigned long Yres;
    unsigned long ApproxXres;
    unsigned long ApproxYres;
    int SDKMode;
    EasyBlendSDK_ClientMesh* ClientMesh;
    void* token;
} EasyBlendSDK_Mesh;

typedef EasyBlendSDK_Mesh MeshSDK_Mesh;

typedef EasyBlendSDKError(*EasyBlendSDKCallback)(EasyBlendSDK_Mesh* msm,
    void* cbData);

typedef EasyBlendSDKError MeshSDKError;

#define EasyBlendSDK_ERR_S_OK                         0
#define EasyBlendSDK_ERR_E_UNIMPLEMENTED              1
#define EasyBlendSDK_ERR_E_UNABLE_TO_OPEN_FILE        2
#define EasyBlendSDK_ERR_E_FILE_NOT_PARSEABLE         3
#define EasyBlendSDK_ERR_E_NOT_LICENSED               4
#define EasyBlendSDK_ERR_E_FAIL                       5
#define EasyBlendSDK_ERR_E_BAD_ARGUMENTS              6
#define EasyBlendSDK_ERR_E_OUT_OF_MEMORY              7
#define EasyBlendSDK_ERR_E_UNABLE_TO_GENERATE_TEXTURE 8
#define EasyBlendSDK_ERR_E_UNABLE_TO_GENERATE_LIST    9
#define EasyBlendSDK_ERR_E_VIEWPORT_INCORRECT         10
#define EasyBlendSDK_ERR_E_INCORRECT_FILE_VERSION     11
#define EasyBlendSDK_ERR_E_ALREADY_INITIALIZED        12
#define EasyBlendSDK_ERR_E_UNABLE_TO_INITIALIZE_GL_EXTENSIONS   13
#define EasyBlendSDK_ERR_E_INCORRECT_INPUT_TYPE       14
#define EasyBlendSDK_ERR_E_UNSUPPORTED_BY_HARDWARE    15
#define EasyBlendSDK_ERR_E_UNABLE_TO_WRITE_FILE       16
#define EasyBlendSDK_ERR_E_DATA_ALTERED               17
#define EasyBlendSDK_ERR_E_INVALID_MACHINE            18
#define EasyBlendSDK_ERR_E_UNABLE_TO_CREATE_CLIENT_MESH 19

#define EasyBlendSDK_SUCCEEDED(x) (EasyBlendSDK_ERR_S_OK == (x))
#define EasyBlendSDK_FAILED(x)    (EasyBlendSDK_ERR_S_OK != (x))

EasyBlendSDK_API EasyBlendSDKError
EasyBlendSDK_Initialize( const char* Filename,
                         EasyBlendSDK_Mesh* msm,
                         int SDKMode = EasyBlendSDK_SDKMODE_OpenGL,
                         EasyBlendSDKCallback cb = NULL,
                         void* cbData = NULL );


EasyBlendSDK_API EasyBlendSDKError
EasyBlendSDK_Uninitialize( EasyBlendSDK_Mesh* msm,
                           EasyBlendSDKCallback cb = NULL,
                           void* cbData  = NULL );

EasyBlendSDK_API EasyBlendSDKError
EasyBlendSDK_TransformInputToOutput ( EasyBlendSDK_Mesh* msm );

EasyBlendSDK_API EasyBlendSDKError EasyBlendSDK_GetHeadingPitchRoll (
    double& rdDegreesHeading, double& rdDegreesPitch, double& rdDegreesRoll,
    EasyBlendSDK_Mesh* msm);

#endif  /* ifndef _EasyBlendSDK_H_ */

