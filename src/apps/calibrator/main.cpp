/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <numeric>

namespace {
    struct {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint iboLine = 0;
        int nVertLine = 0;

        GLint matrixLocation = -1;
    } grid;

    struct {
        GLuint vao = 0;
        GLuint vbo = 0;

        GLint matrixLocation = -1;

        GLuint textureFront = 0;
        GLuint textureRight = 0;
        GLuint textureBack = 0;
        GLuint textureLeft = 0;
        GLuint textureTop = 0;
        GLuint textureBottom = 0;
    } box;

    bool runTests = false;
    uint64_t frameNumber = 0;

    bool takeScreenshot = false;
    bool captureBackbuffer = false;
    bool renderGrid = true;
    bool renderBox = false;
    bool showId = false;
    bool showStats = false;

    float radius = 7.4f;

    constexpr const char* gridVertexShader = R"(
#version 330 core

layout(location = 0) in vec2 in_position;

out vec4 tr_color;

uniform float radius;
uniform mat4 matrix;

const float PI = 3.141592654;
const float PI_HALF = PI / 2.0;

void main() {
  float elevation = in_position[0];
  float azimuth = in_position[1];

  vec3 p = vec3(
    radius * cos(elevation) * sin(azimuth),
    radius * sin(elevation),
    -radius * cos(elevation) * cos(azimuth)
  );
  gl_Position = matrix * vec4(p, 1.0);
  tr_color = vec4(p, 1.0);
}
)";

    constexpr const char* gridFragmentShader = R"(
#version 330 core

in vec4 tr_color;
out vec4 color;

void main() { color = tr_color; }
)";

    constexpr const char* boxVertexShader = R"(
#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 tr_uv;

uniform mat4 matrix;

void main() {
    gl_Position = matrix * vec4(in_position, 1.0);
    tr_uv = in_uv;
}
)";

    constexpr const char* boxFragmentShader = R"(
#version 330 core

in vec2 tr_uv;
out vec4 color;

uniform sampler2D tex;

void main() { color = texture(tex, tr_uv); }
)";

} // namespace

using namespace sgct;

void initializeGrid() {
    constexpr const uint16_t RestartIndex = std::numeric_limits<uint16_t>::max();
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RestartIndex);

    glGenVertexArrays(1, &grid.vao);
    glGenBuffers(1, &grid.vbo);
    glGenBuffers(1, &grid.iboLine);

    glBindVertexArray(grid.vao);
    glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);

    constexpr const int ElevationSteps = 40;
    constexpr const int AzimuthSteps = 160;

    struct Vertex {
        float elevation,
            azimuth;
    };
    std::vector<Vertex> vertices;
    // (abock, 2019-10-09) We generate the vertices ring-wise;  first iterating over the
    // elevation and then the azimuth will lead to the bottom most ring be filled first,
    // before going to the upper rings.  That also means that two vertices on top of each
    // other should be separated in the vertices list by 'AzimuthSteps' positions
    for (int e = 0; e <= ElevationSteps; ++e) {
        for (int a = 0; a < AzimuthSteps; ++a) {
            Vertex vertex;
            float ev = static_cast<float>(e) / static_cast<float>(ElevationSteps - 1);
            float av = static_cast<float>(a) / static_cast<float>(AzimuthSteps - 1);
            vertex.elevation = glm::radians(ev * 90.f);
            vertex.azimuth = glm::radians(av * 360.f);
            vertices.push_back(vertex);
        }
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    {
        // Line representation
        // (abock, 2019-10-09) It's possible to compute a better size of how many indices
        // we'll need, but I can't be asked (and we are seriously not performance limited)
        std::vector<uint16_t> indices;
        indices.reserve(2 * ElevationSteps * AzimuthSteps);

        // First the horizontal, azimuth lines
        for (int e = 0; e < ElevationSteps; ++e) {
            std::vector<uint16_t> t(AzimuthSteps);
            std::iota(t.begin(), t.end(), static_cast<uint16_t>(e * AzimuthSteps));
            t.push_back(static_cast<uint16_t>(e * AzimuthSteps)); // close the ring
            t.push_back(RestartIndex); // restart for the next ring
            indices.insert(indices.end(), t.begin(), t.end());
        }
        indices.push_back(RestartIndex);
        // Then the vertical lines; see above; every vertical vertex is separated by
        // exactly 'AzimuthSteps' positions in the vertex array
        for (int a = 0; a < AzimuthSteps; ++a) {
            for (int e = 0; e < ElevationSteps; ++e) {
                indices.push_back(static_cast<uint16_t>(a + e * AzimuthSteps));
            }
            indices.push_back(RestartIndex);
        }

        grid.nVertLine = static_cast<int>(indices.size());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.iboLine);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(uint16_t),
            indices.data(),
            GL_STATIC_DRAW
        );
    }
    glBindVertexArray(0);


    ShaderManager::instance().addShaderProgram(
        "grid",
        gridVertexShader,
        gridFragmentShader
    );
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("grid");
    prog.bind();
    grid.matrixLocation = glGetUniformLocation(prog.id(), "matrix");
    glUniform1f(glGetUniformLocation(prog.id(), "radius"), radius);
    prog.unbind();
}

