/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/nonlinearprojection.h>

#include <sgct/callbackdata.h>
#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/window.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/cubemaptarget.h>
#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

namespace {
    size_t eyeIndex(sgct::FrustumMode frustumMode) {
        return frustumMode == sgct::FrustumMode::StereoRight ? 1 : 0;
    }
} // namespace

namespace sgct {

NonLinearProjection::NonLinearProjection(const Window& parent)
    : _subViewports{
        BaseViewport(parent),
        BaseViewport(parent),
        BaseViewport(parent),
        BaseViewport(parent),
        BaseViewport(parent),
        BaseViewport(parent)
    }
{}

NonLinearProjection::~NonLinearProjection() = default;

void NonLinearProjection::initialize(const vulkan::SceneFormat& format) {
    ZoneScoped;

    vulkan::Context& ctx = vulkan::Context::instance();

    const int maxResolution = static_cast<int>(ctx.limits().maxImageDimensionCube);
    if (_cubemapResolution.x > maxResolution || _cubemapResolution.y > maxResolution) {
        _cubemapResolution = ivec2(maxResolution, maxResolution);
        Log::Debug(std::format("Cubemap size set to max size: {}", maxResolution));
    }

    _format = format;
    _cubemaps = {};
    _cubemaps[0] = std::make_unique<vulkan::CubemapTarget>(
        ctx,
        static_cast<uint32_t>(_cubemapResolution.x),
        _format
    );
    Log::Debug(std::format(
        "{}x{} cube map generated", _cubemapResolution.x, _cubemapResolution.y
    ));

    initViewports();
    initPipelines(_cubemaps[0]->renderPass());
}

void NonLinearProjection::renderCubemap(VkCommandBuffer cmd, FrustumMode frustumMode,
                                        uint32_t frameIndex, uint32_t imageIndex,
                                        VkFence fence)
{
    ZoneScoped;

    std::unique_ptr<vulkan::CubemapTarget>& target = _cubemaps[eyeIndex(frustumMode)];
    if (!target) {
        target = std::make_unique<vulkan::CubemapTarget>(
            vulkan::Context::instance(),
            static_cast<uint32_t>(_cubemapResolution.x),
            _format
        );
    }

    const std::array<const BaseViewport*, 6> faces = faceViewports();
    for (int face = 0; face < 6; face++) {
        const BaseViewport& vp = *faces[face];
        if (!vp.isEnabled()) {
            continue;
        }

        // The face is cleared entirely, also outside of the sub-viewport of a cropped
        // face, which is never sampled
        target->beginFace(cmd, face);

        // The pixel rectangle of the sub-viewport with OpenGL's bottom-left origin. Like
        // the rest of the scene, it is rendered with a y-flipped viewport
        const ivec4 rect = ivec4{
            static_cast<int>(std::floor(vp.position().x * _cubemapResolution.x + 0.5f)),
            static_cast<int>(std::floor(vp.position().y * _cubemapResolution.y + 0.5f)),
            static_cast<int>(std::floor(vp.size().x * _cubemapResolution.x + 0.5f)),
            static_cast<int>(std::floor(vp.size().y * _cubemapResolution.y + 0.5f))
        };
        const int height = _cubemapResolution.y;
        VkViewport viewport = {};
        viewport.x = static_cast<float>(rect.x);
        viewport.y = static_cast<float>(height - rect.y);
        viewport.width = static_cast<float>(rect.z);
        viewport.height = -static_cast<float>(rect.w);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        VkRect2D scissor = {};
        scissor.offset = {
            std::max(rect.x, 0),
            std::max(height - rect.y - rect.w, 0)
        };
        scissor.extent = {
            static_cast<uint32_t>(std::max(rect.z, 0)),
            static_cast<uint32_t>(std::max(rect.w, 0))
        };
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        const RenderData renderData = {
            vp.window(),
            vp,
            frustumMode,
            ClusterManager::instance().sceneTransform(),
            vp.projection(frustumMode).viewMatrix(),
            vp.projection(frustumMode).projectionMatrix(),
            vp.projection(frustumMode).viewProjectionMatrix() *
                ClusterManager::instance().sceneTransform(),
            _cubemapResolution,
            cmd,
            frameIndex,
            imageIndex,
            Engine::instance().currentFrameNumber(),
            target->extent(),
            viewport,
            scissor
        };
        Engine::instance().drawFunction()(renderData);

        target->endFace(
            cmd,
            face,
            Engine::instance().nearClipPlane(),
            Engine::instance().farClipPlane()
        );

        // Record the readback copy of the face for the Spout/NDI streaming
        recordFaceCopy(cmd, face, frustumMode, fence);
    }
}

void NonLinearProjection::updateFrustums(FrustumMode mode, float nearClip, float farClip)
{
    ZoneScoped;

    if (_subViewports.right.isEnabled()) {
        _subViewports.right.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.left.isEnabled()) {
        _subViewports.left.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.bottom.isEnabled()) {
        _subViewports.bottom.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.top.isEnabled()) {
        _subViewports.top.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.front.isEnabled()) {
        _subViewports.front.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.back.isEnabled()) {
        _subViewports.back.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
}

void NonLinearProjection::setCubemapResolution(int resolution) {
    _cubemapResolution.x = resolution;
    _cubemapResolution.y = resolution;
}

void NonLinearProjection::setInterpolationMode(InterpolationMode im) {
    _interpolationMode = im;
}

void NonLinearProjection::setStereo(bool state) {
    _isStereo = state;
}

void NonLinearProjection::setUser(User& user) {
    _subViewports.right.setUser(user);
    _subViewports.left.setUser(user);
    _subViewports.bottom.setUser(user);
    _subViewports.top.setUser(user);
    _subViewports.front.setUser(user);
    _subViewports.back.setUser(user);
}

ivec2 NonLinearProjection::cubemapResolution() const {
    return _cubemapResolution;
}

const vulkan::SceneFormat& NonLinearProjection::sceneFormat() const {
    return _format;
}

const vulkan::CubemapTarget& NonLinearProjection::cubemap(FrustumMode frustumMode) const
{
    // The right eye's cube map exists once it was rendered; fall back to the left one
    const std::unique_ptr<vulkan::CubemapTarget>& t = _cubemaps[eyeIndex(frustumMode)];
    return t ? *t : *_cubemaps[0];
}

void NonLinearProjection::clearViewport(VkCommandBuffer cmd, const VkRect2D& area) const
{
    std::vector<VkClearAttachment> attachments;
    VkClearAttachment color = {};
    color.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color.colorAttachment = 0;
    color.clearValue.color = {
        { _clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w }
    };
    attachments.push_back(color);
    if (_format.hasNormals) {
        VkClearAttachment normals = {};
        normals.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        normals.colorAttachment = 1;
        attachments.push_back(normals);
    }
    if (_format.hasPositions) {
        VkClearAttachment positions = {};
        positions.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        positions.colorAttachment = 2;
        attachments.push_back(positions);
    }
    VkClearAttachment depth = {};
    depth.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth.clearValue.depthStencil = { 1.f, 0 };
    attachments.push_back(depth);

    VkClearRect rect = {};
    rect.rect = area;
    rect.baseArrayLayer = 0;
    rect.layerCount = 1;
    vkCmdClearAttachments(
        cmd,
        static_cast<uint32_t>(attachments.size()),
        attachments.data(),
        1,
        &rect
    );
}

std::string NonLinearProjection::fragmentShaderHeader() const {
    std::string header = std::format(
        "#version 460\n"
        "#define HAS_NORMALS {}\n"
        "#define HAS_POSITIONS {}\n"
        "#define HAS_DEPTH {}\n",
        _format.hasNormals ? 1 : 0,
        _format.hasPositions ? 1 : 0,
        _format.hasDepthTexture ? 1 : 0
    );
    if (_format.hasNormals) {
        header += "layout (location = 1) out vec4 out_normal;\n";
    }
    if (_format.hasPositions) {
        header += "layout (location = 2) out vec4 out_position;\n";
    }
    return header;
}

vulkan::ProjectionPass::Settings NonLinearProjection::projectionPassSettings() const {
    vulkan::ProjectionPass::Settings settings;
    settings.samplerCount = 4;
    settings.colorAttachmentCount = _format.colorAttachmentCount();
    settings.samples = _format.samples;
    settings.writesAllAttachments = true;
    settings.writesDepth = _format.hasDepthTexture;
    return settings;
}

std::array<VkDescriptorImageInfo, 4> NonLinearProjection::cubeTextures(
                                                                FrustumMode frustumMode,
                                                                TextureView view) const
{
    using Attachment = vulkan::CubemapTarget::Attachment;
    const vulkan::CubemapTarget& target = cubemap(frustumMode);

    std::array<VkDescriptorImageInfo, 4> images = {};
    constexpr std::array<Attachment, 4> Bindings = {
        Attachment::Color, Attachment::Normals, Attachment::Positions, Attachment::Depth
    };
    for (size_t i = 0; i < Bindings.size(); i++) {
        if (!target.has(Bindings[i])) {
            continue;
        }
        images[i].sampler = target.sampler();
        images[i].imageView = view == TextureView::Cube ?
            target.cubeView(Bindings[i]) :
            target.arrayView(Bindings[i]);
        images[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    return images;
}

std::array<VkDescriptorImageInfo, 4> NonLinearProjection::faceTextures(
                                                                FrustumMode frustumMode,
                                                                int face) const
{
    using Attachment = vulkan::CubemapTarget::Attachment;
    const vulkan::CubemapTarget& target = cubemap(frustumMode);

    std::array<VkDescriptorImageInfo, 4> images = {};
    constexpr std::array<Attachment, 4> Bindings = {
        Attachment::Color, Attachment::Normals, Attachment::Positions, Attachment::Depth
    };
    for (size_t i = 0; i < Bindings.size(); i++) {
        if (!target.has(Bindings[i])) {
            continue;
        }
        images[i].sampler = target.sampler();
        images[i].imageView = target.faceView(face, Bindings[i]);
        images[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    return images;
}

std::array<const BaseViewport*, 6> NonLinearProjection::faceViewports() const {
    return {
        &_subViewports.right,
        &_subViewports.left,
        &_subViewports.bottom,
        &_subViewports.top,
        &_subViewports.front,
        &_subViewports.back
    };
}

} // namespace sgct
