/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__NONLINEARPROJECTION__H__
#define __SGCT__NONLINEARPROJECTION__H__

#include <sgct/sgctexports.h>

#include <sgct/baseviewport.h>
#include <sgct/definitions.h>
#include <sgct/vulkan/projectionpass.h>
#include <sgct/vulkan/sceneformat.h>
#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace sgct {

namespace vulkan { class CubemapTarget; }
class Window;

/**
 * Base class for non-linear projections. A non-linear projection renders the scene into
 * the faces of a cube map, one sub-viewport per face, and then maps the cube map onto
 * its viewport in the window's offscreen target.
 */
class SGCT_EXPORT NonLinearProjection {
public:
    enum class InterpolationMode : uint8_t { Linear, Cubic };

    explicit NonLinearProjection(const Window& parent);

    virtual ~NonLinearProjection();

    /**
     * Initializes the non-linear projection, or reinitializes it if it was initialized
     * before. The GPU must not be using the resources of a previous initialization.
     *
     * \param format The format of the parent window's render target, which the cube maps
     *        use as well, so that the pipelines of the draw callback can be used to
     *        render the cube map faces
     */
    void initialize(const vulkan::SceneFormat& format);

    /**
     * Records the rendering of the enabled cube map faces for the provided frustum mode,
     * which calls the draw callback once per face. Must be called outside of a render
     * pass. The right eye has a separate cube map, so that the cube maps of both eyes can
     * be rendered before the projection of either of them is rendered.
     *
     * \param cmd The command buffer of the frame
     * \param frustumMode The eye to render the cube map for
     * \param frameIndex The frame-in-flight slot of the frame, passed to the callback
     * \param imageIndex The swapchain image of the frame, passed to the callback
     * \param fence The fence that is signaled when the frame's command buffer completes,
     *        which the Spout/NDI streaming waits on before reading the cube map faces
     */
    void renderCubemap(VkCommandBuffer cmd, FrustumMode frustumMode, uint32_t frameIndex,
        uint32_t imageIndex, VkFence fence);

    /**
     * Records the rendering of the projection into a viewport. Must be called inside the
     * scene render pass of the window's offscreen target, after the cube map of
     * \p frustumMode was rendered with #renderCubemap, and with the viewport (y-flipped)
     * and scissor of \p viewport set as the dynamic state of \p cmd.
     *
     * \param cmd The command buffer of the frame
     * \param viewport The viewport that the projection belongs to
     * \param frustumMode The eye that is rendered
     * \param area The scissor rectangle of the viewport
     */
    virtual void render(VkCommandBuffer cmd, const BaseViewport& viewport,
        FrustumMode frustumMode, const VkRect2D& area) const = 0;

    virtual void update(const vec2& size) const = 0;

    virtual void updateFrustums(FrustumMode mode, float nearClip, float farClip);

    /**
     * Sends the pixels of the previous frame's cube map faces to the Spout and NDI
     * senders, if they are enabled. The window calls this after the frame's command
     * buffer has been submitted, so the copies of the previous frame have finished. The
     * base class does nothing.
     */
    virtual void sendStreamFrames() {}

    /**
     * Set the resolution of the cubemap faces.
     *
     * \param resolution The pixel resolution of each quad
     */
    void setCubemapResolution(int resolution);

    /**
     * Set the interpolation mode.
     *
     * \param im The selected mode
     */
    void setInterpolationMode(InterpolationMode im);

    /**
     * Set if stereoscopic rendering will be enabled.
     */
    void setStereo(bool state);

    virtual void setUser(User& user);

    /**
     * \return The resolution of the cubemap
     */
    ivec2 cubemapResolution() const;

    /**
     * \return The format that the projection was initialized with
     */
    const vulkan::SceneFormat& sceneFormat() const;

protected:
    virtual void initViewports() = 0;

    /**
     * Creates the pipelines that render the projection into the window's offscreen
     * target. \p renderPass is a scene render pass, which is compatible with the one of
     * the offscreen target.
     */
    virtual void initPipelines(VkRenderPass renderPass) = 0;

    /**
     * \return The cube map of the provided eye
     */
    const vulkan::CubemapTarget& cubemap(FrustumMode frustumMode) const;

    /**
     * Records the clearing of \p area of the offscreen target to the clear color and the
     * far depth (and the normals and positions to 0).
     */
    void clearViewport(VkCommandBuffer cmd, const VkRect2D& area) const;

    /**
     * \return The beginning of the fragment shaders: the version, the defines
     *         `HAS_NORMALS`, `HAS_POSITIONS`, and `HAS_DEPTH` (0 or 1) for the attachments
     *         of the scene format, and the outputs `out_normal` (location 1) and
     *         `out_position` (location 2) if there are such attachments. The shaders
     *         write all attachments, including gl_FragDepth if `HAS_DEPTH` is 1
     */
    std::string fragmentShaderHeader() const;

    /**
     * \return The settings of a projection pass for the scene format, which only lack
     *         the shaders and push constants. The textures are bound to 0 (color),
     *         1 (normals), 2 (positions), and 3 (depth), see #cubeTextures
     */
    vulkan::ProjectionPass::Settings projectionPassSettings() const;

    /// How the cube map is sampled
    enum class TextureView { Cube, Array };

    /**
     * \return The textures of the cube map of \p frustumMode for the bindings 0 to 3. The
     *         image views of the attachments that the format does not have are
     *         VK_NULL_HANDLE
     */
    std::array<VkDescriptorImageInfo, 4> cubeTextures(FrustumMode frustumMode,
        TextureView view) const;

    /**
     * \return The textures of a single face of the cube map of \p frustumMode for the
     *         bindings 0 to 3
     */
    std::array<VkDescriptorImageInfo, 4> faceTextures(FrustumMode frustumMode,
        int face) const;

    /**
     * \return The sub-viewports in the order of the cube map faces
     */
    std::array<const BaseViewport*, 6> faceViewports() const;

    /**
     * Called by #renderCubemap after the render pass of each face, with the cube map
     * image of \p frustumMode in the layout that #endFace left it in. The base class
     * does nothing; the cubemap projection records the readback copies of the faces for
     * the Spout/NDI streaming.
     *
     * \param cmd The command buffer of the frame
     * \param face The index of the face in [0, 6)
     * \param frustumMode The eye that the face was rendered for
     * \param fence The fence that is signaled when the frame's command buffer completes
     */
#ifdef _MSC_VER
    // The default body does not reference the parameters (they are named for the
    // documentation), which MSVC reports as C4100
#pragma warning(push)
#pragma warning(disable : 4100)
#endif
    virtual void recordFaceCopy(VkCommandBuffer cmd, int face, FrustumMode frustumMode,
        VkFence fence) {}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    struct {
        BaseViewport right;
        BaseViewport left;
        BaseViewport bottom;
        BaseViewport top;
        BaseViewport front;
        BaseViewport back;
    } _subViewports;

    InterpolationMode _interpolationMode = InterpolationMode::Linear;
    FrustumMode _preferedMonoFrustumMode = FrustumMode::Mono;

    bool _isStereo = false;

    ivec2 _cubemapResolution = ivec2(512, 512);
    vec4 _clearColor = vec4(0.3f, 0.3f, 0.3f, 1.f);

private:
    vulkan::SceneFormat _format;

    /// The cube maps of the mono or left eye (0) and the right eye (1). The cube map of
    /// the right eye is created when it is first rendered
    std::array<std::unique_ptr<vulkan::CubemapTarget>, 2> _cubemaps;
};

} // namespace sgct

#endif // __SGCT__NONLINEARPROJECTION__H__