void initializeBox() {
    glGenVertexArrays(1, &box.vao);
    glGenBuffers(1, &box.vbo);
    glBindVertexArray(box.vao);

    struct Vertex {
        float x;
        float y;
        float z;

        float s;
        float t;
    };

    std::array<Vertex, 2 * 3 * 6> vertices = {
        // Front
        Vertex{ -10.f, -10.f, -10.f, 0.f, 0.f },  // ---
        Vertex{  10.f,  10.f, -10.f, 1.f, 1.f },  // ++-
        Vertex{ -10.f,  10.f, -10.f, 0.f, 1.f },  // -+-

        Vertex{ -10.f, -10.f, -10.f, 0.f, 0.f },  // ---
        Vertex{  10.f, -10.f, -10.f, 1.f, 0.f },  // +--
        Vertex{  10.f,  10.f, -10.f, 1.f, 1.f },  // ++-

        // Right
        Vertex{  10.f, -10.f, -10.f, 0.f, 0.f },  // +--
        Vertex{  10.f,  10.f,  10.f, 1.f, 1.f },  // +++
        Vertex{  10.f,  10.f, -10.f, 0.f, 1.f },  // ++-

        Vertex{  10.f, -10.f, -10.f, 0.f, 0.f },  // +--
        Vertex{  10.f, -10.f,  10.f, 1.f, 0.f },  // +-+
        Vertex{  10.f,  10.f,  10.f, 1.f, 1.f },  // +++

        // Back
        Vertex{  10.f, -10.f,  10.f, 0.f, 0.f },  // +-+
        Vertex{ -10.f,  10.f,  10.f, 1.f, 1.f },  // -++
        Vertex{  10.f,  10.f,  10.f, 0.f, 1.f },  // +++

        Vertex{  10.f, -10.f,  10.f, 0.f, 0.f },  // +-+
        Vertex{ -10.f, -10.f,  10.f, 1.f, 0.f },  // --+
        Vertex{ -10.f,  10.f,  10.f, 1.f, 1.f },  // -++

        // Left
        Vertex{ -10.f, -10.f,  10.f, 0.f, 0.f },  // --+
        Vertex{ -10.f,  10.f, -10.f, 1.f, 1.f },  // -+-
        Vertex{ -10.f,  10.f,  10.f, 0.f, 1.f },  // -++

        Vertex{ -10.f, -10.f,  10.f, 0.f, 0.f },  // --+
        Vertex{ -10.f, -10.f, -10.f, 1.f, 0.f },  // ---
        Vertex{ -10.f,  10.f, -10.f, 1.f, 1.f },  // -+-

        // Top
        Vertex{ -10.f,  10.f, -10.f, 0.f, 0.f },  // -+-
        Vertex{  10.f,  10.f,  10.f, 1.f, 1.f },  // +++
        Vertex{ -10.f,  10.f,  10.f, 0.f, 1.f },  // -++

        Vertex{ -10.f,  10.f, -10.f, 0.f, 0.f },  // -+-
        Vertex{  10.f,  10.f, -10.f, 1.f, 0.f },  // ++-
        Vertex{  10.f,  10.f,  10.f, 1.f, 1.f },  // +++

        // Bottom
        Vertex{ -10.f, -10.f,  10.f, 0.f, 0.f },  // --+
        Vertex{  10.f, -10.f, -10.f, 1.f, 1.f },  // +--
        Vertex{ -10.f, -10.f, -10.f, 0.f, 1.f },  // ---

        Vertex{ -10.f, -10.f,  10.f, 0.f, 0.f },  // --+
        Vertex{  10.f, -10.f,  10.f, 1.f, 0.f },  // +-+
        Vertex{  10.f, -10.f, -10.f, 1.f, 1.f },  // +--
    };
    glBindBuffer(GL_ARRAY_BUFFER, box.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glBindVertexArray(0);

    Log::Info("Loading test-pattern-0.png");
    box.textureFront = TextureManager::instance().loadTexture("test-pattern-0.png", true);
    Log::Info("Loading test-pattern-1.png");
    box.textureRight = TextureManager::instance().loadTexture("test-pattern-1.png", true);
    Log::Info("Loading test-pattern-2.png");
    box.textureBack = TextureManager::instance().loadTexture("test-pattern-2.png", true);
    Log::Info("Loading test-pattern-3.png");
    box.textureLeft = TextureManager::instance().loadTexture("test-pattern-3.png", true);
    Log::Info("Loading test-pattern-4.png");
    box.textureTop = TextureManager::instance().loadTexture("test-pattern-4.png", true);
    Log::Info("Loading test-pattern-5.png");
    box.textureBottom =
        TextureManager::instance().loadTexture("test-pattern-5.png", true);


    ShaderManager::instance().addShaderProgram(
        "box",
        boxVertexShader,
        boxFragmentShader
    );
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("box");
    prog.bind();
    box.matrixLocation = glGetUniformLocation(prog.id(), "matrix");
    glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);
    prog.unbind();
}

