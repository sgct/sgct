/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_WINDOW_H_
#define _SGCT_WINDOW_H_

#include "ogl_headers.h"
#include "OffScreenBuffer.h"
#include "ScreenCapture.h"
#include "Viewport.h"
#include "ShaderProgram.h"
#include <vector>

#define NUMBER_OF_VBOS 2

namespace sgct_core
{

/*!
Helper class for window data. 
*/
class SGCTWindow
{
public:	
	/*!
		Different stereo modes used for rendering
	*/
	enum StereoMode { NoStereo = 0, Active, Anaglyph_Red_Cyan, Anaglyph_Amber_Blue, Anaglyph_Red_Cyan_Wimmer, Checkerboard, Checkerboard_Inverted, Vertical_Interlaced, Vertical_Interlaced_Inverted, DummyStereo };
	enum VBOIndex { RenderQuad = 0, FishEyeQuad };
	enum FisheyeCropSide { CropLeft = 0, CropRight, CropBottom, CropTop };

public:
	SGCTWindow();
	void close();
	void init(int id);
	void initOGL();
	void setWindowTitle(const char * title);
	void setWindowResolution(const int x, const int y);
	void setFramebufferResolution(const int x, const int y);
	void initWindowResolution(const int x, const int y);
	void swap();
	void update();
	void makeOpenGLContextCurrent();
	void makeSharedOpenGLContextCurrent();
	bool isWindowResized();
	void setWindowPosition(const int x, const int y);
	void setWindowMode(bool fullscreen);
	void setWindowDecoration(bool state);
	void setFullScreenMonitorIndex( int index );
	void setBarrier(const bool state);
	void setFixResolution(const bool state);
	void useSwapGroups(const bool state);
	void useQuadbuffer(const bool state);
	bool openWindow(GLFWwindow* share);
	void initNvidiaSwapGroups();
	void getSwapGroupFrameNumber(unsigned int & frameNumber);
	void resetSwapGroupFrameNumber();
	void bindVAO();
	void bindVAO( VBOIndex index );
	void bindVBO();
	void bindVBO( VBOIndex index );
	void unbindVBO();
	void unbindVAO();
	void captureBuffer();
	sgct_core::OffScreenBuffer * getFBOPtr();
	void getFBODimensions( int & width, int & height );

	/*!
		\returns If the frame buffer has a fix resolution this function returns true.
	*/
	inline bool isFixResolution() { return mUseFixResolution; }
	inline bool isBarrierActive() { return mBarrier; }
	inline bool isUsingSwapGroups() { return mUseSwapGroups; }
	inline bool isSwapGroupMaster() { return mSwapGroupMaster; }
	
	GLFWmonitor * getMonitor() { return mMonitor; }
	GLFWwindow * getWindowHandle() { return mWindowHandle; }

	/*!
		Returns pointer to screen capture ptr
	*/
	sgct_core::ScreenCapture * getScreenCapturePointer() { return mScreenCapture; }

	/*!
		\returns Get the horizontal window resolution.
	*/
	inline int getXResolution() { return mWindowRes[0]; }
	/*!
		\returns Get the vertical window resolution.
	*/
	inline int getYResolution() { return mWindowRes[1]; }
	/*!
		\returns Get the horizontal frame buffer resolution.
	*/
	inline int getXFramebufferResolution() { return mFramebufferResolution[0]; }
	/*!
		\returns Get the vertical frame buffer resolution.
	*/
	inline int getYFramebufferResolution() { return mFramebufferResolution[1]; }

	//! \returns the aspect ratio of the window 
	inline float getAspectRatio() { return mAspectRatio; }

	/*!
		Returns the stereo mode. The value can be compared to the sgct_core::ClusterManager::StereoMode enum
	*/
	StereoMode getStereoMode();

	/*!
		Returns true if any kind of stereo is enabled
	*/
	inline bool isStereo() { return (mStereoMode != NoStereo); }
	/*!
		Set the stereo mode. This must be done before creating the window (before Engine::Init)
	*/
	inline void setStereoMode( StereoMode sm ) { mStereoMode = sm; }

	//viewport stuff
	void addViewport(float left, float right, float bottom, float top);
	void addViewport(Viewport &vp);
	void generateCubeMapViewports();
	Viewport * getCurrentViewport();
	Viewport * getViewport(std::size_t index);
	void getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize);
	std::size_t getNumberOfViewports();
	inline std::size_t getCurrentViewportIndex() { return mCurrentViewportIndex; }
	inline void setCurrentViewport(std::size_t index) { mCurrentViewportIndex = index; }
	inline void setSwapInterval(int val) { mSwapInterval = val; }

	void setNumberOfAASamples(int samples);
	int getNumberOfAASamples();

