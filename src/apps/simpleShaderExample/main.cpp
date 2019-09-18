#include <sgct.h>

namespace {
    sgct::Engine* gEngine;
    
    int timeLoc = -1;
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool reloadShader(false);
} // namespace 

using namespace sgct;

void drawFun() {
    // set current shader program
    ShaderManager::instance()->bindShaderProgram("SimpleColor");
    glUniform1f(timeLoc, static_cast<float>(currentTime.getVal()));

    constexpr const float Speed = 50.f;
    glRotatef(static_cast<float>(currentTime.getVal()) * Speed, 0.f, 1.f, 0.f);

    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f, -0.5f, 0.f);
    glVertex3f(0.f, 0.5f, 0.f);
    glVertex3f(0.5f, -0.5f, 0.f);
    glEnd();

    ShaderManager::instance()->unBindShaderProgram();
}

void initOGLFun() {
    ShaderManager& sm = *ShaderManager::instance();
    sm.addShaderProgram("SimpleColor", "simple.vert", "simple.frag");
    sm.bindShaderProgram("SimpleColor");

    timeLoc = sm.getShaderProgram("SimpleColor").getUniformLocation("curr_time");

    ShaderManager::instance()->unBindShaderProgram();
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeBool(reloadShader);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
    SharedData::instance()->readBool(reloadShader);
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void postSyncPreDrawFun() {
    if (reloadShader.getVal()) {
        reloadShader.setVal(false);

        ShaderManager& sm = *ShaderManager::instance();
        ShaderProgram sp = sm.getShaderProgram("SimpleColor");
        sp.reload();

        sm.bindShaderProgram("SimpleColor");
        timeLoc = sm.getShaderProgram( "SimpleColor").getUniformLocation("curr_time");
        sgct::ShaderManager::instance()->unBindShaderProgram();
    }
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster() && key == key::R && action == action::Press) {
        reloadShader.setVal(true);
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
