/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/statisticsrenderer.h>

#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/profiling.h>
#include <sgct/viewport.h>
#include <sgct/window.h>
#ifdef SGCT_HAS_TEXT
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT
#include <sgct/vulkan/context.h>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstring>
#include <string_view>
#include <tuple>
#include <vector>

namespace {
    // Line parameters
    constexpr sgct::vec4 ColorStaticGrid = sgct::vec4{ 1.f, 1.f, 1.f, 0.2f };
    constexpr sgct::vec4 ColorStaticFrequency = sgct::vec4{ 1.f, 0.f, 0.f, 1.f };
    constexpr sgct::vec4 ColorStaticBackground = sgct::vec4{ 0.f, 0.f, 0.f, 0.85f };

    constexpr sgct::vec4 ColorFrameTime = sgct::vec4{ 1.f, 1.f, 0.f, 0.8f };
    constexpr sgct::vec4 ColorDrawTime = sgct::vec4{ 1.f, 0.1f, 1.1f, 0.8f };
    constexpr sgct::vec4 ColorSyncTime = sgct::vec4{ 0.1f, 1.f, 1.f, 0.8f };
    constexpr sgct::vec4 ColorLoopTimeMin = sgct::vec4{ 0.4f, 0.4f, 1.f, 0.8f };
    constexpr sgct::vec4 ColorLoopTimeMax = sgct::vec4{ 0.15f, 0.15f, 0.8f, 0.8f };