	inline bool isFullScreen() { return mFullScreen; }
	inline unsigned int getFrameBufferTexture(unsigned int index){ return mFrameBufferTextures[index]; }
	/*!
		\returns this window's id
	*/
	inline int getId() { return mId; }
	
	inline void bindStereoShader() { mStereoShader.bind(); }
	inline int getStereoShaderMVPLoc() { return StereoMVP; }
	inline int getStereoShaderLeftTexLoc() { return StereoLeftTex; }
	inline int getStereoShaderRightTexLoc() { return StereoRightTex; }

	//Fisheye settings
	inline void bindFisheyeShader() { mFisheyeShader.bind(); }
	inline int getFisheyeShaderMVPLoc() { return FisheyeMVP; }
	inline int getFisheyeShaderCubemapLoc() { return Cubemap; }
	inline int getFisheyeShaderHalfFOVLoc() { return FishEyeHalfFov; }
	inline int getFisheyeShaderOffsetLoc() { return FisheyeOffset; }
	inline float getFisheyeOffset(unsigned int axis) { return mFisheyeBaseOffset[axis] + mFisheyeOffset[axis]; }
	//! Set to true if alpha should be used in fisheye rendering
	inline bool useFisheyeAlpha() { return mFisheyeAlpha; }

	void setFisheyeRendering(bool state);
	void setCubeMapResolution(int res);
	void setDomeDiameter(float size);
	void setFisheyeAlpha(bool state);
	void setFisheyeTilt(float angle);
	void setFisheyeFOV(float angle);
	void setFisheyeCropValues(float left, float right, float bottom, float top);
	void setFisheyeOffset(float x, float y, float z = 0.0f);
	void setFisheyeBaseOffset(float x, float y, float z = 0.0f);
	void setFisheyeOverlay(std::string filename);
	bool isUsingFisheyeRendering();
	int getCubeMapResolution();
	float getDomeDiameter();
	float getFisheyeTilt();
	float getFisheyeFOV();
	float getFisheyeCropValue(FisheyeCropSide side);
	bool isFisheyeOffaxis();
	const char * getFisheyeOverlay();

	/*!
		\returns the current screenshot number (file index)
	*/
	int getScreenShotNumber() { return mShotCounter; }
	/*!
		Set the screenshot number (exising images will be replaced)
		\param mShotCounter is the frame number which will be added to the filename of the screenshot
	*/
	void setScreenShotNumber(int number) {  mShotCounter = number; }

private:
	static void windowResizeCallback( GLFWwindow * window, int width, int height );
	void initScreenCapture();
	void deleteAllViewports();
	void createTextures();
	void createFBOs();
	void resizeFBOs();
	void createVBOs();
	void loadShaders();
	void initFisheye();

public:
	sgct_core::OffScreenBuffer * mFinalFBO_Ptr;
	sgct_core::OffScreenBuffer * mCubeMapFBO_Ptr;

private:
	bool mUseFixResolution;
	bool mUseSwapGroups;
	bool mBarrier;
	bool mSwapGroupMaster;
	bool mUseQuadBuffer;
	bool mFullScreen;
	bool mSetWindowPos;
	bool mDecorated;
	int mFramebufferResolution[2];
	int mWindowRes[2];
	int mWindowPos[2];
	int mWindowResOld[2];
	int mMonitorIndex;
	GLFWmonitor * mMonitor;
	GLFWwindow * mWindowHandle;
	GLFWwindow * mSharedHandle;
	float mAspectRatio;

	//FBO stuff
	unsigned int mFrameBufferTextures[4];

	sgct_core::ScreenCapture * mScreenCapture;

	StereoMode mStereoMode;
	int mSwapInterval;
	bool mFisheyeMode;
	int mNumberOfAASamples;
	int mId;
	int mShotCounter;

	float mFisheyeQuadVerts[20];
	float mQuadVerts[20];

	//VBO:s
	unsigned int mVBO[NUMBER_OF_VBOS];
	//VAO:s
	unsigned int mVAO[NUMBER_OF_VBOS];

	//Shaders
	sgct::ShaderProgram mFisheyeShader;
	int FisheyeMVP, Cubemap, FishEyeHalfFov, FisheyeOffset;
	sgct::ShaderProgram mStereoShader;
	int StereoMVP, StereoLeftTex, StereoRightTex;

	//Fisheye
	std::string mFisheyeOverlayFilename;
	bool mFisheyeAlpha;
	bool mFisheyeOffaxis;
	float mFisheyeTilt;
	float mFieldOfView;
	float mFisheyeOffset[3];
	float mFisheyeBaseOffset[3]; //set from the config
	float mCropFactors[4];
	float mCubeMapSize;
	int mCubeMapResolution;

	std::size_t mCurrentViewportIndex;
	std::vector<Viewport> mViewports;
};
}

#endif
