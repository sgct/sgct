#include "dome.h"
#include <sgct.h>
#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/image.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <sgct/shareddata.h>
#include <sgct/shareddatatypes.h>
#include <sgct/texturemanager.h>
#include <memory>

#ifdef SGCT_HAS_TEXT
#include <sgct/Font.h>
#include <sgct/FontManager.h>
#include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<Dome> gDome;
    std::vector<unsigned char> gData;

    sgct::SharedInt16 displayState(0);
    sgct::SharedInt16 colorState(0);
    sgct::SharedBool showGeoCorrectionPattern(true);
    sgct::SharedBool showBlendZones(false);
    sgct::SharedBool showChannelZones(false);
    sgct::SharedBool showId(false);
    sgct::SharedBool takeScreenShot(false);
    sgct::SharedBool wireframe(false);
    sgct::SharedBool warping(true);
    sgct::SharedInt32 textureIndex(0);

    const int16_t lastState = 6;
    bool ctrlPressed = false;
    bool shiftPressed = false;
    bool useShader = true;
    bool isTiltSet = false;
    bool useDisplayLists = false;
    float tilt = 0.0;
    float radius = 7.4f;

    std::vector<glm::vec3> colors;
    std::vector<std::pair<std::string, unsigned int>> textures;
} // namespace

using namespace sgct;

void drawGeoCorrPatt() {
    if (showGeoCorrectionPattern.getVal()) {
        gDome->drawGeoCorrPattern();
    }
}

void drawColCorrPatt() {
    gDome->drawColCorrPattern(
        &colors[colorState.getVal()],
        static_cast<Dome::PatternMode>(static_cast<int>((displayState.getVal() - 1)) % 5)
    );
}

void drawTexturedObject() {
    if (static_cast<int32_t>(textures.size()) > textureIndex.getVal()) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[textureIndex.getVal()].second);
        glEnable(GL_TEXTURE_2D);

        gDome->drawTexturedSphere();

        glDisable(GL_TEXTURE_2D);
    }
}

void draw() {
    glDepthMask(GL_FALSE);
    
    switch (displayState.getVal()) {
        case 0:
        default:
            drawGeoCorrPatt();
            break;
        case 1:
            drawColCorrPatt();
            break;
        case 2:
            drawColCorrPatt();
            break;
        case 3:
            drawColCorrPatt();
            break;
        case 4:
            drawColCorrPatt();
            break;
        case 5:
            drawColCorrPatt();
            break;
        case 6:
            drawTexturedObject();
            break;
    }
    
    if (showBlendZones.getVal()) {
        gDome->drawBlendZones();
    }
    
    if (showChannelZones.getVal()) {
        gDome->drawChannelZones();
    }

#ifdef SGCT_HAS_TEXT
    if (showId.getVal()) {
        sgct::Window& win = gEngine->getCurrentWindow();
        sgct_core::BaseViewport* vp = win.getCurrentViewport();
        const float w = static_cast<float>(win.getResolution().x) * vp->getSize().x;
        const float h = static_cast<float>(win.getResolution().y) * vp->getSize().y;
        
        
        const float offset = w / 2.f - w / 7.f;
        
        const float s1 = h / 8.f;
        sgct_text::Font* f1 = sgct_text::FontManager::instance()->getFont(
            "SGCTFont",
            static_cast<unsigned int>(s1)
        );

        sgct_text::print(
            *f1,
            sgct_text::TextAlignMode::TopLeft,
            offset,
            h / 2.f - s1,
            glm::vec4(0.f, 0.f, 1.f, 1.f),
            "%d",
            sgct_core::ClusterManager::instance()->getThisNodeId()
        );

        const float s2 = h / 20.f;
        sgct_text::Font* f2 = sgct_text::FontManager::instance()->getFont(
            "SGCTFont",
            static_cast<unsigned int>(s2)
        );
        sgct_text::print(
            *f2,
            sgct_text::TextAlignMode::TopLeft,
            offset,
            h / 2.f - (s1 + s2) * 1.2f,
            glm::vec4(0.f, 0.f, 1.f, 1.f),
            "%s",
            sgct_core::ClusterManager::instance()->getThisNode()->getAddress().c_str()
        );
    }
#endif // SGCT_HAS_TEXT

    glDepthMask(GL_TRUE);
}

