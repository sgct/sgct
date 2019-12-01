/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__SETTINGS__H__
#define __SGCT__SETTINGS__H__

#include <sgct/ogl_headers.h>
#include <glm/glm.hpp>
#include <string>
#include <thread>

namespace sgct {

namespace config {
    struct Capture;
    struct Settings;
} // namespace config

/// This singleton class will hold global SGCT settings.
class Settings {
public:
    enum class CapturePath { Mono, LeftStereo, RightStereo };
    enum class CaptureFormat { PNG, TGA, JPG };

    enum class DrawBufferType {
        Diffuse,
        DiffuseNormal,
        DiffusePosition,
        DiffuseNormalPosition
    };
    enum class BufferFloatPrecision { Float16Bit, Float32Bit };

    static Settings& instance();
    static void destroy();

    void applySettings(const config::Settings& settings);
    void applyCapture(const config::Capture& capture);

    /**
     * Set swap interval for all windows
     *   -1 = adaptive sync (Nvidia)
     *    0 = vertical sync off
     *    1 = wait for vertical sync
     *    2 = fix when using swapgroups in xp and running half the framerate
     */
    void setSwapInterval(int val);

    /**
     * Set the refreshrate hint of the window in fullscreen mode. If it's not listed in
     * your monitor's video-mode list than it will not be used.
     *
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
     * \param bfp is the float precition that will be used in next resize or creation
     */
    void setBufferFloatPrecision(BufferFloatPrecision bfp);

    /// Set the number of capture threads used by SGCT (multi-threaded screenshots)
    void setNumberOfCaptureThreads(int count);

    /**
     * Set capture/screenshot path used by SGCT.
     *
     * \param path the path including filename without suffix
     * \param cpi index to which path to set (Mono = default, Left or Right)
     */
    void setCapturePath(std::string path, CapturePath cpi = CapturePath::Mono);

    /// Set the screenshot capture format.
    void setCaptureFormat(CaptureFormat format);

    /**
     * Set if capture should capture warped from backbuffer instead of texture. Backbuffer
     * data includes masks and warping.
     */
    void setCaptureFromBackBuffer(bool state);

    /// Set to true if warping meshes should be exported as OBJ files.
    void setExportWarpingMeshes(bool state);

    /**
     * Set if geometry should try to adapt after framebuffer dimensions. This is valid for
     * multi-viewport renderings like fisheye projections.
     */
    void setTryKeepAspectRatio(bool state);
    
    /**
     * Get the capture/screenshot path.
     *
     * \param cpi index to which path to get (Mono = default, Left or Right)
     */
    const std::string& getCapturePath(CapturePath i = CapturePath::Mono) const;

    /**
     * Get swap interval for all windows
     *   -1 = adaptive sync (Nvidia)
     *    0 = vertical sync off
     *    1 = wait for vertical sync
     *    2 = fix when using swapgroups in xp and running half the framerate
     */
    int getSwapInterval() const;

    /// Get the refreshrate hint of the window in fullscreen mode.
    int getRefreshRateHint() const;
    
    /// Get the precision of the float buffers as an GLint (GL_RGB16F or GL_RGB32F)
    GLenum getBufferFloatPrecision() const;

    /**
     * Get if capture should use backbuffer data or texture. Backbuffer data includes
     * masks and warping.
     */
    bool getCaptureFromBackBuffer() const;
    
    /// Get if aspect ratio is taken into acount when generation some display geometries.
    bool getTryKeepAspectRatio() const;
    
    /// Get if warping meshes should be exported as obj-files.
    bool getExportWarpingMeshes() const;

    /**
     * Get the capture/screenshot path
     *
     * \return the captureformat if set, otherwise -1 is returned
     */
    CaptureFormat getCaptureFormat() const;

    /// Return true if depth buffer is rendered to texture
    bool useDepthTexture() const;

    /// Return true if normals are rendered to texture
    bool useNormalTexture() const;

    /// Return true if positions are rendered to texture
    bool usePositionTexture() const;

    /// Get the number of capture threads (for screenshot recording)
    int getNumberOfCaptureThreads() const;

    /// \return the drawBufferType
    DrawBufferType getDrawBufferType() const;

private:
    Settings() = default;

    static Settings* _instance;

    CaptureFormat _captureFormat = CaptureFormat::PNG;
    int _swapInterval = 1;
    int _refreshRate = 0;
    int _nCaptureThreads = std::max(std::thread::hardware_concurrency() - 1, 0u);
    
    bool _useDepthTexture = false;
    bool _useNormalTexture = false;
    bool _usePositionTexture = false;
    bool _captureBackBuffer = false;
    bool _tryKeepAspectRatio = true;
    bool _exportWarpingMeshes = false;

    struct {
        std::string mono = "SGCT";
        std::string left = "SGCT";
        std::string right = "SGCT";
    } _capturePath;

    BufferFloatPrecision _bufferFloatPrecision = BufferFloatPrecision::Float16Bit;
};

} // namespace sgct

#endif // __SGCT__SETTINGS__H__
