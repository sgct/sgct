#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#include <sgct/user.h>
#include <sgct/window.h>
#include <algorithm>
#include <iostream>

namespace {
    sgct::Engine* gEngine;

    enum class Rotation { Deg0 = 0, Deg90, Deg180, Deg270 };
    enum class Sides {
        Right_L = 0, Bottom_L, Top_L, Left_L,
        Right_R, Bottom_R, Top_R, Left_R
    };

    std::string texturePaths[8];
    GLuint textureIndices[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    Rotation sideRotations[] = {
        Rotation::Deg0,
        Rotation::Deg0,
        Rotation::Deg0,
        Rotation::Deg0
    };
    size_t numberOfTextures = 0;

    int stopIndex;
    int numberOfDigits = 0;
    int iterator;
    bool sequence = false;

    int counter = 0;
    int startFrame = 0;
    struct {
        bool alpha = false;
        bool stereo = false;
        bool fxaa = false;
        bool cubic = true;
        int numberOfMSAASamples = 1;
        int resolution = 512;
        int cubemapRes = 256;
        float eyeSeparation = 0.065f;
        float domeDiameter = 14.8f;
    } settings;
    sgct::SharedBool takeScreenshot(false);

    struct {
        GLuint vao;
        GLuint vbo;

        GLuint iboRot0;
        GLuint iboRot90;
        GLuint iboRot180;
        GLuint iboRot270;
    } geometry;
    struct Vertex {
        float x, y;
        float s, t;
    };

    constexpr const char* vertexShader = R"(
#version 330 core

layout(location = 0) in vec2 vertPosition;
layout(location = 1) in vec2 vertUv;

out vec2 uv;

void main() {
  gl_Position = vec4(vertPosition, 0.0, 1.0);
  uv = vertUv;
}
)";

    constexpr const char* fragmentShader = R"(
#version 330 core

uniform sampler2D tex;

in vec2 uv;
out vec4 color;

void main() {
  color = texture(tex, uv);
}
)";

} // namespace

using namespace sgct;

