#include <sgct.h>
#include <sgct/commandline.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    sgct::SharedDouble currentTime(0.0);

    GLuint vertexArray = 0;
    GLuint vertexPositionBuffer = 0;
    GLuint vertexColorBuffer = 0;

    GLint matrixLoc = -1;

    constexpr const char* vertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPosition;
  layout(location = 1) in vec3 vertColor;

  uniform mat4 mvp;
  out vec3 fragColor;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position = mvp * vec4(vertPosition, 1.0);
    fragColor = vertColor;
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
    const GLfloat positionData[] = { 
        -0.5f, -0.5f, 0.f,
         0.f, 0.5f, 0.f,
         0.5f, -0.5f, 0.f
    };

    const GLfloat colorData[] = { 
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f
    };

    // generate the VAO
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    // generate VBO for vertex positions
    glGenBuffers(1, &vertexPositionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer);
    // upload data to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(positionData), positionData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    // generate VBO for vertex colors
    glGenBuffers(1, &vertexColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexColorBuffer);
    // upload data to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(colorData), colorData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    // unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    ShaderManager::instance()->addShaderProgram(
        "xform",
        vertexShader,
        fragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    ShaderManager::instance()->bindShaderProgram("xform");
 
    const ShaderProgram& prg = ShaderManager::instance()->getShaderProgram("xform");
    matrixLoc = prg.getUniformLocation("mvp");
 
    ShaderManager::instance()->unBindShaderProgram();
}

void drawFun() {
    constexpr const float Speed = 0.8f;

    glm::mat4 scene = glm::rotate(
        glm::mat4(1.f),
        static_cast<float>(currentTime.getVal()) * Speed,
        glm::vec3(0.f, 1.f, 0.f)
    );
    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene;

    ShaderManager::instance()->bindShaderProgram("xform");
        
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(MVP));
    glBindVertexArray(vertexArray);
    
    // Draw the triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // unbind
    glBindVertexArray(0);
    ShaderManager::instance()->unBindShaderProgram();
}

void preSyncFun() {
    // set the time only on the master
    if (gEngine->isMaster()) {
        // get the time in seconds
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
    glDeleteBuffers(1, &vertexColorBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    // Bind your functions
    gEngine->setInitOGLFunction(initFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    // Init the engine
    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
