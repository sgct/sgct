/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "SGCTOpenVR.h"
#include <sgct/SGCTWindow.h>
#include <sgct/ClusterManager.h>
#include <sgct/MessageHandler.h>

bool sgct::SGCTOpenVR::isHMDconnected = false;
bool sgct::SGCTOpenVR::isOpenVRInitalized = false;

vr::IVRSystem* sgct::SGCTOpenVR::HMD;
vr::IVRRenderModels* sgct::SGCTOpenVR::renderModels;
vr::TrackedDevicePose_t sgct::SGCTOpenVR::trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
char sgct::SGCTOpenVR::deviceClassChar[vr::k_unMaxTrackedDeviceCount]; // for each device, a character representing its class

sgct::SGCTOpenVR::FBODesc sgct::SGCTOpenVR::leftEyeFBODesc;
sgct::SGCTOpenVR::FBODesc sgct::SGCTOpenVR::rightEyeFBODesc;

// Matries updated every rendering cycle
glm::mat4 sgct::SGCTOpenVR::HMDPose;
glm::mat4 sgct::SGCTOpenVR::devicePoseMat[vr::k_unMaxTrackedDeviceCount];

// Matrices updated on statup
glm::mat4 sgct::SGCTOpenVR::projectionMatEyeLeft;
glm::mat4 sgct::SGCTOpenVR::projectionMatEyeRight;
glm::mat4 sgct::SGCTOpenVR::poseMatEyeLeft;
glm::mat4 sgct::SGCTOpenVR::poseMatEyeRight;

/*!
	Init OpenVR
 */
void sgct::SGCTOpenVR::initialize(float nearClip, float farClip)
{
	if (!isOpenVRInitalized) {
		isHMDconnected = vr::VR_IsHmdPresent();

		//sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Testing if HMD is connected...\n");

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
				createVRFrameBuffer(renderWidth, renderHeight, leftEyeFBODesc);
				createVRFrameBuffer(renderWidth, renderHeight, rightEyeFBODesc);

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

void sgct::SGCTOpenVR::setupWindow(sgct::SGCTWindow* win) 
{
	if (isHMDActive()) {
		unsigned int renderWidth;
		unsigned int renderHeight;
		HMD->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

		win->setWindowResolution(renderWidth, renderHeight / 2);
		win->setFramebufferResolution(renderWidth * 2, renderHeight);
	}
}

void sgct::SGCTOpenVR::copyWindowToHMD(SGCTWindow* win){
	if (isHMDActive()) {
		int windowWidth = win->getXFramebufferResolution();
		int windowHeight = win->getYFramebufferResolution();
		// Assuming side-by-side stereo
		int renderWidth = windowWidth / 2;
		int renderHeight = windowHeight;
		//sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Render dimensions: %d x %d\n", renderWidth, renderHeight);

		// HMD Left Eye
		glBindFramebuffer(GL_READ_FRAMEBUFFER, win->getFrameBufferTexture(0));
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeFBODesc.texID);

		glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR);

		//glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		vr::Texture_t leftEyeTexture = { (void *)(size_t)leftEyeFBODesc.texID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);

		// HMD Right Eye
		//glBindFramebuffer(GL_READ_FRAMEBUFFER, win->getFrameBufferTexture(sgct::Engine::RightEye));
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeFBODesc.texID);

		glBlitFramebuffer(renderWidth, 0, windowWidth, renderHeight, 0, 0, renderWidth, renderHeight,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		vr::Texture_t rightEyeTexture = { (void *)(size_t)rightEyeFBODesc.texID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);

		updateHMDPose();
	}
}

glm::mat4 sgct::SGCTOpenVR::getHMDCurrentViewProjectionMatrix(sgct_core::Frustum::FrustumMode nEye)
{
	if (nEye == sgct_core::Frustum::StereoRightEye)
	{
		return getHMDCurrentViewProjectionMatrix(vr::Eye_Right);
	}
	else
	{
		return getHMDCurrentViewProjectionMatrix(vr::Eye_Left);
	}
}

glm::mat4 sgct::SGCTOpenVR::getHMDCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{
	glm::mat4 matMVP;
	if (nEye == vr::Eye_Left)
	{
		matMVP = projectionMatEyeLeft * poseMatEyeLeft * HMDPose;
	}
	else if (nEye == vr::Eye_Right)
	{
		matMVP = projectionMatEyeRight * poseMatEyeRight *  HMDPose;
	}

	return matMVP;
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

void sgct::SGCTOpenVR::updateHMDPose()
{
	if (!HMD)
		return;

	vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	//int iValidPoseCount = 0;
	//std::string strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (trackedDevicePose[nDevice].bPoseIsValid)
		{
			//iValidPoseCount++;
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
			//strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		HMDPose = devicePoseMat[vr::k_unTrackedDeviceIndex_Hmd];
		glm::inverse(HMDPose);
	}

	// Update user transform
	sgct_core::ClusterManager::instance()->getDefaultUserPtr()->setTransform(HMDPose);
}

void sgct::SGCTOpenVR::updateHMDMatrices(float nearClip, float farClip) {
	projectionMatEyeLeft = getHMDMatrixProjectionEye(vr::Eye_Left, nearClip, farClip);
	projectionMatEyeRight = getHMDMatrixProjectionEye(vr::Eye_Right, nearClip, farClip);
	poseMatEyeLeft = getHMDMatrixPoseEye(vr::Eye_Left);
	poseMatEyeRight = getHMDMatrixPoseEye(vr::Eye_Right);
}

glm::mat4 sgct::SGCTOpenVR::getHMDMatrixProjectionEye(vr::Hmd_Eye nEye, float nearClip, float farClip)
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


glm::mat4 sgct::SGCTOpenVR::getHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!HMD)
		return glm::mat4();

	vr::HmdMatrix34_t matEyeRight = HMD->GetEyeToHeadTransform(nEye);
	glm::mat4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
	);

	return glm::inverse(matrixObj);
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

bool sgct::SGCTOpenVR::createVRFrameBuffer(int width, int height, FBODesc &fboDesc) {
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