void initGL(GLFWwindow*) {
    initializeGrid();
    initializeBox();
}

void postSyncPreDraw() {
    Settings::instance().setCaptureFromBackBuffer(captureBackbuffer);
    Engine::instance().setStatsGraphVisibility(showStats);
    if (takeScreenshot) {
        Log::Info("Triggering screenshot");
        Engine::instance().takeScreenshot();
        takeScreenshot = false;
    }
}

void draw(const RenderData& data) {
    const mat4 mvp = data.modelViewProjectionMatrix;
    if (renderGrid) {
        ShaderManager::instance().shaderProgram("grid").bind();
        glUniformMatrix4fv(grid.matrixLocation, 1, GL_FALSE, mvp.values);
        glBindVertexArray(grid.vao);
        glDrawElements(GL_LINE_STRIP, grid.nVertLine, GL_UNSIGNED_SHORT, nullptr);
        glBindVertexArray(0);
        ShaderManager::instance().shaderProgram("grid").unbind();
    }

    if (renderBox) {
        ShaderManager::instance().shaderProgram("box").bind();
        glUniformMatrix4fv(box.matrixLocation, 1, GL_FALSE, mvp.values);
        glBindVertexArray(box.vao);

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, box.textureFront);
        glDrawArrays(GL_TRIANGLES,  0, 6);
        glBindTexture(GL_TEXTURE_2D, box.textureRight);
        glDrawArrays(GL_TRIANGLES,  6, 6);
        glBindTexture(GL_TEXTURE_2D, box.textureBack);
        glDrawArrays(GL_TRIANGLES, 12, 6);
        glBindTexture(GL_TEXTURE_2D, box.textureLeft);
        glDrawArrays(GL_TRIANGLES, 18, 6);
        glBindTexture(GL_TEXTURE_2D, box.textureTop);
        glDrawArrays(GL_TRIANGLES, 24, 6);
        glBindTexture(GL_TEXTURE_2D, box.textureBottom);
        glDrawArrays(GL_TRIANGLES, 30, 6);

        glBindVertexArray(0);
        ShaderManager::instance().shaderProgram("grid").unbind();
    }
}

void draw2D(const RenderData& data) {
#ifdef SGCT_HAS_TEXT
    if (showId) {
        const float w =
            static_cast<float>(data.window.resolution().x)* data.viewport.size().x;
        const float h =
            static_cast<float>(data.window.resolution().y)* data.viewport.size().y;

        const float offset = w / 2.f - w / 7.f;

        const float s1 = h / 8.f;
        const unsigned int fontSize1 = static_cast<unsigned int>(s1);
        text::Font* f1 = text::FontManager::instance().font("SGCTFont", fontSize1);

        text::print(
            data.window,
            data.viewport,
            *f1,
            text::Alignment::TopLeft,
            offset,
            h / 2.f - s1,
            vec4{ 0.f, 0.f, 1.f, 1.f },
            "%d",
            ClusterManager::instance().thisNodeId()
        );

        const float s2 = h / 20.f;
        const unsigned int fontSize2 = static_cast<unsigned int>(s2);
        text::Font* f2 = text::FontManager::instance().font("SGCTFont", fontSize2);
        text::print(
            data.window,
            data.viewport,
            *f2,
            text::Alignment::TopLeft,
            offset,
            h / 2.f - (s1 + s2) * 1.2f,
            vec4{ 0.f, 0.f, 1.f, 1.f },
            "%s",
            ClusterManager::instance().thisNode().address().c_str()
        );
    }
#endif // SGCT_HAS_TEXT
}

