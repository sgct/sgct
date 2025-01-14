/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/cubemap.h>

#include <sgct/internalshaders.h>
#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <glm/gtc/matrix_transform.hpp>

#ifdef SGCT_HAS_SPOUT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <SpoutLibrary.h>
#endif // SGCT_HAS_SPOUT

namespace {
    constexpr std::string_view FragmentShader = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform sampler2D right;
  uniform sampler2D zLeft;
  uniform sampler2D bottom;
  uniform sampler2D top;
  uniform sampler2D left;
  uniform sampler2D zRight;

  void main() {
    float column = tr_uv.x * 3.0; // 3 columns
    float row = tr_uv.y * 2.0;    // 2 rows
    
    int col = int(floor(column));
    int rowId = int(floor(row));
    
    vec2 localCoord = vec2(fract(column), fract(row));
    
    if (rowId == 0 && col == 0) {
        out_diffuse = texture(zRight, localCoord);
    } 
    else if (rowId == 0 && col == 1) {
        out_diffuse = texture(top, localCoord);
    } 
    else if (rowId == 0 && col == 2) {
        out_diffuse = texture(left, localCoord);
    } 
    else if (rowId == 1 && col == 0) {
        out_diffuse = texture(zLeft, localCoord);
    } 
    else if (rowId == 1 && col == 1) {
        out_diffuse = texture(bottom, localCoord);
    } 
    else if (rowId == 1 && col == 2) {
        out_diffuse = texture(right, localCoord);
    } 
  }
)";
} // namespace

