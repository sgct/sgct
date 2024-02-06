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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <SpoutLibrary.h>

namespace {
    std::unique_ptr<sgct::utils::Box> box;
    GLint matrixLoc = -1;
    GLuint texture = 0;

    struct SpoutData {
        SPOUTHANDLE spoutSender;
        char senderName[256];
        bool initialized;
    };
    std::vector<SpoutData> spoutSendersData;

    size_t spoutSendersCount = 0;
    std::vector<std::pair<int, bool>> windowData; // index and if lefteye
    std::vector<std::string> senderNames;

    // variables to share across cluster
    double currentTime = 0.0;

    constexpr std::string_view VertexShader = R"(
  #version 330 core

  layout(location = 0) in vec2 texCoords;
  layout(location = 1) in vec3 normals;
  layout(location = 2) in vec3 vertPositions;

  uniform mat4 mvp;
  uniform int flip;

  out vec2 uv;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPositions, 1.0);
    uv = texCoords;
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

void draw(RenderData data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr double Speed = 0.44;

    // create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime * Speed),
        glm::vec3(0.f, -1.f, 0.f)
    );
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime * (Speed / 2.0)),
        glm::vec3(1.f, 0.f, 0.f)
    );
    const glm::mat4 mvp = glm::make_mat4(data.modelViewProjectionMatrix.values) * scene;

    glActiveTexture(GL_TEXTURE0);
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    box->draw();

    prog.unbind();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void postDraw() {
    glActiveTexture(GL_TEXTURE0);

    for (size_t i = 0; i < spoutSendersCount; i++) {
        if (!spoutSendersData[i].initialized) {
            continue;
        }
        const int winIndex = windowData[i].first;
        const bool isLeft = windowData[i].second;

        const std::unique_ptr<Window>& win = Engine::instance().windows()[winIndex];
        const GLuint texId = win->frameBufferTexture(
            isLeft ? Window::TextureIndex::LeftEye : Window::TextureIndex::RightEye
        );

        glBindTexture(GL_TEXTURE_2D, texId);

        spoutSendersData[i].spoutSender->SendTexture(
            texId,
            static_cast<GLuint>(GL_TEXTURE_2D),
            win->framebufferResolution().x,
            win->framebufferResolution().y
        );
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void preSync() {
    if (Engine::instance().isMaster()) {
        currentTime = time();
    }
}

void preWindowInit() {
    std::string baseName = "SGCT_Window";

    const std::vector<std::unique_ptr<Window>>& win = Engine::instance().windows();
    for (int i = 0; i < win.size(); i++) {
        win[i]->setFixResolution(true);

        if (win[i]->isStereo()) {
            senderNames.push_back(baseName + std::to_string(i) + "_Left");
            windowData.push_back(std::pair(i, true));

            senderNames.push_back(baseName + std::to_string(i) + "_Right");
            windowData.push_back(std::pair(i, false));
        }
        else {
            senderNames.push_back(baseName + std::to_string(i));
            windowData.push_back(std::pair(i, true));
        }
    }

    spoutSendersCount = senderNames.size();
}

void initOGL(GLFWwindow*) {
    // setup spout
    // Create a new SpoutData for every SGCT window
    spoutSendersData.resize(spoutSendersCount);
    for (size_t i = 0; i < spoutSendersCount; i++) {
        spoutSendersData[i].spoutSender = GetSpout();

        strcpy_s(spoutSendersData[i].senderName, senderNames[i].c_str());
        int winIndex = windowData[i].first;

        const bool success = spoutSendersData[i].spoutSender->CreateSender(
            spoutSendersData[i].senderName,
            Engine::instance().windows()[winIndex]->framebufferResolution().x,
            Engine::instance().windows()[winIndex]->framebufferResolution().y
        );
        spoutSendersData[i].initialized = success;
    }

    texture = TextureManager::instance().loadTexture("box.png", true);

    box = std::make_unique<utils::Box>(2.f, utils::Box::TextureMappingMode::Regular);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance().addShaderProgram("xform", VertexShader, FragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();

    matrixLoc = glGetUniformLocation(prog.id(), "mvp");
    glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);

    prog.unbind();
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, currentTime);
    return data;
}

void decode(const std::vector<std::byte>& data) {
    unsigned int pos = 0;
    deserializeObject(data, pos, currentTime);
}

void cleanup() {
    box = nullptr;

    for (size_t i = 0; i < spoutSendersCount; i++) {
        spoutSendersData[i].spoutSender->ReleaseSender();
        spoutSendersData[i].spoutSender->Release();
    }
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (key == Key::Esc && action == Action::Press) {
        Engine::instance().terminate();
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
    callbacks.preWindow = preWindowInit;
    callbacks.initOpenGL = initOGL;
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.draw = draw;
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
    Engine::instance().exec();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
