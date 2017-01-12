#include "sgct.h"
#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <openvr.h>

struct FBODesc
{
	GLuint fboID;
	GLuint texID;
};

sgct::Engine * gEngine;

void myDrawFun();
void myPostDrawFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

// OpenVR
bool isHMDActive();
std::string getTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL);
void updateHMDPose();
glm::mat4 getHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
glm::mat4 getHMDMatrixPoseEye(vr::Hmd_Eye nEye);
glm::mat4 getHMDCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
glm::mat4 convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose);
bool createVRFrameBuffer(int width, int height, FBODesc &fboDesc);

//input callbacks
void keyCallback(int key, int action);
void mouseButtonCallback(int button, int action);

void drawXZGrid(glm::mat4& MVP);
void drawPyramid(glm::mat4& MVP, int index);
void createXZGrid(int size, float yPos);
void createPyramid(float width);

float rotationSpeed = 0.0017f;
float walkingSpeed = 2.5f;

const int landscapeSize = 50;
const int numberOfPyramids = 150;

//OpenVR
bool isHMDconnected = false;
bool isOpenVRInitalized = false;
vr::IVRSystem *HMD;
vr::IVRRenderModels *renderModels;
vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
char deviceClassChar[vr::k_unMaxTrackedDeviceCount]; // for each device, a character representing its class

FBODesc leftEyeFBODesc;
FBODesc rightEyeFBODesc;

// Matries updated every frame
glm::mat4 HMDPose;
glm::mat4 devicePoseMat[vr::k_unMaxTrackedDeviceCount];

// Matrices updated on statup
glm::mat4 projectionMatEyeLeft;
glm::mat4 projectionMatEyeRight;
glm::mat4 poseMatEyeLeft;
glm::mat4 poseMatEyeRight;

bool arrowButtons[4];
enum directions { FORWARD = 0, BACKWARD, LEFT, RIGHT };

//to check if left mouse button is pressed
bool mouseLeftButton = false;
/* Holds the difference in position between when the left mouse button
    is pressed and when the mouse button is held. */
double mouseDx = 0.0;
/* Stores the positions that will be compared to measure the difference. */
double mouseXPos[] = { 0.0, 0.0 };

