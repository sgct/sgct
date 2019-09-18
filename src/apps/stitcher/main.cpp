#include <sgct.h>
#include <sgct/user.h>
#include <sgct/window.h>
#include <algorithm>
#include <iostream>

namespace {
    sgct::Engine* gEngine;

    enum class Rotation { ROT_0_DEG = 0, ROT_90_DEG, ROT_180_DEG, ROT_270_DEG };
    enum class Sides {
        RIGHT_SIDE_L = 0, BOTTOM_SIDE_L, TOP_SIDE_L, LEFT_SIDE_L,
        RIGHT_SIDE_R, BOTTOM_SIDE_R, TOP_SIDE_R, LEFT_SIDE_R
    };

    std::string texturePaths[8];
    GLuint textureIndices[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    Rotation sideRotations[] = {
        Rotation::ROT_0_DEG,
        Rotation::ROT_0_DEG,
        Rotation::ROT_0_DEG,
        Rotation::ROT_0_DEG
    };
    size_t activeTexture = 0;
    size_t numberOfTextures = 0;

    int startIndex;
    int stopIndex;
    int numberOfDigits = 0;
    int iterator;
    bool sequence = false;
    bool cubic = true;

    int counter = 0;
    int startFrame = 0;
    bool alpha = false;
    bool stereo = false;
    bool fxaa = false;
    int numberOfMSAASamples = 1;
    int resolution = 512;
    int cubemapRes = 256;
    float eyeSeparation = 0.065f;
    float domeDiameter = 14.8f;

    //sgct_utils::Dome * dome = NULL;

    //variables to share across cluster
    sgct::SharedBool takeScreenshot(false);

} // namespace

using namespace sgct;

void face(Rotation rot) {
    switch (rot) {
        case Rotation::ROT_0_DEG:
        default:
            glBegin(GL_QUADS);
            glTexCoord2f(0.f, 0.f);
            glVertex2f(0.f, 0.f);
            glTexCoord2f(0.f, 1.f);
            glVertex2f(0.f, 1.f);
            glTexCoord2f(1.f, 1.f);
            glVertex2f(1.f, 1.f);
            glTexCoord2f(1.f, 0.f);
            glVertex2f(1.f, 0.f);
            glEnd();
            break;
        case Rotation::ROT_90_DEG:
            glBegin(GL_QUADS);
            glTexCoord2f(1.f, 0.f);
            glVertex2f(0.f, 0.f);
            glTexCoord2f(0.f, 0.f);
            glVertex2f(0.f, 1.f);
            glTexCoord2f(0.f, 1.f);
            glVertex2f(1.f, 1.f);
            glTexCoord2f(1.f, 1.f);
            glVertex2f(1.f, 0.f);
            glEnd();
            break;
        case Rotation::ROT_180_DEG:
            glBegin(GL_QUADS);
            glTexCoord2f(1.f, 1.f);
            glVertex2f(0.f, 0.f);
            glTexCoord2f(1.f, 0.f);
            glVertex2f(0.f, 1.f);
            glTexCoord2f(0.f, 0.f);
            glVertex2f(1.f, 1.f);
            glTexCoord2f(0.f, 1.f);
            glVertex2f(1.f, 0.f);
            glEnd();
            break;
        case Rotation::ROT_270_DEG:
            glBegin(GL_QUADS);
            glTexCoord2f(0.f, 1.f);
            glVertex2f(0.f, 0.f);
            glTexCoord2f(1.f, 1.f);
            glVertex2f(0.f, 1.f);
            glTexCoord2f(1.f, 0.f);
            glVertex2f(1.f, 1.f);
            glTexCoord2f(0.f, 0.f);
            glVertex2f(1.f, 0.f);
            glEnd();
            break;
    }
}


void drawFun() {
    size_t index = counter % numberOfTextures;
    counter++;

    if (!textureIndices[index]) {
        return;
    }

    // if valid
    // enter ortho mode
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glPushMatrix();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glColor3f(1.f, 1.f, 1.f);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, textureIndices[index]);