void drawFace(Rotation rot) {
    glBindVertexArray(geometry.vao);
    switch (rot) {
        case Rotation::Deg0:
        default:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboRot0);
            break;
        case Rotation::Deg90:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboRot90);
            break;
        case Rotation::Deg180:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboRot180);
            break;
        case Rotation::Deg270:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboRot270);
            break;
    }

    ShaderManager::instance()->getShaderProgram("simple").bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void drawFun() {
    if (numberOfTextures == 0) {
        return;
    }

    size_t index = counter % numberOfTextures;
    counter++;

    if (!textureIndices[index]) {
        return;
    }

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIndices[index]);

    Sides side = static_cast<Sides>(index);
    if (side == Sides::Left_L || side == Sides::Left_R) {
        drawFace(sideRotations[0]);
    }
    else if (side == Sides::Right_L || side == Sides::Right_R) {
        drawFace(sideRotations[1]);
    }
    else if (side == Sides::Top_L || side == Sides::Top_R) {
        drawFace(sideRotations[2]);
    }
    else {
        // button
        drawFace(sideRotations[3]);
    }

    glPopAttrib();
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
            textureIndices[i] = TextureManager::instance()->loadTexture(
                str,
                true,
                1
            );
        }

        takeScreenshot.setVal(true);
        iterator++;
    }
    else if (sequence && iterator <= stopIndex && numberOfDigits == 0 ) {
        for (size_t i = 0; i < numberOfTextures; i++) {
            textureIndices[i] = TextureManager::instance()->loadTexture(
                texturePaths[i],
                true,
                1
            );
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
    gEngine->getDefaultUser().setEyeSeparation(settings.eyeSeparation);
    for (int i = 0; i < gEngine->getNumberOfWindows(); i++) {
        Window& win = gEngine->getWindow(i);
        gEngine->setScreenShotNumber(startFrame);
        win.setAlpha(settings.alpha);

        for (int j = 0; j < gEngine->getWindow(i).getNumberOfViewports(); j++) {
            core::Viewport& vp = win.getViewport(j);
            if (!vp.hasSubViewports()) {
                continue;
            }
            vp.getNonLinearProjection()->setClearColor(glm::vec4(0.f, 0.f, 0.f, 1.f));
            vp.getNonLinearProjection()->setCubemapResolution(settings.cubemapRes);
            vp.getNonLinearProjection()->setInterpolationMode(
                settings.cubic ?
                core::NonLinearProjection::InterpolationMode::Cubic :
                core::NonLinearProjection::InterpolationMode::Linear
            );

            core::FisheyeProjection* p = dynamic_cast<core::FisheyeProjection*>(
                vp.getNonLinearProjection()
            );
            if (p) {
                p->setDomeDiameter(settings.domeDiameter);
            }
        }
        
        win.setNumberOfAASamples(settings.numberOfMSAASamples);
        win.setFramebufferResolution(
            glm::ivec2(settings.resolution, settings.resolution)
        );
        win.setUseFXAA(settings.fxaa);
        win.setStereoMode(
            settings.stereo ? Window::StereoMode::Dummy : Window::StereoMode::NoStereo
        );
    }
}

void initOGLFun() {
    // load all textures
    if (!sequence) {
        for (size_t i = 0; i < numberOfTextures; i++) {
            textureIndices[i] = TextureManager::instance()->loadTexture(
                texturePaths[i],
                true,
                1,
                TextureManager::CompressionMode::None,
                8.f
            );
        }
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &geometry.vao);
    glGenBuffers(1, &geometry.vbo);
    glGenBuffers(1, &geometry.iboRot0);
    glGenBuffers(1, &geometry.iboRot90);
    glGenBuffers(1, &geometry.iboRot180);
    glGenBuffers(1, &geometry.iboRot270);

    glBindVertexArray(geometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);

    std::vector<Vertex> vertices;
    vertices.push_back({ -1.f, -1.f, 0.f, 0.f }); // 0
    vertices.push_back({ -1.f,  1.f, 0.f, 1.f }); // 1
    vertices.push_back({ 1.f,  1.f, 1.f, 1.f }); // 2
    vertices.push_back({ 1.f, -1.f, 1.f, 0.f }); // 3

    vertices.push_back({ -1.f, -1.f, 1.f, 0.f }); // 4
    vertices.push_back({ -1.f,  1.f, 0.f, 0.f }); // 5
    vertices.push_back({ 1.f,  1.f, 0.f, 1.f }); // 6
    vertices.push_back({ 1.f, -1.f, 1.f, 1.f }); // 7

    vertices.push_back({ -1.f, -1.f, 1.f, 1.f }); // 8
    vertices.push_back({ -1.f,  1.f, 1.f, 0.f }); // 9
    vertices.push_back({ 1.f,  1.f, 0.f, 0.f }); // 10
    vertices.push_back({ 1.f, -1.f, 0.f, 1.f }); // 11

    vertices.push_back({ -1.f, -1.f, 0.f, 1.f }); // 12
    vertices.push_back({ -1.f,  1.f, 1.f, 1.f }); // 13
    vertices.push_back({  1.f,  1.f, 1.f, 0.f }); // 14
    vertices.push_back({  1.f, -1.f, 0.f, 0.f }); // 15

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        nullptr
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<const void*>(2 * sizeof(float))
    );
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboRot0);
        std::vector<uint8_t> indicesRot0 = { 0, 2, 1, 0, 3, 2 };
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indicesRot0.size() * sizeof(uint8_t),
            indicesRot0.data(),
            GL_STATIC_DRAW
        );
    }
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboRot90);
        std::vector<uint8_t> indicesRot90 = { 0, 2, 1, 0, 3, 2 };
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indicesRot90.size() * sizeof(uint8_t),
            indicesRot90.data(),
            GL_STATIC_DRAW
        );
    }
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboRot180);
        std::vector<uint8_t> indicesRot180 = { 0, 2, 1, 0, 3, 2 };
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indicesRot180.size() * sizeof(uint8_t),
            indicesRot180.data(),
            GL_STATIC_DRAW
        );
    }
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboRot270);
        std::vector<uint8_t> indicesRot270 = { 0, 2, 1, 0, 3, 2 };
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indicesRot270.size() * sizeof(uint8_t),
            indicesRot270.data(),
            GL_STATIC_DRAW
        );
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    ShaderManager::instance()->addShaderProgram("simple", vertexShader, fragmentShader);
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
            return Sides::Left_L;
        case 1:
            return Sides::Right_L;
        case 2:
            return Sides::Top_L;
        case 3:
            return Sides::Bottom_L;
        case 4:
            return Sides::Left_R;
        case 5:
            return Sides::Right_R;
        case 6:
            return Sides::Top_R;
        case 7:
            return Sides::Bottom_R;
    }
}