glm::vec3 view(0.0f, 0.0f, 1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
glm::vec3 pos(0.0f, 0.0f, 0.0f);

sgct::SharedObject<glm::mat4> xform;
glm::mat4 pyramidTransforms[numberOfPyramids];

enum geometryType { PYRAMID = 0, GRID };
GLuint VAOs[2] = { GL_FALSE, GL_FALSE };
GLuint VBOs[2] = { GL_FALSE, GL_FALSE };
//shader locations
GLint Matrix_Locs[2] = { -1, -1 };
GLint linecolor_loc = -1;
GLint alpha_Loc = -1;

int numberOfVerts[2] = { 0, 0 };

class Vertex
{
public:
	Vertex() { mX = mY = mZ = 0.0f; }
	Vertex(float z, float y, float x) { mX = x; mY = y; mZ = z; }
	float mX, mY, mZ;
};

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPostDrawFunction( myPostDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );
	gEngine->setMouseButtonCallbackFunction( mouseButtonCallback );
	gEngine->setClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	gEngine->setCleanUpFunction( myCleanUpFun );

	for(int i=0; i<4; i++)
		arrowButtons[i] = false;

	if (!gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ))
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::instance()->setEncodeFunction( myEncodeFun );
	sgct::SharedData::instance()->setDecodeFunction( myDecodeFun );

	// Main loop
	gEngine->render();

	// Clean up
	if (isOpenVRInitalized) {
		vr::VR_Shutdown();

		glDeleteTextures(1, &leftEyeFBODesc.texID);
		glDeleteFramebuffers(1, &leftEyeFBODesc.fboID);

		glDeleteTextures(1, &rightEyeFBODesc.texID);
		glDeleteFramebuffers(1, &rightEyeFBODesc.fboID);
	}

	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myInitOGLFun()
{
	isHMDconnected = vr::VR_IsHmdPresent();

	sgct::SGCTWindow* win = gEngine->getWindowPtr(0);

	//MSAA 4 samples
	win->setNumberOfAASamples(4);

	//sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Testing if HMD is connected...\n");
	
	if (isHMDconnected) {
		// Loading the SteamVR Runtime
		vr::EVRInitError eError = vr::VRInitError_None;
		HMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

		if (eError != vr::VRInitError_None)
		{
			HMD = NULL;
			char buf[1024];
			snprintf(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "VR_Init Failed", buf);
		}
		else {
			isOpenVRInitalized = true;

			unsigned int renderWidth;
			unsigned int renderHeight;
			HMD->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
			//win->setWindowResolution(renderWidth * 2, renderHeight);
			//win->setFramebufferResolution(renderWidth, renderHeight);
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Render dimensions: %d x %d\n", renderWidth, renderHeight);

			// Create FBO and Texture used for sending data top HMD
			createVRFrameBuffer(renderWidth, renderHeight, leftEyeFBODesc);
			createVRFrameBuffer(renderWidth, renderHeight, rightEyeFBODesc);

			std::string HMDDriver = getTrackedDeviceString(HMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
			std::string HMDDisplay = getTrackedDeviceString(HMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
			std::string windowTitle = "OpenVR Example - " + HMDDriver + " " + HMDDisplay;
			win->setWindowTitle(windowTitle.c_str());

			renderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
			if (!renderModels)
			{
				HMD = NULL;
				vr::VR_Shutdown();

				char buf[1024];
				snprintf(buf, sizeof(buf), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "VR_Init Failed", buf);
			}

			projectionMatEyeLeft = getHMDMatrixProjectionEye(vr::Eye_Left);
			projectionMatEyeRight = getHMDMatrixProjectionEye(vr::Eye_Right);
			poseMatEyeLeft = getHMDMatrixPoseEye(vr::Eye_Left);
			poseMatEyeRight = getHMDMatrixPoseEye(vr::Eye_Right);
		}
	}
	else {
		std::string windowTitle = "OpenVR Example - No HMD Connected";
		win->setWindowTitle(windowTitle.c_str());
	}

	//generate the VAOs
	glGenVertexArrays(2, &VAOs[0]);
	//generate VBOs for vertex positions
	glGenBuffers(2, &VBOs[0]);

	createXZGrid(landscapeSize, -1.5f);
	createPyramid(0.6f);

	//pick a seed for the random function (must be same on all nodes)
	srand(9745);
	for(int i=0; i<numberOfPyramids; i++)
	{
		float xPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);
		float zPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);

		pyramidTransforms[i] = glm::translate(glm::mat4(1.0f), glm::vec3(xPos, -1.5f, zPos));
	}

	sgct::ShaderManager::instance()->addShaderProgram("gridShader",
		"gridShader.vert",
		"gridShader.frag");
	sgct::ShaderManager::instance()->bindShaderProgram("gridShader");
	Matrix_Locs[GRID] = sgct::ShaderManager::instance()->getShaderProgram("gridShader").getUniformLocation("MVP");
	linecolor_loc = sgct::ShaderManager::instance()->getShaderProgram("gridShader").getUniformLocation("linecolor");
	sgct::ShaderManager::instance()->unBindShaderProgram();

	sgct::ShaderManager::instance()->addShaderProgram("pyramidShader",
		"pyramidShader.vert",
		"pyramidShader.frag");
	sgct::ShaderManager::instance()->bindShaderProgram("pyramidShader");
	Matrix_Locs[PYRAMID] = sgct::ShaderManager::instance()->getShaderProgram("pyramidShader").getUniformLocation("MVP");
	alpha_Loc = sgct::ShaderManager::instance()->getShaderProgram("pyramidShader").getUniformLocation("alpha");
	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		if( mouseLeftButton )
		{
			double tmpYPos;
			//get the mouse pos from first window
			sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[0], &tmpYPos );
			mouseDx = mouseXPos[0] - mouseXPos[1];
		}
		else
		{
			mouseDx = 0.0;
		}

		static float panRot = 0.0f;
		panRot += (static_cast<float>(mouseDx) * rotationSpeed * static_cast<float>(gEngine->getDt()));

		glm::mat4 ViewRotateX = glm::rotate(
			glm::mat4(1.0f),
			panRot,
			glm::vec3(0.0f, 1.0f, 0.0f)); //rotation around the y-axis

		view = glm::inverse(glm::mat3(ViewRotateX)) * glm::vec3(0.0f, 0.0f, 1.0f);

		glm::vec3 right = glm::cross(view, up);

		if( arrowButtons[FORWARD] )
			pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * view);
		if( arrowButtons[BACKWARD] )
			pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * view);
		if( arrowButtons[LEFT] )
			pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);
		if( arrowButtons[RIGHT] )
			pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);

		/*
			To get a first person camera, the world needs
			to be transformed around the users head.

			This is done by:
			1, Transform the user to coordinate system origin
			2, Apply navigation
			3, Apply rotation
			4, Transform the user back to original position

			However, mathwise this process need to be reversed
			due to the matrix multiplication order.
		*/

		glm::mat4 result;
		//4. transform user back to original position
		result = glm::translate(glm::mat4(1.0f), sgct::Engine::getDefaultUserPtr()->getPos());
		//3. apply view rotation
		result *= ViewRotateX;
		//2. apply navigation translation
		result *= glm::translate(glm::mat4(1.0f), pos);
		//1. transform user to coordinate system origin
		result *= glm::translate(glm::mat4(1.0f), -sgct::Engine::getDefaultUserPtr()->getPos());

		xform.setVal( result );
	}
}

