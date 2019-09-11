#include <sgct.h>
#include <sgct/ClusterManager.h>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct_utils::SGCTBox> box;

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
    box = std::make_unique<sgct_utils::SGCTBox>(
        0.5f,
        sgct_utils::SGCTBox::TextureMappingMode::Regular
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
    if (gEngine->isMaster() && action == SGCT_PRESS) {
        switch (key) {
            case 'S':
                stats = !stats;
                break;
            case 'I':
                info = !info;
                break;
            case 'P':
            case SGCT_KEY_F10:
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
    gEngine = new sgct::Engine(arg);

    sgct_core::SGCTNode* thisNode = sgct_core::ClusterManager::instance()->getThisNode();
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

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
