/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <numeric>
#include <thread>

namespace {
    struct {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint iboLine = 0;
        int nVertLine = 0;

        GLint mvpMatrixLocation = -1;
        GLint cameraMatrixLocation = -1;
    } grid;

    struct {
        GLuint vao = 0;
        GLuint vbo = 0;

        GLint mvpMatrixLocation = -1;
        GLint cameraMatrixLocation = -1;

        GLuint textureFront = 0;
        GLuint textureRight = 0;
        GLuint textureBack = 0;
        GLuint textureLeft = 0;
        GLuint textureTop = 0;
        GLuint textureBottom = 0;
    } box;

    bool runTests = false;
    uint64_t frameNumber = 0;

    float phi = glm::pi<float>();
    float theta = 0.f;

    bool takeScreenshot = false;
    bool captureBackbuffer = false;
    bool renderGrid = true;
    bool renderBox = false;
    bool showId = false;
    bool showStats = false;

    float radius = 7.4f;

    constexpr std::string_view GridVertexShader = R"(
#version 330 core

layout(location = 0) in vec2 in_position;

out vec4 tr_color;

uniform float radius;
uniform mat4 mvp;
uniform mat4 camera;

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
  gl_Position = mvp * camera * vec4(p, 1.0);
  tr_color = vec4(p, 1.0);
}
)";

    constexpr std::string_view GridFragmentShader = R"(
#version 330 core

in vec4 tr_color;
out vec4 color;

void main() { color = tr_color; }
)";

    constexpr std::string_view BoxVertexShader = R"(
#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in int in_textureId;

out vec2 tr_uv;
flat out int tr_textureId;

uniform mat4 mvp;
uniform mat4 camera;

void main() {
    gl_Position = mvp * camera * vec4(in_position, 1.0);
    tr_uv = in_uv;
    tr_textureId = in_textureId;
}
)";

    constexpr std::string_view BoxFragmentShader = R"(
#version 330 core

in vec2 tr_uv;
flat in int tr_textureId;
out vec4 color;

uniform sampler2D tex[6];

void main() {
    color = texture(tex[tr_textureId], tr_uv);
}
)";

} // namespace

using namespace sgct;

void initializeGrid() {
    constexpr uint16_t RestartIndex = std::numeric_limits<uint16_t>::max();
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RestartIndex);

    glGenVertexArrays(1, &grid.vao);
    glGenBuffers(1, &grid.vbo);
    glGenBuffers(1, &grid.iboLine);

    glBindVertexArray(grid.vao);
    glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);

    constexpr int ElevationSteps = 40;
    constexpr int AzimuthSteps = 160;

    struct GridVertex {
        float elevation;
        float azimuth;
    };
    std::vector<GridVertex> vertices;
    // (abock, 2019-10-09) We generate the vertices ring-wise;  first iterating over the
    // elevation and then the azimuth will lead to the bottom most ring be filled first,
    // before going to the upper rings.  That also means that two vertices on top of each
    // other should be separated in the vertices list by 'AzimuthSteps' positions
    for (int e = 0; e <= ElevationSteps; ++e) {
        for (int a = 0; a < AzimuthSteps; ++a) {
            float ev = static_cast<float>(e) / static_cast<float>(ElevationSteps - 1);
            float av = static_cast<float>(a) / static_cast<float>(AzimuthSteps - 1);
            
            GridVertex vertex;
            vertex.elevation = glm::radians(ev * 90.f);
            vertex.azimuth = glm::radians(av * 360.f);
            vertices.push_back(vertex);
        }
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GridVertex), nullptr);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(GridVertex),
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
        GridVertexShader,
        GridFragmentShader
    );
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("grid");
    prog.bind();
    grid.mvpMatrixLocation = glGetUniformLocation(prog.id(), "mvp");
    assert(grid.mvpMatrixLocation != -1);
    grid.cameraMatrixLocation = glGetUniformLocation(prog.id(), "camera");
    assert(grid.cameraMatrixLocation != -1);
    GLuint radiusLocation = glGetUniformLocation(prog.id(), "radius");
    assert(radiusLocation != std::numeric_limits<GLuint>::max());
    glUniform1f(radiusLocation, radius);
    prog.unbind();
}

