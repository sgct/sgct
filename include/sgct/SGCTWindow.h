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
#include "PostFX.h"
#include <vector>

#define NUMBER_OF_VBOS 2
#define NUMBER_OF_TEXTURES 10

namespace sgct
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
	enum StereoMode { No_Stereo = 0, Active_Stereo, Anaglyph_Red_Cyan_Stereo, Anaglyph_Amber_Blue_Stereo, Anaglyph_Red_Cyan_Wimmer_Stereo,
		Checkerboard_Stereo, Checkerboard_Inverted_Stereo, Vertical_Interlaced_Stereo, Vertical_Interlaced_Inverted_Stereo, Dummy_Stereo,
		Side_By_Side_Stereo, Side_By_Side_Inverted_Stereo, Top_Bottom_Stereo, Top_Bottom_Inverted_Stereo, Number_Of_Stereo_Items };

	enum VBOIndex { RenderQuad = 0, FishEyeQuad };
	enum FisheyeCropSide { CropLeft = 0, CropRight, CropBottom, CropTop };
	enum OGL_Context { Shared_Context = 0, Window_Context };

public:
	SGCTWindow(int id);
	void close();
	void init(int id);
	void initOGL();
	void initContextSpecificOGL();
	static void initNvidiaSwapGroups();
	void initWindowResolution(const int x, const int y);
	void swap();
	void update();
	void captureBuffer();
	bool openWindow(GLFWwindow* share);
	void makeOpenGLContextCurrent( OGL_Context context );
	static void resetSwapGroupFrameNumber();

	// ------------- set functions ----------------- //
	void setName(const std::string & name);
	void setVisibility(bool state);
	void setFocused(bool state);
	void setIconified(bool state);
	void setWindowTitle(const char * title);
	void setWindowResolution(const int x, const int y);
	void setFramebufferResolution(const int x, const int y);
	void setWindowPosition(const int x, const int y);
	void setWindowMode(bool fullscreen);
	void setWindowDecoration(bool state);
	void setFullResolutionMode(bool state);
	void setFullScreenMonitorIndex( int index );
	static void setBarrier(const bool state);
	void setFixResolution(const bool state);
	void setUsePostFX(bool state);
	void setUseFXAA(bool state);
	static void setUseSwapGroups(const bool state);
	void setUseQuadbuffer(const bool state);
	void setNumberOfAASamples(int samples);
	void setStereoMode( StereoMode sm );
	void setScreenShotNumber(int number);
	void setCurrentViewport(std::size_t index);

	// -------------- is functions --------------- //
	bool				isFullScreen();
	bool				isFocused();
	bool				isIconified();
	bool				isWindowResized();
	bool				isVisible();
	bool				isFixResolution();
	bool				isStereo();
	bool				isUsingFisheyeRendering();
	static inline bool	isBarrierActive() { return mBarrier; }
	static inline bool	isUsingSwapGroups() { return mUseSwapGroups; }
	static inline bool	isSwapGroupMaster() { return mSwapGroupMaster; }
		
	// -------------- get functions ----------------- //
	std::string						getName();
	int								getId();
	unsigned int					getFrameBufferTexture(unsigned int index);
	sgct_core::ScreenCapture *		getScreenCapturePointer();
	int								getNumberOfAASamples();
	int								getScreenShotNumber();
	StereoMode						getStereoMode();
	bool							getFullResolutionMode();
	static void						getSwapGroupFrameNumber(unsigned int & frameNumber);
	void							getDrawFBODimensions( int & width, int & height );
	void							getFinalFBODimensions( int & width, int & height );
	sgct_core::OffScreenBuffer *	getFBOPtr();
	GLFWmonitor *					getMonitor();
	GLFWwindow *					getWindowHandle();
	sgct_core::Viewport *			getCurrentViewport();
	sgct_core::Viewport *			getViewport(std::size_t index);
	void							getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize);
	std::size_t						getNumberOfViewports();
	std::string						getStereoModeStr();
	
    // ------------------ Inline functions ----------------------- //
	/*!
		\returns the index of the current viewport
	*/
	inline std::size_t		getCurrentViewportIndex() { return mCurrentViewportIndex; }
	/*!
		\returns the pointer to a specific post effect
	*/
	inline sgct::PostFX *	getPostFXPtr( std::size_t index ) {  return &mPostFXPasses[ index ]; }
	/*!
		\returns the number of post effects
	*/
    inline std::size_t		getNumberOfPostFXs() { return mPostFXPasses.size(); }

	/*!
		\returns Get the horizontal window resolution.
	*/
	inline int				getXResolution() { return mWindowRes[0]; }
	/*!
		\returns Get the vertical window resolution.
	*/
	inline int				getYResolution() { return mWindowRes[1]; }
	/*!
		\returns Get the horizontal frame buffer resolution.
	*/
	inline int				getXFramebufferResolution() { return mFramebufferResolution[0]; }
	/*!
		\returns Get the vertical frame buffer resolution.
	*/
	inline int				getYFramebufferResolution() { return mFramebufferResolution[1]; }
	/*!
		\returns Get the initial horizontal window resolution.
	*/
	inline int				getXInitialResolution() { return mWindowInitialRes[0]; }
	/*!
		\returns Get the initial vertical window resolution.
	*/
	inline int				getYInitialResolution() { return mWindowInitialRes[1]; }

	//! \returns the aspect ratio of the window 
	inline float			getAspectRatio() { return mAspectRatio; }

	// -------------- bind functions -------------------//
	void bindVAO();
	void bindVAO( VBOIndex index );
	void bindVBO();
	void bindVBO( VBOIndex index );
	void unbindVBO();
	void unbindVAO();

	//------------- Other ------------------------- //
	void addPostFX( sgct::PostFX & fx );
	void addViewport(float left, float right, float bottom, float top);
	void addViewport(sgct_core::Viewport &vp);
	void generateCubeMapViewports();

	/*! \return true if any masks are used */
	inline bool hasAnyMasks() { return mHasAnyMasks; }
	/*! \returns true if FXAA should be used */
	inline bool useFXAA() { return mUseFXAA; }
	/*! \returns true if PostFX pass should be used */
	inline bool usePostFX() { return mUsePostFX; }

	inline void bindStereoShaderProgram() { mStereoShader.bind(); }
	inline int getStereoShaderMVPLoc() { return StereoMVP; }
	inline int getStereoShaderLeftTexLoc() { return StereoLeftTex; }
	inline int getStereoShaderRightTexLoc() { return StereoRightTex; }

	//Fisheye settings
	inline void bindFisheyeShaderProgram() { mFisheyeShader.bind(); }
	inline int getFisheyeShaderMVPLoc() { return FisheyeMVP; }
	inline int getFisheyeShaderCubemapLoc() { return Cubemap; }
	inline int getFisheyeShaderCubemapDepthLoc() { return DepthCubemap; }
	inline int getFisheyeShaderCubemapNormalsLoc() { return NormalCubemap; }
	inline int getFisheyeShaderHalfFOVLoc() { return FishEyeHalfFov; }
	inline int getFisheyeBGColorLoc() { return FishEyeBGColor; }
	inline int getFisheyeShaderOffsetLoc() { return FisheyeOffset; }

	inline void bindFisheyeDepthCorrectionShaderProgram() { mFisheyeDepthCorrectionShader.bind(); }
	inline int getFisheyeSwapShaderMVPLoc() { return FishEyeSwapMVP; }
	inline int getFisheyeSwapShaderColorLoc() { return FishEyeSwapColor; }
	inline int getFisheyeSwapShaderDepthLoc() { return FishEyeSwapDepth; }
	inline int getFisheyeSwapShaderNearLoc() { return FishEyeSwapNear; }
	inline int getFisheyeSwapShaderFarLoc() { return FishEyeSwapFar; }
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
	int getCubeMapResolution();
	float getDomeDiameter();
	float getFisheyeTilt();
	float getFisheyeFOV();
	float getFisheyeCropValue(FisheyeCropSide side);
	bool isFisheyeOffaxis();
	const char * getFisheyeOverlay();

