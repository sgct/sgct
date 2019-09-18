/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__SETTINGS__H__
#define __SGCT__SETTINGS__H__

#include <glm/glm.hpp>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace tinyxml2 { class XMLElement; }

namespace sgct {

/**
 * This singleton class will hold global SGCT settings.
 */
class Settings {
public:
    enum class CapturePath {
        Mono = 0,
        LeftStereo,
        RightStereo
    };

    enum class CaptureFormat {
        PNG,
        TGA,
        JPG
    };

    enum class DrawBufferType {
        Diffuse = 0,
        DiffuseNormal,
        DiffusePosition,
        DiffuseNormalPosition
    };
    enum class BufferFloatPrecision {
        Float_16Bit = 0,
        Float_32Bit
    };

    /// Get the Settings instance
    static Settings* instance();

    /// Destroy the Settings instance
    static void destroy();

    void configure(tinyxml2::XMLElement* element);

    /**
     * Set swap interval for all windows
     *   -1 = adaptive sync (Nvidia)
     *    0  = vertical sync off
     *    1  = wait for vertical sync
     *    2  = fix when using swapgroups in xp and running half the framerate
     */
    void setSwapInterval(int val);

    /**
     * Set the refreshrate hint of the window in fullscreen mode.
     * If it's not listed in your monitor's video-mode list than it will not be used.

     * \param freq the refresh frequency/rate
     */
    void setRefreshRateHint(int freq);

    /// Set to true if depth buffer textures should be allocated and used.
    void setUseDepthTexture(bool state);

    /// Set to true if normal textures should be allocated and used.
    void setUseNormalTexture(bool state);

    /// Set to true if position buffer textures should be allocated and used.
    void setUsePositionTexture(bool state);

    /**
     * Set the float precision of the float buffers (normal and position buffer).
     * \param bfp is the float precition that will be used in next buffer resize or
                  creation
     */
    void setBufferFloatPrecision(BufferFloatPrecision bfp);

    /// Set the FBO mode. This is done internally using SGCT config file.
    void setUseFBO(bool state);

    /// Set the number of capture threads used by SGCT (multi-threaded screenshots)
    void setNumberOfCaptureThreads(int count);

    /**
     * Set the zlib compression level used for saving png files
     * Compression levels 1-9.
     *   -1 = Default compression\n
     *    0 = No compression\n
     *    1 = Best speed\n
     *    9 = Best compression\n
     */
    void setPNGCompressionLevel(int level);

    /// Set the JPEG quality in range [0-100].
    void setJPEGQuality(int quality);

    /**
     * Set capture/screenshot path used by SGCT
     *
     * \param path the path including filename without suffix
     * \param cpi index to which path to set (Mono = default, Left or Right)
     */
    void setCapturePath(std::string path, CapturePath cpi = CapturePath::Mono);

    /**
     * Set the capture format which can be one of the following:
     *   - PNG
     *   - TGA
     */
    void setCaptureFormat(CaptureFormat format);

    /**
     * Set if capture should capture warped from backbuffer instead of texture. Backbuffer
     * data includes masks and warping.
     */
    void setCaptureFromBackBuffer(bool state);

    /// Set to true if warping meshes should be exported as OBJ files.
    void setExportWarpingMeshes(bool state);
    /**
     * Controls removal of sub-pixel aliasing.
     *   1/2 - low removal
     *   1/3 - medium removal
     *   1/4 - default removal
     *   1/8 - high removal
     *     0 - complete removal
     */
    void setFXAASubPixTrim(float val);

    /**
     * Set the pixel offset for contrast/edge detection. Values should be in the range
     * [1.0f/8.0f, 1.0f]. Default is 0.5f.
     */
    void setFXAASubPixOffset(float val);

    /// Set the OSD text Offset between 0.0 and 1.0
    void setOSDTextOffset(glm::vec2 val);
    
    /// Set the OSD text font size
    void setOSDTextFontSize(unsigned int size);

    /// Set the OSD text font name
    void setOSDTextFontName(std::string name);

    /// Set the OSD text font path
    void setOSDTextFontPath(std::string path);

    /// Set the default number of AA samples (MSAA) for all windows
    void setDefaultNumberOfAASamples(int samples);

    /// Set the default FXAA state for all windows (enabled or disabled)
    void setDefaultFXAAState(bool state);

    /**
     * Set the glTexImage2D (legacy) should be used instead of glTexStorage2D (modern).
     * For example gDebugger can't display textures created using glTexStorage2D.
     */
    void setForceGlTexImage2D(bool state);
    void setUsePBO(bool state);

    /// Set if run length encoding (RLE) should be used in PNG and TGA export.
    void setUseRLE(bool state);

    /// Set if screen warping should be used or not
    void setUseWarping(bool state);

    /// Set if warping mesh wireframe should be rendered
    void setShowWarpingWireframe(bool state);