    Sides side = static_cast<Sides>(index);
    if (side == Sides::LEFT_SIDE_L || side == Sides::LEFT_SIDE_R) {
        face(sideRotations[0]);
    }
    else if (side == Sides::RIGHT_SIDE_L || side == Sides::RIGHT_SIDE_R) {
        face(sideRotations[1]);
    }
    else if (side == Sides::TOP_SIDE_L || side == Sides::TOP_SIDE_R) {
        face(sideRotations[2]);
    }
    else {
        // button
        face(sideRotations[3]);
    }

    glDisable(GL_TEXTURE_2D);

    glPopAttrib();

    // exit ortho mode
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void preSyncFun() {
    if (sequence && iterator <= stopIndex) {
        for (size_t i = 0; i < numberOfTextures; i++) {
            char tmpStr[256];

            if (numberOfDigits == 0) {
#if (_MSC_VER >= 1400)
                sprintf_s(tmpStr, 256, "%s.png", texturePaths[i].c_str());
#else
                sprintf(tmpStr, "%s.png", texturePaths[i].c_str());
#endif
            }
            else {
                char digitStr[16];
                char zeros[16];
                zeros[0] = '\0';

#if (_MSC_VER >= 1400)
                sprintf_s(digitStr, 16, "%d", iterator);
#else
                sprintf(digitStr, "%d", iterator);
#endif


                size_t currentSize = strlen(digitStr);

                for (size_t j = 0; j < (numberOfDigits - currentSize); j++) {
                    zeros[j] = '0';
                    zeros[j + 1] = '\0';
                }

#if (_MSC_VER >= 1400)
                sprintf_s(
                    tmpStr,
                    256,
                    "%s%s%d.png",
                    texturePaths[i].c_str(),
                    zeros,
                    iterator
                );
#else
                sprintf(tmpStr, "%s%s%d.png", texturePaths[i].c_str(), zeros, iterator);
#endif
            }

            // load the texture
            std::string str(tmpStr);
            const bool loadSuccess = TextureManager::instance()->loadTexture(
                texturePaths[i],
                str,
                true,
                1
            );
            if (!loadSuccess) {
                std::cout << "Error: texture: " << textureIndices[i] << " file: "
                          << str << " count: " << numberOfTextures << '\n';
            }
            else {
                textureIndices[i] = TextureManager::instance()->getTextureId(
                    texturePaths[i]
                );
            }

        }

        takeScreenshot.setVal(true);
        iterator++;
    }
    else if (sequence && iterator <= stopIndex && numberOfDigits == 0 ) {
        for (size_t i = 0; i < numberOfTextures; i++) {
            const bool loadSuccess = TextureManager::instance()->loadTexture(
                texturePaths[i],
                texturePaths[i],
                true,
                1
            );
            if (loadSuccess) {
                textureIndices[i] = TextureManager::instance()->getTextureId(
                    texturePaths[i]
                );
            }
        }

        takeScreenshot.setVal(true);
        iterator++;
    }

    counter = 0;
}

void myPostSyncPreDrawFun() {
    if (takeScreenshot.getVal()) {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void preWinInitFun() {
    gEngine->getDefaultUser().setEyeSeparation(eyeSeparation);
    for (int i = 0; i < gEngine->getNumberOfWindows(); i++) {
        Window& win = gEngine->getWindow(i);
        gEngine->setScreenShotNumber(startFrame);
        win.setAlpha(alpha);

        for (int j = 0; j < gEngine->getWindow(i).getNumberOfViewports(); j++) {
            sgct_core::Viewport& vp = win.getViewport(j);
            if (!vp.hasSubViewports()) {
                continue;
            }
            vp.getNonLinearProjection()->setClearColor(glm::vec4(0.f, 0.f, 0.f, 1.f));
            vp.getNonLinearProjection()->setCubemapResolution(cubemapRes);
            vp.getNonLinearProjection()->setInterpolationMode(
                sgct_core::NonLinearProjection::InterpolationMode::Cubic
            );

            sgct_core::FisheyeProjection* p = dynamic_cast<sgct_core::FisheyeProjection*>(
                vp.getNonLinearProjection()
            );
            if (p) {
                p->setDomeDiameter(domeDiameter);
            }
        }
        
        win.setNumberOfAASamples(numberOfMSAASamples);
        win.setFramebufferResolution(glm::ivec2(resolution, resolution));
        win.setUseFXAA(fxaa);
        if (stereo) {
            win.setStereoMode(Window::StereoMode::Dummy);
        }
        else {
            win.setStereoMode(Window::StereoMode::NoStereo);
        }
    }
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::None);
    TextureManager::instance()->setOverWriteMode(true);

    // load all textures
    if (!sequence) {
        for (size_t i = 0; i < numberOfTextures; i++) {
            const bool loadSuccess = TextureManager::instance()->loadTexture(
                texturePaths[i],
                texturePaths[i],
                true,
                1
            );
            if (loadSuccess) {
                textureIndices[i] = TextureManager::instance()->getTextureId(
                    texturePaths[i]
                );
            }
        }
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void encodeFun() {
    SharedData::instance()->writeBool(takeScreenshot);
}

void decodeFun() {
    SharedData::instance()->readBool(takeScreenshot);
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster() && action == action::Press) {
        switch (key) {
            case key::P:
            case key::F10:
                takeScreenshot.setVal(true);
                break;
        }
    }
}

Sides getSideIndex(size_t index) {
    switch (index) {
        case 0:
        default:
            return Sides::LEFT_SIDE_L;
        case 1:
            return Sides::RIGHT_SIDE_L;
        case 2:
            return Sides::TOP_SIDE_L;
        case 3:
            return Sides::BOTTOM_SIDE_L;
        case 4:
            return Sides::LEFT_SIDE_R;
        case 5:
            return Sides::RIGHT_SIDE_R;
        case 6:
            return Sides::TOP_SIDE_R;
        case 7:
            return Sides::BOTTOM_SIDE_R;
    }
}


int main(int argc, char* argv[]) {
    std::vector<std::string> arguments(argv + 1, argv + argc);
    gEngine = new Engine(arguments);

    // parse arguments
    for (int i = 0; i < argc; i++) {
        std::string_view arg = argv[i];
        if (arg == "-tex" && argc > (i + 1)) {
            std::string tmpStr(argv[i + 1]);
            size_t found = tmpStr.find_last_of(".");
            if (found != std::string::npos && found > 0) {
                numberOfDigits = 0;
                for (size_t j = found - 1; j != 0; j--) {
                    // if not numerical
                    if (tmpStr[j] < '0' || tmpStr[j] > '9') {
                        tmpStr = tmpStr.substr(0, i + 1);
                        break;
                    }
                    else {
                        numberOfDigits++;
                    }
                }
            }

            texturePaths[static_cast<int>(getSideIndex(numberOfTextures))] = tmpStr;

            numberOfTextures++;
            MessageHandler::instance()->print("Adding texture: %s\n", argv[i + 1]);
        }
        else if (arg == "-seq" && argc > (i + 2)) {
            sequence = true;
            startIndex = atoi(argv[i + 1]);
            stopIndex = atoi(argv[i + 2]);
            iterator = startIndex;
            MessageHandler::instance()->print(
                "Loading sequence from %d to %d\n", startIndex, stopIndex
            );
        }
        else if (arg == "-rot" && argc > (i + 4)) {
            glm::ivec4 rotations = glm::ivec4(
                atoi(argv[i + 1]),
                atoi(argv[i + 2]),
                atoi(argv[i + 3]),
                atoi(argv[i + 4])
            );
            MessageHandler::instance()->print(
                "Setting image rotations to L: %d, R: %d, T: %d, B: %d\n",
                rotations.x, rotations.y, rotations.z, rotations.w
            );

            auto convertRotations = [](int v) {
                switch (v) {
                    case 0:
                    default:
                        return Rotation::ROT_0_DEG;
                    case 90:
                        return Rotation::ROT_90_DEG;
                    case 180:
                        return Rotation::ROT_180_DEG;
                    case 270:
                        return Rotation::ROT_270_DEG;
                }
            };
            sideRotations[0] = convertRotations(rotations[0]);
            sideRotations[1] = convertRotations(rotations[1]);
            sideRotations[2] = convertRotations(rotations[2]);
            sideRotations[3] = convertRotations(rotations[3]);
        }
        else if (arg == "-start" && argc > (i + 1)) {
            startFrame = atoi(argv[i + 1]);
            MessageHandler::instance()->print("Start frame set to %d\n", startFrame);
        }
        else if (arg == "-alpha" && argc > (i + 1)) {
            alpha = std::string_view(argv[i + 1]) == "1";
            MessageHandler::instance()->print(
                "Setting alpha to %s\n", alpha ? "true" : "false"
            );
        }
        else if (arg == "-stereo" && argc > (i + 1)) {
            stereo = std::string_view(argv[i + 1]) == "1";
            MessageHandler::instance()->print(
                "Setting stereo to %s\n", stereo ? "true" : "false"
            );
        }
        else if (arg == "-cubic" && argc > (i + 1)) {
            cubic = std::string_view(argv[i + 1]) == "1";
            MessageHandler::instance()->print(
                "Setting cubic interpolation to %s\n", cubic ? "true" : "false"
            );
        }
        else if (arg == "-fxaa" && argc > (i + 1)) {
            fxaa = std::string_view(argv[i + 1]) == "1";
            MessageHandler::instance()->print(
                "Setting fxaa to %s\n", fxaa ? "true" : "false"
            );
        }
        else if (arg == "-eyeSep" && argc > (i + 1)) {
            eyeSeparation = static_cast<float>(atof(argv[i + 1]));
            MessageHandler::instance()->print(
                "Setting eye separation to %f\n", eyeSeparation
            );
        }
        else if (arg == "-diameter" && argc > (i + 1)) {
            domeDiameter = static_cast<float>(atof(argv[i + 1]));
            MessageHandler::instance()->print(
                "Setting dome diameter to %f\n", domeDiameter
            );
        }
        else if (arg == "-msaa" && argc > (i + 1)) {
            numberOfMSAASamples = atoi(argv[i + 1]);
            MessageHandler::instance()->print(
                "Number of MSAA samples set to %d\n", numberOfMSAASamples
            );
        }
        else if (arg == "-res" && argc > (i + 1)) {
            resolution = atoi(argv[i + 1]);
            MessageHandler::instance()->print("Resolution set to %d\n", resolution);
        }
        else if (arg == "-cubemap" && argc > (i + 1)) {
            cubemapRes = atoi(argv[i + 1]);
            MessageHandler::instance()->print(
                "Cubemap resolution set to %d\n", cubemapRes
            );
        }
        else if (arg == "-format" && argc > (i + 1)) {
            std::string_view arg2 = argv[i + 1];
            sgct::Settings::CaptureFormat f = [](std::string_view format) {
                if (format == "png" || format == "PNG") {
                    return sgct::Settings::CaptureFormat::PNG;
                }
                else if (format == "tga" || format == "TGA") {
                    return sgct::Settings::CaptureFormat::TGA;
                }
                else if (format == "jpg" || format == "JPG") {
                    return sgct::Settings::CaptureFormat::JPG;
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        "Unknown capturing format. Using PNG\n"
                    );
                    return sgct::Settings::CaptureFormat::PNG;
                }
            } (arg2);
            Settings::instance()->setCaptureFormat(f);
            sgct::MessageHandler::instance()->print("Format set to %s\n", argv[i + 1]);
        }
        else if (arg == "-leftPath" && argc > (i + 1)) {
            Settings::instance()->setCapturePath(
                argv[i + 1],
                Settings::CapturePath::Mono
            );
            Settings::instance()->setCapturePath(
                argv[i + 1],
                Settings::CapturePath::LeftStereo
            );

            MessageHandler::instance()->print("Left path set to %s\n", argv[i + 1]);
        }
        else if (arg == "-rightPath" && argc > (i + 1)) {
            Settings::instance()->setCapturePath(
                argv[i + 1],
                Settings::CapturePath::RightStereo
            );

            MessageHandler::instance()->print("Right path set to %s\n", argv[i + 1]);
        }
        else if (arg == "-compression" && argc > (i + 1)) {
            int tmpi = atoi(argv[i + 1]);
            Settings::instance()->setPNGCompressionLevel(tmpi);

            MessageHandler::instance()->print("Compression set to %d\n", tmpi);
        }
    }

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setPreWindowFunction(preWinInitFun);

    gEngine->setClearColor(0.f, 0.f, 0.f, 1.f);

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
