#include <sgct.h>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct::utils::Box> box;

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool takeScreenshot(false);

    // shader locs
    int textureID = -1;
    int worldMatrixTransposeId = -1;

} // namespace

using namespace sgct;

void drawFun() {
    constexpr const double Speed = 25.0;
    
    glTranslatef(0.f, 0.f, -3.f);
    glRotated(currentTime.getVal() * Speed, 0.0, -1.0, 0.0);
    glRotated(currentTime.getVal() * (Speed / 2.0), 1.0, 0.0, 0.0);
    glColor3f(1.f, 1.f, 1.f);

    float worldMatrix[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, worldMatrix);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));
    
    // set MRT shader program
    ShaderManager::instance()->bindShaderProgram("MRT");

    glUniform1i(textureID, 0);
    // transpose in transfere
    glUniformMatrix4fv(worldMatrixTransposeId, 1, GL_TRUE, worldMatrix);

    box->draw();
    ShaderManager::instance()->unBindShaderProgram();
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void postSyncPreDrawFun() {
    if (takeScreenshot.getVal()) {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void initOGLFun() {
    ShaderManager::instance()->addShaderProgram("MRT", "mrt.vert", "mrt.frag");
    ShaderManager::instance()->bindShaderProgram("MRT");

    const ShaderProgram& prg = ShaderManager::instance()->getShaderProgram("MRT");
    textureID = prg.getUniformLocation("tDiffuse");
    worldMatrixTransposeId = prg.getUniformLocation("worldMatrixTranspose");

    ShaderManager::instance()->unBindShaderProgram();
    
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "box.png", true);

    box = std::make_unique<sgct::utils::Box>(
        2.f,
        sgct::utils::Box::TextureMappingMode::Regular
    );
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    // Set up backface culling
    glCullFace(GL_BACK);
    // our polygon winding is counter clockwise
    glFrontFace(GL_CCW);
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeBool(takeScreenshot);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
    SharedData::instance()->readBool(takeScreenshot);
}

void cleanUpFun() {
    box = nullptr;
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster()) {
        switch (key) {
            case key::P:
            case key::F10:
                if (action == action::Press) {
                    takeScreenshot.setVal(true);
                }
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    gEngine = new Engine(config);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);

    Settings::instance()->setUseNormalTexture(true);
    Settings::instance()->setUsePositionTexture(true);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
