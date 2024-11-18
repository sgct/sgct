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
    bool showId = false;
    bool showStats = false;

    float radius = 7.4f;

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

void initGL(GLFWwindow*) {
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
        Log::Error(std::format("Could not find image '{}'", front.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t1(loadImage, std::ref(front));

    ImageData right;
    right.filename = "test-pattern-1.png";
    if (!std::filesystem::exists(right.filename)) {
        Log::Error(std::format("Could not find image '{}'", right.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t2(loadImage, std::ref(right));

    ImageData back;
    back.filename = "test-pattern-2.png";
    if (!std::filesystem::exists(back.filename)) {
        Log::Error(std::format("Could not find image '{}'", back.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t3(loadImage, std::ref(back));

    ImageData left;
    left.filename = "test-pattern-3.png";
    if (!std::filesystem::exists(left.filename)) {
        Log::Error(std::format("Could not find image '{}'", left.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t4(loadImage, std::ref(left));

    ImageData top;
    top.filename = "test-pattern-4.png";
    if (!std::filesystem::exists(top.filename)) {
        Log::Error(std::format("Could not find image '{}'", top.filename));
        exit(EXIT_FAILURE);
    }
    std::thread t5(loadImage, std::ref(top));

    ImageData bottom;
    bottom.filename = "test-pattern-5.png";
    if (!std::filesystem::exists(bottom.filename)) {
        Log::Error(std::format("Could not find image '{}'", bottom.filename));
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
        Log::Info(std::format("Frame: {}", frameNumber));
    }

    if (Engine::instance().isMaster() && runTests) {
        constexpr int FrameGridFirstScreenshot = 5;
        constexpr int FrameGridSettingBackBuffer = 10;
        constexpr int FrameGridSecondScreenshot = 15;
        constexpr int FrameTerminate = 20;

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
    deserializeObject(data, pos, runTests);
    deserializeObject(data, pos, frameNumber);
    deserializeObject(data, pos, showId);
    deserializeObject(data, pos, showStats);
    deserializeObject(data, pos, theta);
    deserializeObject(data, pos, phi);
}

void cleanup() {
    TextureManager::instance().removeTexture(box.textureFront);
    TextureManager::instance().removeTexture(box.textureRight);
    TextureManager::instance().removeTexture(box.textureBack);
    TextureManager::instance().removeTexture(box.textureLeft);
    TextureManager::instance().removeTexture(box.textureTop);
    TextureManager::instance().removeTexture(box.textureBottom);
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
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