private:
	static void windowResizeCallback( GLFWwindow * window, int width, int height );
	static void windowFocusCallback( GLFWwindow * window, int state );
	static void windowIconifyCallback( GLFWwindow * window, int state );
	void initScreenCapture();
	void deleteAllViewports();
	void createTextures();
	void generateTexture(unsigned int id, int xSize, int ySize, bool anisotropicFiltering, bool depth, bool interpolate);
	void generateCubeMap(unsigned int id, bool depth);
	void createFBOs();
	void resizeFBOs();
	void createVBOs();
	void loadShaders();
	void initFisheye();

public:
	sgct_core::OffScreenBuffer * mFinalFBO_Ptr;
	sgct_core::OffScreenBuffer * mCubeMapFBO_Ptr;

private:
	std::string mName;

	bool mVisible;
	bool mFocused;
	bool mIconified;
	bool mUseFixResolution;
	static bool mUseSwapGroups;
	static bool mBarrier;
	static bool mSwapGroupMaster;
	bool mUseQuadBuffer;
	bool mFullScreen;
	bool mSetWindowPos;
	bool mDecorated;
	bool mFullRes; //for mac retina screens and similar
	int mFramebufferResolution[2];
	int mWindowInitialRes[2];
	int mWindowRes[2];
	int mWindowPos[2];
	int mWindowResOld[2];
	int mMonitorIndex;
	GLFWmonitor * mMonitor;
	GLFWwindow * mWindowHandle;
	GLFWwindow * mSharedHandle;
	float mAspectRatio;

	bool mUseFXAA;
	bool mUsePostFX;

	//FBO stuff
	unsigned int mFrameBufferTextures[NUMBER_OF_TEXTURES];

	sgct_core::ScreenCapture * mScreenCapture;

	StereoMode mStereoMode;
	bool mFisheyeMode; //if fisheye rendering is used
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
	sgct::ShaderProgram mFisheyeShader, mFisheyeDepthCorrectionShader;
	int FisheyeMVP, Cubemap, DepthCubemap, NormalCubemap, FishEyeHalfFov, FishEyeBGColor, FisheyeOffset, FishEyeSwapMVP, FishEyeSwapColor, FishEyeSwapDepth, FishEyeSwapNear, FishEyeSwapFar;
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
	bool mAreCubeMapViewPortsGenerated;
	bool mHasAnyMasks;
	int mCubeMapResolution;

	std::size_t mCurrentViewportIndex;
	std::vector<sgct_core::Viewport> mViewports;
	std::vector<sgct::PostFX> mPostFXPasses;
};
}

#endif
