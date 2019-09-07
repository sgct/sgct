#include <sgct.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct_utils::SGCTBox> box;
    GLint matrixLoc = -1;

    sgct::SharedDouble currentTime(0.0);
} // namespace

using namespace sgct;

void drawFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr const double Speed = 0.44;

    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3( 0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * Speed),
        glm::vec3(0.f, -1.f, 0.f)
    );
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * (Speed / 2.0)),
        glm::vec3(1.f, 0.f, 0.f)
    );

    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix() * scene;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));

    sgct::ShaderManager::instance()->bindShaderProgram("xform");

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    box->draw();

    ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun(){
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    box = std::make_unique<sgct_utils::SGCTBox>(
        2.f,
        sgct_utils::SGCTBox::TextureMappingMode::Regular
    );

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance()->addShaderProgram(
        "xform",
        "SimpleVertexShader.vertexshader",
        "SimpleFragmentShader.fragmentshader"
    );

    ShaderManager::instance()->bindShaderProgram("xform");
    const ShaderProgram& prg = ShaderManager::instance()->getShaderProgram("xform");
    matrixLoc = prg.getUniformLocation("MVP");
    GLint textureLoc = prg.getUniformLocation("Tex");
    glUniform1i(textureLoc, 0);

    ShaderManager::instance()->unBindShaderProgram();
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
}

void cleanUpFun() {
    box = nullptr;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new sgct::Engine(arg);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setCleanUpFunction(cleanUpFun);

    if (!gEngine->init(sgct::Engine::RunMode::OpenGL_3_3_Core_Profile)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
