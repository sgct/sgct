#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::SharedDouble currentTime(0.0);

    GLuint vertexArray = 0;
    GLuint vertexPositionBuffer = 0;

    GLint matrixLoc = -1;
    GLint timeLoc = -1;

    constexpr const char* vertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPosition;

  uniform mat4 mvp;
  uniform float currTime;
  out vec3 fragColor;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPosition, 1.0);
    float cVal = abs(sin(currTime * 3.1415926 * 0.2));
    fragColor = vec3(cVal, 1.0 - cVal, 0.0);
  })";

    constexpr const char* fragmentShader = R"(
  #version 330 core

  in vec3 fragColor;
  out vec4 color;

  void main() { color = vec4(fragColor, 1.0); }
)";
} // namespace

using namespace sgct;

void initFun() {
    const GLfloat data[] = { 
        -0.5f, -0.5f, 0.0f,
         0.f, 0.5f, 0.f,
         0.5f, -0.5f, 0.0f
    };

    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    glGenBuffers(1, &vertexPositionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    ShaderManager::instance()->addShaderProgram("xform", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("xform");
    prog.bind();
    matrixLoc = prog.getUniformLocation("mvp");
    timeLoc = prog.getUniformLocation("currTime");
    prog.unbind();
}

void drawFun() {
    constexpr const float Speed = 0.87f;

    const glm::mat4 scene = glm::rotate(
        glm::mat4(1.f),
        static_cast<float>(currentTime.getVal()) * Speed,
        glm::vec3(0.f, 1.f, 0.f)
    );
    const glm::mat4 mvp = Engine::instance()->getCurrentModelViewProjectionMatrix() *
                          scene;

    ShaderManager::instance()->getShaderProgram("xform").bind();
        
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(timeLoc, static_cast<float>(currentTime.getVal()));
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    ShaderManager::instance()->getShaderProgram("xform").unbind();
}

void preSyncFun() {
    if (Engine::instance()->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
}

void cleanUpFun() {
    glDeleteBuffers(1, &vertexPositionBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);

    // Bind your functions
    Engine::instance()->setInitOGLFunction(initFun);
    Engine::instance()->setDrawFunction(drawFun);
    Engine::instance()->setPreSyncFunction(preSyncFun);
    Engine::instance()->setCleanUpFunction(cleanUpFun);
    Engine::instance()->setEncodeFunction(encodeFun);
    Engine::instance()->setDecodeFunction(decodeFun);

    // Init the engine
    if (!Engine::instance()->init(Engine::RunMode::OpenGL_3_3_Core_Profile, cluster)) {
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance()->render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
