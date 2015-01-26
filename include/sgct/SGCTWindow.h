/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
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
#define NUMBER_OF_TEXTURES 14

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
	enum OGL_Context { Shared_Context = 0, Window_Context, Unset_Context };
	enum TextureType { ColorTexture = 0, DepthTexture, NormalTexture, PositionTexture};

public:
	SGCTWindow(int id);
	void close();
	void init();
	void initOGL();
	void initContextSpecificOGL();
	static void initNvidiaSwapGroups();
	void initWindowResolution(const int x, const int y);
	void swap(bool takeScreenshot);
	void update();
	bool openWindow(GLFWwindow* share);
	void makeOpenGLContextCurrent( OGL_Context context );
    static void restoreSharedContext();
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
	void setFloating(bool floating);
	void setDoubleBuffered(bool doubleBuffered);
	void setWindowDecoration(bool state);
	void setFullResolutionMode(bool state);
	void setFullScreenMonitorIndex( int index );
	static void setBarrier(const bool state);
	void setFixResolution(const bool state);
	void setUsePostFX(bool state);
	void setUseFXAA(bool state);
	void setUseQuadbuffer(const bool state);
	void setNumberOfAASamples(int samples);
	void setStereoMode( StereoMode sm );
	void setCurrentViewport(std::size_t index);
	void setAlpha(bool state);
	void setGamma(float gamma);
	void setContrast(float contrast);
	void setBrightness(float brightness);

	// -------------- is functions --------------- //
	const bool &		isFullScreen() const;
	const bool &		isFloating() const;
	const bool &		isDoubleBuffered() const;
	const bool &		isFocused() const;
	const bool &		isIconified() const;
	const bool &		isVisible() const;
	const bool &		isFixResolution() const;
	const bool &		isUsingFisheyeRendering() const;
	bool				isStereo() const;
	bool				isWindowResized() const;
	static inline bool	isBarrierActive() { return mBarrier; }
	static inline bool	isUsingSwapGroups() { return mUseSwapGroups; }
	static inline bool	isSwapGroupMaster() { return mSwapGroupMaster; }
		
	// -------------- get functions ----------------- //
	const std::string &				getName() const;
	const int &						getId() const;
	unsigned int					getFrameBufferTexture(unsigned int index);
	sgct_core::ScreenCapture *		getScreenCapturePointer(unsigned int eye) const;
	const int &						getNumberOfAASamples() const;
	const StereoMode &				getStereoMode() const;
	const bool &					getFullResolutionMode() const;
	static void						getSwapGroupFrameNumber(unsigned int & frameNumber);
	void							getDrawFBODimensions(int & width, int & height) const;
	void							getFinalFBODimensions(int & width, int & height) const;
	sgct_core::OffScreenBuffer *	getFBOPtr() const;
	GLFWmonitor *					getMonitor() const;
	GLFWwindow *					getWindowHandle() const;
	sgct_core::Viewport *			getCurrentViewport() const;
	sgct_core::Viewport *			getViewport(std::size_t index) const;
	void							getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize) const;
	std::size_t						getNumberOfViewports() const;
	std::string						getStereoModeStr() const;
	const bool &					getAlpha() const;
	const float &					getGamma() const;
	const float &					getContrast() const;
	const float &					getBrightness() const;
	
    // ------------------ Inline functions ----------------------- //
	/*!
		\returns the index of the current viewport
	*/
	inline const std::size_t & getCurrentViewportIndex() const { return mCurrentViewportIndex; }
	/*!
		\returns the pointer to a specific post effect
	*/
	inline sgct::PostFX * getPostFXPtr(std::size_t index) { return &mPostFXPasses[index]; }
	/*!
		\returns the number of post effects
	*/
	inline std::size_t getNumberOfPostFXs() const { return mPostFXPasses.size(); }

	/*!
		\returns Get the horizontal window resolution.
	*/
	inline const int & getXResolution() const { return mWindowRes[0]; }
	/*!
		\returns Get the vertical window resolution.
	*/
	inline const int & getYResolution() const { return mWindowRes[1]; }
	/*!
		\returns Get the horizontal frame buffer resolution.
	*/
	inline const int & getXFramebufferResolution() const { return mFramebufferResolution[0]; }
	/*!
		\returns Get the vertical frame buffer resolution.
	*/
	inline const int & getYFramebufferResolution() const { return mFramebufferResolution[1]; }
	/*!
		\returns Get the initial horizontal window resolution.
	*/
	inline const int & getXInitialResolution() const { return mWindowInitialRes[0]; }
	/*!
		\returns Get the initial vertical window resolution.
	*/
	inline const int & getYInitialResolution() const { return mWindowInitialRes[1]; }

	//! \returns the aspect ratio of the window 
	inline const float & getAspectRatio() const { return mAspectRatio; }

	// -------------- bind functions -------------------//
	void bindVAO() const;
	void bindVAO(VBOIndex index) const;
	void bindVBO() const;
	void bindVBO(VBOIndex index) const;
	void unbindVBO() const;
	void unbindVAO() const;

	//------------- Other ------------------------- //
	void addPostFX( sgct::PostFX & fx );
	void addViewport(float left, float right, float bottom, float top);
	void addViewport(sgct_core::Viewport * vpPtr);
	void generateCubeMapViewports();

	/*! \return true if any masks are used */
	inline const bool & hasAnyMasks() const { return mHasAnyMasks; }
	/*! \returns true if FXAA should be used */
	inline const bool & useFXAA() const { return mUseFXAA; }
	/*! \returns true if PostFX pass should be used */
	inline const bool & usePostFX() const { return mUsePostFX; }

	inline void bindStereoShaderProgram() const { mStereoShader.bind(); }
	inline const int & getStereoShaderMVPLoc() const { return StereoMVP; }
	inline const int & getStereoShaderLeftTexLoc() const { return StereoLeftTex; }
	inline const int & getStereoShaderRightTexLoc() const { return StereoRightTex; }

	//Fisheye settings
	inline void bindFisheyeShaderProgram() const { mFisheyeShader.bind(); }
	inline const int & getFisheyeShaderCubemapLoc() const { return Cubemap; }
	inline const int & getFisheyeShaderCubemapDepthLoc() const { return DepthCubemap; }
	inline const int & getFisheyeShaderCubemapNormalsLoc() const { return NormalCubemap; }
	inline const int & getFisheyeShaderCubemapPositionsLoc() const { return PositionCubemap; }
	inline const int & getFisheyeShaderHalfFOVLoc() const { return FishEyeHalfFov; }
	inline const int & getFisheyeShaderOffsetLoc() const { return FisheyeOffset; }

	inline void bindFisheyeDepthCorrectionShaderProgram() const { mFisheyeDepthCorrectionShader.bind(); }
	inline const int & getFisheyeSwapShaderColorLoc() const { return FishEyeSwapColor; }
	inline const int & getFisheyeSwapShaderDepthLoc() const { return FishEyeSwapDepth; }
	inline const int & getFisheyeSwapShaderNearLoc() const { return FishEyeSwapNear; }
	inline const int & getFisheyeSwapShaderFarLoc() const { return FishEyeSwapFar; }
	inline float getFisheyeOffset(unsigned int axis) const { return mFisheyeBaseOffset[axis] + mFisheyeOffset[axis]; }

	void setFisheyeRendering(bool state);
	void setCubeMapResolution(int res);
	void setDomeDiameter(float size);
	void setFisheyeTilt(float angle);
	void setFisheyeFOV(float angle);
	void setFisheyeCropValues(float left, float right, float bottom, float top);
	void setFisheyeOffset(float x, float y, float z = 0.0f);
	void setFisheyeBaseOffset(float x, float y, float z = 0.0f);
	void setFisheyeOverlay(std::string filename);
    void setFisheyeMask(std::string filename);
	void setFisheyeUseCubicInterpolation(bool state);

	const int & getCubeMapResolution() const;
	const float & getDomeDiameter() const;
	const float & getFisheyeTilt() const;
	const float & getFisheyeFOV() const;
	const float & getFisheyeCropValue(FisheyeCropSide side) const;
	const bool & getFisheyeUseCubicInterpolation() const;
	const bool & isFisheyeOffaxis() const;
	const char * getFisheyeOverlay() const;
	const char * getFisheyeMask() const;