void postDraw() {
    if (runTests) {
        frameNumber++;
        Log::Info(fmt::format("Frame: {}", frameNumber));
    }

    if (Engine::instance().isMaster() && runTests) {
        constexpr const int FrameGridFirstScreenshot = 5;
        constexpr const int FrameGridSettingBackBuffer = 10;
        constexpr const int FrameGridSecondScreenshot = 15;
        constexpr const int ChangeToBox = 20;
        constexpr const int FrameBoxFirstScreenshot = 25;
        constexpr const int FrameBoxSettingBackBuffer = 30;
        constexpr const int FrameBoxSecondScreenshot = 35;
        constexpr const int FrameTerminate = 40;

        switch (frameNumber) {
            case FrameGridFirstScreenshot:
                Log::Info("Setting to take first grid screenshot");
                takeScreenshot = true;
                break;
            case FrameGridFirstScreenshot + 2:
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case FrameGridSettingBackBuffer:
                Log::Info("Setting to capture from Back buffer");
                captureBackbuffer = true;
                break;
            case FrameGridSecondScreenshot:
                Log::Info("Setting to take second grid screenshot");
                takeScreenshot = true;
                break;
            case FrameGridSecondScreenshot + 2:
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case ChangeToBox:
                Log::Info("Setting to capture from front buffer");
                Log::Info("Changing the rendering to the test box");
                captureBackbuffer = false;
                renderGrid = false;
                renderBox = true;
                break;
            case FrameBoxFirstScreenshot:
                Log::Info("Setting to take first box screenshot");
                takeScreenshot = true;
                break;
            case FrameBoxFirstScreenshot + 2:
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case FrameBoxSettingBackBuffer:
                Log::Info("Setting to capture from Back buffer");
                captureBackbuffer = true;
                break;
            case FrameBoxSecondScreenshot:
                Log::Info("Setting to take second box screenshot");
                takeScreenshot = true;
                break;
            case FrameBoxSecondScreenshot + 2:
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case FrameTerminate:
                std::this_thread::sleep_for(std::chrono::seconds(5));
                Engine::instance().terminate();
                break;
            default:
                break;
        }
    }
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, takeScreenshot);
    serializeObject(data, captureBackbuffer);
    serializeObject(data, renderGrid);
    serializeObject(data, renderBox);
    serializeObject(data, runTests);
    serializeObject(data, frameNumber);
    serializeObject(data, showId);
    serializeObject(data, showStats);
    return data;
}

void decode(const std::vector<std::byte>& data, unsigned int pos) {
    deserializeObject(data, pos, takeScreenshot);
    deserializeObject(data, pos, captureBackbuffer);
    deserializeObject(data, pos, renderGrid);
    deserializeObject(data, pos, renderBox);
    deserializeObject(data, pos, runTests);
    deserializeObject(data, pos, frameNumber);
    deserializeObject(data, pos, showId);
    deserializeObject(data, pos, showStats);
}

void cleanup() {
    glDeleteVertexArrays(1, &grid.vao);
    glDeleteBuffers(1, &grid.vbo);
    glDeleteBuffers(1, &grid.iboLine);

    TextureManager::instance().removeTexture(box.textureFront);
    TextureManager::instance().removeTexture(box.textureRight);
    TextureManager::instance().removeTexture(box.textureBack);
    TextureManager::instance().removeTexture(box.textureLeft);
    TextureManager::instance().removeTexture(box.textureTop);
    TextureManager::instance().removeTexture(box.textureBottom);
}

void keyboard(Key key, Modifier, Action action, int) {
    if (key == Key::Space && action == Action::Press) {
        renderGrid = !renderGrid;
        renderBox = !renderBox;
    }

    if (key == Key::Enter && action == Action::Press) {
        frameNumber = 0;
        runTests = true;
    }

    if (key == Key::Esc && action == Action::Press) {
        Engine::instance().terminate();
    }

    if (key == Key::I && action == Action::Press) {
        showId = !showId;
    }

    if (key == Key::S && action == Action::Press) {
        showStats = !showStats;
    }

    if (key == Key::P && action == Action::Press) {
        takeScreenshot = true;
    }

    if (key == Key::B && action == Action::Press) {
        captureBackbuffer = !captureBackbuffer;
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    runTests = std::find(arg.begin(), arg.end(), "-runTests") != arg.end();

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initGL;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.draw2D = draw2D;
    callbacks.postDraw = postDraw;
    callbacks.cleanup = cleanup;
    callbacks.keyboard = keyboard;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error& e) {
        Log::Error(e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance().render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
