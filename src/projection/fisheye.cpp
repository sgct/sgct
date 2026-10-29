/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/fisheye.h>

#include <sgct/engine.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/user.h>
#include <sgct/window.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/cubemaptarget.h>
#include <sgct/vulkan/projectionpass.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {
    struct PushConstants {
        std::array<sgct::vec4, 4> corners;
        sgct::vec4 bgColor;
        sgct::vec3 offset;
        float halfFov = 0.f;
        float size = 0.f;
    };
    static_assert(offsetof(PushConstants, bgColor) == 64);
    static_assert(offsetof(PushConstants, offset) == 80);
    static_assert(offsetof(PushConstants, halfFov) == 92);
    static_assert(offsetof(PushConstants, size) == 96);

    // The building blocks of the fragment shader, which mirror the shaders of the former
    // OpenGL renderer. The offset of the off-axis variant is 0 for on-axis rendering,
    // which makes both variants identical
    constexpr std::string_view FisheyeFragHeader = R"(
  layout (location = 0) in vec2 in_texCoords;
  layout (location = 0) out vec4 out_diffuse;

  layout (set = 0, binding = 0) uniform samplerCube cubemap;
#if HAS_NORMALS
  layout (set = 0, binding = 1) uniform samplerCube normalmap;
#endif
#if HAS_POSITIONS
  layout (set = 0, binding = 2) uniform samplerCube positionmap;
#endif
#if HAS_DEPTH
  layout (set = 0, binding = 3) uniform samplerCube depthmap;
#endif

  layout (push_constant) uniform Constants {
    layout (offset = 64) vec4 bgColor;
    vec3 offset;
    float halfFov;
    float size;
  } pc;

  // The cube map is the mirror image of the OpenGL one along the y axis (see
  // vulkan::CubemapTarget)
  vec4 sampleCube(samplerCube map, vec3 dir) {
    return texture(map, vec3(dir.x, -dir.y, dir.z));
  }
)";

    constexpr std::string_view RotationFourFaceCubeFun = R"(
  vec3 rotate(vec3 dir) {
    const float Angle = 0.7071067812;
    float x = Angle * dir.x + Angle * dir.z;
    float y = dir.y;
    float z = -Angle * dir.x + Angle * dir.z;
    return vec3(x, y, z);
  }
)";

    constexpr std::string_view RotationFiveSixFaceCubeFun = R"(
  vec3 rotate(vec3 dir) {
    const float Angle = 0.7071067812;
    float x = Angle * dir.x - Angle * dir.y;
    float y = Angle * dir.x + Angle * dir.y;
    float z = dir.z;
    return vec3(x, y, z);
  }
)";

    constexpr std::string_view SampleFun = R"(
  vec4 sampleCubeTexture(vec2 texel, samplerCube map, vec4 background) {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s * s + t * t;
    if (r2 <= 1.0) {
      float phi = sqrt(r2) * pc.halfFov;
      float theta = atan(s, t);
      float x = sin(phi) * sin(theta) - pc.offset.x;
      float y = -sin(phi) * cos(theta) - pc.offset.y;
      float z = cos(phi) - pc.offset.z;
      return sampleCube(map, rotate(vec3(x, y, z)));
    }
    else {
      return background;
    }
  }
)";

    constexpr std::string_view InterpolateLinearFun = R"(
  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background) {
    return sampleCubeTexture(texCoords, map, background);
  }
)";

    constexpr std::string_view InterpolateCubicFun = R"(
  vec4 cubic(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w = vec4(-x + 2 * x2 - x3, 2 - 5 * x2 + 3 * x3, x + 4 * x2 - 3 * x3, -x2 + x3);
    return w / 2.0;
  }

  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background) {
    vec2 transTex = texCoords * vec2(pc.size, pc.size);
    vec2 frac = fract(transTex);
    transTex -= frac;

    vec4 xcubic = cubic(frac.x);
    vec4 ycubic = cubic(frac.y);

    const float h = 1.0;
    vec4 c = transTex.xxyy + vec4(-h, +h, -h, +h);
    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = c + vec4(xcubic.yw, ycubic.yw) / s;

    vec4 sample0 = sampleCubeTexture(offset.xz / pc.size, map, background);
    vec4 sample1 = sampleCubeTexture(offset.yz / pc.size, map, background);
    vec4 sample2 = sampleCubeTexture(offset.xw / pc.size, map, background);
    vec4 sample3 = sampleCubeTexture(offset.yw / pc.size, map, background);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
  }
)";

    constexpr std::string_view FisheyeFragMain = R"(
  void main() {
    out_diffuse = cubeSample(in_texCoords, cubemap, pc.bgColor);
#if HAS_NORMALS
    out_normal = cubeSample(in_texCoords, normalmap, vec4(0.0));
#endif
#if HAS_POSITIONS
    out_position = cubeSample(in_texCoords, positionmap, vec4(0.0));
#endif
#if HAS_DEPTH
    gl_FragDepth = cubeSample(in_texCoords, depthmap, vec4(1.0)).x;
#endif
  }
)";
} // namespace

