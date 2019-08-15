/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__WINDOW__H__
#define __SGCT__WINDOW__H__

#include <sgct/OffScreenBuffer.h>
#include <sgct/PostFX.h>
#include <sgct/ScreenCapture.h>
#include <sgct/Viewport.h>
#include <glm/glm.hpp>
#include <vector>

#define NUMBER_OF_TEXTURES 8

struct GLFWmonitor;
struct GLFWwindow;

namespace sgct_core {
    class BaseViewport;
    class ScreenCapture;
} // namespace sgct_core

namespace sgct {

/*!
Helper class for window data. 
*/
class SGCTWindow {
public:    
    /*!
        Different stereo modes used for rendering
    */
    enum class StereoMode {
        NoStereo = 0,
        Active,
        AnaglyphRedCyan,
        AnaglyphAmberBlue,
        AnaglyphRedCyanWimmer,
        Checkerboard,
        CheckerboardInverted,
        VerticalInterlaced,
        VerticalInterlacedInverted,
        Dummy,
        SideBySide,
        SideBySideInverted,
        TopBottom,
        TopBottomInverted
    };

    enum class Context { Shared = 0, Window, Unset };
    
    enum class ColorBitDepth {
        Depth8,
        Depth16,
        Depth16Float,
        Depth32Float,
        Depth16Int,
        Depth32Int,
        Depth16UInt,
        Depth32UInt
    };

    explicit SGCTWindow(int id);

    void close();
    void init();
    void initOGL();
    void initContextSpecificOGL();
    static void initNvidiaSwapGroups();
    void initWindowResolution(int x, int y);
    void swap(bool takeScreenshot);
    void updateResolutions();
    bool update();
    bool openWindow(GLFWwindow* share, size_t lastWindowIdx);
    void makeOpenGLContextCurrent(Context context);
    static void restoreSharedContext();
    static void resetSwapGroupFrameNumber();

    // ------------- set functions ----------------- //
    void setName(std::string name);
    void setTags(std::string tags);
    void setVisibility(bool state);
    void setRenderWhileHidden(bool state);
    void setFocused(bool state);
    void setIconified(bool state);
    void setWindowTitle(const char* title);
    void setWindowResolution(int x, int y);
    void setFramebufferResolution(int x, int y);
    void setWindowPosition(int x, int y);
    void setWindowMode(bool fullscreen);
    void setFloating(bool floating);
    void setDoubleBuffered(bool doubleBuffered);
    void setWindowDecoration(bool state);
    void setFullScreenMonitorIndex(int index);
    static void setBarrier(bool state);
    void setFixResolution(bool state);
    void setHorizFieldOfView(float hFovDeg);
    void setUsePostFX(bool state);
    void setUseFXAA(bool state);
    void setUseQuadbuffer(bool state);
    void setCallDraw2DFunction(bool state);
    void setCallDraw3DFunction(bool state);
    void setCopyPreviousWindowToCurrentWindow(bool state);
    void setNumberOfAASamples(int samples);
    void setStereoMode(StereoMode sm);
    void setCurrentViewport(size_t index);
    void setCurrentViewport(sgct_core::BaseViewport* vp);
    void setAlpha(bool state);
    void setGamma(float gamma);
    void setContrast(float contrast);
    void setBrightness(float brightness);
    void setColorBitDepth(ColorBitDepth cbd);
    void setPreferBGR(bool state);
    void setAllowCapture(bool state);

    // -------------- is functions --------------- //
    bool isFullScreen() const;
    bool isFloating() const;
    bool isDoubleBuffered() const;
    bool isFocused() const;
    bool isIconified() const;
    bool isVisible() const;
    bool isRenderingWhileHidden() const;
    bool isFixResolution() const;
    bool isWindowResolutionSet() const;

    bool isStereo() const;
    bool isWindowResized() const;
    static bool isBarrierActive();
    static bool isUsingSwapGroups();
    static bool isSwapGroupMaster();
    bool isBGRPrefered() const;
    bool isCapturingAllowed() const;
        
    // -------------- get functions ----------------- //
    const std::string& getName() const;
    const std::vector<std::string>& getTags() const;
    bool checkIfTagExists(const std::string& tag) const;
    int getId() const;
    unsigned int getFrameBufferTexture(unsigned int index);
    sgct_core::ScreenCapture* getScreenCapturePointer(unsigned int eye) const;
    int getNumberOfAASamples() const;
    StereoMode getStereoMode() const;
    static unsigned int getSwapGroupFrameNumber();
    glm::ivec2 getFinalFBODimensions() const;
    sgct_core::OffScreenBuffer* getFBOPtr() const;
    GLFWmonitor* getMonitor() const;
    GLFWwindow* getWindowHandle() const;
    sgct_core::BaseViewport* getCurrentViewport() const;
    sgct_core::Viewport& getViewport(size_t index);
    const sgct_core::Viewport& getViewport(size_t index) const;
    void getCurrentViewportPixelCoords(int& x, int& y, int& xSize, int& ySize) const;
    size_t getNumberOfViewports() const;
    std::string getStereoModeStr() const;
    bool getAlpha() const;
    float getGamma() const;
    float getContrast() const;
    float getBrightness() const;
    ColorBitDepth getColorBitDepth() const;
    float getHorizFieldOfViewDegrees();
    
    // ------------------ Inline functions ----------------------- //
    /*!
        \returns the pointer to a specific post effect
    */
    sgct::PostFX& getPostFX(size_t index);
    /*!
        \returns the number of post effects
    */
    size_t getNumberOfPostFXs() const;

    /*!
        \returns Get the window resolution.
    */
    glm::ivec2 getResolution() const;