    /**
     * Set if geometry should try to adapt after framebuffer dimensions. This is valid for
     * multi-viewport renderings like fisheye projections.
     */
    void setTryMaintainAspectRatio(bool state);
    
    /**
     * Get the capture/screenshot path
     *
     * \param cpi index to which path to get (Mono = default, Left or Right)
     */
    const std::string& getCapturePath(CapturePath i = CapturePath::Mono) const;

    /**
     * Get swap interval for all windows
     *   -1 = adaptive sync (Nvidia)
     *    0  = vertical sync off
     *    1  = wait for vertical sync
     *    2  = fix when using swapgroups in xp and running half the framerate
     */
    int getSwapInterval() const;

    /// Get the refreshrate hint of the window in fullscreen mode.
    int getRefreshRateHint() const;
    
    /// Get the OSD text font size
    unsigned int getOSDTextFontSize() const;

    /// Get the OSD text font name
    const std::string& getOSDTextFontName() const;

    /// Get the OSD text font path
    const std::string& getOSDTextFontPath() const;

    /// Get the precision of the float buffers as an GLint (GL_RGB16F or GL_RGB32F)
    int getBufferFloatPrecisionAsGLint() const;

    /// Get the default MSAA setting
    int getDefaultNumberOfAASamples() const;

    /// Get the FXAA default state
    bool getDefaultFXAAState() const;

    /**
     * Get if glTexImage2D(legacy) should be used instead of glTexStorage2D(modern).
     * For example gDebugger can't display textures created using glTexStorage2D
     */
    bool getForceGlTexImage2D() const;

    /// Get if pixel buffer object transferes should be used
    bool getUsePBO() const;

    /// Get if screen warping is used
    bool getUseWarping() const;

    /// Get if warping wireframe mesh should be rendered
    bool getShowWarpingWireframe() const;

    /**
     * Get if capture should use backbuffer data or texture. Backbuffer data includes
     * masks and warping.
     */
    bool getCaptureFromBackBuffer() const;
    
    /// Get if aspect ratio is taken into acount when generation some display geometries.
    bool getTryMaintainAspectRatio() const;
    
    /// Get if warping meshes should be exported as obj-files.
    bool getExportWarpingMeshes() const;

    /// Get if run length encoding (RLE) is used in PNG and TGA export.
    bool getUseRLE();

    /**
     * Get the capture/screenshot path
     *
     * \return the captureformat if set, otherwise -1 is returned
     */
    CaptureFormat getCaptureFormat();

    /// Get the zlib compression level used in png export.
    int getPNGCompressionLevel();

    /// Get the JPEG quality settings (0-100)
    int getJPEGQuality();

    /// Return true if depth buffer is rendered to texture
    bool useDepthTexture() const;

    /// Return true if normals are rendered to texture
    bool useNormalTexture() const;

    /// Return true if positions are rendered to texture
    bool usePositionTexture() const;

    /// Returns true if FBOs are used
    bool useFBO() const;

    /// Get the number of capture threads (for screenshot recording)
    int getNumberOfCaptureThreads() const;

    /// The relative On-Screen-Display text offset in range [0, 1]
    glm::vec2 getOSDTextOffset() const;

    /// \returns the FXAA removal of sub-pixel aliasing
    float getFXAASubPixTrim() const;

    /// \returns the FXAA sub-pixel offset
    float getFXAASubPixOffset() const;

    /// \returns the current drawBufferType
    DrawBufferType getCurrentDrawBufferType() const;

private:
    Settings() = default;

    static Settings* mInstance;

    std::atomic<CaptureFormat> mCaptureFormat = CaptureFormat::PNG;
    int mSwapInterval = 1;
    int mRefreshRate = 0;
    int mNumberOfCaptureThreads = std::thread::hardware_concurrency();
    std::atomic_int mPNGCompressionLevel = 1;
    std::atomic_int mJPEGQuality = 100;
    int mDefaultNumberOfAASamples = 1;
    
    bool mUseDepthTexture = false;
    bool mUseNormalTexture = false;
    bool mUsePositionTexture = false;
    bool mUseFBO = true;
    bool mDefaultFXAA = false;
    bool mForceGlTexImage2D = false;
    bool mUsePBO = true;
    std::atomic_bool mUseRLE = false;
    bool mUseWarping = true;
    bool mShowWarpingWireframe = false;
    bool mCaptureBackBuffer = false;
    bool mTryMaintainAspectRatio = true;
    bool mExportWarpingMeshes = false;

    glm::vec2 mOSDTextOffset = glm::vec2(0.05f, 0.05f);
    float mFXAASubPixTrim = 1.f / 4.f;
    float mFXAASubPixOffset = 1.f / 2.f;

    struct {
        std::string mono = "SGCT";
        std::string left = "SGCT";
        std::string right = "SGCT";
    } mCapturePath;

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

    BufferFloatPrecision mCurrentBufferFloatPrecision = BufferFloatPrecision::Float_16Bit;
};

} // namespace sgct

#endif // __SGCT__SETTINGS__H__
