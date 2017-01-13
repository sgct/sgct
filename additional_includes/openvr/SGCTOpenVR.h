/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_OPENVR_H_
#define _SGCT_OPENVR_H_

#include <string>
#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sgct/ogl_headers.h>
#include <sgct/Frustum.h>
#include <openvr.h>

namespace sgct
{
    
class SGCTWindow;

/*!
Class for using OpenVR in SGCT in an easy manor
*/
class SGCTOpenVR
{
public:
	struct FBODesc
	{
		GLuint fboID;
		GLuint texID;
	};

	static void initialize(float nearClip, float farClip);
	static void shutdown();

	static bool isHMDActive();

	static void copyWindowToHMD(SGCTWindow* win);

	static glm::mat4 getHMDCurrentViewProjectionMatrix(sgct_core::Frustum::FrustumMode nEye);
	static glm::mat4 getHMDCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);

	static void updatePoses();
	static void updateHMDMatrices(float nearClip, float farClip);

	static std::string getTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL);
	static glm::mat4 getHMDEyeProjectionMatrix(vr::Hmd_Eye nEye, float nearClip, float farClip);
	static glm::mat4 getHMDEyeToHeadTransform(vr::Hmd_Eye nEye);

private:
	static glm::mat4 convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose);
	static bool createHMDFrameBuffer(int width, int height, FBODesc &fboDesc);

	static bool isHMDconnected;
	static bool isOpenVRInitalized;
	static vr::IVRSystem *HMD;
	static vr::IVRRenderModels *renderModels;
	static vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	static char deviceClassChar[vr::k_unMaxTrackedDeviceCount]; // for each device, a character representing its class

	static FBODesc leftEyeFBODesc;
	static FBODesc rightEyeFBODesc;

	// Matries updated every rendering cycle
	static glm::mat4 poseHMDMat;
	static glm::mat4 devicePoseMat[vr::k_unMaxTrackedDeviceCount];

	// Matrices updated on statup
	static glm::mat4 eyeLeftProjectionMat;
    static glm::mat4 eyeLeftToHeadMat;
	static glm::mat4 eyeRightProjectionMat;
	static glm::mat4 eyeRightToHeadMat;
};

} // sgct_core

#endif
