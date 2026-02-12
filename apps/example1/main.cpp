/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    double currentTime = 0.0;

    GLuint vao = 0;
    GLuint vbo = 0;

    GLint matrixLoc = -1;

    constexpr std::string_view VertexShader = R"(
  #version 460 core

  layout(location = 0) in vec3 in_position;
  layout(location = 1) in vec3 in_color;

  out Data {
    vec3 color;
  } out_data;

  uniform mat4 mvp;


  void main() {
    gl_Position = mvp * vec4(in_position, 1.0);
    out_data.color = in_color;
  })";

    constexpr std::string_view FragmentShader = R"(
  #version 460 core

  in Data {
    vec3 color;
  } in_data;

  out vec4 out_color;


  void main() { out_color = vec4(in_data.color, 1.0); }
)";
} // namespace

using namespace sgct;

void initOGL(GLFWwindow*) {
    struct Vertex {
        float x;
        float y;
        float z;
        float r;
        float g;
        float b;
    };

    glCreateBuffers(1, &vbo);
    glCreateVertexArrays(1, &vao);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 3 * sizeof(float));

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, r));
    glVertexArrayAttribBinding(vao, 1, 0);

    constexpr std::array<Vertex, 3> Vertices = {
        Vertex{ -0.5f, -0.5f, 0.f,  1.f, 0.f, 0.f },
        Vertex{  0.f,   0.5f, 0.f,  0.f, 1.f, 0.f },
        Vertex{  0.5f, -0.5f, 0.f,  0.f, 0.f, 1.f }
    };
    glNamedBufferStorage(vbo, 3 * sizeof(Vertices), Vertices.data(), GL_NONE_BIT);

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
    const glm::mat4 mvp =
        glm::make_mat4(data.modelViewProjectionMatrix.values.data()) * scene;

    ShaderManager::instance().shaderProgram("xform").bind();

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glBindVertexArray(vao);
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
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
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
