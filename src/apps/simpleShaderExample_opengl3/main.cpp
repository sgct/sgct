#include <sgct.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool reloadShader(false);

    GLuint vertexArray = 0;
    GLuint vertexPositionBuffer = 0;

    GLint matrixLoc = -1;
    GLint timeLoc = -1;
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

    ShaderManager::instance()->addShaderProgram("xform", "simple.vert", "simple.frag");
    ShaderManager::instance()->bindShaderProgram("xform");
    const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("xform");
 
    matrixLoc = prog.getUniformLocation("mvp");
    timeLoc = prog.getUniformLocation("currTime");
 
    ShaderManager::instance()->unBindShaderProgram();
}

void drawFun() {
    constexpr const float Speed = 0.87f;

    const glm::mat4 scene = glm::rotate(
        glm::mat4(1.f),
        static_cast<float>(currentTime.getVal()) * Speed,
        glm::vec3(0.f, 1.f, 0.f)
    );
    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix() * scene;

    ShaderManager::instance()->bindShaderProgram("xform");
        
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(timeLoc, static_cast<float>(currentTime.getVal()));
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    ShaderManager::instance()->unBindShaderProgram();
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void postSyncPreDrawFun() {
    if (reloadShader.getVal()) {
        reloadShader.setVal(false);

        ShaderProgram sp = ShaderManager::instance()->getShaderProgram("xform");
        sp.reload();

        ShaderManager::instance()->bindShaderProgram("xform");
        const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("xform");
        timeLoc = prog.getUniformLocation("currTime");
        matrixLoc = prog.getUniformLocation("mvp");
        sgct::ShaderManager::instance()->unBindShaderProgram();
    }
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster() && key == key::R && action == action::Press) {
        reloadShader.setVal(true);
    }
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeBool(reloadShader);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
    SharedData::instance()->readBool(reloadShader);
}

void cleanUpFun() {
    glDeleteBuffers(1, &vertexPositionBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    // Bind your functions
    gEngine->setInitOGLFunction(initFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    // Init the engine
    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