private:
	static void windowResizeCallback( GLFWwindow * window, int width, int height );
	static void windowFocusCallback( GLFWwindow * window, int state );
	static void windowIconifyCallback( GLFWwindow * window, int state );
	void initScreenCapture();
	void deleteAllViewports();
	void createTextures();
	void generateTexture(unsigned int id, int xSize, int ySize, TextureType type, bool interpolate);
	void generateCubeMap(unsigned int id, TextureType type);
	void createFBOs();
	void resizeFBOs();
	void createVBOs();
	void loadShaders();
	void initFisheye();
	void updateTransferCurve();

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
	bool mFloating;
	bool mDoubleBuffered;
	bool mSetWindowPos;
	bool mDecorated;
	bool mFullRes; //for mac retina screens and similar
	bool mAlpha;
	int mFramebufferResolution[2];
	int mWindowInitialRes[2];
	int mWindowRes[2];
	int mWindowPos[2];
	int mWindowResOld[2];
	int mMonitorIndex;
	GLFWmonitor * mMonitor;
	GLFWwindow * mWindowHandle;
	static GLFWwindow * mSharedHandle;
    static GLFWwindow * mCurrentContextOwner;
	float mAspectRatio;
	float mGamma;
	float mContrast;
	float mBrightness;

	bool mUseFXAA;
	bool mUsePostFX;

	//FBO stuff
	unsigned int mFrameBufferTextures[NUMBER_OF_TEXTURES];

	sgct_core::ScreenCapture * mScreenCapture[2];

	StereoMode mStereoMode;
	bool mFisheyeMode; //if fisheye rendering is used
	int mNumberOfAASamples;
	int mId;

	float mFisheyeQuadVerts[20];
	float mQuadVerts[20];

	//VBO:s
	unsigned int mVBO[NUMBER_OF_VBOS];
	//VAO:s
	unsigned int mVAO[NUMBER_OF_VBOS];

	//Shaders
	sgct::ShaderProgram mFisheyeShader, mFisheyeDepthCorrectionShader;
	int Cubemap, DepthCubemap, NormalCubemap, PositionCubemap, FishEyeHalfFov, FisheyeOffset, FishEyeSwapColor, FishEyeSwapDepth, FishEyeSwapNear, FishEyeSwapFar;
	sgct::ShaderProgram mStereoShader;
	int StereoMVP, StereoLeftTex, StereoRightTex;

	//Fisheye
	std::string mFisheyeOverlayFilename;
    std::string mFisheyeMaskFilename;
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
	bool mCubicInterpolation;
	bool mUseRightEyeTexture;

	std::size_t mCurrentViewportIndex;
	std::vector<sgct_core::Viewport *> mViewports;
	std::vector<sgct::PostFX> mPostFXPasses;
};
}

#endif
