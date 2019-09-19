#include <sgct.h>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct_utils::Box> box;
    sgct::SharedDouble currentTime(0.0);
} // namespace

using namespace sgct;


void drawFun() {
    constexpr const double Speed = 25.0;

    glTranslatef(0.f, 0.f, -3.f);
    glRotated(currentTime.getVal() * Speed, 0.0, -1.0, 0.0);
    glRotated(currentTime.getVal() * (Speed / 2.0), 1.0, 0.0, 0.0);
    glColor3f(1.f, 1.f, 1.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));

    box->draw();
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "box.png", true);

    box = std::make_unique<sgct_utils::Box>(
        2.f,
        sgct_utils::Box::TextureMappingMode::Regular
    );

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
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
