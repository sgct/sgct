/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/statisticsrenderer.h>

#include <sgct/format.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/shaderprogram.h>
#include <sgct/viewport.h>
#include <sgct/window.h>
#ifdef SGCT_HAS_TEXT
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT
#include <glad/glad.h>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
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

    constexpr std::string_view StatsVertShader = R"(
#version 460 core

layout (location = 0) in vec2 in_vertPosition;

uniform mat4 mvp;


void main() { gl_Position = mvp * vec4(in_vertPosition, 0.0, 1.0); }
)";

    constexpr std::string_view StatsFragShader = R"(
#version 460 core

out vec4 out_color;

uniform vec4 col;


void main() { out_color = col; }
)";

    // Histogram parameters
    constexpr double HistogramScaleFrame = 35.0 / 1000.0; // 35ms
    constexpr double HistogramScaleSync = 1.0 / 1000.0; // 1ms
} // namespace

namespace sgct {

StatisticsRenderer::StatisticsRenderer(const Engine::Statistics& statistics)
    : _statistics(statistics)
{
    ZoneScoped;

    // Setup shaders
    _shader = ShaderProgram("General Statistics Shader");
    _shader.addVertexShader(StatsVertShader);
    _shader.addFragmentShader(StatsFragShader);
    _shader.createAndLinkProgram();
    _mvpLoc = glGetUniformLocation(_shader.id(), "mvp");
    _colorLoc = glGetUniformLocation(_shader.id(), "col");

    {
        unsigned int& vao = _lines.staticDraw.vao;
        unsigned int& vbo = _lines.staticDraw.vbo;

        constexpr float Length = static_cast<float>(Engine::Statistics::HistoryLength);
        // Static background quad
        std::vector<Vertex> vs;
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

        glCreateBuffers(1, &_lines.staticDraw.vbo);
        glNamedBufferStorage(vbo, vs.size() * sizeof(Vertex), vs.data(), GL_NONE_BIT);

        glCreateVertexArrays(1, &vao);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));

        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
    }

    {
        unsigned int& vao = _lines.dynamicDraw.vao;
        unsigned int& vbo = _lines.dynamicDraw.vbo;

        glCreateBuffers(1, &vbo);
        glNamedBufferStorage(
            vbo,
            sizeof(StatisticsRenderer::Lines::Vertices),
            nullptr,
            GL_DYNAMIC_STORAGE_BIT
        );

        glCreateVertexArrays(1, &vao);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));

        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
    }

    {
        unsigned int& vao = _histogram.staticDraw.vao;
        unsigned int& vbo = _histogram.staticDraw.vbo;

        glCreateBuffers(1, &vbo);
        constexpr std::array<Vertex, 4> Vertices = {
            Vertex{ 0.f, 0.f }, Vertex{ 1.f, 0.f }, Vertex{ 0.f, 0.f }, Vertex{ 0.f, 1.f }
        };
        glNamedBufferStorage(vbo, 4 * sizeof(Vertex), Vertices.data(), GL_NONE_BIT);

        glCreateVertexArrays(1, &vao);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));

        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
    }

    {
        unsigned int& vao = _histogram.dynamicDraw.vao;
        unsigned int& vbo = _histogram.dynamicDraw.vbo;

        glCreateBuffers(1, &vbo);
        glNamedBufferStorage(
            vbo,
            sizeof(Histogram::Vertices),
            nullptr,
            GL_DYNAMIC_STORAGE_BIT
        );

        glCreateVertexArrays(1, &vao);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));

        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
    }
}