void myDrawFun()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_MULTISAMPLE);

	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);
	//glDisable(GL_DEPTH_TEST);

	glm::mat4 MVP;
	if (isHMDActive()) {
		//sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Eye: %d\n", gEngine->getCurrentFrustumMode());
		if (gEngine->getCurrentFrustumMode() == sgct_core::Frustum::StereoRightEye)
			MVP = getHMDCurrentViewProjectionMatrix(vr::Eye_Right);
		else
			MVP = getHMDCurrentViewProjectionMatrix(vr::Eye_Left);
	}
	else {
		MVP = gEngine->getCurrentModelViewProjectionMatrix() * xform.getVal();
	}

	drawXZGrid(MVP);

	for (int i = 0; i < numberOfPyramids; i++)
		drawPyramid(MVP, i);

	//glEnable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_TEST);

	glDisable(GL_MULTISAMPLE);

	glDisable(GL_BLEND);
}

void myPostDrawFun()
{
	if (isHMDActive()) {
		sgct::SGCTWindow* win = gEngine->getWindowPtr(0);
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

void myEncodeFun()
{
	sgct::SharedData::instance()->writeObj( &xform );
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readObj( &xform );
}

bool isHMDActive() {
	return (vr::VR_IsHmdPresent() && isOpenVRInitalized);
}

std::string getTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError)
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}

void updateHMDPose()
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

glm::mat4 getHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!HMD)
		return glm::mat4();

	float nearClip = 0.1f;
	float farClip = 30.0f;
	vr::HmdMatrix44_t mat = HMD->GetProjectionMatrix(nEye, nearClip, farClip);

	return glm::mat4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}


glm::mat4 getHMDMatrixPoseEye(vr::Hmd_Eye nEye)
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


glm::mat4 getHMDCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
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

glm::mat4 convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
	glm::mat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return glm::inverse(matrixObj);
}

bool createVRFrameBuffer(int width, int height, FBODesc &fboDesc) {
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

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	return true;
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case SGCT_KEY_UP:
		case SGCT_KEY_W:
			arrowButtons[FORWARD] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
			break;

		case SGCT_KEY_DOWN:
		case SGCT_KEY_S:
			arrowButtons[BACKWARD] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
			break;

		case SGCT_KEY_LEFT:
		case SGCT_KEY_A:
			arrowButtons[LEFT] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
			break;

		case SGCT_KEY_RIGHT:
		case SGCT_KEY_D:
			arrowButtons[RIGHT] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
			break;
		}
	}
}

void mouseButtonCallback(int button, int action)
{
	if( gEngine->isMaster() )
	{
		switch( button )
		{
		case SGCT_MOUSE_BUTTON_LEFT:
			mouseLeftButton = (action == SGCT_PRESS ? true : false);
			double tmpYPos;
			//set refPos
			sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[1], &tmpYPos );
			break;
		}
	}
}