namespace sgct {

CubemapProjection::CubemapProjection(const config::CubemapProjection& config,
                                     const Window& parent, User& user)
    : NonLinearProjection(parent)
    , _cubeFaces{
        Cubeface { config.channels ? config.channels->right : true },
        Cubeface { config.channels ? config.channels->left : true },
        Cubeface { config.channels ? config.channels->bottom : true },
        Cubeface { config.channels ? config.channels->top : true },
        Cubeface { config.channels ? config.channels->zLeft : true },
        Cubeface { config.channels ? config.channels->zRight : true },
    }
#ifdef SGCT_HAS_SPOUT
    , _spoutEnabled(config.spout ? config.spout->enabled : false)
    , _spoutName(config.spout ? config.spout->name.value_or("") : "")
#endif // SGCT_HAS_SPOUT
#ifdef SGCT_HAS_NDI
    , _ndiEnabled(config.ndi ? config.ndi->enabled : false)
    , _ndiName(config.ndi ? config.ndi->name.value_or("OpenSpace") : "OpenSpace")
    , _ndiGroups(config.ndi ? config.ndi->groups.value_or("") : "")
#endif // SGCT_HAS_NDI
    , _rigOrientation(config.orientation.value_or(vec3{ 0.f, 0.f, 0.f }))
{
    setUser(user);
    if (config.quality) {
        setCubemapResolution(*config.quality);
    }

    _clearColor = vec4(0.f, 0.f, 0.f, 1.f);
}

CubemapProjection::~CubemapProjection() {
    for (const Cubeface& info : _cubeFaces) {
        glDeleteTextures(1, &info.texture);

#ifdef SGCT_HAS_SPOUT
        if (info.spout.handle) {
            reinterpret_cast<SPOUTHANDLE>(info.spout.handle)->ReleaseSender();
            reinterpret_cast<SPOUTHANDLE>(info.spout.handle)->Release();
        }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
        if (info.ndi.handle) {
            // One of the two buffers might be still in flight so we have to flush the
            // pipeline before we can destroy the video or we might risk NDI accessing
            // dead memory and crashing
            NDIlib_send_send_video_async_v2(info.ndi.handle, nullptr);
            NDIlib_send_destroy(info.ndi.handle);
        }
#endif // SGCT_HAS_NDI
    }

    glDeleteFramebuffers(1, &_blitFbo);

    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.deleteProgram();
}

void CubemapProjection::update(const vec2&) const {}

void CubemapProjection::render(const BaseViewport& viewport,
                               FrustumMode frustumMode) const
{
    ZoneScoped;

    glEnable(GL_SCISSOR_TEST);
    viewport.setupViewport(frustumMode);
    glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    _shader.bind();

    for (int i = 0; i < 6; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, _cubeFaces[i].enabled ? _cubeFaces[i].texture : 0);
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    ShaderProgram::unbind();

    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    ShaderProgram::unbind();

    for (int i = 0; i < 6; i++) {
        if (!_cubeFaces[i].enabled) {
            continue;
        }

#ifdef SGCT_HAS_SPOUT
        if (_spoutEnabled) {
            glBindTexture(GL_TEXTURE_2D, _cubeFaces[i].texture);
            SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(_cubeFaces[i].spout.handle);
            const bool s = h->SendTexture(
                _cubeFaces[i].texture,
                static_cast<GLuint>(GL_TEXTURE_2D),
                _cubemapResolution.x,
                _cubemapResolution.y
            );
            if (!s) {
                Log::Error(std::format(
                    "Error sending texture '{}' for face {}", _cubeFaces[i].texture, i
                ));
            }
        }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
        if (_ndiEnabled) {
            // Download the texture data from the GPU
            glBindTexture(GL_TEXTURE_2D, _cubeFaces[i].texture);
            glGetTexImage(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                _cubeFaces[i].ndi.currentVideoBuffer->data()
            );

            // We are using a negative line stride to correct for the y-axis flip going
            // from OpenGL to DirectX. So our start point has to be the beginning of the
            // *last* line of the image as NDI then steps backwards through the image to
            // send it to the receiver.
            // So we start at data() (=0), move to the end (+size) and then backtrack one
            // line (- -line_stride = +line_stride)
            _cubeFaces[i].ndi.videoFrame.p_data = reinterpret_cast<uint8_t*>(
                _cubeFaces[i].ndi.currentVideoBuffer->data() +
                _cubeFaces[i].ndi.currentVideoBuffer->size() +
                _cubeFaces[i].ndi.videoFrame.line_stride_in_bytes
            );

            NDIlib_send_send_video_async_v2(
                _cubeFaces[i].ndi.handle,
                &_cubeFaces[i].ndi.videoFrame
            );

            // Switch the current buffer
            _cubeFaces[i].ndi.currentVideoBuffer =
                _cubeFaces[i].ndi.currentVideoBuffer ==
                    &_cubeFaces[i].ndi.videoBufferPing ?
                &_cubeFaces[i].ndi.videoBufferPong :
                &_cubeFaces[i].ndi.videoBufferPing;
        }
#endif // SGCT_HAS_NDI
    }
}

void CubemapProjection::setSpoutRigOrientation(vec3 orientation) {
    _rigOrientation = std::move(orientation);
}

void CubemapProjection::initTextures(unsigned int internalFormat, unsigned int format,
                                         unsigned int type)
{
    NonLinearProjection::initTextures(internalFormat, format, type);

    Log::Debug("CubemapProjection initTextures");

    for (int i = 0; i < 6; i++) {
        Log::Debug(std::format("CubemapProjection initTextures {}", i));
        if (!_cubeFaces[i].enabled) {
            continue;
        }

        glGenTextures(1, &_cubeFaces[i].texture);
        glBindTexture(GL_TEXTURE_2D, _cubeFaces[i].texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            internalFormat,
            _cubemapResolution.x,
            _cubemapResolution.y,
            0,
            format,
            type,
            nullptr
        );

#ifdef SGCT_HAS_SPOUT
        if (_spoutEnabled) {
            _cubeFaces[i].spout.handle = GetSpout();
            if (_cubeFaces[i].spout.handle) {
                constexpr std::array<const char*, 6> CubeMapFaceName = {
                    "Right", "zLeft", "Bottom", "Top", "Left", "zRight"
                };

                SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(_cubeFaces[i].spout.handle);
                const std::string fullName =
                    _spoutName.empty() ?
                    CubeMapFaceName[i] :
                    std::format("{}-{}", _spoutName, CubeMapFaceName[i]);
                bool success = h->CreateSender(
                    fullName.c_str(),
                    _cubemapResolution.x,
                    _cubemapResolution.y
                );
                if (!success) {
                    Log::Error(std::format(
                        "Error creating SPOUT handle for {}", CubeMapFaceName[i]
                    ));
                }
            }
        }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
        if (_ndiEnabled) {
            constexpr std::array<const char*, 6> CubeMapFaceName = {
                "Right", "zLeft", "Bottom", "Top", "Left", "zRight"
            };

            _cubeFaces[i].ndi.name = std::format("{}-{}", _ndiName, CubeMapFaceName[i]);

            NDIlib_send_create_t createDesc;
            createDesc.p_ndi_name = _cubeFaces[i].ndi.name.c_str();
            if (!_ndiGroups.empty()) {
                createDesc.p_groups = _ndiGroups.c_str();
            }

            _cubeFaces[i].ndi.handle = NDIlib_send_create(&createDesc);
            if (!_cubeFaces[i].ndi.handle) {
                Log::Error("Error creating NDI sender");
            }

            _cubeFaces[i].ndi.videoFrame.xres = _cubemapResolution.x;
            _cubeFaces[i].ndi.videoFrame.yres = _cubemapResolution.y;
            _cubeFaces[i].ndi.videoFrame.FourCC = NDIlib_FourCC_type_RGBX;
            // We have a negative stride to account for the fact that OpenGL textures have
            // their y-axis flipped compared to DirectX textures
            _cubeFaces[i].ndi.videoFrame.line_stride_in_bytes = -_cubemapResolution.x * 4;
            _cubeFaces[i].ndi.videoFrame.frame_rate_N = 60000; // 60 fps
            _cubeFaces[i].ndi.videoFrame.frame_rate_D = 1000;  // 60 fps
            _cubeFaces[i].ndi.videoFrame.picture_aspect_ratio = 1.f;
            _cubeFaces[i].ndi.videoFrame.frame_format_type =
                NDIlib_frame_format_type_progressive;
            _cubeFaces[i].ndi.videoFrame.timecode = 0;

            _cubeFaces[i].ndi.videoBufferPing.resize(
                _cubemapResolution.x * _cubemapResolution.y * 4
            );
            _cubeFaces[i].ndi.videoBufferPong.resize(
                _cubemapResolution.x * _cubemapResolution.y * 4
            );

            _cubeFaces[i].ndi.currentVideoBuffer = &_cubeFaces[i].ndi.videoBufferPing;
        }
#endif // SGCT_HAS_NDI
    }
}

void CubemapProjection::initVBO() {
    struct Vertex {
        float x;
        float y;
        float z;
        float s;
        float t;
    };

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    constexpr std::array<float, 20> v = {
        -1.f, -1.f, -1.f, 0.f, 0.f,
        -1.f,  1.f, -1.f, 0.f, 1.f,
         1.f, -1.f, -1.f, 1.f, 0.f,
         1.f,  1.f, -1.f, 1.f, 1.f
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, s))
    );

    glBindVertexArray(0);
}