namespace sgct {

FisheyeProjection::FisheyeProjection(const config::FisheyeProjection& config,
                                     const Window& parent, User& user)
    : NonLinearProjection(parent)
    , _fov(config.fov.value_or(180.f))
    , _tilt(config.tilt.value_or(0.f))
    , _diameter(config.diameter.value_or(14.8f))
    , _cropLeft(config.crop ? config.crop->left : 0.f)
    , _cropRight(config.crop ? config.crop->right : 0.f)
    , _cropBottom(config.crop ? config.crop->bottom : 0.f)
    , _cropTop(config.crop ? config.crop->top : 0.f)
    , _keepAspectRatio(config.keepAspectRatio.value_or(true))
{
    setUser(user);

    if (config.quality) {
        setCubemapResolution(*config.quality);
    }
    if (config.interpolation) {
        const NonLinearProjection::InterpolationMode interp =
            [](config::FisheyeProjection::Interpolation i) {
            switch (i) {
                case config::FisheyeProjection::Interpolation::Linear:
                    return NonLinearProjection::InterpolationMode::Linear;
                case config::FisheyeProjection::Interpolation::Cubic:
                    return NonLinearProjection::InterpolationMode::Cubic;
                default: throw std::logic_error("Unhandled case label");
            }
            }(*config.interpolation);
        setInterpolationMode(interp);
    }
    if (config.offset) {
        setBaseOffset(*config.offset);
    }
    _clearColor = config.background.value_or(_clearColor);
}

FisheyeProjection::~FisheyeProjection() = default;

void FisheyeProjection::update(const vec2& size) const {
    // Do the cropping in the fragment shader and not by changing the vbo

    const float cropAspect =
        ((1.f - 2.f * _cropBottom) + (1.f - 2.f * _cropTop)) /
        ((1.f - 2.f * _cropLeft) + (1.f - 2.f * _cropRight));

    const float frameBufferAspect = _ignoreAspectRatio ? 1.f : (size.x / size.y);

    float x = 1.f;
    float y = 1.f;
    if (_keepAspectRatio) {
        const float aspect = frameBufferAspect * cropAspect;
        if (aspect >= 1.f) {
            x = 1.f / aspect;
        }
        else {
            y = aspect;
        }
    }

    _quad = {
        vec4{ -x, -y, _cropLeft, _cropBottom },
        vec4{ -x,  y, _cropLeft, 1.f - _cropTop },
        vec4{  x, -y, 1.f - _cropRight, _cropBottom },
        vec4{  x,  y, 1.f - _cropRight, 1.f - _cropTop }
    };
}

void FisheyeProjection::render(VkCommandBuffer cmd, const BaseViewport&,
                               FrustumMode frustumMode, const VkRect2D& area) const
{
    ZoneScoped;

    // The quad does not cover the whole viewport if the aspect ratio is kept
    clearViewport(cmd, area);

    PushConstants constants;
    constants.corners = _quad;
    constants.bgColor = _clearColor;
    constants.halfFov = glm::radians(_fov / 2.f);
    constants.size = static_cast<float>(_cubemapResolution.x);
    // For stereo rendering, the fisheye is rendered off-axis from the position of the eye
    switch (frustumMode) {
        case FrustumMode::Mono:
            constants.offset = _totalOffset;
            break;
        case FrustumMode::StereoLeft:
            constants.offset = vec3{
                _baseOffset.x - Engine::defaultUser().eyeSeparation() / _diameter,
                _baseOffset.y,
                _baseOffset.z
            };
            break;
        case FrustumMode::StereoRight:
            constants.offset = vec3{
                _baseOffset.x + Engine::defaultUser().eyeSeparation() / _diameter,
                _baseOffset.y,
                _baseOffset.z
            };
            break;
    }

    _pass->bind(cmd, &constants);
    _pass->setTextures(cmd, cubeTextures(frustumMode, TextureView::Cube));
    _pass->drawQuad(cmd);
}

void FisheyeProjection::setDomeDiameter(float diameter) {
    _diameter = diameter;
}

void FisheyeProjection::setTilt(float angle) {
    _tilt = angle;
}

void FisheyeProjection::setFOV(float angle) {
    _fov = angle;
}

void FisheyeProjection::setCropFactors(float left, float right, float bottom, float top) {
    _cropLeft = std::clamp(left, 0.f, 1.f);
    _cropRight = std::clamp(right, 0.f, 1.f);
    _cropBottom = std::clamp(bottom, 0.f, 1.f);
    _cropTop = std::clamp(top, 0.f, 1.f);
}

void FisheyeProjection::setOffset(vec3 offset) const {
    _offset = std::move(offset);
    _totalOffset.x = _baseOffset.x + _offset.x;
    _totalOffset.y = _baseOffset.y + _offset.y;
    _totalOffset.z = _baseOffset.z + _offset.z;
}

void FisheyeProjection::setBaseOffset(vec3 offset) {
    _baseOffset = std::move(offset);
    _totalOffset.x = _baseOffset.x + _offset.x;
    _totalOffset.y = _baseOffset.y + _offset.y;
    _totalOffset.z = _baseOffset.z + _offset.z;
}

void FisheyeProjection::setIgnoreAspectRatio(bool state) {
    _ignoreAspectRatio = state;
}

void FisheyeProjection::setKeepAspectRatio(bool state) {
    _keepAspectRatio = state;
}

void FisheyeProjection::initViewports() {
    // Radius is needed to calculate the distance to all view planes
    const float radius = _diameter / 2.f;

    // Setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.f);

    // 250.5287794 degree FOV covers exactly five sides of a cube, larger FOV needs six
    const float fiveFaceLimit = 2.f * glm::degrees(std::acos(-1.f / std::sqrt(3.f)));
    // 109.4712206 degree FOV is needed to cover the entire top face
    const float topFaceLimit = 2.f * glm::degrees(std::acos(1.f / std::sqrt(3.f)));


    // Four faces doesn't cover more than 180 degrees
    if (_fov > 180.f && _fov <= fiveFaceLimit) {
        _method = FisheyeMethod::FiveFaceCube;
    }

    // How much of the side faces that are used
    float cropLevel = 0.5f;
    float projectionOffset = 0.f;
    if (_method == FisheyeMethod::FiveFaceCube &&
        _fov >= topFaceLimit && _fov <= fiveFaceLimit)
    {
        const float cosAngle = std::cos(glm::radians(_fov / 2.f));
        const float normalizedProjectionOffset =
            _fov < 180.f ?
                1.f - _fov / 180.f :
                std::sqrt((2.f * cosAngle * cosAngle) / (1.f - cosAngle * cosAngle));

        projectionOffset = normalizedProjectionOffset * radius;
        cropLevel = (1.f - normalizedProjectionOffset) / 2.f;
    }
    else if (_fov > fiveFaceLimit) {
        _method = FisheyeMethod::SixFaceCube;
        cropLevel = 0.f;
        projectionOffset = radius;
    }

    const glm::mat4 tiltMat = glm::rotate(
        glm::mat4(1.f),
        glm::radians(90.f - _tilt),
        glm::vec3(1.f, 0.f, 0.f)
    );

    if (_method == FisheyeMethod::FiveFaceCube || _method == FisheyeMethod::SixFaceCube) {
        const glm::mat4 rollRot = glm::rotate(
            tiltMat,
            glm::radians(45.f),
            glm::vec3(0.f, 0.f, 1.f)
        );

        // +X face
        {
            _subViewports.right.setSize(vec2{ 1.f - cropLevel, 1.f });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 upperRight = upperRightBase;
            upperRight.x = projectionOffset;

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            const glm::vec3 ur = glm::vec3(rotMat * upperRight);
            _subViewports.right.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // -X face
        {
            _subViewports.left.setPosition(vec2{ cropLevel, 0.f });
            _subViewports.left.setSize(vec2{ 1.f - cropLevel, 1.f });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.x = -projectionOffset;
            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.x = -projectionOffset;

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeft);
            const glm::vec3 ul = glm::vec3(rotMat * upperLeft);
            const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
            _subViewports.left.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // +Y face
        {
            _subViewports.bottom.setPosition(vec2{ 0.f, cropLevel });
            _subViewports.bottom.setSize(vec2{ 1.f, 1.f - cropLevel });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.y = -projectionOffset;

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeft);
            const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
            _subViewports.bottom.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // -Y face
        {
            _subViewports.top.setSize(vec2{ 1.f, 1.f - cropLevel });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.y = projectionOffset;
            glm::vec4 upperRight = upperRightBase;
            upperRight.y = projectionOffset;

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(rotMat * upperLeft);
            const glm::vec3 ur = glm::vec3(rotMat * upperRight);
            _subViewports.top.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // +Z face
        {
            const glm::vec3 ll = glm::vec3(rollRot * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(rollRot * upperLeftBase);
            const glm::vec3 ur = glm::vec3(rollRot * upperRightBase);
            _subViewports.front.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // -Z face
        {
            if (_method == FisheyeMethod::FiveFaceCube) {
                _subViewports.back.setEnabled(false);
            }
            else {
                const glm::mat4 rotMat = glm::rotate(
                    rollRot,
                    glm::radians(180.f),
                    glm::vec3(0.f, 1.f, 0.f)
                );

                const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
                const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
                const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
                _subViewports.back.projectionPlane().setCoordinates(
                    vec3(ll.x, ll.y, ll.z),
                    vec3(ul.x, ul.y, ul.z),
                    vec3(ur.x, ur.y, ur.z)
                );
            }
        }
    }
    else {
        // FourFaceCube
        const glm::mat4 panRot = glm::rotate(
            tiltMat,
            glm::radians(45.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        // +X face
        {
            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(-90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
            _subViewports.right.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // -X face
        _subViewports.left.setEnabled(false);

        // +Y face
        {
            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
            _subViewports.bottom.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // -Y face
        {
            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
            _subViewports.top.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // +Z face
        {
            const glm::vec3 ll = glm::vec3(panRot * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(panRot * upperLeftBase);
            const glm::vec3 ur = glm::vec3(panRot * upperRightBase);
            _subViewports.front.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // -Z face
        _subViewports.back.setEnabled(false);
    }
}

void FisheyeProjection::initPipelines(VkRenderPass renderPass) {
    const bool isCubic = (_interpolationMode == InterpolationMode::Cubic);

    std::string fragmentShader = fragmentShaderHeader();
    fragmentShader += FisheyeFragHeader;
    fragmentShader += _method == FisheyeMethod::FourFaceCube ?
        RotationFourFaceCubeFun :
        RotationFiveSixFaceCubeFun;
    fragmentShader += SampleFun;
    fragmentShader += isCubic ? InterpolateCubicFun : InterpolateLinearFun;
    fragmentShader += FisheyeFragMain;

    vulkan::ProjectionPass::Settings settings = projectionPassSettings();
    settings.fragmentShader = std::move(fragmentShader);
    settings.pushConstantSize = sizeof(PushConstants);
    _pass = std::make_unique<vulkan::ProjectionPass>(
        vulkan::Context::instance(),
        renderPass,
        settings
    );
}

} // namespace sgct