int main(int argc, char* argv[]) {
    std::vector<std::string> arguments(argv + 1, argv + argc);
    Configuration config = parseArguments(arguments);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);
    gEngine = Engine::instance();

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
                        tmpStr = tmpStr.substr(0, j + 1);
                        break;
                    }
                    else {
                        numberOfDigits++;
                    }
                }
            }

            texturePaths[static_cast<int>(getSideIndex(numberOfTextures))] = tmpStr;

            numberOfTextures++;
            MessageHandler::printInfo("Adding texture: %s", argv[i + 1]);
        }
        else if (arg == "-seq" && argc > (i + 2)) {
            sequence = true;
            int startIndex = atoi(argv[i + 1]);
            stopIndex = atoi(argv[i + 2]);
            iterator = startIndex;
            MessageHandler::printInfo(
                "Loading sequence from %d to %d", startIndex, stopIndex
            );
        }
        else if (arg == "-rot" && argc > (i + 4)) {
            glm::ivec4 rotations = glm::ivec4(
                atoi(argv[i + 1]),
                atoi(argv[i + 2]),
                atoi(argv[i + 3]),
                atoi(argv[i + 4])
            );
            MessageHandler::printInfo(
                "Setting image rotations to L: %d, R: %d, T: %d, B: %d",
                rotations.x, rotations.y, rotations.z, rotations.w
            );

            auto convertRotations = [](int v) {
                switch (v) {
                    case 0:
                    default:
                        return Rotation::Deg0;
                    case 90:
                        return Rotation::Deg90;
                    case 180:
                        return Rotation::Deg180;
                    case 270:
                        return Rotation::Deg270;
                }
            };
            sideRotations[0] = convertRotations(rotations[0]);
            sideRotations[1] = convertRotations(rotations[1]);
            sideRotations[2] = convertRotations(rotations[2]);
            sideRotations[3] = convertRotations(rotations[3]);
        }
        else if (arg == "-start" && argc > (i + 1)) {
            startFrame = atoi(argv[i + 1]);
            MessageHandler::printInfo("Start frame set to %d", startFrame);
        }
        else if (arg == "-alpha" && argc > (i + 1)) {
            settings.alpha = std::string_view(argv[i + 1]) == "1";
            MessageHandler::printInfo(
                "Setting alpha to %s", settings.alpha ? "true" : "false"
            );
        }
        else if (arg == "-stereo" && argc > (i + 1)) {
            settings.stereo = std::string_view(argv[i + 1]) == "1";
            MessageHandler::printInfo(
                "Setting stereo to %s", settings.stereo ? "true" : "false"
            );
        }
        else if (arg == "-cubic" && argc > (i + 1)) {
            settings.cubic = std::string_view(argv[i + 1]) == "1";
            MessageHandler::printInfo(
                "Setting cubic interpolation to %s", settings.cubic ? "true" : "false"
            );
        }
        else if (arg == "-fxaa" && argc > (i + 1)) {
            settings.fxaa = std::string_view(argv[i + 1]) == "1";
            MessageHandler::printInfo(
                "Setting fxaa to %s", settings.fxaa ? "true" : "false"
            );
        }
        else if (arg == "-eyeSep" && argc > (i + 1)) {
            settings.eyeSeparation = static_cast<float>(atof(argv[i + 1]));
            MessageHandler::printInfo(
                "Setting eye separation to %f", settings.eyeSeparation
            );
        }
        else if (arg == "-diameter" && argc > (i + 1)) {
            settings.domeDiameter = static_cast<float>(atof(argv[i + 1]));
            MessageHandler::printInfo(
                "Setting dome diameter to %f", settings.domeDiameter
            );
        }
        else if (arg == "-msaa" && argc > (i + 1)) {
        settings.numberOfMSAASamples = atoi(argv[i + 1]);
            MessageHandler::printInfo(
                "Number of MSAA samples set to %d", settings.numberOfMSAASamples
            );
        }
        else if (arg == "-res" && argc > (i + 1)) {
        settings.resolution = atoi(argv[i + 1]);
            MessageHandler::printInfo(
                "Resolution set to %d", settings.resolution
            );
        }
        else if (arg == "-cubemap" && argc > (i + 1)) {
        settings.cubemapRes = atoi(argv[i + 1]);
            MessageHandler::printInfo(
                "Cubemap resolution set to %d", settings.cubemapRes
            );
        }
        else if (arg == "-format" && argc > (i + 1)) {
            std::string_view arg2 = argv[i + 1];
            Settings::CaptureFormat f = [](std::string_view format) {
                if (format == "png" || format == "PNG") {
                    return Settings::CaptureFormat::PNG;
                }
                else if (format == "tga" || format == "TGA") {
                    return Settings::CaptureFormat::TGA;
                }
                else if (format == "jpg" || format == "JPG") {
                    return Settings::CaptureFormat::JPG;
                }
                else {
                    MessageHandler::printInfo("Unknown capturing format. Using PNG");
                    return Settings::CaptureFormat::PNG;
                }
            } (arg2);
            Settings::instance()->setCaptureFormat(f);
            MessageHandler::printInfo("Format set to %s", argv[i + 1]);
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

            MessageHandler::printInfo("Left path set to %s", argv[i + 1]);
        }
        else if (arg == "-rightPath" && argc > (i + 1)) {
            Settings::instance()->setCapturePath(
                argv[i + 1],
                Settings::CapturePath::RightStereo
            );

            MessageHandler::printInfo("Right path set to %s", argv[i + 1]);
        }
    }

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setPreWindowFunction(preWinInitFun);
    gEngine->setEncodeFunction(encodeFun);
    gEngine->setDecodeFunction(decodeFun);

    gEngine->setClearColor(glm::vec4(0.f, 0.f, 0.f, 1.f));

    if (!gEngine->init(Engine::RunMode::Default_Mode, cluster)) {
        Engine::destroy();
        return EXIT_FAILURE;
    }

    gEngine->render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