void initGL() {
    colors.push_back(glm::vec3(1.00f, 1.00f, 1.00f)); //white
    colors.push_back(glm::vec3(0.25f, 0.25f, 0.25f)); //25% gray
    colors.push_back(glm::vec3(0.50f, 0.50f, 0.50f)); //50% gray
    colors.push_back(glm::vec3(0.75f, 0.75f, 0.75f)); //75% gray
    colors.push_back(glm::vec3(1.00f, 0.00f, 0.00f)); //red
    colors.push_back(glm::vec3(1.00f, 0.50f, 0.00f)); //orange
    colors.push_back(glm::vec3(1.00f, 1.00f, 0.00f)); //yellow
    colors.push_back(glm::vec3(0.50f, 1.00f, 0.00f)); //yellow-green
    colors.push_back(glm::vec3(0.00f, 1.00f, 0.00f)); //green
    colors.push_back(glm::vec3(0.00f, 1.00f, 0.50f)); //green-cyan
    colors.push_back(glm::vec3(0.00f, 1.00f, 1.00f)); //cyan
    colors.push_back(glm::vec3(0.00f, 0.50f, 1.00f)); //cyan-blue
    colors.push_back(glm::vec3(0.00f, 0.00f, 1.00f)); //blue
    colors.push_back(glm::vec3(0.50f, 0.00f, 1.00f)); //blue-magenta
    colors.push_back(glm::vec3(1.00f, 0.00f, 1.00f)); //magenta
    colors.push_back(glm::vec3(1.00f, 0.00f, 0.50f)); //magenta-red
    colors.push_back(glm::vec3(1.00f, 0.00f, 0.00f)); //red
    
    gDome = std::make_unique<Dome>(radius, isTiltSet ? tilt : 0.f);
    gDome->generateDisplayList();
    
    TextureManager::instance()->setAnisotropicFilterSize(4.f);
    for (size_t i = 0; i < textures.size(); i++) {
        TextureManager::instance()->loadUnManagedTexture(
            textures[i].second,
            textures[i].first,
            true,
            4
        );
    }
    
    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_COLOR_MATERIAL);
    //glEnable(GL_NORMALIZE);
}

void postSync() {
    if (takeScreenShot.getVal()) {
        takeScreenShot.setVal(false);
        gEngine->takeScreenshot();
    }
    
    gEngine->setWireframe(wireframe.getVal());
    sgct::Settings::instance()->setUseWarping(warping.getVal());
}

void encode() {
    sgct::SharedData::instance()->writeInt16(displayState);
    sgct::SharedData::instance()->writeInt16(colorState);
    sgct::SharedData::instance()->writeBool(showGeoCorrectionPattern);
    sgct::SharedData::instance()->writeBool(showBlendZones);
    sgct::SharedData::instance()->writeBool(showChannelZones);
    sgct::SharedData::instance()->writeBool(takeScreenShot);
    sgct::SharedData::instance()->writeBool(wireframe);
    sgct::SharedData::instance()->writeBool(warping);
    sgct::SharedData::instance()->writeBool(showId);
    sgct::SharedData::instance()->writeInt32(textureIndex);
}

void decode()
{
    sgct::SharedData::instance()->readInt16(displayState);
    sgct::SharedData::instance()->readInt16(colorState);
    sgct::SharedData::instance()->readBool(showGeoCorrectionPattern);
    sgct::SharedData::instance()->readBool(showBlendZones);
    sgct::SharedData::instance()->readBool(showChannelZones);
    sgct::SharedData::instance()->readBool(takeScreenShot);
    sgct::SharedData::instance()->readBool(wireframe);
    sgct::SharedData::instance()->readBool(warping);
    sgct::SharedData::instance()->readBool(showId);
    sgct::SharedData::instance()->readInt32(textureIndex);
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster()) {
        switch (key) {
            case key::LeftControl:
            case key::RightControl:
                ctrlPressed = (action == action::Repeat || action == action::Press);
                break;
            case key::LeftShift:
            case key::RightShift:
                shiftPressed = (action == action::Repeat || action == action::Press);
                break;
            case key::Left:
                if (action == action::Press) {
                    if (displayState.getVal() > 0) {
                        displayState.setVal(displayState.getVal() - 1);
                    }
                    else {
                        displayState.setVal(lastState);
                    }
                }
                break;
            case key::Right:
                if (action == action::Press) {
                    if (displayState.getVal() < lastState) {
                        displayState.setVal(displayState.getVal() + 1);
                    }
                    else {
                        displayState.setVal(0);
                    }
                }
                break;
            case key::Down:
                if (action == action::Press) {
                    if (colorState.getVal() > 0) {
                        colorState.setVal(colorState.getVal() - 1);
                    }
                    else {
                        colorState.setVal(static_cast<int16_t>(colors.size() - 1));
                    }
                }
                break;
            case key::Up:
                if (action == action::Press) {
                    if (colorState.getVal() < static_cast<int16_t>(colors.size() - 1)) {
                        colorState.setVal(colorState.getVal() + 1);
                    }
                    else {
                        colorState.setVal(0);
                    }
                }
                break;
            case key::B:
                if (action == action::Press) {
                    showBlendZones.setVal(!showBlendZones.getVal());
                }
                break;
            case key::C:
                if (action == action::Press) {
                    showChannelZones.setVal(!showChannelZones.getVal());
                }
                break;
            case key::G:
                if (action == action::Press) {
                    showGeoCorrectionPattern.setVal(!showGeoCorrectionPattern.getVal());
                }
                break;
            case key::I:
                if (action == action::Press) {
                    showId.setVal(showId.getVal());
                }
                break;
            case key::P:
                if (action == action::Press) {
                    takeScreenShot.setVal(true);
                }
                break;
            case key::W:
                if (action == action::Press) {
                    if (ctrlPressed) {
                        warping.setVal(!warping.getVal());
                    }
                    else {
                        wireframe.setVal(!wireframe.getVal());
                    }
                }
                break;
            case key::Space:
                if (action == action::Press) {
                    textureIndex.setVal((textureIndex.getVal() + 1) % textures.size());
                }
                break;
        }
    }
}

