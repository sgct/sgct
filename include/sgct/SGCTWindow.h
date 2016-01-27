/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
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

#define NUMBER_OF_TEXTURES 8

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

	enum OGL_Context { Shared_Context = 0, Window_Context, Unset_Context };
	enum ColorBitDepth { BufferColorBitDepth8, BufferColorBitDepth16, BufferColorBitDepth16Float, BufferColorBitDepth32Float, BufferColorBitDepth16Int, BufferColorBitDepth32Int, BufferColorBitDepth16UInt, BufferColorBitDepth32UInt };

public:
	SGCTWindow(int id);
	void close();
	void init();
	void initOGL();
	void initContextSpecificOGL();
	static void initNvidiaSwapGroups();
	void initWindowResolution(const int x, const int y);
	void swap(bool takeScreenshot);
	bool update();
	bool openWindow(GLFWwindow* share);
	void makeOpenGLContextCurrent( OGL_Context context );
    static void restoreSharedContext();
	static void resetSwapGroupFrameNumber();

	// ------------- set functions ----------------- //
	void setName(const std::string & name);
	void setVisibility(bool state);
	void setRenderWhileHidden(bool state);
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
	void setFullScreenMonitorIndex( int index );
	static void setBarrier(const bool state);
	void setFixResolution(const bool state);
	void setUsePostFX(bool state);
	void setUseFXAA(bool state);
	void setUseQuadbuffer(const bool state);
	void setNumberOfAASamples(int samples);
	void setStereoMode( StereoMode sm );
	void setCurrentViewport(std::size_t index);
	void setCurrentViewport(sgct_core::BaseViewport * vp);
	void setAlpha(bool state);
	void setGamma(float gamma);
	void setContrast(float contrast);
	void setBrightness(float brightness);
	void setColorBitDepth(ColorBitDepth cbd);
	void setPreferBGR(bool state);
	void setAllowCapture(bool state);

	// -------------- is functions --------------- //
	const bool &		isFullScreen() const;
	const bool &		isFloating() const;
	const bool &		isDoubleBuffered() const;
	const bool &		isFocused() const;
	const bool &		isIconified() const;
	const bool &		isVisible() const;
	const bool &		isRenderingWhileHidden() const;
	const bool &		isFixResolution() const;
	bool				isStereo() const;
	bool				isWindowResized() const;
	static inline bool	isBarrierActive() { return mBarrier; }
	static inline bool	isUsingSwapGroups() { return mUseSwapGroups; }
	static inline bool	isSwapGroupMaster() { return mSwapGroupMaster; }
	bool				isBGRPrefered() const;
	bool				isCapturingAllowed() const;
		
	// -------------- get functions ----------------- //
	const std::string &				getName() const;
	const int &						getId() const;
	unsigned int					getFrameBufferTexture(unsigned int index);
	sgct_core::ScreenCapture *		getScreenCapturePointer(unsigned int eye) const;
	const int &						getNumberOfAASamples() const;
	const StereoMode &				getStereoMode() const;
	static void						getSwapGroupFrameNumber(unsigned int & frameNumber);
	void							getFinalFBODimensions(int & width, int & height) const;
	sgct_core::OffScreenBuffer *	getFBOPtr() const;
	GLFWmonitor *					getMonitor() const;
	GLFWwindow *					getWindowHandle() const;
	sgct_core::BaseViewport *		getCurrentViewport() const;
	sgct_core::Viewport *			getViewport(std::size_t index) const;
	void							getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize) const;
	std::size_t						getNumberOfViewports() const;
	std::string						getStereoModeStr() const;
	const bool &					getAlpha() const;
	const float &					getGamma() const;
	const float &					getContrast() const;
	const float &					getBrightness() const;
	ColorBitDepth					getColorBitDepth() const;
	
    // ------------------ Inline functions ----------------------- //
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
    /*!
     \returns Get the horizontal scale value (relation between pixel and point size). Normally this value is 1.0f but 2.0f on retina computers.
     */
    inline const float & getXScale() const { return mScale[0]; }
    /*!
     \returns Get the vertical scale value (relation between pixel and point size). Normally this value is 1.0f but 2.0f on retina computers.
     */
    inline const float & getYScale() const { return mScale[1]; }

	//! \returns the aspect ratio of the window 
	inline const float & getAspectRatio() const { return mAspectRatio; }

	/*!
	\returns Get the frame buffer bytes per color component (BPCC) count.
	*/
	inline const int & getFramebufferBPCC() const { return mBytesPerColor; }

	// -------------- bind functions -------------------//
	void bindVAO() const;
	void bindVBO() const;
	void unbindVBO() const;
	void unbindVAO() const;

	//------------- Other ------------------------- //
	void addPostFX( sgct::PostFX & fx );
	void addViewport(float left, float right, float bottom, float top);
	void addViewport(sgct_core::Viewport * vpPtr);

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

private:
	enum TextureType { ColorTexture = 0, DepthTexture, NormalTexture, PositionTexture };

	static void windowResizeCallback( GLFWwindow * window, int width, int height );
    static void frameBufferResizeCallback( GLFWwindow * window, int width, int height );
	static void windowFocusCallback( GLFWwindow * window, int state );
	static void windowIconifyCallback( GLFWwindow * window, int state );
	void initScreenCapture();
	void deleteAllViewports();
	void createTextures();
	void generateTexture(unsigned int id, const int xSize, const int ySize, const TextureType type, const bool interpolate);
	void createFBOs();
	void resizeFBOs();
	void createVBOs();
	void loadShaders();
	void updateTransferCurve();
	void updateColorBufferData();

public:
	sgct_core::OffScreenBuffer * mFinalFBO_Ptr;

private:
	std::string mName;

	bool mVisible;
	bool mRenderWhileHidden;
	bool mFocused;
	bool mIconified;
	bool mUseFixResolution;
	bool mAllowCapture;
	static bool mUseSwapGroups;
	static bool mBarrier;
	static bool mSwapGroupMaster;
	bool mUseQuadBuffer;
	bool mFullScreen;
	bool mFloating;
	bool mDoubleBuffered;
	bool mSetWindowPos;
	bool mDecorated;
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
    float mScale[2];

	bool mUseFXAA;
	bool mUsePostFX;

	ColorBitDepth mBufferColorBitDepth;
	int mInternalColorFormat;
	unsigned int mColorFormat;
	unsigned int mColorDataType;
	bool mPreferBGR;
	int mBytesPerColor;

	//FBO stuff
	unsigned int mFrameBufferTextures[NUMBER_OF_TEXTURES];

	sgct_core::ScreenCapture * mScreenCapture[2];

	StereoMode mStereoMode;
	int mNumberOfAASamples;
	int mId;

	float mQuadVerts[20];

	//VBO:s
	unsigned int mVBO;
	//VAO:s
	unsigned int mVAO;

	//Shaders
	sgct::ShaderProgram mStereoShader;
	int StereoMVP, StereoLeftTex, StereoRightTex;

	bool mUseRightEyeTexture;
	bool mHasAnyMasks;

	sgct_core::BaseViewport * mCurrentViewport;
	std::vector<sgct_core::Viewport *> mViewports;
	std::vector<sgct::PostFX> mPostFXPasses;
};
}

#endif
