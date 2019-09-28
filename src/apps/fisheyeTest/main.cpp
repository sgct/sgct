#include <sgct.h>
#include <sgct/ClusterManager.h>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct::utils::Box> box;

    bool info = false;
    bool stats = false;
    bool takeScreenshot = false;
} // namespace

using namespace sgct;

void drawFun() {
    glColor3f(1.f, 1.f, 1.f);
    glEnable(GL_TEXTURE_2D);

    constexpr const float DomeRadius = 7.5f;

    // right face
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("right"));
    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f);
    glVertex3f(0.f, -DomeRadius, -DomeRadius);
    glTexCoord2f(1.f, 0.f);
    glVertex3f(DomeRadius, -DomeRadius, 0.f);
    glTexCoord2d(1.f, 1.f);
    glVertex3f(DomeRadius, DomeRadius, 0.f);
    glTexCoord2d(0.f, 1.f);
    glVertex3f(0.f, DomeRadius, -DomeRadius);
    glEnd();

    // left face
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("left"));
    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f);
    glVertex3f(-DomeRadius, -DomeRadius, 0.f);
    glTexCoord2f(1.f, 0.f);
    glVertex3f(0.f, -DomeRadius, -DomeRadius);
    glTexCoord2f(1.f, 1.f);
    glVertex3f(0.f, DomeRadius, -DomeRadius);
    glTexCoord2f(0.f, 1.f);
    glVertex3f(-DomeRadius, DomeRadius, 0.f);
    glEnd();

    // top face
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("top"));
    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f);
    glVertex3f(0.f, DomeRadius, DomeRadius);
    glTexCoord2f(1.f, 0.f);
    glVertex3f(DomeRadius, DomeRadius, 0.f);
    glTexCoord2f(1.f, 1.f);
    glVertex3f(0.f, DomeRadius,  -DomeRadius);
    glTexCoord2f(0.f, 1.f);
    glVertex3f(-DomeRadius, DomeRadius, 0.f);
    glEnd();

    // bottom face
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("bottom"));
    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f);
    glVertex3f(0.f, -DomeRadius, DomeRadius);
    glTexCoord2f(1.f, 0.f);
    glVertex3f(-DomeRadius,-DomeRadius, 0.f);
    glTexCoord2f(1.f, 1.f);
    glVertex3f(0.f, -DomeRadius, -DomeRadius);
    glTexCoord2f(0.f, 1.f);
    glVertex3f(DomeRadius, -DomeRadius, 0.f);
    glEnd();
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::None);
    TextureManager::instance()->loadTexture("right", "grid_right.png", true, 4);
    TextureManager::instance()->loadTexture("left", "grid_left.png", true, 4);
    TextureManager::instance()->loadTexture("top", "grid_top.png", true, 4);
    TextureManager::instance()->loadTexture("bottom", "grid_bottom.png", true, 4);

    TextureManager::instance()->loadTexture("box", "box.png", true, 4);
    box = std::make_unique<sgct::utils::Box>(
        0.5f,
        sgct::utils::Box::TextureMappingMode::Regular
    );

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
}

void myPostSyncPreDrawFun() {
    gEngine->setDisplayInfoVisibility(info);
    gEngine->setStatsGraphVisibility(stats);

    if (takeScreenshot) {
        gEngine->takeScreenshot();
        takeScreenshot = false;
    }
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster() && action == action::Press) {
        switch (key) {
            case key::S:
                stats = !stats;
                break;
            case key::I:
                info = !info;
                break;
            case key::P:
            case key::F10:
                takeScreenshot = true;
                break;
        }
    }
}

void cleanUpFun() {
    box = nullptr;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    sgct::core::Node* thisNode = sgct::core::ClusterManager::instance()->getThisNode();
    if (thisNode) {
        for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
            thisNode->getWindow(i).setAlpha(true);
        }
    }

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setCleanUpFunction(cleanUpFun);

    if (!gEngine->init(Engine::RunMode::Default_Mode, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
