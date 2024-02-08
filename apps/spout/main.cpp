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
    std::unique_ptr<sgct::utils::Box> box;
    GLint matrixLoc = -1;
    GLint flipLoc = -1;
    GLuint texture = 0;

    SPOUTHANDLE receiver = nullptr;
    char senderName[256];
    unsigned int width;
    unsigned int height;
    bool initialized = false;

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
    gl_Position = mvp * vec4(vertPositions, 1.0);
    uv.x = texCoords.x;
    if (flip == 0) {
      uv.y = texCoords.y;
    }
    else {
      uv.y = 1.0 - texCoords.y;
    }
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
    const bool creationSuccess = receiver->CreateReceiver(senderName, width, height);
    if (!initialized && creationSuccess) {
        Log::Info(fmt::format(
            "Spout: Initing {}x{} texture from '{}'", width, height, senderName
        ));
        initialized = true;
    }

    if (initialized) {
        const bool receiveSucess = receiver->ReceiveTexture(senderName, width, height);
        if (receiveSucess) {
            return receiver->BindSharedTexture();
        }
        else {
            Log::Info("Spout disconnected");

            // reset if disconnected
            initialized = false;
            senderName[0] = '\0';
            receiver->ReleaseReceiver();
        }
    }

    return false;
}

void draw(RenderData data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr double Speed = 0.44;

    //create scene transform (animation)
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

    // spout init
    bool spoutStatus = false;
    // check if spout supported (DX11 interop)
    if (glfwExtensionSupported("WGL_NV_DX_interop2")) {
        spoutStatus = bindSpout();
    }

    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();

    // DirectX textures are flipped around the Y axis compared to OpenGL
    if (!spoutStatus) {
        glUniform1i(flipLoc, 0);
        glBindTexture(GL_TEXTURE_2D, texture);
    }
    else {
        glUniform1i(flipLoc, 1);
    }

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    box->draw();

    prog.unbind();

    if (spoutStatus) {
        receiver->UnBindSharedTexture();
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSync() {
    if (Engine::instance().isMaster()) {
        currentTime = time();
    }
}

void initOGL(GLFWwindow*) {
    // setup spout
    senderName[0] = '\0';
    receiver = GetSpout();

    box = std::make_unique<utils::Box>(2.f, utils::Box::TextureMappingMode::Regular);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance().addShaderProgram("xform", VertexShader, FragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();

    matrixLoc = glGetUniformLocation(prog.id(), "mvp");
    glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);
    flipLoc = glGetUniformLocation(prog.id(), "flip");
    glUniform1i(flipLoc, 0);

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

    if (receiver) {
        receiver->ReleaseReceiver();
        receiver->Release();
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
    callbacks.initOpenGL = initOGL;
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
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