    /*!
        \returns Get the frame buffer resolution.
    */
    glm::ivec2 getFramebufferResolution() const;

    /*!
        \returns Get the initial window resolution.
    */
    glm::ivec2 getInitialResolution() const;

    /*!
     \returns Get the scale value (relation between pixel and point size). Normally this
              value is 1.0f but 2.0f on retina computers.
     */
    glm::vec2 getScale() const;

    //! \returns the aspect ratio of the window 
    float getAspectRatio() const;

    /*!
    \returns Get the frame buffer bytes per color component (BPCC) count.
    */
    int getFramebufferBPCC() const;

    // -------------- bind functions -------------------//
    void bindVAO() const;
    void bindVBO() const;
    void unbindVBO() const;
    void unbindVAO() const;

    //------------- Other ------------------------- //
    void addPostFX(PostFX& fx);
    void addViewport(std::unique_ptr<sgct_core::Viewport> vpPtr);

    /*! \return true if any masks are used */
    bool hasAnyMasks() const;
    /*! \returns true if FXAA should be used */
    bool useFXAA() const;
    /*! \returns true if PostFX pass should be used */
    bool usePostFX() const;

    void bindStereoShaderProgram() const;
    int getStereoShaderMVPLoc() const;
    int getStereoShaderLeftTexLoc() const;
    int getStereoShaderRightTexLoc() const;

    bool getCallDraw2DFunction() const;
    bool getCallDraw3DFunction() const;
    bool getCopyPreviousWindowToCurrentWindow() const;

private:
    enum TextureType { ColorTexture = 0, DepthTexture, NormalTexture, PositionTexture };

    static void windowResizeCallback(GLFWwindow* window, int width, int height);
    static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
    static void windowFocusCallback(GLFWwindow* window, int state);
    static void windowIconifyCallback(GLFWwindow* window, int state);
    void initScreenCapture();
    void deleteAllViewports();
    void createTextures();
    void generateTexture(unsigned int id, glm::ivec2 size, TextureType type,
        bool interpolate);
    void createFBOs();
    void resizeFBOs();
    void createVBOs();
    void loadShaders();
    void updateTransferCurve();
    void updateColorBufferData();
    bool useRightEyeTexture() const;

    std::string mName;
    std::vector<std::string> mTags;

    bool mVisible = true;
    bool mRenderWhileHidden = false;
    bool mFocused = false;
    bool mIconified = false;
    bool mUseFixResolution = false;
    bool mIsWindowResSet = false;
    bool mAllowCapture = true;
    static bool mUseSwapGroups;
    static bool mBarrier;
    static bool mSwapGroupMaster;
    bool mCallDraw2DFunction = true;
    bool mCallDraw3DFunction = true;
    bool mCopyPreviousWindowToCurrentWindow = false;
    bool mUseQuadBuffer = false;
    bool mFullScreen = false;
    bool mFloating = false;
    bool mDoubleBuffered = true;
    bool mSetWindowPos = false;
    bool mDecorated = true;
    bool mAlpha = false;
    glm::ivec2 mFramebufferResolution = glm::ivec2(512, 256);
    glm::ivec2 mWindowInitialRes = glm::ivec2(640, 480);
    bool mHasPendingWindowRes = false;
    glm::ivec2 mPendingWindowRes = glm::ivec2(0, 0);
    bool mHasPendingFramebufferRes = false;
    glm::ivec2 mPendingFramebufferRes = glm::ivec2(0, 0);
    glm::ivec2 mWindowRes = glm::ivec2(640, 480);
    glm::ivec2 mWindowPos = glm::ivec2(0, 0);
    glm::ivec2 mWindowResOld = glm::ivec2(640, 480);
    int mMonitorIndex = 0;
    GLFWmonitor* mMonitor = nullptr;
    GLFWwindow* mWindowHandle = nullptr;
    static GLFWwindow* mSharedHandle;
    static GLFWwindow* mCurrentContextOwner;
    float mAspectRatio = 1.f;
    float mGamma = 1.f;
    float mContrast = 1.f;
    float mBrightness = 1.f;
    glm::vec2 mScale = glm::vec2(0.f, 0.f);
    float mHorizontalFovDegrees = 90.f;

    bool mUseFXAA;
    bool mUsePostFX = false;

    ColorBitDepth mBufferColorBitDepth = ColorBitDepth::Depth8;
    int mInternalColorFormat;
    unsigned int mColorFormat;
    unsigned int mColorDataType;
    bool mPreferBGR = true;
    int mBytesPerColor;

    //FBO stuff
    unsigned int mFrameBufferTextures[NUMBER_OF_TEXTURES];

    std::unique_ptr<sgct_core::ScreenCapture> mScreenCaptureLeftOrMono;
    std::unique_ptr<sgct_core::ScreenCapture> mScreenCaptureRight;

    StereoMode mStereoMode = StereoMode::NoStereo;
    int mNumberOfAASamples;
    int mId;

    unsigned int mVBO = 0;
    unsigned int mVAO = 0;

    //Shaders
    ShaderProgram mStereoShader;
    int StereoMVP = -1;
    int StereoLeftTex = -1;
    int StereoRightTex = -1;

    bool mHasAnyMasks = false;

    sgct_core::BaseViewport* mCurrentViewport = nullptr;
    std::vector<std::unique_ptr<sgct_core::Viewport>> mViewports;
    std::vector<sgct::PostFX> mPostFXPasses;
    std::unique_ptr<sgct_core::OffScreenBuffer> mFinalFBO;
};

} // namespace sgct

#endif // __SGCT__WINDOW__H__
