/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/statisticsrenderer.h>

#ifdef SGCT_HAS_TEXT
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    // Line parameters
    const glm::vec4 ColorStaticGrid = glm::vec4(1.f, 1.f, 1.f, 0.2f);
    const glm::vec4 ColorStaticFrequency = glm::vec4(1.f, 0.f, 0.f, 1.f);
    const glm::vec4 ColorStaticBackground = glm::vec4(0.f, 0.f, 0.f, 0.5f);

    const glm::vec4 ColorFrameTime = glm::vec4(1.f, 1.f, 0.f, 0.8f);
    const glm::vec4 ColorDrawTime = glm::vec4(1.f, 0.1f, 1.1f, 0.8f);
    const glm::vec4 ColorSyncTime = glm::vec4(0.1f, 1.f, 1.f, 0.8f);
    const glm::vec4 ColorLoopTimeMax = glm::vec4(0.4f, 0.4f, 1.f, 0.8f);
    const glm::vec4 ColorLoopTimeMin = glm::vec4(0.15f, 0.15f, 0.8f, 0.8f);

    constexpr const char* StatsVertShader = R"(
#version 330 core
layout (location = 0) in vec2 in_vertPosition;
uniform mat4 mvp;
void main() { gl_Position = mvp * vec4(in_vertPosition, 0.0, 1.0); }
)";

    constexpr const char* StatsFragShader = R"(
#version 330 core

uniform vec4 col;
out vec4 out_color;

void main() { out_color = col; }
)";

    // Histogram parameters
    constexpr const double MaxHistogramValue = 20.0 / 1000.0; // 20ms
} // namespace

