/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/statisticsrenderer.h>

#include <sgct/opengl.h>
#include <sgct/profiling.h>
#ifdef SGCT_HAS_TEXT
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace {
    // Line parameters
    constexpr sgct::vec4 ColorStaticGrid = sgct::vec4{ 1.f, 1.f, 1.f, 0.2f };
    constexpr sgct::vec4 ColorStaticFrequency = sgct::vec4{ 1.f, 0.f, 0.f, 1.f };
    constexpr sgct::vec4 ColorStaticBackground = sgct::vec4{ 0.f, 0.f, 0.f, 0.5f };

    constexpr sgct::vec4 ColorFrameTime = sgct::vec4{ 1.f, 1.f, 0.f, 0.8f };
    constexpr sgct::vec4 ColorDrawTime = sgct::vec4{ 1.f, 0.1f, 1.1f, 0.8f };
    constexpr sgct::vec4 ColorSyncTime = sgct::vec4{ 0.1f, 1.f, 1.f, 0.8f };
    constexpr sgct::vec4 ColorLoopTimeMin = sgct::vec4{ 0.4f, 0.4f, 1.f, 0.8f };
    constexpr sgct::vec4 ColorLoopTimeMax = sgct::vec4{ 0.15f, 0.15f, 0.8f, 0.8f };

    constexpr std::string_view StatsVertShader = R"(
#version 330 core
layout (location = 0) in vec2 in_vertPosition;
uniform mat4 mvp;
void main() { gl_Position = mvp * vec4(in_vertPosition, 0.0, 1.0); }
)";

    constexpr std::string_view StatsFragShader = R"(
