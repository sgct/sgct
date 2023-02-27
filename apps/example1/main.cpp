/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    double currentTime = 0.0;

    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;

    GLint matrixLoc = -1;

    constexpr std::string_view VertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPosition;
  layout(location = 1) in vec3 vertColor;

  uniform mat4 mvp;
  out vec3 fragColor;

  void main() {
    gl_Position = mvp * vec4(vertPosition, 1.0);
    fragColor = vertColor;
  })";

    constexpr std::string_view FragmentShader = R"(
  #version 330 core

  in vec3 fragColor;
  out vec4 color;

  void main() { color = vec4(fragColor, 1.0); }
)";
} // namespace

using namespace sgct;

void initOGL(GLFWwindow*) {
    constexpr GLfloat PositionData[] = {
        // position           color
        -0.5f, -0.5f, 0.f,   1.f, 0.f, 0.f,
         0.f, 0.5f, 0.f,     0.f, 1.f, 0.f,
         0.5f, -0.5f, 0.f,   0.f, 0.f, 1.f
    };

    // generate the VAO
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    // generate VBO
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    // upload data to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(PositionData), PositionData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const void*>(3 * sizeof(GLfloat))
    );

    glBindVertexArray(0);

    ShaderManager::instance().addShaderProgram("xform", VertexShader, FragmentShader);
    const ShaderProgram& prg = ShaderManager::instance().shaderProgram("xform");
    prg.bind();
    matrixLoc = glGetUniformLocation(prg.id(), "mvp");
    prg.unbind();
}

void draw(const RenderData& data) {
    constexpr float Speed = 0.8f;

    glm::mat4 scene =
        glm::rotate(
        glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -4.f)),
        static_cast<float>(currentTime) * Speed,
        glm::vec3(0.f, 1.f, 0.f)
    );
    const glm::mat4 mvp = glm::make_mat4(data.modelViewProjectionMatrix.values) * scene;

    ShaderManager::instance().shaderProgram("xform").bind();

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    ShaderManager::instance().shaderProgram("xform").unbind();
}

void preSync() {
    if (Engine::instance().isMaster()) {
        currentTime = time();
    }
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
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (key == Key::Esc && action == Action::Press) {
        Engine::instance().terminate();
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
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.draw = draw;
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
    return EXIT_SUCCESS;
}
