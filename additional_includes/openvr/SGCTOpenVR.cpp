/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "SGCTOpenVR.h"
#ifdef __APPLE__
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#undef __gl_h_
#endif
#include <sgct/SGCTWindow.h>
#include <sgct/ClusterManager.h>
#include <sgct/MessageHandler.h>

bool sgct::SGCTOpenVR::isHMDconnected = false;
bool sgct::SGCTOpenVR::isOpenVRInitalized = false;

vr::IVRSystem* sgct::SGCTOpenVR::HMD = NULL;
vr::IVRRenderModels* sgct::SGCTOpenVR::renderModels = NULL;
vr::TrackedDevicePose_t sgct::SGCTOpenVR::trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
char sgct::SGCTOpenVR::deviceClassChar[vr::k_unMaxTrackedDeviceCount]; // for each device, a character representing its class

sgct::SGCTOpenVR::FBODesc sgct::SGCTOpenVR::leftEyeFBODesc;
sgct::SGCTOpenVR::FBODesc sgct::SGCTOpenVR::rightEyeFBODesc;

// Matries updated every rendering cycle
glm::mat4 sgct::SGCTOpenVR::poseHMDMat;
glm::mat4 sgct::SGCTOpenVR::devicePoseMat[vr::k_unMaxTrackedDeviceCount];

// Matrices updated on statup
glm::mat4 sgct::SGCTOpenVR::eyeLeftProjectionMat;
glm::mat4 sgct::SGCTOpenVR::eyeLeftToHeadMat;
glm::mat4 sgct::SGCTOpenVR::eyeRightProjectionMat;
glm::mat4 sgct::SGCTOpenVR::eyeRightToHeadMat;

/*!
	Init OpenVR
 */
void sgct::SGCTOpenVR::initialize(float nearClip, float farClip)
{
	if (!isOpenVRInitalized) {
		isHMDconnected = vr::VR_IsHmdPresent();

		if (isHMDconnected) {
			// Loading the SteamVR Runtime
			vr::EVRInitError eError = vr::VRInitError_None;
			HMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

			if (eError != vr::VRInitError_None)
			{
				shutdown();
				char buf[1024];
				snprintf(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "VR_Init Failed", buf);
			}
			else {
				isOpenVRInitalized = true;

				unsigned int renderWidth;
				unsigned int renderHeight;
				HMD->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "OpenVR render dimensions per eye: %d x %d\n", renderWidth, renderHeight);

				// Create FBO and Texture used for sending data top HMD
				createHMDFrameBuffer(renderWidth, renderHeight, leftEyeFBODesc);
				createHMDFrameBuffer(renderWidth, renderHeight, rightEyeFBODesc);

				std::string HMDDevice = getTrackedDeviceString(HMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "OpenVR Device Name : %s\n", HMDDevice.c_str());

				std::string HMDNumber = getTrackedDeviceString(HMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "OpenVR Device Number : %s\n", HMDNumber.c_str());

				renderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
				if (!renderModels)
				{
					shutdown();
					char buf[1024];
					snprintf(buf, sizeof(buf), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
					sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "VR_Init Failed\n", buf);
				}

				updateHMDMatrices(nearClip, farClip);
			}
		}
	}
	else {
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "OpenVR has already been initialized...\n");
	}
}

/*!
Shutdown OpenVR
*/
void sgct::SGCTOpenVR::shutdown()
{
	if (isOpenVRInitalized) {
		vr::VR_Shutdown();

		glDeleteTextures(1, &leftEyeFBODesc.texID);
		glDeleteFramebuffers(1, &leftEyeFBODesc.fboID);

		glDeleteTextures(1, &rightEyeFBODesc.texID);
		glDeleteFramebuffers(1, &rightEyeFBODesc.fboID);
	}
	else if (HMD) {
		vr::VR_Shutdown();
	}
	HMD = NULL;
}

bool sgct::SGCTOpenVR::isHMDActive()
{
	return (vr::VR_IsHmdPresent() && isOpenVRInitalized);
}

// Assuming side-by-side stereo, i.e. one FBO, one texture for both eyes
void sgct::SGCTOpenVR::copyWindowToHMD(SGCTWindow* win){
	if (isHMDActive()) {
		int windowWidth, windowHeight;
		win->getFinalFBODimensions(windowWidth, windowHeight);
		int renderWidth = windowWidth / 2;
		int renderHeight = windowHeight;
		//sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Render dimensions: %d x %d\n", renderWidth, renderHeight);

		// HMD Left Eye
		glBindFramebuffer(GL_READ_FRAMEBUFFER, win->mFinalFBO_Ptr->getBufferID());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeFBODesc.fboID);

		glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR);

		vr::Texture_t leftEyeTexture = { (void *)(size_t)leftEyeFBODesc.texID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);

		// HMD Right Eye
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeFBODesc.fboID);

		glBlitFramebuffer(renderWidth, 0, windowWidth, renderHeight, 0, 0, renderWidth, renderHeight,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		vr::Texture_t rightEyeTexture = { (void *)(size_t)rightEyeFBODesc.texID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}
}

