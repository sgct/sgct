/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <fmt/format.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <SpoutLibrary.h>

namespace {
    struct {
        GLuint vao;
        GLuint vbo;
    } geometry;

    bool shouldTakeScreenshot = false;

    SPOUTHANDLE receiver = nullptr;
    std::string sender;
    unsigned int width;
    unsigned int height;
    bool initialized = false;

    constexpr std::string_view VertexShader = R"(
  #version 330 core

  layout(location = 0) in vec2 vertPositions;

  out vec2 uv;

  void main() {
    gl_Position = vec4(vertPositions, 0.0, 1.0);
    uv = (vertPositions + vec2(1.0)) / vec2(2.0);
    uv.y = 1.0 - uv.y;
  })";

    constexpr std::string_view FragmentShader = R"(
  #version 330 core
  uniform sampler2D tex;
  in vec2 uv;
  out vec4 color;
  void main() { color = texture(tex, uv); }
)";
} // namespace

using namespace sgct;

bool bindSpout() {
    const bool creationSuccess = receiver->CreateReceiver(sender.data(), width, height);
    if (!initialized && creationSuccess) {
        Log::Info(fmt::format(
            "Spout: Initing {}x{} texture from '{}'", width, height, sender
        ));
        initialized = true;
    }

    if (initialized) {
        const bool receiveSucess = receiver->ReceiveTexture(sender.data(), width, height);
        if (receiveSucess) {
            return receiver->BindSharedTexture();
        }
        else {
            Log::Info("Spout disconnected");

            // reset if disconnected
            initialized = false;
            sender = '\0';
            receiver->ReleaseReceiver();
        }

    }

    return false;
}

void draw(RenderData) {
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();

    glActiveTexture(GL_TEXTURE0);
    bindSpout();

    glBindVertexArray(geometry.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    prog.unbind();
    receiver->UnBindSharedTexture();

    if (shouldTakeScreenshot) {
        Engine::instance().takeScreenshot();
        shouldTakeScreenshot = false;
    }
}

void initOGL(GLFWwindow*) {
    glGenVertexArrays(1, &geometry.vao);
    glGenBuffers(1, &geometry.vbo);

    glBindVertexArray(geometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);

    struct Vertex { float x, y; };

    std::array<Vertex, 6> vertices;
    vertices[0] = { -1.f, -1.f };
    vertices[1] = { 1.f,  1.f };
    vertices[2] = {-1.f, 1.f };
    vertices[3] = { -1.f, -1.f };
    vertices[4] = { 1.f, -1.f };
    vertices[5] = { 1.f,  1.f };

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    // setup spout
    sender = '\0';
    receiver = GetSpout();

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance().addShaderProgram("xform", VertexShader, FragmentShader);

    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();

    glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);

    prog.unbind();
}

void cleanup() {
    glDeleteVertexArrays(1, &geometry.vao);
    glDeleteBuffers(1, &geometry.vbo);

    if (receiver) {
        receiver->ReleaseReceiver();
        receiver->Release();
    }
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (action == Action::Press) {
        switch (key) {
            case Key::Esc:
                Engine::instance().terminate();
                break;
            case Key::Key1:
                sender = "Right\0";
                Log::Info("Settings receiver to 'Right'");
                break;
            case Key::Key2:
                sender = "zLeft\0";
                Log::Info("Settings receiver to 'zLeft'");
                break;
            case Key::Key3:
                sender = "Bottom\0";
                Log::Info("Settings receiver to 'Bottom'");
                break;
            case Key::Key4:
                sender = "Top\0";
                Log::Info("Settings receiver to 'Top'");
                break;
            case Key::Key5:
                sender = "Left\0";
                Log::Info("Settings receiver to 'Left'");
                break;
            case Key::Key6:
                sender = "zRight\0";
                Log::Info("Settings receiver to 'zRight'");
                break;
            case Key::Space:
                shouldTakeScreenshot = true;
                break;
            default:
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    if (!cluster.success) {
        return -1;
    }

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGL;
    callbacks.draw = draw;
    callbacks.cleanup = cleanup;
    callbacks.keyboard = keyboard;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error & e) {
        Log::Error(e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }
    Engine::instance().exec();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