void initializeBox() {
    glGenVertexArrays(1, &box.vao);
    glGenBuffers(1, &box.vbo);
    glBindVertexArray(box.vao);

    struct BoxVertex {
        float x;
        float y;
        float z;

        float s;
        float t;

        uint8_t textureId;
    };

    constexpr std::array<BoxVertex, 2 * 3 * 6> Vertices = {
        // Front
        BoxVertex{ -10.f, -10.f, -10.f, 0.f, 0.f, 0 },  // ---
        BoxVertex{  10.f,  10.f, -10.f, 1.f, 1.f, 0 },  // ++-
        BoxVertex{ -10.f,  10.f, -10.f, 0.f, 1.f, 0 },  // -+-

        BoxVertex{ -10.f, -10.f, -10.f, 0.f, 0.f, 0 },  // ---
        BoxVertex{  10.f, -10.f, -10.f, 1.f, 0.f, 0 },  // +--
        BoxVertex{  10.f,  10.f, -10.f, 1.f, 1.f, 0 },  // ++-

        // Right
        BoxVertex{  10.f, -10.f, -10.f, 0.f, 0.f, 1 },  // +--
        BoxVertex{  10.f,  10.f,  10.f, 1.f, 1.f, 1 },  // +++
        BoxVertex{  10.f,  10.f, -10.f, 0.f, 1.f, 1 },  // ++-

        BoxVertex{  10.f, -10.f, -10.f, 0.f, 0.f, 1 },  // +--
        BoxVertex{  10.f, -10.f,  10.f, 1.f, 0.f, 1 },  // +-+
        BoxVertex{  10.f,  10.f,  10.f, 1.f, 1.f, 1 },  // +++

        // Back
        BoxVertex{  10.f, -10.f,  10.f, 0.f, 0.f, 2 },  // +-+
        BoxVertex{ -10.f,  10.f,  10.f, 1.f, 1.f, 2 },  // -++
        BoxVertex{  10.f,  10.f,  10.f, 0.f, 1.f, 2 },  // +++

        BoxVertex{  10.f, -10.f,  10.f, 0.f, 0.f, 2 },  // +-+
        BoxVertex{ -10.f, -10.f,  10.f, 1.f, 0.f, 2 },  // --+
        BoxVertex{ -10.f,  10.f,  10.f, 1.f, 1.f, 2 },  // -++

        // Left
        BoxVertex{ -10.f, -10.f,  10.f, 0.f, 0.f, 3 },  // --+
        BoxVertex{ -10.f,  10.f, -10.f, 1.f, 1.f, 3 },  // -+-
        BoxVertex{ -10.f,  10.f,  10.f, 0.f, 1.f, 3 },  // -++

        BoxVertex{ -10.f, -10.f,  10.f, 0.f, 0.f, 3 },  // --+
        BoxVertex{ -10.f, -10.f, -10.f, 1.f, 0.f, 3 },  // ---
        BoxVertex{ -10.f,  10.f, -10.f, 1.f, 1.f, 3 },  // -+-

        // Top
        BoxVertex{ -10.f,  10.f, -10.f, 0.f, 0.f, 4 },  // -+-
        BoxVertex{  10.f,  10.f,  10.f, 1.f, 1.f, 4 },  // +++
        BoxVertex{ -10.f,  10.f,  10.f, 0.f, 1.f, 4 },  // -++

        BoxVertex{ -10.f,  10.f, -10.f, 0.f, 0.f, 4 },  // -+-
        BoxVertex{  10.f,  10.f, -10.f, 1.f, 0.f, 4 },  // ++-
        BoxVertex{  10.f,  10.f,  10.f, 1.f, 1.f, 4 },  // +++

        // Bottom
        BoxVertex{ -10.f, -10.f,  10.f, 0.f, 0.f, 5 },  // --+
        BoxVertex{  10.f, -10.f, -10.f, 1.f, 1.f, 5 },  // +--
        BoxVertex{ -10.f, -10.f, -10.f, 0.f, 1.f, 5 },  // ---

        BoxVertex{ -10.f, -10.f,  10.f, 0.f, 0.f, 5 },  // --+
        BoxVertex{  10.f, -10.f,  10.f, 1.f, 0.f, 5 },  // +-+
        BoxVertex{  10.f, -10.f, -10.f, 1.f, 1.f, 5 },  // +--
    };
    glBindBuffer(GL_ARRAY_BUFFER, box.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        Vertices.size() * sizeof(BoxVertex),
        Vertices.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BoxVertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(BoxVertex),
        reinterpret_cast<void*>(offsetof(BoxVertex, s))
    );
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(
        2,
        1,
        GL_UNSIGNED_BYTE,
        sizeof(BoxVertex),
        reinterpret_cast<void*>(offsetof(BoxVertex, textureId))
    );

    glBindVertexArray(0);


    struct ImageData {
        std::string filename;
        Image img;
        std::atomic_bool imageDone = false;
        std::atomic_bool uploadDone = false;
        std::atomic_bool threadDone = false;
    };
    auto loadImage = [](ImageData& data) {
        data.img.load(data.filename);
        data.imageDone = true;
        while (!data.uploadDone) {}
        data.img = Image();
        data.threadDone = true;
    };

    Log::Info("Loading test pattern images...");

    ImageData front;
    front.filename = "test-pattern-0.png";
    if (!std::filesystem::exists(front.filename)) {
        Log::Error(fmt::format("Could not find image '{}'", front.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t1(loadImage, std::ref(front));

    ImageData right;
    right.filename = "test-pattern-1.png";
    if (!std::filesystem::exists(right.filename)) {
        Log::Error(fmt::format("Could not find image '{}'", right.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t2(loadImage, std::ref(right));

    ImageData back;
    back.filename = "test-pattern-2.png";
    if (!std::filesystem::exists(back.filename)) {
        Log::Error(fmt::format("Could not find image '{}'", back.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t3(loadImage, std::ref(back));

    ImageData left;
    left.filename = "test-pattern-3.png";
    if (!std::filesystem::exists(left.filename)) {
        Log::Error(fmt::format("Could not find image '{}'", left.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t4(loadImage, std::ref(left));

    ImageData top;
    top.filename = "test-pattern-4.png";
    if (!std::filesystem::exists(top.filename)) {
        Log::Error(fmt::format("Could not find image '{}'", top.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t5(loadImage, std::ref(top));

    ImageData bottom;
    bottom.filename = "test-pattern-5.png";
    if (!std::filesystem::exists(bottom.filename)) {
        Log::Error(fmt::format("Could not find image '{}'", bottom.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t6(loadImage, std::ref(bottom));

    while (!front.imageDone || !right.imageDone || !back.imageDone ||
           !left.imageDone || !top.imageDone || !bottom.imageDone)
    {}

    box.textureFront = TextureManager::instance().loadTexture(std::move(front.img));
    front.uploadDone = true;
    box.textureRight = TextureManager::instance().loadTexture(std::move(right.img));
    right.uploadDone = true;
    box.textureBack = TextureManager::instance().loadTexture(std::move(back.img));
    back.uploadDone = true;
    box.textureLeft = TextureManager::instance().loadTexture(std::move(left.img));
    left.uploadDone = true;
    box.textureTop = TextureManager::instance().loadTexture(std::move(top.img));
    top.uploadDone = true;
    box.textureBottom = TextureManager::instance().loadTexture(std::move(bottom.img));
    bottom.uploadDone = true;

    while (!front.threadDone) {}
    t1.join();
    while (!right.threadDone) {}
    t2.join();
    while (!back.threadDone) {}
    t3.join();
    while (!left.threadDone) {}
    t4.join();
    while (!top.threadDone) {}
    t5.join();
    while (!bottom.threadDone) {}
    t6.join();

    ShaderManager::instance().addShaderProgram(
        "box",
        BoxVertexShader,
        BoxFragmentShader
    );
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("box");
    prog.bind();
    box.mvpMatrixLocation = glGetUniformLocation(prog.id(), "mvp");
    assert(box.mvpMatrixLocation != -1);
    box.cameraMatrixLocation = glGetUniformLocation(prog.id(), "camera");
    assert(box.cameraMatrixLocation != -1);

    GLuint texLoc = glGetUniformLocation(prog.id(), "tex");
    assert(texLoc != std::numeric_limits<GLuint>::max());
    constexpr std::array<int, 6> Indices = { 0, 1, 2, 3, 4, 5 };
    glUniform1iv(texLoc, 6, Indices.data());
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

    glm::vec3 direction = {
        std::cos(theta) * std::sin(phi),
        std::sin(theta),
        std::cos(theta) * std::cos(phi)
    };
    glm::vec3 right = {
        std::sin(phi - glm::half_pi<float>()),
        0.f,
        std::cos(phi - glm::half_pi<float>())
    };
    glm::vec3 up = glm::cross(right, direction);
    const glm::mat4 c = glm::lookAt(glm::vec3(0.f), direction, up);

    if (renderGrid) {
        ShaderManager::instance().shaderProgram("grid").bind();
        glUniformMatrix4fv(grid.mvpMatrixLocation, 1, GL_FALSE, mvp.values);
        glUniformMatrix4fv(grid.cameraMatrixLocation, 1, GL_FALSE, glm::value_ptr(c));
        glBindVertexArray(grid.vao);
        glDrawElements(GL_LINE_STRIP, grid.nVertLine, GL_UNSIGNED_SHORT, nullptr);
        glBindVertexArray(0);
        ShaderManager::instance().shaderProgram("grid").unbind();
    }

    if (renderBox) {
        ShaderManager::instance().shaderProgram("box").bind();
        glUniformMatrix4fv(box.mvpMatrixLocation, 1, GL_FALSE, mvp.values);
        glUniformMatrix4fv(box.cameraMatrixLocation, 1, GL_FALSE, glm::value_ptr(c));
        glBindVertexArray(box.vao);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, box.textureFront);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, box.textureRight);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, box.textureBack);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, box.textureLeft);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, box.textureTop);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, box.textureBottom);

        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        ShaderManager::instance().shaderProgram("grid").unbind();
    }
}

void draw2D(const RenderData& data) {
#ifdef SGCT_HAS_TEXT
    if (showId) {
        const float w =
            static_cast<float>(data.window.resolution().x) * data.viewport.size().x;
        const float h =
            static_cast<float>(data.window.resolution().y) * data.viewport.size().y;

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
            std::to_string(ClusterManager::instance().thisNodeId())
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
            ClusterManager::instance().thisNode().address()
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
        constexpr int FrameGridFirstScreenshot = 5;
        constexpr int FrameGridSettingBackBuffer = 10;
        constexpr int FrameGridSecondScreenshot = 15;
        constexpr int ChangeToBox = 20;
        constexpr int FrameBoxFirstScreenshot = 25;
        constexpr int FrameBoxSettingBackBuffer = 30;
        constexpr int FrameBoxSecondScreenshot = 35;
        constexpr int FrameTerminate = 40;

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
    serializeObject(data, theta);
    serializeObject(data, phi);
    return data;
}

void decode(const std::vector<std::byte>& data) {
    unsigned pos = 0;
    deserializeObject(data, pos, takeScreenshot);
    deserializeObject(data, pos, captureBackbuffer);
    deserializeObject(data, pos, renderGrid);
    deserializeObject(data, pos, renderBox);
    deserializeObject(data, pos, runTests);
    deserializeObject(data, pos, frameNumber);
    deserializeObject(data, pos, showId);
    deserializeObject(data, pos, showStats);
    deserializeObject(data, pos, theta);
    deserializeObject(data, pos, phi);
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

void keyboard(Key key, Modifier, Action action, int, Window*) {
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

    if (key == Key::Left && (action == Action::Press || action == Action::Repeat)) {
        phi += 0.1f;
        if (phi > glm::two_pi<float>()) {
            phi -= glm::two_pi<float>();
        }
    }

    if (key == Key::Right && (action == Action::Press || action == Action::Repeat)) {
        phi -= 0.1f;
        if (phi < -glm::two_pi<float>()) {
            phi += glm::two_pi<float>();
        }
    }

    if (key == Key::Down && (action == Action::Press || action == Action::Repeat)) {
        theta -= 0.1f;
        theta = std::clamp(theta, -glm::half_pi<float>(), glm::half_pi<float>());
    }

    if (key == Key::Up && (action == Action::Press || action == Action::Repeat)) {
        theta += 0.1f;
        theta = std::clamp(theta, -glm::half_pi<float>(), glm::half_pi<float>());
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    if (!cluster.success) {
        return -1;
    }

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

    Log::Info("===========");
    Log::Info("Keybindings");
    Log::Info("SPACE: Toggle between grid and box");
    Log::Info("LEFT:  Move camera pointing to the left");
    Log::Info("RIGHT: Move camera pointing to the right");
    Log::Info("UP:    Move camera pointing to up");
    Log::Info("DOWN:  Move camera pointing to down");
    Log::Info("Enter: Run Frontbuffer/Backbuffer tests");
    Log::Info("ESC:   Terminate the program");
    Log::Info("I:     Show node id and IP");
    Log::Info("S:     Show statistics graphs");
    Log::Info("P:     Take screenshot");
    Log::Info("B:     Toggle capturing the back buffer");
    Log::Info("===========");


    Engine::instance().exec();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