void CubemapProjection::initViewports() {
    // distance is needed to calculate the distance to all view planes
    constexpr float Distance = 1.f;

    // setup base viewport that will be rotated to create the other cubemap views
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

    // right
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

    // left
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

    // bottom
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

    // top
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

    // front
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

    // back
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
}

void CubemapProjection::initShaders() {
    _shader = ShaderProgram("CubemapShader");
    _shader.addVertexShader(shaders::BaseVert);
    _shader.addFragmentShader(FragmentShader);
    _shader.createAndLinkProgram();
    _shader.bind();

    glUniform1i(glGetUniformLocation(_shader.id(), "right"), 0);
    glUniform1i(glGetUniformLocation(_shader.id(), "zLeft"), 1);
    glUniform1i(glGetUniformLocation(_shader.id(), "bottom"), 2);
    glUniform1i(glGetUniformLocation(_shader.id(), "top"), 3);
    glUniform1i(glGetUniformLocation(_shader.id(), "left"), 4);
    glUniform1i(glGetUniformLocation(_shader.id(), "zRight"), 5);

    ShaderProgram::unbind();
}

void CubemapProjection::initFBO(unsigned int internalFormat, int nSamples) {
    NonLinearProjection::initFBO(internalFormat, nSamples);

    _spoutFBO = std::make_unique<OffScreenBuffer>(internalFormat);
    _spoutFBO->createFBO(_cubemapResolution.x, _cubemapResolution.y, 1);
    glGenFramebuffers(1, &_blitFbo);
}

void CubemapProjection::renderCubemap(FrustumMode frustumMode) const {
    ZoneScoped;

    auto render = [this](const BaseViewport& vp, int index, FrustumMode mode) {
        if (!_cubeFaces[index].enabled) {
            return;
        }

        renderCubeFace(vp, index, mode);

        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, _blitFbo);
        glFramebufferTexture2D(
            GL_READ_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + index,
            _textures.cubeMapColor,
            0
        );
        glFramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT1,
            GL_TEXTURE_2D,
            _cubeFaces[index].texture,
            0
        );
        glDrawBuffer(GL_COLOR_ATTACHMENT1);
        glBlitFramebuffer(
            0,
            0,
            _cubemapResolution.x,
            _cubemapResolution.y,
            0,
            0,
            _cubemapResolution.x,
            _cubemapResolution.y,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST
        );
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };

    render(_subViewports.right, 0, frustumMode);
    render(_subViewports.left, 1, frustumMode);
    render(_subViewports.bottom, 2, frustumMode);
    render(_subViewports.top, 3, frustumMode);
    render(_subViewports.front, 4, frustumMode);
    render(_subViewports.back, 5, frustumMode);
}

} // namespace sgct