StatisticsRenderer::~StatisticsRenderer() {
    ZoneScoped;

    _shader.deleteProgram();

    glDeleteVertexArrays(1, &_lines.staticDraw.vao);
    glDeleteBuffers(1, &_lines.staticDraw.vbo);
    glDeleteVertexArrays(1, &_lines.dynamicDraw.vao);
    glDeleteBuffers(1, &_lines.dynamicDraw.vbo);

    glDeleteVertexArrays(1, &_histogram.staticDraw.vao);
    glDeleteBuffers(1, &_histogram.staticDraw.vbo);
    glDeleteVertexArrays(1, &_histogram.dynamicDraw.vao);
    glDeleteBuffers(1, &_histogram.dynamicDraw.vbo);
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

    glNamedBufferSubData(
        _lines.dynamicDraw.vbo,
        0,
        sizeof(Lines::Vertices),
        &_lines.buffer.frametimes
    );


    // Histogram update
    auto updateHist = [](std::array<int, Histogram::Bins>& hValues,
                         const std::array<double, Histogram::Bins>& sValues, double scale)
    {
        std::fill(hValues.begin(), hValues.end(), 0);

        for (const double d : sValues) {
            // convert from d into [0, 1];  0 for d=0  and 1 for d=MaxHistogramValue
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

    glNamedBufferSubData(
        _histogram.dynamicDraw.vbo,
        0,
        sizeof(Histogram::Vertices),
        _histogram.buffer.frametimes.data()
    );
}

void StatisticsRenderer::render(const Window& window, const Viewport& viewport) const {
    ZoneScoped;

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


    glLineWidth(1.f);

    {
        //
        // Render lines
        //
        ZoneScopedN("Lines");

        _shader.bind();

        const glm::vec2 size = glm::vec2(
            res.x / static_cast<float>(Engine::Statistics::HistoryLength),
            res.y * 15.f
        );


        glm::mat4 m = glm::translate(orthoMat, glm::vec3(0.f, 250.f * _scale, 0.f));
        m = glm::translate(m, glm::vec3(_offset.x * res.x, _offset.y * res.y, 0.f));
        m = glm::scale(m, glm::vec3(size.x, size.y, 1.f));
        glProgramUniformMatrix4fv(_shader.id(), _mvpLoc, 1, GL_FALSE, glm::value_ptr(m));

        glBindVertexArray(_lines.staticDraw.vao);

        // draw background (1024x1024 canvas)
        glProgramUniform4fv(_shader.id(), _colorLoc, 1, &ColorStaticBackground.x);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // 1 ms lines
        glProgramUniform4fv(_shader.id(), _colorLoc, 1, &ColorStaticGrid.x);
        glDrawArrays(GL_LINES, 4, _lines.staticDraw.nLines * 2);

        // zero line, 60hz & 30hz
        glProgramUniform4fv(_shader.id(), _colorLoc, 1, &ColorStaticFrequency.x);
        glDrawArrays(GL_LINES, 4 + _lines.staticDraw.nLines * 2, 6);

        glBindVertexArray(_lines.dynamicDraw.vao);

        // frametime
        constexpr int StatsLength = Engine::Statistics::HistoryLength;
        glProgramUniform4fv(_shader.id(), _colorLoc, 1, &ColorFrameTime.x);
        glDrawArrays(GL_LINE_STRIP, 0 * StatsLength, StatsLength);

        // drawtime
        glProgramUniform4fv(_shader.id(), _colorLoc, 1, &ColorDrawTime.x);
        glDrawArrays(GL_LINE_STRIP, 1 * StatsLength, StatsLength);

        // synctime
        glProgramUniform4fv(_shader.id(), _colorLoc, 1, &ColorSyncTime.x);
        glDrawArrays(GL_LINE_STRIP, 2 * StatsLength, StatsLength);

        // looptimemin
        glProgramUniform4fv(_shader.id(), _colorLoc, 1, &ColorLoopTimeMin.x);
        glDrawArrays(GL_LINE_STRIP, 3 * StatsLength, StatsLength);

        // looptimemax
        glProgramUniform4fv(_shader.id(), _colorLoc, 1, &ColorLoopTimeMax.x);
        glDrawArrays(GL_LINE_STRIP, 4 * StatsLength, StatsLength);

        glBindVertexArray(0);
        ShaderProgram::unbind();

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
            glUniformMatrix4fv(_mvpLoc, 1, GL_FALSE, glm::value_ptr(m));
            glUniform4fv(_colorLoc, 1, glm::value_ptr(glm::vec4(1.f)));

            glBindVertexArray(_histogram.staticDraw.vao);
            glDrawArrays(GL_LINES, 0, 4);

            glBindVertexArray(_histogram.dynamicDraw.vao);
            glUniform4fv(_colorLoc, 1, &color.x);
            glDrawArrays(GL_TRIANGLES, i * 6 * Histogram::Bins, 6 * Histogram::Bins);
        };

        _shader.bind();
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