void screenShot(sgct_core::Image* im, size_t winIndex,
                sgct_core::ScreenCapture::EyeIndex ei)
{
    std::string eye;
    switch (ei) {
        case sgct_core::ScreenCapture::EyeIndex::Mono:
        default:
            eye = "mono";
            break;
        case sgct_core::ScreenCapture::EyeIndex::StereoLeft:
            eye = "left";
            break;
        case sgct_core::ScreenCapture::EyeIndex::StereoRight:
            eye = "Right";
            break;
    }
    
    MessageHandler::instance()->print(
        "Taking screenshot %dx%d %d bpp, win=%u %s\n",
        im->getWidth(), im->getHeight(), im->getChannels() * 8, winIndex, eye.c_str()
    );

    size_t lastAllocSize = 0;
    const size_t dataSize = im->getWidth() * im->getChannels() * im->getChannels();

    if (gData.empty()) {
        gData.resize(dataSize);
        lastAllocSize = dataSize;
    }
    else if (lastAllocSize < dataSize) {
        MessageHandler::instance()->print("Re-allocating image data to\n");
        gData.resize(dataSize);
        lastAllocSize = dataSize;
    }
    
    double t0 = Engine::getTime();
    memcpy(gData.data(), im->getData(), dataSize);
    MessageHandler::instance()->print(
        "Time to copy %.3f ms\n",
        (sgct::Engine::getTime() - t0) * 1000.0
    );
}

void cleanUp() {
    for (size_t i = 0; i < textures.size(); i++) {
        glDeleteTextures(1, &(textures[i].second));
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    MessageHandler::instance()->setNotifyLevel(MessageHandler::Level::NotifyAll);

    // parse arguments
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-tex") == 0 && argc > (i + 1)) {
            std::pair<std::string, unsigned int> tmpPair(argv[i + 1], GL_FALSE);
            textures.push_back(tmpPair);

            MessageHandler::instance()->print("Adding texture: %s\n", argv[i + 1]);
        }
        else if (strcmp(argv[i], "-tilt") == 0 && argc > (i + 1)) {
            tilt = static_cast<float>(atof(argv[i + 1]));
            isTiltSet = true;

            MessageHandler::instance()->print("Setting tilt to: %f\n", tilt);
        }
        else if (strcmp(argv[i], "-radius") == 0 && argc > (i + 1)) {
            radius = static_cast<float>(atof(argv[i + 1]));
            isTiltSet = true;

            MessageHandler::instance()->print("Setting radius to: %f\n", radius);
        }
        else if (strcmp(argv[i], "--use-display-lists") == 0) {
            useDisplayLists = true;
            MessageHandler::instance()->print(
                "Display lists will be used in legacy pipeline\n"
            );
        }
    }

    if (useDisplayLists) {
        sgct_core::ClusterManager::instance()->setMeshImplementation(
            sgct_core::ClusterManager::MeshImplementation::DisplayList
        );
    }
    else {
        sgct_core::ClusterManager::instance()->setMeshImplementation(
            sgct_core::ClusterManager::MeshImplementation::BufferObjects
        );
    }

    Settings::instance()->setCaptureFromBackBuffer(true);

    // Bind your functions
    gEngine->setDrawFunction(draw);
    gEngine->setInitOGLFunction(initGL);
    gEngine->setPostSyncPreDrawFunction(postSync);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setCleanUpFunction(cleanUp);
    //gEngine->setScreenShotCallback( screenShot );
    sgct::SharedData::instance()->setEncodeFunction(encode);
    sgct::SharedData::instance()->setDecodeFunction(decode);

    // Init the engine
    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    // Main loop
    gEngine->render();

    // Clean up (de-allocate)
    delete gEngine;

    // Exit program
    exit(EXIT_SUCCESS);
}
