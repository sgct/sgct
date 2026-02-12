/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include "box.h"
#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <format>

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
    std::unique_ptr<Box> box;
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
  #version 460 core

  layout(location = 0) in vec2 texCoords;
  layout(location = 1) in vec3 normals;
  layout(location = 2) in vec3 vertPositions;

  out Data {
    vec2 texCoords;
  } out_data;

  uniform mat4 mvp;
  uniform int flip;


  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position = mvp * vec4(vertPositions, 1.0);
    out_data.texCoords.x = texCoords.x;
    if (flip == 0) {
      out_data.texCoords.y = texCoords.y;
    }
    else {
      out_data.texCoords.y = 1.0 - texCoords.y;
    }
  })";

    constexpr std::string_view FragmentShader = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  out vec4 out_color;

  uniform sampler2D tex;


  void main() { out_color = texture(tex, in_data.texCoords); }
)";
} // namespace

using namespace sgct;

bool bindSpout() {
    const bool creationSuccess = receiver->CreateReceiver(senderName, width, height);
    if (!initialized && creationSuccess) {
        Log::Info(std::format(
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

void draw(const RenderData& data) {
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
    const glm::mat4 mvp =
        glm::make_mat4(data.modelViewProjectionMatrix.values.data()) * scene;

    // spout init
    bool spoutStatus = false;
    // check if spout supported (DX11 interop)
    if (glfwExtensionSupported("WGL_NV_DX_interop2")) {
        spoutStatus = bindSpout();
    }

    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");

    // DirectX textures are flipped around the Y axis compared to OpenGL
    if (!spoutStatus) {
        glProgramUniform1i(prog.id(), flipLoc, 0);
        glBindTextureUnit(0, texture);
    }
    else {
        glProgramUniform1i(prog.id(), flipLoc, 1);
    }

    glProgramUniformMatrix4fv(prog.id(), matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    prog.bind();
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

    box = std::make_unique<Box>(2.f, Box::TextureMappingMode::Regular);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance().addShaderProgram("xform", VertexShader, FragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");

    matrixLoc = glGetUniformLocation(prog.id(), "mvp");
    glProgramUniform1i(prog.id(), glGetUniformLocation(prog.id(), "tex"), 0);
    flipLoc = glGetUniformLocation(prog.id(), "flip");
    glProgramUniform1i(prog.id(), flipLoc, 0);
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
