/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/cubemap.h>

#include <sgct/config.h>
#include <sgct/error.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/window.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/cubemaptarget.h>
#include <sgct/vulkan/projectionpass.h>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#ifdef SGCT_HAS_SPOUT
#include <SpoutLibrary/SpoutLibrary.h>
#endif // SGCT_HAS_SPOUT

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw sgct::Error(
                sgct::Error::Component::Window, 4500,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    struct PushConstants {
        std::array<sgct::vec4, 4> corners;
        /// The layer of the cube map image that each face is stored in
        std::array<int32_t, 6> layers;
        /// A bit mask of the faces that are rendered
        int32_t enabledFaces = 0;
    };
    static_assert(offsetof(PushConstants, layers) == 64);
    static_assert(offsetof(PushConstants, enabledFaces) == 88);

    // The quad covering the whole viewport (xy is the position, zw the texture
    // coordinates), drawn as a triangle strip
    constexpr std::array<sgct::vec4, 4> FullQuad = {
        sgct::vec4{ -1.f, -1.f, 0.f, 0.f },
        sgct::vec4{ -1.f,  1.f, 0.f, 1.f },
        sgct::vec4{  1.f, -1.f, 1.f, 0.f },
        sgct::vec4{  1.f,  1.f, 1.f, 1.f }
    };

    // The faces are laid out in two rows of three columns, like the OpenGL shader did
    constexpr std::string_view FragmentShader = R"(
  layout (location = 0) in vec2 in_texCoords;
  layout (location = 0) out vec4 out_diffuse;

  layout (set = 0, binding = 0) uniform sampler2DArray faces;
#if HAS_NORMALS
  layout (set = 0, binding = 1) uniform sampler2DArray normals;
#endif
#if HAS_POSITIONS
  layout (set = 0, binding = 2) uniform sampler2DArray positions;
#endif
#if HAS_DEPTH
  layout (set = 0, binding = 3) uniform sampler2DArray depths;
#endif

  layout (push_constant) uniform Constants {
    layout (offset = 64) int layers[6];
    int enabledFaces;
  } pc;

  const int Right = 0;
  const int ZLeft = 1;
  const int Bottom = 2;
  const int Top = 3;
  const int Left = 4;
  const int ZRight = 5;


  void main() {
    float column = in_texCoords.x * 3.0; // 3 columns
    float row = in_texCoords.y * 2.0;    // 2 rows

    int col = clamp(int(floor(column)), 0, 2);
    int rowId = clamp(int(floor(row)), 0, 1);

    vec2 localCoord = vec2(fract(column), fract(row));

    const int Faces[6] = int[6](ZRight, Top, Left, ZLeft, Bottom, Right);
    int face = Faces[rowId * 3 + col];
    if ((pc.enabledFaces & (1 << face)) == 0) {
      // The faces that are not rendered are black, like the unbound OpenGL textures
      out_diffuse = vec4(0.0, 0.0, 0.0, 1.0);
#if HAS_NORMALS
      out_normal = vec4(0.0);
#endif
#if HAS_POSITIONS
      out_position = vec4(0.0);
#endif
#if HAS_DEPTH
      gl_FragDepth = 1.0;
#endif
      return;
    }

    // The faces are stored upside down compared to OpenGL (see vulkan::CubemapTarget)
    vec3 tex = vec3(localCoord.x, 1.0 - localCoord.y, float(pc.layers[face]));
    out_diffuse = texture(faces, tex);
#if HAS_NORMALS
    out_normal = texture(normals, tex);
#endif
#if HAS_POSITIONS
    out_position = texture(positions, tex);
#endif
#if HAS_DEPTH
    gl_FragDepth = texture(depths, tex).r;
#endif
  }
)";
} // namespace