#version 330 core
uniform vec4 col;
out vec4 out_color;
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

    // Static background quad
    struct sVertex {
        sVertex(float x_, float y_) : x(x_), y(y_) {}
        float x = 0.f;
        float y = 0.f;
    };
    std::vector<sVertex> vs;
    vs.emplace_back(0.f, 0.f);
    vs.emplace_back(static_cast<float>(_statistics.HistoryLength), 0.f);
    vs.emplace_back(0.f, 1.f / 30.f);
    vs.emplace_back(static_cast<float>(_statistics.HistoryLength), 1.f / 30.f);

    // Static 1 ms lines
    _lines.staticDraw.nLines = 0;
    for (float f = 0.001f; f < (1.f / 30.f); f += 0.001f) {
        vs.emplace_back(0.f, f);
        vs.emplace_back(static_cast<float>(_statistics.HistoryLength), f);
        _lines.staticDraw.nLines++;
    }

    // Static 0, 30 & 60 FPS lines
    vs.emplace_back(0.f, 0.f);
    vs.emplace_back(static_cast<float>(_statistics.HistoryLength), 0.f);

    vs.emplace_back(0.f, 1.f / 30.f);
    vs.emplace_back(static_cast<float>(_statistics.HistoryLength), 1.f / 30.f);

    vs.emplace_back(0.f, 1.f / 60.f);
    vs.emplace_back(static_cast<float>(_statistics.HistoryLength), 1.f / 60.f);

    // Setup shaders
    _shader = ShaderProgram("General Statistics Shader");
    _shader.addShaderSource(StatsVertShader, GL_VERTEX_SHADER);
    _shader.addShaderSource(StatsFragShader, GL_FRAGMENT_SHADER);
    _shader.createAndLinkProgram();
    _shader.bind();
    _mvpLoc = glGetUniformLocation(_shader.id(), "mvp");
    _colorLoc = glGetUniformLocation(_shader.id(), "col");
    _shader.unbind();

    // OpenGL objects for lines
    glGenVertexArrays(1, &_lines.staticDraw.vao);
    glGenBuffers(1, &_lines.staticDraw.vbo);
    glBindVertexArray(_lines.staticDraw.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _lines.staticDraw.vbo);
    glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(sVertex), vs.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenVertexArrays(1, &_lines.dynamicDraw.vao);
    glGenBuffers(1, &_lines.dynamicDraw.vbo);
    glBindVertexArray(_lines.dynamicDraw.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _lines.dynamicDraw.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Engine::Statistics), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    // OpenGL objects for histogram
    glGenVertexArrays(1, &_histogram.staticDraw.vao);
    glGenBuffers(1, &_histogram.staticDraw.vbo);
    glBindVertexArray(_histogram.staticDraw.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _histogram.staticDraw.vbo);
    constexpr std::array<float, 8> HistogramAxisVertices = {
        0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f
    };
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(HistogramAxisVertices),
        HistogramAxisVertices.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenVertexArrays(1, &_histogram.dynamicDraw.vao);
    glGenBuffers(1, &_histogram.dynamicDraw.vbo);
    glBindVertexArray(_histogram.dynamicDraw.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _histogram.dynamicDraw.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Histogram::Vertices),
        nullptr,
        GL_DYNAMIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
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

    glBindBuffer(GL_ARRAY_BUFFER, _lines.dynamicDraw.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Lines::Vertices),
        &_lines.buffer.frametimes,
        GL_DYNAMIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    // Histogram update
    auto updateHist = [](std::array<int, Histogram::Bins>& hValues,
                         const std::array<double, Histogram::Bins>& sValues, double scale)
    {
        std::fill(hValues.begin(), hValues.end(), 0);

        for (double d : sValues) {
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


    glBindBuffer(GL_ARRAY_BUFFER, _histogram.dynamicDraw.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Histogram::Vertices),
        _histogram.buffer.frametimes.data(),
        GL_DYNAMIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void StatisticsRenderer::render(const Window& window, const Viewport& viewport) {
    ZoneScoped;

    ivec2 res = window.framebufferResolution();
    const glm::mat4 orthoMat = glm::ortho(
        0.f,
        static_cast<float>(res.x),
        0.f,
        static_cast<float>(res.y)
    );

    glLineWidth(1.f);

    {
        //
        // Render lines
        //
        ZoneScopedN("Lines");

        _shader.bind();

        const glm::vec2 size = glm::vec2(
            res.x / static_cast<float>(_statistics.HistoryLength),
            res.y * 15.f
        );


        glm::mat4 m = glm::translate(orthoMat, glm::vec3(0.f, 250.f, 0.f));
        m = glm::scale(m, glm::vec3(size.x, size.y, 1.f));
        glUniformMatrix4fv(_mvpLoc, 1, GL_FALSE, glm::value_ptr(m));

        glBindVertexArray(_lines.staticDraw.vao);

        // draw background (1024x1024 canvas)
        glUniform4fv(_colorLoc, 1, &ColorStaticBackground.x);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // 1 ms lines
        glUniform4fv(_colorLoc, 1, &ColorStaticGrid.x);
        glDrawArrays(GL_LINES, 4, _lines.staticDraw.nLines * 2);

        // zero line, 60hz & 30hz
        glUniform4fv(_colorLoc, 1, &ColorStaticFrequency.x);
        glDrawArrays(GL_LINES, 4 + _lines.staticDraw.nLines * 2, 6);

        glBindVertexArray(_lines.dynamicDraw.vao);

        // frametime
        constexpr int StatsLength = Engine::Statistics::HistoryLength;
        glUniform4fv(_colorLoc, 1, &ColorFrameTime.x);
        glDrawArrays(GL_LINE_STRIP, 0 * StatsLength, StatsLength);

        // drawtime
        glUniform4fv(_colorLoc, 1, &ColorDrawTime.x);
        glDrawArrays(GL_LINE_STRIP, 1 * StatsLength, StatsLength);

        // synctime
        glUniform4fv(_colorLoc, 1, &ColorSyncTime.x);
        glDrawArrays(GL_LINE_STRIP, 2 * StatsLength, StatsLength);

        // looptimemin
        glUniform4fv(_colorLoc, 1, &ColorLoopTimeMin.x);
        glDrawArrays(GL_LINE_STRIP, 3 * StatsLength, StatsLength);

        // looptimemax
        glUniform4fv(_colorLoc, 1, &ColorLoopTimeMax.x);
        glDrawArrays(GL_LINE_STRIP, 4 * StatsLength, StatsLength);

        glBindVertexArray(0);
        _shader.unbind();

#ifdef SGCT_HAS_TEXT
        constexpr glm::vec2 Pos = glm::vec2(15.f, 50.f);
        constexpr float Offset = 20.f;
        constexpr text::Alignment mode = text::Alignment::TopLeft;

        text::Font& f1 = *text::FontManager::instance().font("SGCTFont", 20);
        text::Font& f2 = *text::FontManager::instance().font("SGCTFont", 12);

        text::print(
            window,
            viewport,
            f1,
            mode,
            Pos.x, Pos.y + 7 * Offset,
            vec4{ 1.f, 0.8f, 0.8f, 1.f },
            fmt::format("Frame number: {}", Engine::instance().currentFrameNumber())
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            Pos.x, Pos.y + 4 * Offset,
            ColorFrameTime,
            fmt::format("Frame time: {} ms", _statistics.frametimes[0] * 1000.0)
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            Pos.x, Pos.y + 3 * Offset,
            ColorDrawTime,
            fmt::format("Draw time: {} ms", _statistics.drawTimes[0] * 1000.0)
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            Pos.x, Pos.y + 2 * Offset,
            ColorSyncTime,
            fmt::format("Sync time: {} ms", _statistics.syncTimes[0] * 1000.0)
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            Pos.x, Pos.y + Offset,
            ColorLoopTimeMin,
            fmt::format("Min Loop time: {} ms", _statistics.loopTimeMin[0] * 1000.0)
        );
        text::print(
            window,
            viewport,
            f2,
            mode,
            Pos.x, Pos.y,
            ColorLoopTimeMax,
            fmt::format("Max Loop time: {} ms", _statistics.loopTimeMax[0] * 1000.0)
        );
#endif // SGCT_HAS_TEXT
    }

    {
        //
        // Render Histogram
        //

        ZoneScopedN("Histogram");

        auto renderHistogram = [&](int i, const vec4& color) {
            const auto [pos, size] = [](int j) -> std::tuple<glm::vec2, glm::vec2> {
                constexpr glm::vec2 Pos(400.f, 10.f);
                constexpr glm::vec2 Size(425.f, 200.f);

                if (j == 0) {
                    // Full size
                    return { Pos, Size };
                }
                else {
                    // Half size in a grid
                    const int idx = j - 1;
                    const glm::vec2 tSize = Size / 2.f;
                    const float iMod = static_cast<float>(idx % 2);
                    const float iDiv = static_cast<float>(idx / 2);
                    const glm::vec2 offset = glm::vec2(
                        (tSize.x + 10.f) * iMod,
                        (tSize.y + 10.f) * iDiv
                    );
                    const glm::vec2 p = Pos + offset;
                    return { p + glm::vec2(Size.x + 10.f, 0.f), tSize };
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
        constexpr glm::vec2 Pos = glm::vec2(15.f, 10.f);
        constexpr text::Alignment mode = text::Alignment::TopLeft;

        text::Font& f = *text::FontManager::instance().font("SGCTFont", 8);
        text::print(
            window,
            viewport,
            f,
            mode,
            Pos.x, Pos.y,
            vec4{ 0.8f, 0.8f, 0.8f, 1.f },
            fmt::format(
                "Histogram Scale (frametime, drawtime): {:.0f} ms",
                HistogramScaleFrame * 1000.0
            )
        );
        text::print(
            window,
            viewport,
            f,
            mode,
            Pos.x, Pos.y + 12.f,
            vec4{ 0.8f, 0.8f, 0.8f, 1.f },
            fmt::format(
                "Histogram Scale (sync time): {:.0f} ms",
                HistogramScaleSync * 1000.0
            )
        );
#endif // SGCT_HAS_TEXT
    }
}

} // namespace sgct