glm::mat4 sgct::SGCTOpenVR::getHMDCurrentViewProjectionMatrix(sgct_core::Frustum::FrustumMode nEye)
{
	if (nEye == sgct_core::Frustum::StereoLeftEye)
	{
		return eyeLeftProjectionMat * eyeLeftToHeadMat * poseHMDMat;
	}
	else if (nEye == sgct_core::Frustum::StereoRightEye)
	{
		return eyeRightProjectionMat * eyeRightToHeadMat *  poseHMDMat;
	}
    else // Mono
    {
		return eyeLeftProjectionMat * poseHMDMat;
    }
}

std::string sgct::SGCTOpenVR::getTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError)
{
	if (isHMDActive()) {
		uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
		if (unRequiredBufferLen == 0)
			return "";

		char *pchBuffer = new char[unRequiredBufferLen];
		unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
		std::string sResult = pchBuffer;
		delete[] pchBuffer;
		return sResult;
	}
	else
		return "";
}

//Updates pose matrices for all tracked OpenVR devices
void sgct::SGCTOpenVR::updatePoses()
{
	if (!HMD)
		return;

	vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (trackedDevicePose[nDevice].bPoseIsValid)
		{
			devicePoseMat[nDevice] = convertSteamVRMatrixToMatrix4(trackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (deviceClassChar[nDevice] == 0)
			{
				switch (HMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        deviceClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               deviceClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           deviceClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    deviceClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: deviceClassChar[nDevice] = 'T'; break;
				default:                                       deviceClassChar[nDevice] = '?'; break;
				}
			}
		}
	}

	if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		poseHMDMat = devicePoseMat[vr::k_unTrackedDeviceIndex_Hmd];
		glm::inverse(poseHMDMat);
	}
}

//Updates matrices for both eyes of tracked HMD.
void sgct::SGCTOpenVR::updateHMDMatrices(float nearClip, float farClip) {
	eyeLeftProjectionMat = getHMDEyeProjectionMatrix(vr::Eye_Left, nearClip, farClip);
    eyeLeftToHeadMat = getHMDEyeToHeadTransform(vr::Eye_Left);
	eyeRightProjectionMat = getHMDEyeProjectionMatrix(vr::Eye_Right, nearClip, farClip);
	eyeRightToHeadMat = getHMDEyeToHeadTransform(vr::Eye_Right);
}

glm::mat4 sgct::SGCTOpenVR::getHMDEyeProjectionMatrix(vr::Hmd_Eye nEye, float nearClip, float farClip)
{
	if (!HMD)
		return glm::mat4();

	vr::HmdMatrix44_t mat = HMD->GetProjectionMatrix(nEye, nearClip, farClip);

	return glm::mat4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}


glm::mat4 sgct::SGCTOpenVR::getHMDEyeToHeadTransform(vr::Hmd_Eye nEye)
{
    if (!HMD)
		return glm::mat4();

	vr::HmdMatrix34_t mat = HMD->GetEyeToHeadTransform(nEye);
	glm::mat4 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0,
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
	);

	return glm::inverse(matrixObj);
}

glm::mat4 sgct::SGCTOpenVR::getHMDPoseMatrix() {
	return poseHMDMat;
}

glm::quat sgct::SGCTOpenVR::getInverseRotation(glm::mat4 matPose) {
	glm::quat q;
	q.w = sqrt(fmaxf(0, 1 + matPose[0][0] + matPose[1][1] + matPose[2][2])) / 2;
	q.x = sqrt(fmaxf(0, 1 + matPose[0][0] - matPose[1][1] - matPose[2][2])) / 2;
	q.y = sqrt(fmaxf(0, 1 - matPose[0][0] + matPose[1][1] - matPose[2][2])) / 2;
	q.z = sqrt(fmaxf(0, 1 - matPose[0][0] - matPose[1][1] + matPose[2][2])) / 2;
	q.x = copysign(q.x, matPose[2][1] - matPose[1][2]);
	q.y = copysign(q.y, matPose[0][2] - matPose[2][0]);
	q.z = copysign(q.z, matPose[1][0] - matPose[0][1]);
	return q;
}

glm::mat4 sgct::SGCTOpenVR::convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
	glm::mat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return glm::inverse(matrixObj);
}

bool sgct::SGCTOpenVR::createHMDFrameBuffer(int width, int height, FBODesc &fboDesc) {
	glGenFramebuffers(1, &fboDesc.fboID);
	glBindFramebuffer(GL_FRAMEBUFFER, fboDesc.fboID);

	glGenTextures(1, &fboDesc.texID);
	glBindTexture(GL_TEXTURE_2D, fboDesc.texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboDesc.texID, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}
