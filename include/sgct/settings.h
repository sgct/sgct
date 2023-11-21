/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SETTINGS__H__
#define __SGCT__SETTINGS__H__

#include <sgct/sgctexports.h>
#include <algorithm>
#include <optional>
#include <string>
#include <thread>

namespace sgct {

namespace config {
    struct Capture;
    struct Settings;
} // namespace config

/**
 * This singleton class will hold global SGCT settings.
 */
class SGCT_EXPORT Settings {
public:
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
     * \param freq The refresh frequency/rate
     */
    void setRefreshRateHint(int freq);

    /**
     * Set to true if depth buffer textures should be allocated and used.
     */
    void setUseDepthTexture(bool state);

    /**
     * Set to true if normal textures should be allocated and used.
     */
    void setUseNormalTexture(bool state);

    /**
     * Set to true if position buffer textures should be allocated and used.
     */
    void setUsePositionTexture(bool state);

    /**
     * Set the float precision of the float buffers (normal and position buffer).
     *
     * \param bfp The float precition that will be used in next resize or creation
     */
    void setBufferFloatPrecision(BufferFloatPrecision bfp);

    /**
     * Set the number of capture threads used by SGCT (multi-threaded screenshots).
     */
    void setNumberOfCaptureThreads(int count);

    /**
     * Set capture/screenshot path used by SGCT.
     *
     * \param path The path including filename without suffix
     */
    void setCapturePath(std::string path);

    /**
     * Set the screenshot capture format.
     */
    void setCaptureFormat(CaptureFormat format);

    /**
     * Sets the prefix to be used for all screenshots.
     */
    void setScreenshotPrefix(std::string prefix);

    /**
     * Set if capture should capture warped from backbuffer instead of texture. Backbuffer
     * data includes masks and warping.
     */
    void setCaptureFromBackBuffer(bool state);

    /**
     * Set to true if warping meshes should be exported as OBJ files.
     */
    void setExportWarpingMeshes(bool state);

    /**
     * If set to true, the node name is added to screenshots.
     */
    void setAddNodeNameToScreenshot(bool state);

    /**
     * If set to true, the window name is added to screenshots.
     */
    void setAddWindowNameToScreenshot(bool state);

    /**
     * Get the capture/screenshot path.
     */
    const std::string& capturePath() const;

    /**
     * Get swap interval for all windows.
     *   -1 = adaptive sync (Nvidia)
     *    0 = vertical sync off
     *    1 = wait for vertical sync
     *    2 = fix when using swapgroups in xp and running half the framerate
     */
    int swapInterval() const;

    /**
     * Get the refreshrate hint of the window in fullscreen mode.
     */
    int refreshRateHint() const;

    /**
     * Get the precision of the float buffers as an `GLint` (`GL_RGB16F` or `GL_RGB32F`)
     */
    unsigned int bufferFloatPrecision() const;

    /**
     * Get if capture should use backbuffer data or texture. Backbuffer data includes
     * masks and warping.
     */
    bool captureFromBackBuffer() const;

    /**
     * Get if warping meshes should be exported as obj-files.
     */
    bool exportWarpingMeshes() const;

    /**
     * Get the capture/screenshot path.
     *
     * \return The captureformat if set, otherwise -1 is returned
     */
    CaptureFormat captureFormat() const;

    /**
     * \return `true` if depth buffer is rendered to texture
     */
    bool useDepthTexture() const;

    /**
     * \return `true` if normals are rendered to texture
     */
    bool useNormalTexture() const;

    /**
     * \return `true` if positions are rendered to texture
     */
    bool usePositionTexture() const;

    /**
     * \return The number of capture threads (for screenshot recording)
     */
    int numberCaptureThreads() const;

    /**
     * \return Should screenshots contain the node name
     */
    bool addNodeNameToScreenshot() const;

    /**
     * \return Whether screenshots should contain the window name
     */
    bool addWindowNameToScreenshot() const;

    /**
     * \return The prefix that is used for all screenshots
     */
    const std::string& prefixScreenshot() const;

    /**
     * \return `true` if the screenshots written out should be limited based on the begin
     *         and end ranges
     */
    bool hasScreenshotLimit() const;

    /**
     * The index of the first screenshot that will actually be rendered. If this value is
     * set, all previous screenshots will be ignored, but the counter will be increased
     * either way
     */
    uint64_t screenshotLimitBegin() const;

    /**
     * The index of the last screenshot that will not be rendered anymore. If this value
     * is set, all screenshots starting with this index will be ignored.
     */
    uint64_t screenshotLimitEnd() const;

    /**
     * \return The drawBufferType
     */
    DrawBufferType drawBufferType() const;

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
    bool _exportWarpingMeshes = false;

    struct Capture {
        std::string capturePath;
        std::string prefix;
        bool addNodeName = false;
        bool addWindowName = true;

        struct Limits {
            uint64_t begin;
            uint64_t end;
        };
        std::optional<Limits> limits;

    };
    Capture _screenshot;

    BufferFloatPrecision _bufferFloatPrecision = BufferFloatPrecision::Float32Bit;
};

} // namespace sgct

#endif // __SGCT__SETTINGS__H__
