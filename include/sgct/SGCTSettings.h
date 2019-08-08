/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__SETTINGS__H__
#define __SGCT__SETTINGS__H__

#include <mutex>
#include <stdio.h>
#include <string>
#ifndef SGCT_DONT_USE_EXTERNAL
    #include <external/tinyxml2.h>
#else
    #include <tinyxml2.h>
#endif

namespace sgct {

/*!
    This singleton class will hold global SGCT settings.
*/
class SGCTSettings {
public:
    enum CapturePathIndex {
        Mono = 0,
        LeftStereo,
        RightStereo
    };

    enum DrawBufferType {
        Diffuse = 0,
        Diffuse_Normal,
        Diffuse_Position,
        Diffuse_Normal_Position
    };
    enum BufferFloatPrecision {
        Float_16Bit = 0,
        Float_32Bit
    };

    /*! Get the SGCTSettings instance */
    static SGCTSettings* instance();

    /*! Destroy the SGCTSettings instance */
    static void destroy();

    void configure(tinyxml2::XMLElement* element);

    // ----------- set functions ---------------- //
    void setSwapInterval(int val);
    void setRefreshRateHint(int freq);
    void setUseDepthTexture(bool state);
    void setUseNormalTexture(bool state);
    void setUsePositionTexture(bool state);
    void setBufferFloatPrecision(BufferFloatPrecision bfp);
    void setUseFBO(bool state);
    void setNumberOfCaptureThreads(int count);
    void setPNGCompressionLevel(int level);
    void setJPEGQuality(int quality);
    void setCapturePath(std::string path, CapturePathIndex cpi = Mono);
    void appendCapturePath(std::string str, CapturePathIndex cpi = Mono);
    void setCaptureFormat(const char* format);
    void setCaptureFromBackBuffer(bool state);
    void setExportWarpingMeshes(bool state);
    void setFXAASubPixTrim(float val);
    void setFXAASubPixOffset(float val);
    void setOSDTextXOffset(float val);
    void setOSDTextYOffset(float val);
    void setOSDTextFontSize( unsigned int size );
    void setOSDTextFontName(std::string name);
    void setOSDTextFontPath(std::string path);
    void setDefaultNumberOfAASamples(int samples);
    void setDefaultFXAAState(bool state);
    void setForceGlTexImage2D(bool state);
    void setUsePBO(bool state);
    void setUseRLE(bool state);
    void setUseWarping(bool state);
    void setShowWarpingWireframe(bool state);
    void setTryMaintainAspectRatio(bool state);
    
    // ----------- get functions ---------------- //
    const char *        getCapturePath(CapturePathIndex cpi = Mono) const;
    int getSwapInterval() const;
    int getRefreshRateHint() const;
    unsigned int getOSDTextFontSize() const;
    const std::string& getOSDTextFontName() const;
    const std::string& getOSDTextFontPath() const;
    int getBufferFloatPrecisionAsGLint() const;
    int getDefaultNumberOfAASamples() const;
    bool getDefaultFXAAState() const;
    bool getForceGlTexImage2D() const;
    bool getUsePBO() const;
    bool getUseWarping() const;
    bool getShowWarpingWireframe() const;
    bool getCaptureFromBackBuffer() const;
    bool getTryMaintainAspectRatio() const;
    bool getExportWarpingMeshes() const;

    // -- mutex protected get functions ---------- //
    bool getUseRLE();
    int getCaptureFormat();
    int getPNGCompressionLevel();
    int getJPEGQuality();

    // ----------- inline functions ---------------- //
    //! Return true if depth buffer is rendered to texture
    bool useDepthTexture();
    //! Return true if normals are rendered to texture
    bool useNormalTexture();
    //! Return true if positions are rendered to texture
    bool usePositionTexture();
    //! Returns true if FBOs are used
    bool useFBO();
    //! Get the number of capture threads (for screenshot recording)
    int getNumberOfCaptureThreads();
    //! The relative On-Screen-Display text x-offset in range [0, 1]
    float getOSDTextXOffset();
    //! The relative On-Screen-Display text y-offset in range [0, 1]
    float getOSDTextYOffset();
    /*! \returns the FXAA removal of sub-pixel aliasing */
    float getFXAASubPixTrim();
    /*! \returns the FXAA sub-pixel offset */
    float getFXAASubPixOffset();
    /*! \returns the current drawBufferType */
    DrawBufferType getCurrentDrawBufferType();

private:
    SGCTSettings();

    void updateDrawBufferFlag();

private:
    static SGCTSettings* mInstance;

    int mCaptureFormat;
    int mSwapInterval = 1;
    int mRefreshRate = 0;
    int mNumberOfCaptureThreads = std::thread::hardware_concurrency();
    int mPNGCompressionLevel = 1;
    int mJPEGQuality = 100;
    int mDefaultNumberOfAASamples = 1;
    
    bool mUseDepthTexture = false;
    bool mUseNormalTexture = false;
    bool mUsePositionTexture = false;
    bool mUseFBO = true;
    bool mDefaultFXAA = false;
    bool mForceGlTexImage2D = false;
    bool mUsePBO = true;
    bool mUseRLE = false;
    bool mUseWarping = true;
    bool mShowWarpingWireframe = false;
    bool mCaptureBackBuffer = false;
    bool mTryMaintainAspectRatio = true;
    bool mExportWarpingMeshes = false;

    float mOSDTextOffset[2] = { 0.05f, 0.05f };
    float mFXAASubPixTrim = 1.f / 4.f;
    float mFXAASubPixOffset = 1.f / 2.f;

    std::string mCapturePath[3] = { "SGCT", "SGCT", "SGCT" };

    //fontdata
#ifdef WIN32
    std::string mFontName = "verdanab.ttf";
#elif __APPLE__
    std::string mFontName = "Tahoma Bold.ttf";
#else
    std::string mFontName = "FreeSansBold.ttf";

#endif
    std::string mFontPath;
    unsigned int mFontSize = 10;

    DrawBufferType mCurrentDrawBuffer = Diffuse;
    BufferFloatPrecision mCurrentBufferFloatPrecision = Float_16Bit;

    //mutex
    std::mutex mMutex;
};

} // namespace sgct

#endif // __SGCT__SETTINGS__H__