namespace sgct::core {

StatisticsRenderer::StatisticsRenderer(const Engine::Statistics& statistics)
    : _statistics(statistics)
{
    // Static background quad
    std::vector<float> staticVerts;
    staticVerts.push_back(0.f);
    staticVerts.push_back(0.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(0.f);
    staticVerts.push_back(0.f);
    staticVerts.push_back(1.f / 30.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(1.f / 30.f);

    // Static 1 ms lines
    _lines.staticDraw.nLines = 0;
    for (float f = 0.001f; f < (1.f / 30.f); f += 0.001f) {
        staticVerts.push_back(0.f);
        staticVerts.push_back(f);
        staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
        staticVerts.push_back(f);
        _lines.staticDraw.nLines++;
    }

    // Static 0, 30 & 60 FPS lines
    staticVerts.push_back(0.f);
    staticVerts.push_back(0.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(0.f);

    staticVerts.push_back(0.f);
    staticVerts.push_back(1.f / 30.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(1.f / 30.f);

    staticVerts.push_back(0.f);
    staticVerts.push_back(1.f / 60.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(1.f / 60.f);

    // Setup shaders
    _shader = ShaderProgram("General Statistics Shader");
    _shader.addShaderSource(StatsVertShader, StatsFragShader);
    _shader.createAndLinkProgram();
    _shader.bind();
    _mvpLoc = glGetUniformLocation(_shader.getId(), "mvp");
    _colorLoc = glGetUniformLocation(_shader.getId(), "col");
    _shader.unbind();

    // OpenGL objects for lines
    glGenVertexArrays(1, &_lines.staticDraw.vao);
    glGenBuffers(1, &_lines.staticDraw.vbo);
    glBindVertexArray(_lines.staticDraw.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _lines.staticDraw.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        staticVerts.size() * sizeof(float),
        staticVerts.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenVertexArrays(1, &_lines.dynamicDraw.vao);
    glGenBuffers(1, &_lines.dynamicDraw.vbo);
    glBindVertexArray(_lines.dynamicDraw.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _lines.dynamicDraw.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        5 * _statistics.HistoryLength * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    // OpenGL objects for histogram
    glGenVertexArrays(1, &_histogram.staticDraw.vao);
    glGenBuffers(1, &_histogram.staticDraw.vbo);
    glBindVertexArray(_histogram.staticDraw.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _histogram.staticDraw.vbo);
    constexpr const std::array<float, 8> HistogramAxisVertices = {
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
    // Lines rendering
    // The statistics are stored as 1D double arrays, but we need 2D float arrays
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
        _lines.buffer.frametimes[i].x = static_cast<float>(i);
        _lines.buffer.frametimes[i].y = static_cast<float>(_statistics.frametimes[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
        _lines.buffer.drawTimes[i].x = static_cast<float>(i);
        _lines.buffer.drawTimes[i].y = static_cast<float>(_statistics.drawTimes[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
        _lines.buffer.syncTimes[i].x = static_cast<float>(i);
        _lines.buffer.syncTimes[i].y = static_cast<float>(_statistics.syncTimes[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
        _lines.buffer.loopTimeMin[i].x = static_cast<float>(i);
        _lines.buffer.loopTimeMin[i].y = static_cast<float>(_statistics.loopTimeMin[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
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
    auto updateHistogram = [](std::array<int, Histogram::Bins>& hValues,
                              const std::array<double, 512>& sValues) -> int
    {
        std::fill(hValues.begin(), hValues.end(), 0);

        for (double d : sValues) {
            // convert from d into [0, 1];  0 for d=0  and 1 for d=MaxHistogramValue
            const double dp = d / MaxHistogramValue;
            const int bin = std::min(static_cast<int>(dp * Histogram::Bins), Histogram::Bins - 1);
            hValues[bin] += 1;
        }

        return *std::max_element(hValues.begin(), hValues.end());
    };
    int f = updateHistogram(_histogram.values.frametimes, _statistics.frametimes);
    int d = updateHistogram(_histogram.values.drawTimes, _statistics.drawTimes);
    int s = updateHistogram(_histogram.values.syncTimes, _statistics.syncTimes);
    int lmin = updateHistogram(_histogram.values.loopTimeMin, _statistics.loopTimeMin);
    int lmax = updateHistogram(_histogram.values.loopTimeMax, _statistics.loopTimeMax);
    _histogram.maxBinValue = std::max({ f, d, s, lmin, lmax });


    auto convertValues = [&](std::array<Vertex, 6 * Histogram::Bins>& buffer,
                            const std::array<int, Histogram::Bins>& values)
    {
        for (int i = 0; i < Histogram::Bins; ++i) {
            const int val = values[i];

            const float x0 = static_cast<float>(i) / Histogram::Bins;
            const float x1 = static_cast<float>(i + 1) / Histogram::Bins;
            const float y0 = 0.f;
            const float y1 = static_cast<float>(val) / _histogram.maxBinValue;

            const int idx = i * 6;
            buffer[idx + 0] = { x0, y0 };
            buffer[idx + 1] = { x1, y1 };
            buffer[idx + 2] = { x0, y1 };

            buffer[idx + 3] = { x0, y0 };
            buffer[idx + 4] = { x1, y0 };
            buffer[idx + 5] = { x1, y1 };
        }
    };

    convertValues(_histogram.buffer.frametimes, _histogram.values.frametimes);
    convertValues(_histogram.buffer.drawTimes, _histogram.values.drawTimes);
    convertValues(_histogram.buffer.syncTimes, _histogram.values.syncTimes);
    convertValues(_histogram.buffer.loopTimeMin, _histogram.values.loopTimeMin);
    convertValues(_histogram.buffer.loopTimeMax, _histogram.values.loopTimeMax);
    

    glBindBuffer(GL_ARRAY_BUFFER, _histogram.dynamicDraw.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Histogram::Vertices),
        _histogram.buffer.frametimes.data(),
        GL_DYNAMIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void StatisticsRenderer::render() {
    const glm::vec2 res = glm::vec2(Engine::instance().getCurrentResolution());
    const glm::mat4 orthoMat = glm::ortho(0.f, res.x, 0.f, res.y);

    {
        //
        // Render lines
        //
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
        glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorStaticBackground));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // 1 ms lines
        glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorStaticGrid));
        glDrawArrays(GL_LINES, 4, _lines.staticDraw.nLines * 2);

        // zero line, 60hz & 30hz
        glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorStaticFrequency));
        glDrawArrays(GL_LINES, 4 + _lines.staticDraw.nLines * 2, 6);

        glBindVertexArray(_lines.dynamicDraw.vao);

        // frametime
        constexpr const int StatsLength = Engine::Statistics::HistoryLength;
        glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorFrameTime));
        glDrawArrays(GL_LINE_STRIP, 0 * StatsLength, StatsLength);

        // drawtime
        glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorDrawTime));
        glDrawArrays(GL_LINE_STRIP, 1 * StatsLength, StatsLength);

        // synctime
        glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorSyncTime));
        glDrawArrays(GL_LINE_STRIP, 2 * StatsLength, StatsLength);

        // looptimemax
        glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorLoopTimeMax));
        glDrawArrays(GL_LINE_STRIP, 3 * StatsLength, StatsLength);

        // looptimemin
        glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorLoopTimeMin));
        glDrawArrays(GL_LINE_STRIP, 4 * StatsLength, StatsLength);

        glBindVertexArray(0);
        _shader.unbind();

#ifdef SGCT_HAS_TEXT
        constexpr const glm::vec2 Pos = glm::vec2(20.f, 50.f);
        constexpr const float Offset = 20.f;
        constexpr const text::TextAlignMode mode = text::TextAlignMode::TopLeft;

        text::Font& f1 = *text::FontManager::instance().getFont("SGCTFont", 22);
        text::Font& f2 = *text::FontManager::instance().getFont("SGCTFont", 12);

        text::print(
            f1,
            mode,
            Pos.x,
            Pos.y + 7 * Offset,
            glm::vec4(1.0f, 0.8f, 0.8f, 1.f),
            "Frame number: %i", Engine::instance().getCurrentFrameNumber()
        );
        text::print(
            f2,
            mode,
            Pos.x,
            Pos.y + 4 * Offset,
            ColorFrameTime,
            "Frame time: %f ms", _statistics.frametimes[0] * 1000.0
        );
        text::print(
            f2,
            mode,
            Pos.x,
            Pos.y + 3 * Offset,
            ColorDrawTime,
            "Draw time: %f ms", _statistics.drawTimes[0] * 1000.0
        );
        text::print(
            f2,
            mode,
            Pos.x,
            Pos.y + 2 * Offset,
            ColorSyncTime,
            "Sync time: %f ms", _statistics.syncTimes[0] * 1000.0
        );
        text::print(
            f2,
            mode,
            Pos.x,
            Pos.y + Offset,
            ColorLoopTimeMin,
            "Min Loop time: %f ms", _statistics.loopTimeMin[0] * 1000.0
        );
        text::print(
            f2,
            mode,
            Pos.x,
            Pos.y,
            ColorLoopTimeMax,
            "Max Loop time: %f ms", _statistics.loopTimeMax[0] * 1000.0
        );
#endif // SGCT_HAS_TEXT
    }

    {
        //
        // Render Histogram
        //
        auto renderHistogram = [&](int i, const glm::vec4& color) {
            const auto [pos, size] = [&](int i) -> std::tuple<glm::vec2, glm::vec2> {
                constexpr const glm::vec2 Pos(375.f, 10.f);
                constexpr const glm::vec2 Size(425.f, 200.f);

                if (i == 0) {
                    // Full size
                    return { Pos, Size };
                }
                else {
                    // Half size in a grid
                    const int idx = i - 1;
                    const glm::vec2 size = Size / 2.f;
                    const float iMod = static_cast<float>(idx % 2);
                    const float iDiv = static_cast<float>(idx / 2);
                    const glm::vec2 offset = glm::vec2(
                        (size.x + 10.f) * iMod,
                        (size.y + 10.f) * iDiv
                    );
                    const glm::vec2 p = Pos + offset;
                    return { p + glm::vec2(Size.x + 10.f, 0.f), size };
                }
            }(i);

            glm::mat4 m = glm::translate(orthoMat, glm::vec3(pos.x, pos.y, 0.f));
            m = glm::scale(m, glm::vec3(size.x, size.y, 1.f));
            glUniformMatrix4fv(_mvpLoc, 1, GL_FALSE, glm::value_ptr(m));
            glUniform4fv(_colorLoc, 1, glm::value_ptr(glm::vec4(1.f)));

            glBindVertexArray(_histogram.staticDraw.vao);
            glDrawArrays(GL_LINES, 0, 4);

            glBindVertexArray(_histogram.dynamicDraw.vao);
            constexpr const int nSize = 6 * Histogram::Bins;
            glUniform4fv(_colorLoc, 1, glm::value_ptr(color));
            glDrawArrays(GL_TRIANGLES, i * nSize, nSize);
        };

        _shader.bind();
        renderHistogram(0, ColorFrameTime);
        renderHistogram(1, ColorDrawTime);
        renderHistogram(2, ColorSyncTime);
        renderHistogram(3, ColorLoopTimeMin);
        renderHistogram(4, ColorLoopTimeMax);
    }
}

} // namespace sgct::core