void drawXZGrid(glm::mat4& MVP)
{
	sgct::ShaderManager::instance()->bindShaderProgram("gridShader");

	glUniformMatrix4fv(Matrix_Locs[GRID], 1, GL_FALSE, &MVP[0][0]);

	glBindVertexArray(VAOs[GRID]);

	/*if (gEngine->getCurrentFrustumMode() == sgct_core::Frustum::StereoLeftEye)
		glUniform4f(linecolor_loc, 1.f, 0.f, 0.f, 0.8f);
	else if (gEngine->getCurrentFrustumMode() == sgct_core::Frustum::StereoRightEye)
		glUniform4f(linecolor_loc, 0.f, 1.f, 0.f, 0.8f);
	else*/
		glUniform4f(linecolor_loc, 1.f, 1.f, 1.f, 0.8f);

	glLineWidth(3.0f);
	glPolygonOffset(0.0f, 0.0f); //offset to avoid z-buffer fighting
	glDrawArrays(GL_LINES, 0, numberOfVerts[GRID]);

	//unbind
	glBindVertexArray(0);
	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void drawPyramid(glm::mat4& MVP, int index)
{
	glm::mat4 MVP_pyramid = MVP * pyramidTransforms[index];

	sgct::ShaderManager::instance()->bindShaderProgram("pyramidShader");

	glUniformMatrix4fv(Matrix_Locs[PYRAMID], 1, GL_FALSE, &MVP_pyramid[0][0]);

	glBindVertexArray(VAOs[PYRAMID]);

	//draw lines
	glLineWidth(2.0f);
	glPolygonOffset(1.0f, 0.1f); //offset to avoid z-buffer fighting
	glUniform1f(alpha_Loc, 0.8f);
	glDrawArrays(GL_LINES, 0, 16);
	//draw triangles
	glPolygonOffset(0.0f, 0.0f); //offset to avoid z-buffer fighting
	glUniform1f(alpha_Loc, 0.3f);
	glDrawArrays(GL_TRIANGLES, 16, 12);

	//unbind
	glBindVertexArray(0);
	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void createXZGrid(int size, float yPos)
{
	numberOfVerts[GRID] = size * 4;
	Vertex * vertData = new (std::nothrow) Vertex[numberOfVerts[GRID]];
	
	int i = 0;
	for (int x = -(size / 2); x < (size / 2); x++)
	{
		vertData[i].mX = static_cast<float>(x);
		vertData[i].mY = yPos;
		vertData[i].mZ = static_cast<float>(-(size / 2));

		vertData[i + 1].mX = static_cast<float>(x);
		vertData[i + 1].mY = yPos;
		vertData[i + 1].mZ = static_cast<float>(size / 2);

		i += 2;
	}

	for (int z = -(size / 2); z < (size / 2); z++)
	{
		vertData[i].mX = static_cast<float>(-(size / 2));
		vertData[i].mY = yPos;
		vertData[i].mZ = static_cast<float>(z);

		vertData[i + 1].mX = static_cast<float>(size / 2);
		vertData[i + 1].mY = yPos;
		vertData[i + 1].mZ = static_cast<float>(z);

		i += 2;
	}

	glBindVertexArray(VAOs[GRID]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[GRID]);
	
	//upload data to GPU
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*numberOfVerts[GRID], vertData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		reinterpret_cast<void*>(0) // array buffer offset
		);

	//unbind
	glBindVertexArray(GL_FALSE);
	glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);

	//clean up
	delete[] vertData;
	vertData = NULL;
}

void createPyramid(float width)
{
	std::vector<Vertex> vertData;

	//enhance the pyramids with lines in the edges
	//-x
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	//+x
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	//-z
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	//+z
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
	
	//triangles
	//-x
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	//+x
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	//-z
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	//+z
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));

	numberOfVerts[PYRAMID] = static_cast<int>( vertData.size() );

	glBindVertexArray(VAOs[PYRAMID]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[PYRAMID]);

	//upload data to GPU
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*numberOfVerts[PYRAMID], &vertData[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		reinterpret_cast<void*>(0) // array buffer offset
		);

	//unbind
	glBindVertexArray(GL_FALSE);
	glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);

	//clean up
	vertData.clear();
}

void myCleanUpFun()
{
	if (VBOs[0])
		glDeleteBuffers(2, &VBOs[0]);
	if (VAOs[0])
		glDeleteVertexArrays(2, &VAOs[0]);
}