namespace sgct {

CubemapProjection::CubemapProjection(const config::CubemapProjection& config,
                                     const Window& parent, User& user)
    : NonLinearProjection(parent)
    , _isFaceEnabled{
        config.channels ? config.channels->right : true,
        config.channels ? config.channels->left : true,
        config.channels ? config.channels->bottom : true,
        config.channels ? config.channels->top : true,
        config.channels ? config.channels->zLeft : true,
        config.channels ? config.channels->zRight : true
    }
    , _rigOrientation(config.orientation.value_or(vec3{ 0.f, 0.f, 0.f }))
#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
#ifdef SGCT_HAS_SPOUT
    , _spoutEnabled(config.spout ? config.spout->enabled : false)
    , _spoutName(config.spout ? config.spout->name.value_or("") : "")
#endif // SGCT_HAS_SPOUT
#ifdef SGCT_HAS_NDI
    , _ndiEnabled(config.ndi ? config.ndi->enabled : false)
    , _ndiName(config.ndi ? config.ndi->name.value_or("OpenSpace") : "OpenSpace")
    , _ndiGroups(config.ndi ? config.ndi->groups.value_or("") : "")
#endif // SGCT_HAS_NDI
#endif // defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
{
    setUser(user);
    if (config.quality) {
        setCubemapResolution(*config.quality);
    }

    _clearColor = vec4(0.f, 0.f, 0.f, 1.f);
}

CubemapProjection::~CubemapProjection() {
#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
    for (FaceStream& stream : _faceStreams) {
#ifdef SGCT_HAS_SPOUT
        if (stream.spout) {
            stream.spout->ReleaseSender();
            stream.spout->Release();
        }
#endif // SGCT_HAS_SPOUT
#ifdef SGCT_HAS_NDI
        if (stream.ndi) {
            // Flush the pipeline before destroying the sender, so NDI does not access
            // a readback buffer that is being destroyed
            NDIlib_send_send_video_async_v2(stream.ndi, nullptr);
            NDIlib_send_destroy(stream.ndi);
        }
#endif // SGCT_HAS_NDI
    }
#endif // defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
}

void CubemapProjection::update(const vec2&) const {}

void CubemapProjection::render(VkCommandBuffer cmd, const BaseViewport&,
                               FrustumMode frustumMode, const VkRect2D& area) const
{
    ZoneScoped;

    clearViewport(cmd, area);

    PushConstants constants;
    constants.corners = FullQuad;
    for (int i = 0; i < 6; i++) {
        constants.layers[i] = static_cast<int32_t>(vulkan::CubemapTarget::layer(i));
        if (_isFaceEnabled[i]) {
            constants.enabledFaces |= 1 << i;
        }
    }

    _pass->bind(cmd, &constants);
    _pass->setTextures(cmd, cubeTextures(frustumMode, TextureView::Array));
    _pass->drawQuad(cmd);
}

void CubemapProjection::setSpoutRigOrientation(vec3 orientation) {
    _rigOrientation = std::move(orientation);
}

void CubemapProjection::initViewports() {
    // Distance is needed to calculate the distance to all view planes
    constexpr float Distance = 1.f;

    // Setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase = glm::vec4(-Distance, -Distance, Distance, 1.f);
    const glm::vec4 upperLeftBase = glm::vec4(-Distance, Distance, Distance, 1.f);
    const glm::vec4 upperRightBase = glm::vec4(Distance, Distance, Distance, 1.f);

    const glm::mat4 pitchRot = glm::rotate(
        glm::mat4(1.f),
        glm::radians(-_rigOrientation.x),
        glm::vec3(0.f, 1.f, 0.f)
    );
    const glm::mat4 yawRot = glm::rotate(
        pitchRot,
        glm::radians(_rigOrientation.y),
        glm::vec3(1.f, 0.f, 0.f)
    );
    const glm::mat4 rollRot = glm::rotate(
        yawRot,
        glm::radians(-_rigOrientation.z),
        glm::vec3(0.f, 0.f, 1.f)
    );

    // Right
    {
        glm::vec4 upperRight = upperRightBase;
        upperRight.x = Distance;

        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRight);
        _subViewports.right.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // Left
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.x = -Distance;
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.x = -Distance;

        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeft);
        const glm::vec3 ul = glm::vec3(r * upperLeft);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.left.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // Bottom
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.y = -Distance;

        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeft);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.bottom.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // Top
    {
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.y = Distance;
        glm::vec4 upperRight = upperRightBase;
        upperRight.y = Distance;

        _subViewports.top.setSize(vec2{ 1.f, 1.f });

        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeft);
        const glm::vec3 ur = glm::vec3(r * upperRight);
        _subViewports.top.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // Front
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

    // Back
    {
        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(180.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.back.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // Only the faces of the enabled channels are rendered
    _subViewports.right.setEnabled(_isFaceEnabled[0]);
    _subViewports.left.setEnabled(_isFaceEnabled[1]);
    _subViewports.bottom.setEnabled(_isFaceEnabled[2]);
    _subViewports.top.setEnabled(_isFaceEnabled[3]);
    _subViewports.front.setEnabled(_isFaceEnabled[4]);
    _subViewports.back.setEnabled(_isFaceEnabled[5]);
}

void CubemapProjection::initPipelines(VkRenderPass renderPass) {
    vulkan::ProjectionPass::Settings settings = projectionPassSettings();
    settings.fragmentShader = fragmentShaderHeader() + std::string(FragmentShader);
    settings.pushConstantSize = sizeof(PushConstants);
    _pass = std::make_unique<vulkan::ProjectionPass>(
        vulkan::Context::instance(),
        renderPass,
        settings
    );
}

#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
void CubemapProjection::initializeStream() {
    if (_streamInitialized) {
        return;
    }
    _streamInitialized = true;

    vulkan::Context& ctx = vulkan::Context::instance();
    const vulkan::CubemapTarget& target = cubemap(FrustumMode::Mono);
    _streamResolution = target.resolution();
    _streamFormat = target.format().color;
    const VkDeviceSize size =
        static_cast<VkDeviceSize>(_streamResolution) * _streamResolution * 4;

    const bool isBgra =
        _streamFormat == VK_FORMAT_B8G8R8A8_UNORM ||
        _streamFormat == VK_FORMAT_B8G8R8A8_SRGB;

    // Spout expects RGBA pixels, so if the cube map is BGRA allocate a temporary
    // buffer for the conversion
    if (isBgra) {
        _streamRgba.resize(size);
    }

    // The names of the faces, in the order of the face indices
    constexpr std::array<const char*, 6> FaceName = {
        "Right", "zLeft", "Bottom", "Top", "Left", "zRight"
    };

    for (int face = 0; face < 6; face++) {
        FaceStream& stream = _faceStreams[face];
        // The readback ring. The buffers are host-visible and persistently mapped, so
        // the pixels can be read directly once the copy's fence is signaled
        for (auto& buffer : stream.buffers) {
            buffer = std::make_unique<vulkan::Buffer>(
                ctx, size, vulkan::Buffer::Type::Readback
            );
        }
        for (bool& hasData : stream.hasData) {
            hasData = false;
        }

#ifdef SGCT_HAS_SPOUT
        if (_spoutEnabled) {
            const std::string name = _spoutName.empty() ?
                FaceName[face] : std::format("{}-{}", _spoutName, FaceName[face]);
            stream.spout = GetSpout();
            if (stream.spout && !stream.spout->CreateSender(
                    name.c_str(), _streamResolution, _streamResolution
                ))
            {
                Log::Error(std::format(
                    "Cubemap projection: Error creating the Spout sender '{}'", name
                ));
            }
        }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
        if (_ndiEnabled) {
            const std::string name = std::format("{}-{}", _ndiName, FaceName[face]);
            NDIlib_send_create_t createDesc;
            createDesc.p_ndi_name = name.c_str();
            if (!_ndiGroups.empty()) {
                createDesc.p_groups = _ndiGroups.c_str();
            }
            stream.ndi = NDIlib_send_create(&createDesc);
            if (!stream.ndi) {
                Log::Error(std::format(
                    "Cubemap projection: Error creating the NDI sender '{}'", name
                ));
            }
            else {
                // The readback is top-down, so the stride is positive
                stream.ndiFrame.xres = _streamResolution;
                stream.ndiFrame.yres = _streamResolution;
                stream.ndiFrame.line_stride_in_bytes =
                    static_cast<int>(_streamResolution) * 4;
                stream.ndiFrame.picture_aspect_ratio = 1.f;
                stream.ndiFrame.FourCC =
                    isBgra ? NDIlib_FourCC_type_BGRA : NDIlib_FourCC_type_RGBA;
                stream.ndiFrame.frame_rate_N = 60000; // 60 fps
                stream.ndiFrame.frame_rate_D = 1000;  // 60 fps
                stream.ndiFrame.frame_format_type = NDIlib_frame_format_type_progressive;
                stream.ndiFrame.timecode = 0;
            }
        }
#endif // SGCT_HAS_NDI
    }
}

void CubemapProjection::recordFaceCopy(VkCommandBuffer cmd, int face,
                                       FrustumMode frustumMode, VkFence fence)
{
    // Only stream the left/mono eye
    if (frustumMode == FrustumMode::StereoRight) {
        return;
    }
    if (!_spoutEnabled && !_ndiEnabled) {
        return;
    }
    if (!_isFaceEnabled[face]) {
        return;
    }

    initializeStream();

    const uint32_t slot = static_cast<uint32_t>(_streamFrame % 3);
    FaceStream& stream = _faceStreams[face];
    cubemap(frustumMode).copyFace(cmd, face, stream.buffers[slot]->handle());
    stream.fences[slot] = fence;
    stream.hasData[slot] = true;
}

void CubemapProjection::sendStreamFrames() {
    if (!_spoutEnabled && !_ndiEnabled) {
        return;
    }

    // The frame to send is the one recorded in the previous frame
    const uint32_t slot = static_cast<uint32_t>((_streamFrame + 2) % 3);
    _streamFrame++;

    vulkan::Context& ctx = vulkan::Context::instance();
    const bool isBgra =
        _streamFormat == VK_FORMAT_B8G8R8A8_UNORM ||
        _streamFormat == VK_FORMAT_B8G8R8A8_SRGB;

    for (int face = 0; face < 6; face++) {
        FaceStream& stream = _faceStreams[face];
        if (!stream.hasData[slot]) {
            continue;
        }
        stream.hasData[slot] = false;

        // Wait for the copy of the previous frame to have finished
        check(
            vkWaitForFences(
                ctx.device(), 1, &stream.fences[slot], VK_TRUE,
                std::numeric_limits<uint64_t>::max()
            ),
            "vkWaitForFences"
        );

        const uint8_t* data = static_cast<const uint8_t*>(
            stream.buffers[slot]->map()
        );

#ifdef SGCT_HAS_SPOUT
        if (stream.spout) {
            const uint8_t* spoutData = data;
            if (isBgra) {
                // Spout expects RGBA pixels, so convert the BGRA readback
                uint8_t* rgba = _streamRgba.data();
                const size_t nPixels =
                    static_cast<size_t>(_streamResolution) * _streamResolution;
                for (size_t i = 0; i < nPixels; i++) {
                    rgba[i * 4 + 0] = data[i * 4 + 2];
                    rgba[i * 4 + 1] = data[i * 4 + 1];
                    rgba[i * 4 + 2] = data[i * 4 + 0];
                    rgba[i * 4 + 3] = data[i * 4 + 3];
                }
                spoutData = rgba;
            }
            if (!stream.spout->SendImage(
                    spoutData, _streamResolution, _streamResolution
                ))
            {
                Log::Error(std::format(
                    "Cubemap projection: Error sending the Spout image for face {}",
                    face
                ));
            }
        }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
        if (stream.ndi) {
            // NDI converts the BGRA (or RGBA) pixels on its own thread, so the readback
            // buffer can be passed directly; it stays valid until the next asynchronous
            // send to this sender
            stream.ndiFrame.p_data = const_cast<uint8_t*>(data);
            stream.ndiFrame.timestamp = 0;
            NDIlib_send_send_video_async_v2(stream.ndi, &stream.ndiFrame);
        }
#endif // SGCT_HAS_NDI
    }
}
#endif // defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)

} // namespace sgct
