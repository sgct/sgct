/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CUBEMAP__H__
#define __SGCT__CUBEMAP__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/math.h>
#include <sgct/vulkan/buffer.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

#ifdef SGCT_HAS_NDI
#include <Processing.NDI.Lib.h>
#include <Processing.NDI.Send.h>
#include <Processing.NDI.Structs.h>
#endif // SGCT_HAS_NDI

#ifdef SGCT_HAS_SPOUT
struct SPOUTLIBRARY;
using SPOUTHANDLE = SPOUTLIBRARY*;
#endif // SGCT_HAS_SPOUT

namespace sgct {

namespace config { struct CubemapProjection; }
namespace vulkan { class ProjectionPass; }
class BaseViewport;
class User;
class Window;

/**
 * This class renders the six faces of a cube map side by side, in two rows of three.
 */
class SGCT_EXPORT CubemapProjection final : public NonLinearProjection {
public:
    CubemapProjection(const config::CubemapProjection& config,
        const Window& parent, User& user);
    ~CubemapProjection() override;

    /**
     * Update projection when aspect ratio changes for the viewport.
     */
    void update(const vec2& size) const override;

    /**
     * Render the faces of the cube map into the viewport.
     */
    void render(VkCommandBuffer cmd, const BaseViewport& viewport,
        FrustumMode frustumMode, const VkRect2D& area) const override;

    void setSpoutRigOrientation(vec3 orientation);

#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
    /// Sends the pixels of the previous frame's cube map faces to Spout/NDI
    void sendStreamFrames() override;
#endif // defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)

private:
    void initViewports() override;
    void initPipelines(VkRenderPass renderPass) override;

    /// Which of the faces are rendered
    std::array<bool, 6> _isFaceEnabled;

    vec3 _rigOrientation = vec3{ 0.f, 0.f, 0.f };

    std::unique_ptr<vulkan::ProjectionPass> _pass;

#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
    // Records the readback copy of a face for the Spout/NDI streaming; the base class
    // calls it after the render pass of each face of #renderCubemap
    void recordFaceCopy(VkCommandBuffer cmd, int face, FrustumMode frustumMode,
        VkFence fence) override;

    // Allocates the readback ring and creates the Spout/NDI senders; called lazily the
    // first time a face is copied, once the cube map resolution and format are known
    void initializeStream();

    struct FaceStream {
        // Readback ring: one buffer per slot, one slot deeper than the frames in flight
        std::array<std::unique_ptr<vulkan::Buffer>, 3> buffers;
        std::array<VkFence, 3> fences = {};
        std::array<bool, 3> hasData = {};
#ifdef SGCT_HAS_SPOUT
        SPOUTHANDLE spout = nullptr;
#endif // SGCT_HAS_SPOUT
#ifdef SGCT_HAS_NDI
        NDIlib_send_instance_t ndi = nullptr;
        NDIlib_video_frame_v2_t ndiFrame;
#endif // SGCT_HAS_NDI
    };
    std::array<FaceStream, 6> _faceStreams;
    // Monotonically increasing counter that indexes the ring
    uint64_t _streamFrame = 0;
    // The format of the readback pixels; it determines the NDI FourCC and whether the
    // Spout sender needs a BGRA to RGBA conversion
    VkFormat _streamFormat = VK_FORMAT_UNDEFINED;
    uint32_t _streamResolution = 0;
    bool _streamInitialized = false;
    bool _spoutEnabled = false;
    std::string _spoutName;
    bool _ndiEnabled = false;
    std::string _ndiName;
    std::string _ndiGroups;
    // Temporary RGBA pixels for the Spout senders, used when the cube map is BGRA
    std::vector<uint8_t> _streamRgba;
#endif // defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
};

} // namespace sgct

#endif // __SGCT__CUBEMAP__H__
