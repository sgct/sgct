/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <sgct/utils/box.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <SpoutLibrary.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {
    GLuint vao = 0;
    GLuint vbo = 0;

    struct Sender {
        std::string name;
        unsigned int width = 0;
        unsigned int height = 0;
    };

    std::vector<Sender> senders;
    size_t currentSender = 0;

    SPOUTHANDLE receiver = nullptr;
    bool isInitialized = false;

    constexpr std::string_view VertexShader = R"(
  #version 330 core
  layout(location = 0) in vec2 in_position;
  out vec2 uv;

  uniform ivec2 texSize;
  uniform ivec2 windowSize;

  void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);
    vec2 texCoords = (in_position + vec2(1.0)) / vec2(2.0);
    texCoords.y = 1.0 - texCoords.y;

    float texAspect = float(texSize.x) / float(texSize.y);
    float winAspect = float(windowSize.x) / float(windowSize.y);
    if (texAspect > winAspect) {
        texCoords.y = texCoords.y / (winAspect / texAspect);
        //texCoords.y = clamp(texCoords.y, 0.0, 1.0);
    }
    else if (texAspect < winAspect) {
        texCoords.x = texCoords.x / (texAspect / winAspect);
        //texCoords.x = clamp(texCoords.x, 0.0, 1.0);
    }
    uv = texCoords;
  })";

    constexpr std::string_view FragmentShader = R"(
  #version 330 core
  uniform sampler2D tex;
  in vec2 uv;
  out vec4 color;
  void main() {
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        color = vec4(0.0);
    }
    else {
        color = texture(tex, uv);
    }
  }
)";
} // namespace

using namespace sgct;

bool bindSpout() {
    unsigned int width = 0;
    unsigned int height = 0;
    std::array<char, 256> name = {};
    if (currentSender < senders.size()) {
        std::memcpy(
            name.data(),
            senders[currentSender].name.c_str(), senders[currentSender].name.size() + 1
        );
    }
    const bool creationSuccess = receiver->CreateReceiver(name.data(), width, height);
    if (!isInitialized && creationSuccess) {
        Log::Info(fmt::format(
            "Spout: Initializing {}x{} texture from '{}'", width, height, name.data()
        ));
        isInitialized = true;
    }

    if (!isInitialized) {
        return false;
    }
    const bool success  = receiver->ReceiveTexture(name.data(), width, height);
    if (success) {
        return receiver->BindSharedTexture();
    }
    else {
        Log::Info("Spout disconnected");

        isInitialized = false;
        receiver->ReleaseReceiver();
        return false;
    }
}

void postSyncPreDraw() {
    const int count = receiver->GetSenderCount();

    std::optional<std::string> currentSendername;
    if (currentSender < senders.size()) {
        currentSendername = senders[currentSender].name;
    }
    currentSender = 0;

    senders.clear();
    senders.reserve(count);
    for (int i = 0; i < count; ++i) {
        char Buffer[256];
        bool success = receiver->GetSenderName(i, Buffer, 256);

        Sender sender;
        sender.name = Buffer;
        if (success) {
            HANDLE handle;
            DWORD format;
            receiver->GetSenderInfo(Buffer, sender.width, sender.height, handle, format);
        }

        if (sender.name == currentSendername) {
            currentSender = static_cast<size_t>(i);
        }

        senders.push_back(sender);
    }
}

void draw(const RenderData& data) {
    glActiveTexture(GL_TEXTURE0);

    bool spoutStatus = bindSpout();
    if (!spoutStatus) {
        return;
    }

    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();

    glUniform2i(
        glGetUniformLocation(prog.id(), "texSize"),
        senders[currentSender].width, senders[currentSender].height
    );
    ivec2 res = data.window.resolution();
    glUniform2i(glGetUniformLocation(prog.id(), "windowSize"), res.x, res.y);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    prog.unbind();

    receiver->UnBindSharedTexture();
}

void draw2D(const RenderData& data) {
#ifdef SGCT_HAS_TEXT
    constexpr std::string_view Format = "{}:  {} ({} x {})";
    constexpr std::string_view FormatSelected = "{}:  {} ({} x {})   <<---";

    if (senders.empty()) {
        return;
    }

    float xPos = 10.f;
    float yPos = 10.f;
    for (int i = static_cast<int>(senders.size()) - 1; i >= 0; i--) {
        const Sender& sender = senders[i];
        text::print(
            data.window,
            data.viewport,
            *text::FontManager::instance().font("SGCTFont", 16),
            text::Alignment::TopLeft,
            xPos, yPos,
            vec4{ 0.9f, 0.9f, 0.9f, 1.f },
            fmt::format(
                fmt::runtime(i == currentSender ? FormatSelected : Format),
                i, sender.name, sender.width, sender.height)
        );
        yPos += 20.f;
    }
#endif // SGCT_HAS_TEXT
}

void initOGL(GLFWwindow*) {
    receiver = GetSpout();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    constexpr std::array<float, 6 * 2> Vertices = {
        -1.f, -1.f,   1.f, -1.f,   1.f, 1.f,  // bottom right triangle
        -1.f, -1.f,   1.f,  1.f,  -1.f, 1.f   // top left triangle
    };
    glBufferData(
        GL_ARRAY_BUFFER,
        Vertices.size() * sizeof(float),
        Vertices.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);

    ShaderManager::instance().addShaderProgram("xform", VertexShader, FragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();

    glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);

    prog.unbind();
}

void cleanup() {
    if (receiver) {
        receiver->ReleaseReceiver();
        receiver->Release();
    }

    glDeleteBuffers(1, &vbo);
    vbo = 0;
    glDeleteVertexArrays(1, &vao);
    vao = 0;
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (key == Key::Esc && action == Action::Press) {
        Engine::instance().terminate();
    }
    if (key == Key::Down && action == Action::Press) {
        const int nSenders = static_cast<int>(senders.size() - 1);
        currentSender = std::clamp(static_cast<int>(currentSender + 1), 0, nSenders);
    }
    if (key == Key::Up && action == Action::Press) {
        const int nSenders = static_cast<int>(senders.size() - 1);
        currentSender = std::clamp(static_cast<int>(currentSender - 1), 0, nSenders);
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    if (!cluster.success) {
        return -1;
    }

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGL;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.draw2D = draw2D;
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