    void check(VkResult result, const char* what) {
        if (result != VK_SUCCESS) {
            throw sgct::Error(
                sgct::Error::Component::Engine,
                4450,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    // The per-draw parameters
    struct Constants {
        sgct::mat4 mvp;
        sgct::vec4 color;
    };

    constexpr const char* StatsVertShader = R"(
#version 460

layout (location = 0) in vec2 in_vertPosition;

layout (push_constant) uniform Constants {
  mat4 mvp;
  vec4 color;
} pc;

void main() { gl_Position = pc.mvp * vec4(in_vertPosition, 0.0, 1.0); }
)";

    constexpr const char* StatsFragShader = R"(
#version 460

layout (push_constant) uniform Constants {
  mat4 mvp;
  vec4 color;
} pc;

layout (location = 0) out vec4 out_color;

void main() { out_color = pc.color; }
)";

    sgct::mat4 toMat4(const glm::mat4& m) {
        sgct::mat4 res;
        std::memcpy(res.values.data(), glm::value_ptr(m), sizeof(sgct::mat4));
        return res;
    }

    // Histogram parameters
    constexpr double HistogramScaleFrame = 35.0 / 1000.0; // 35ms
    constexpr double HistogramScaleSync = 1.0 / 1000.0; // 1ms
} // namespace

namespace sgct {

StatisticsRenderer::StatisticsRenderer(const Engine::Statistics& statistics)
    : _statistics(statistics)
{
    ZoneScoped;

    vulkan::Context& ctx = vulkan::Context::instance();

    VkPushConstantRange pushConstants = {};
    pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstants.offset = 0;
    pushConstants.size = sizeof(Constants);
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstants;
    check(
        vkCreatePipelineLayout(ctx.device(), &layoutInfo, nullptr, &_pipelineLayout),
        "vkCreatePipelineLayout"
    );

    // The static buffer contains the background quad, the 1 ms lines, the 0, 30 & 60
    // FPS lines, and the axes of the histograms
    std::vector<Vertex> vs;
    {
        constexpr float Length = static_cast<float>(Engine::Statistics::HistoryLength);
        // Static background quad
        vs.emplace_back(0.f, 0.f);
        vs.emplace_back(Length, 0.f);
        vs.emplace_back(0.f, 1.f / 30.f);
        vs.emplace_back(Length, 1.f / 30.f);

        // Static 1 ms lines
        _lines.staticDraw.nLines = 0;
        for (float f = 0.001f; f < (1.f / 30.f); f += 0.001f) {
            vs.emplace_back(0.f, f);
            vs.emplace_back(Length, f);
            _lines.staticDraw.nLines++;
        }

        // Static 0, 30 & 60 FPS lines
        vs.emplace_back(0.f, 0.f);
        vs.emplace_back(Length, 0.f);

        vs.emplace_back(0.f, 1.f / 30.f);
        vs.emplace_back(Length, 1.f / 30.f);

        vs.emplace_back(0.f, 1.f / 60.f);
        vs.emplace_back(Length, 1.f / 60.f);
    }
    {
        // Histogram axes
        _histogram.staticDraw.first = static_cast<uint32_t>(vs.size());
        vs.emplace_back(0.f, 0.f);
        vs.emplace_back(1.f, 0.f);
        vs.emplace_back(0.f, 0.f);
        vs.emplace_back(0.f, 1.f);
    }
    _staticBuffer = std::make_unique<vulkan::Buffer>(
        ctx, vs.size() * sizeof(Vertex), vulkan::Buffer::Type::Vertex
    );
    _staticBuffer->upload(vs.data());

    // The dynamic buffers contain the lines of the graphs followed by the histogram bars
    for (std::unique_ptr<vulkan::Buffer>& buffer : _dynamicBuffers) {
        buffer = std::make_unique<vulkan::Buffer>(
            ctx,
            sizeof(Lines::Vertices) + sizeof(Histogram::Vertices),
            vulkan::Buffer::Type::DynamicVertex
        );
        std::memset(buffer->map(), 0, buffer->size());
    }
}

StatisticsRenderer::~StatisticsRenderer() {
    ZoneScoped;

    // The renderer can be removed at runtime while frames that use it are in flight
    vulkan::Context& ctx = vulkan::Context::instance();
    vkDeviceWaitIdle(ctx.device());

    _pipelines.clear();
    vkDestroyPipelineLayout(ctx.device(), _pipelineLayout, nullptr);
}

const StatisticsRenderer::Pipelines& StatisticsRenderer::pipelines(VkRenderPass renderPass,
                                                     const vulkan::SceneFormat& format)
{
    // All render passes with the same format that the statistics are drawn into are
    // compatible, so the pipelines can be shared between them
    Pipelines& p = _pipelines[format];
    if (!p.triangleStrip) {
        vulkan::Context& ctx = vulkan::Context::instance();

        vulkan::Pipeline::Settings settings;
        settings.renderPass = renderPass;
        settings.setSceneFormat(format);
        settings.layout = _pipelineLayout;
        settings.vertexBindings = { { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX } };
        settings.vertexAttributes = { { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 } };
        settings.cullMode = VK_CULL_MODE_NONE;
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) of the OpenGL path, which
        // applies to the alpha channel as well
        settings.blend = true;
        settings.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        settings.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        auto create = [&](VkPrimitiveTopology topology) {
            settings.topology = topology;
            return std::make_unique<vulkan::Pipeline>(
                ctx, StatsVertShader, StatsFragShader, settings
            );
        };
        p.triangleStrip = create(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        p.triangleList = create(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        p.lineList = create(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        p.lineStrip = create(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
    }
    return p;
}

void StatisticsRenderer::draw(VkCommandBuffer cmd, const vulkan::Pipeline& pipeline,
                              VkBuffer buffer, const vec4& color, const mat4& mvp,
                              uint32_t first, uint32_t count) const
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle());
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &buffer, &offset);

    Constants constants;
    constants.mvp = mvp;
    constants.color = color;
    vkCmdPushConstants(
        cmd,
        _pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(Constants),
        &constants
    );
    vkCmdDraw(cmd, count, 1, first, 0);
}

void StatisticsRenderer::update() {
    ZoneScoped;

    // Lines rendering
    // The statistics are stored as 1D double arrays, but we need 2D float arrays
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; i++) {
        _lines.buffer.frametimes[i].x = static_cast<float>(i);
        _lines.buffer.frametimes[i].y = static_cast<float>(_statistics.frametimes[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; i++) {
        _lines.buffer.drawTimes[i].x = static_cast<float>(i);
        _lines.buffer.drawTimes[i].y = static_cast<float>(_statistics.drawTimes[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; i++) {
        _lines.buffer.syncTimes[i].x = static_cast<float>(i);
        _lines.buffer.syncTimes[i].y = static_cast<float>(_statistics.syncTimes[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; i++) {
        _lines.buffer.loopTimeMin[i].x = static_cast<float>(i);
        _lines.buffer.loopTimeMin[i].y = static_cast<float>(_statistics.loopTimeMin[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; i++) {
        _lines.buffer.loopTimeMax[i].x = static_cast<float>(i);
        _lines.buffer.loopTimeMax[i].y = static_cast<float>(_statistics.loopTimeMax[i]);
    }



    // Histogram update
    auto updateHist = [](std::array<int, Histogram::Bins>& hValues,
                         const std::array<double, Histogram::Bins>& sValues, double scale)
    {
        std::fill(hValues.begin(), hValues.end(), 0);

        for (const double d : sValues) {
            // Convert from d into [0, 1];  0 for d=0  and 1 for d=MaxHistogramValue
            const double dp = d / scale;
            const int dpScaled = static_cast<int>(dp * Histogram::Bins);
            const int bin = std::clamp(dpScaled, 0, Histogram::Bins - 1);
            hValues[bin] += 1;
        }

        return *std::max_element(hValues.cbegin(), hValues.cend());
    };
    auto& h = _histogram;
    h.maxBinValue.frametimes =
        updateHist(h.values.frametimes, _statistics.frametimes, HistogramScaleFrame);
    h.maxBinValue.drawTimes =
        updateHist(h.values.drawTimes, _statistics.drawTimes, HistogramScaleFrame);
    h.maxBinValue.syncTimes =
        updateHist(h.values.syncTimes, _statistics.syncTimes, HistogramScaleSync);
    h.maxBinValue.loopTimeMin =
        updateHist(h.values.loopTimeMin, _statistics.loopTimeMin, HistogramScaleSync);
    h.maxBinValue.loopTimeMax =
        updateHist(h.values.loopTimeMax, _statistics.loopTimeMax, HistogramScaleSync);


    auto convertValues = [&](std::array<Vertex, 6 * Histogram::Bins>& buffer,
                            const std::array<int, Histogram::Bins>& values, int maxBinVal)
    {
        for (size_t i = 0; i < static_cast<size_t>(Histogram::Bins); i++) {
            const int val = values[i];

            const float x0 = static_cast<float>(i) / Histogram::Bins;
            const float x1 = static_cast<float>(i + 1) / Histogram::Bins;
            const float y0 = 0.f;
            const float y1 = static_cast<float>(val) / maxBinVal;

            const size_t idx = i * 6;
            buffer[idx + 0] = { x0, y0 };
            buffer[idx + 1] = { x1, y1 };
            buffer[idx + 2] = { x0, y1 };

            buffer[idx + 3] = { x0, y0 };
            buffer[idx + 4] = { x1, y0 };
            buffer[idx + 5] = { x1, y1 };
        }
    };

    convertValues(h.buffer.frametimes, h.values.frametimes, h.maxBinValue.frametimes);
    convertValues(h.buffer.drawTimes, h.values.drawTimes, h.maxBinValue.drawTimes);
    convertValues(h.buffer.syncTimes, h.values.syncTimes, h.maxBinValue.syncTimes);
    convertValues(h.buffer.loopTimeMin, h.values.loopTimeMin, h.maxBinValue.loopTimeMin);
    convertValues(h.buffer.loopTimeMax, h.values.loopTimeMax, h.maxBinValue.loopTimeMax);

    // Frames in flight might still read the previously written buffers. The buffer of
    // frame N is overwritten in frame N + DynamicBufferCount, by when every window has
    // waited for its submission of frame N + 1, the last one that reads it
    _currentDynamicBuffer = static_cast<int>(
        Engine::instance().currentFrameNumber() % DynamicBufferCount
    );
    std::byte* dst = static_cast<std::byte*>(_dynamicBuffers[_currentDynamicBuffer]->map());
    std::memcpy(dst, &_lines.buffer, sizeof(Lines::Vertices));
    std::memcpy(
        dst + sizeof(Lines::Vertices),
        &_histogram.buffer,
        sizeof(Histogram::Vertices)
    );
}

void StatisticsRenderer::render(const Window& window, const Viewport& viewport) {
    ZoneScoped;

    VkCommandBuffer cmd = window.currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        return;
    }
    const Pipelines& p = pipelines(window.renderPass(), window.sceneFormat());
    const VkBuffer staticBuffer = _staticBuffer->handle();
    const VkBuffer dynamicBuffer = _dynamicBuffers[_currentDynamicBuffer]->handle();

    const ivec2 res = window.framebufferResolution();
    const glm::vec2 scaleOffset = glm::vec2(res.x / 2.f, res.y / 2.f);

    glm::mat4 orthoMat = glm::ortho(
        0.f,
        static_cast<float>(res.x),
        0.f,
        static_cast<float>(res.y)
    );
    orthoMat = glm::scale(orthoMat, glm::vec3(_scale, _scale, 1.f));
    orthoMat = glm::translate(
        orthoMat,
        // The extra `/ _scale` in this calculation is due to the glm::scale just above
        // which means that every translation is scaled as well. Moving the division into
        // the parantheses would make it harder to understand as we don't really care too
        // much about the performance here
        glm::vec3(
            scaleOffset.x * (1.f - _scale) / _scale,
            scaleOffset.y * (1.f - _scale) / _scale,
            0.f
        )
    );

    const glm::vec2 penPosition = glm::vec2(
        15.f * _scale + scaleOffset.x * (1.f - _scale) + _offset.x * res.x * _scale,
        10.f * _scale + scaleOffset.y * (1.f - _scale) + _offset.y * res.y * _scale
    );
    const float penOffset = 20.f * _scale;


    {
        //
        // Render lines
        //
        ZoneScopedN("Lines");

        const glm::vec2 size = glm::vec2(
            res.x / static_cast<float>(Engine::Statistics::HistoryLength),
            res.y * 15.f
        );


        glm::mat4 m = glm::translate(orthoMat, glm::vec3(0.f, 250.f * _scale, 0.f));
        m = glm::translate(m, glm::vec3(_offset.x * res.x, _offset.y * res.y, 0.f));
        m = glm::scale(m, glm::vec3(size.x, size.y, 1.f));
        const mat4 mvp = toMat4(m);
        const uint32_t nLines = static_cast<uint32_t>(_lines.staticDraw.nLines);

        // Draw background (1024x1024 canvas)
        draw(cmd, *p.triangleStrip, staticBuffer, ColorStaticBackground, mvp, 0, 4);

        // 1 ms lines
        draw(cmd, *p.lineList, staticBuffer, ColorStaticGrid, mvp, 4, nLines * 2);

        // Zero line, 60hz & 30hz
        draw(
            cmd, *p.lineList, staticBuffer, ColorStaticFrequency, mvp, 4 + nLines * 2, 6
        );

        // Frametime
        constexpr uint32_t StatsLength = Engine::Statistics::HistoryLength;
        draw(
            cmd, *p.lineStrip, dynamicBuffer, ColorFrameTime, mvp,
            0 * StatsLength, StatsLength
        );

        // Drawtime
        draw(
            cmd, *p.lineStrip, dynamicBuffer, ColorDrawTime, mvp,
            1 * StatsLength, StatsLength
        );

        // Synctime
        draw(
            cmd, *p.lineStrip, dynamicBuffer, ColorSyncTime, mvp,
            2 * StatsLength, StatsLength
        );

        // Looptimemin
        draw(
            cmd, *p.lineStrip, dynamicBuffer, ColorLoopTimeMin, mvp,
            3 * StatsLength, StatsLength
        );

        // Looptimemax
        draw(
            cmd, *p.lineStrip, dynamicBuffer, ColorLoopTimeMax, mvp,
            4 * StatsLength, StatsLength
        );

#ifdef SGCT_HAS_TEXT
        constexpr text::Alignment mode = text::Alignment::TopLeft;

        const int f1Size = static_cast<int>(20 * _scale);
        const int f2Size = static_cast<int>(12 * _scale);
        text::Font& f1 = *text::FontManager::instance().font("SGCTFont", f1Size);
        text::Font& f2 = *text::FontManager::instance().font("SGCTFont", f2Size);

        text::print(
            window,
            viewport,
            f1,
            mode,
            penPosition.x, penPosition.y + 9 * penOffset,
            vec4{ 1.f, 0.8f, 0.8f, 1.f },
            std::format("Frame number: {}", Engine::instance().currentFrameNumber())
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            penPosition.x, penPosition.y + 6 * penOffset,
            ColorFrameTime,
            std::format("Frame time: {} ms", _statistics.frametimes[0] * 1000.0)
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            penPosition.x, penPosition.y + 5 * penOffset,
            ColorDrawTime,
            std::format("Draw time: {} ms", _statistics.drawTimes[0] * 1000.0)
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            penPosition.x, penPosition.y + 4 * penOffset,
            ColorSyncTime,
            std::format("Sync time: {} ms", _statistics.syncTimes[0] * 1000.0)
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            penPosition.x, penPosition.y + 3 * penOffset,
            ColorLoopTimeMin,
            std::format("Min Loop time: {} ms", _statistics.loopTimeMin[0] * 1000.0)
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            penPosition.x, penPosition.y + 2 * penOffset,
            ColorLoopTimeMax,
            std::format("Max Loop time: {} ms", _statistics.loopTimeMax[0] * 1000.0)
        );
#endif // SGCT_HAS_TEXT
    }

    {
        //
        // Render Histogram
        //

        ZoneScopedN("Histogram");

        auto renderHistogram = [&](int i, const vec4& color) {
            const auto [pos, size] = [&](int j) -> std::tuple<glm::vec2, glm::vec2> {
                const glm::vec2 p = glm::vec2(
                    400.f * _scale + _offset.x * res.x,
                    10.f * _scale + _offset.y * res.y
                );
                const glm::vec2 s = glm::vec2(425.f, 200.f) * _scale;

                if (j == 0) {
                    // Full size
                    return { p, s };
                }
                else {
                    // Half size in a grid
                    const int idx = j - 1;
                    const glm::vec2 tSize = s / 2.f;
                    const float iMod = static_cast<float>(idx % 2);
                    const float iDiv = static_cast<float>(idx / 2);
                    const glm::vec2 offset = glm::vec2(
                        (tSize.x + 10.f) * iMod,
                        (tSize.y + 10.f) * iDiv
                    );
                    return { p + offset + glm::vec2(s.x + 10.f, 0.f), tSize };
                }
            }(i);

            glm::mat4 m = glm::translate(orthoMat, glm::vec3(pos.x, pos.y, 0.f));
            m = glm::scale(m, glm::vec3(size.x, size.y, 1.f));
            const mat4 mvp = toMat4(m);

            draw(
                cmd, *p.lineList, staticBuffer, vec4{ 1.f, 1.f, 1.f, 1.f }, mvp,
                _histogram.staticDraw.first, 4
            );

            // The histogram bars follow the lines in the dynamic buffer
            constexpr uint32_t HistogramFirst = sizeof(Lines::Vertices) / sizeof(Vertex);
            constexpr uint32_t BinVertices = 6 * Histogram::Bins;
            draw(
                cmd, *p.triangleList, dynamicBuffer, color, mvp,
                HistogramFirst + static_cast<uint32_t>(i) * BinVertices, BinVertices
            );
        };

        renderHistogram(0, ColorFrameTime);
        renderHistogram(1, ColorDrawTime);
        renderHistogram(2, ColorSyncTime);
        renderHistogram(3, ColorLoopTimeMin);
        renderHistogram(4, ColorLoopTimeMax);

#ifdef SGCT_HAS_TEXT
        constexpr text::Alignment mode = text::Alignment::TopLeft;

        const int fontSize = static_cast<int>(8 * _scale);
        text::Font& f = *text::FontManager::instance().font("SGCTFont", fontSize);
        text::print(
            window,
            viewport,
            f,
            mode,
            penPosition.x, penPosition.y + penOffset,
            vec4{ 0.8f, 0.8f, 0.8f, 1.f },
            std::format(
                "Histogram Scale (sync time): {:.0f} ms",
                HistogramScaleSync * 1000.0
            )
        );
        text::print(
            window,
            viewport,
            f,
            mode,
            penPosition.x, penPosition.y,
            vec4{ 0.8f, 0.8f, 0.8f, 1.f },
            std::format(
                "Histogram Scale (frametime, drawtime): {:.0f} ms",
                HistogramScaleFrame * 1000.0
            )
        );
#endif // SGCT_HAS_TEXT
    }
}

float StatisticsRenderer::scale() const {
    return _scale;
}

void StatisticsRenderer::setScale(float scale) {
    _scale = scale;
}

vec2 StatisticsRenderer::offset() const {
    return _offset;
}

void StatisticsRenderer::setOffset(vec2 offset) {
    _offset = std::move(offset);
}

} // namespace sgct
