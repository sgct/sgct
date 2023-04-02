/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <sgct/projection/fisheye.h>
#include <sgct/projection/nonlinearprojection.h>
#include <sgct/user.h>
#include <fmt/format.h>
#include <cstring>

namespace {
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
    bool takeScreenshot = false;

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

    constexpr std::string_view VertexShader = R"(
#version 330 core

layout(location = 0) in vec2 vertPosition;
layout(location = 1) in vec2 vertUv;

out vec2 uv;

void main() {
  gl_Position = vec4(vertPosition, 0.0, 1.0);
  uv = vertUv;
}
)";

    constexpr std::string_view FragmentShader = R"(
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

    ShaderManager::instance().shaderProgram("simple").bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void draw(const RenderData&) {
    if (numberOfTextures == 0) {
        return;
    }

    size_t index = counter % numberOfTextures;
    counter++;

    if (!textureIndices[index]) {
        return;
    }

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
}

void preSync() {
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
            textureIndices[i] = TextureManager::instance().loadTexture(
                str,
                true,
                1
            );
        }

        takeScreenshot = true;
        iterator++;
    }
    else if (sequence && iterator <= stopIndex && numberOfDigits == 0) {
        for (size_t i = 0; i < numberOfTextures; i++) {
            textureIndices[i] = TextureManager::instance().loadTexture(
                texturePaths[i],
                true,
                1
            );
        }

        takeScreenshot = true;
        iterator++;
    }

    counter = 0;
}

void postSyncPreDraw() {
    if (takeScreenshot) {
        Engine::instance().takeScreenshot();
        takeScreenshot = false;
    }
}

void preWinInit() {
    Engine::instance().defaultUser().setEyeSeparation(settings.eyeSeparation);
    for (const std::unique_ptr<Window>& win : Engine::instance().windows()) {
        Engine::instance().setScreenShotNumber(startFrame);
        win->setAlpha(settings.alpha);

        for (const std::unique_ptr<Viewport>& vp : win->viewports()) {
            if (!vp->hasSubViewports()) {
                continue;
            }
            vp->nonLinearProjection()->setClearColor(vec4{ 0.f, 0.f, 0.f, 1.f });
            vp->nonLinearProjection()->setCubemapResolution(settings.cubemapRes);
            vp->nonLinearProjection()->setInterpolationMode(
                settings.cubic ?
                NonLinearProjection::InterpolationMode::Cubic :
                NonLinearProjection::InterpolationMode::Linear
            );

            FisheyeProjection* p = dynamic_cast<FisheyeProjection*>(
                vp->nonLinearProjection()
            );
            if (p) {
                p->setDomeDiameter(settings.domeDiameter);
            }
        }

        win->setNumberOfAASamples(settings.numberOfMSAASamples);
        win->setFramebufferResolution(ivec2{ settings.resolution, settings.resolution });
        win->setUseFXAA(settings.fxaa);
        win->setStereoMode(
            settings.stereo ? Window::StereoMode::Dummy : Window::StereoMode::NoStereo
        );
    }
}

void initOGL(GLFWwindow*) {
    // load all textures
    if (!sequence) {
        for (size_t i = 0; i < numberOfTextures; i++) {
            textureIndices[i] = TextureManager::instance().loadTexture(
                texturePaths[i],
                true,
                8.f
            );
        }
    }

    glEnable(GL_DEPTH_TEST);
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

    ShaderManager::instance().addShaderProgram("simple", vertexShader, fragmentShader);
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, takeScreenshot);
    return data;
}

void decode(const std::vector<std::byte>& data, unsigned int pos) {
    deserializeObject(data, pos, takeScreenshot);
}

void keyboard(Key key, Modifier, Action action, int) {
    if (Engine::instance().isMaster() && action == Action::Press) {
        switch (key) {
            case Key::Esc:
                Engine::instance().terminate();
                break;
            case Key::P:
            case Key::F10:
                takeScreenshot = true;
                break;
            default:
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


int main(int argc, char** argv) {
    std::vector<std::string> arguments(argv + 1, argv + argc);
    Configuration config = parseArguments(arguments);
    config::Cluster cluster = loadCluster(config.configFilename);
    if (!cluster.success) {
        return -1;
    }

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
            Log::Info(fmt::format("Adding texture: {}", argv[i + 1]));
        }
        else if (arg == "-seq" && argc > (i + 2)) {
            sequence = true;
            int startIndex = atoi(argv[i + 1]);
            stopIndex = atoi(argv[i + 2]);
            iterator = startIndex;
            Log::Info(fmt::format(
                "Loading sequence from {} to {}", startIndex, stopIndex
            ));
        }
        else if (arg == "-rot" && argc > (i + 4)) {
            ivec4 rotations = ivec4{
                atoi(argv[i + 1]),
                atoi(argv[i + 2]),
                atoi(argv[i + 3]),
                atoi(argv[i + 4])
            };
            Log::Info(fmt::format(
                "Setting image rotations to L: {}, R: {}, T: {}, B: {}",
                rotations.x, rotations.y, rotations.z, rotations.w
            ));

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
            sideRotations[0] = convertRotations(rotations.x);
            sideRotations[1] = convertRotations(rotations.y);
            sideRotations[2] = convertRotations(rotations.z);
            sideRotations[3] = convertRotations(rotations.w);
        }
        else if (arg == "-start" && argc > (i + 1)) {
            startFrame = atoi(argv[i + 1]);
            Log::Info(fmt::format("Start frame set to {}", startFrame));
        }
        else if (arg == "-alpha" && argc > (i + 1)) {
            settings.alpha = std::string_view(argv[i + 1]) == "1";
            Log::Info(fmt::format(
                "Setting alpha to {}", settings.alpha ? "true" : "false"
            ));
        }
        else if (arg == "-stereo" && argc > (i + 1)) {
            settings.stereo = std::string_view(argv[i + 1]) == "1";
            Log::Info(fmt::format(
                "Setting stereo to {}", settings.stereo ? "true" : "false"
            ));
        }
        else if (arg == "-cubic" && argc > (i + 1)) {
            settings.cubic = std::string_view(argv[i + 1]) == "1";
            Log::Info(fmt::format(
                "Setting cubic interpolation to {}", settings.cubic ? "true" : "false"
            ));
        }
        else if (arg == "-fxaa" && argc > (i + 1)) {
            settings.fxaa = std::string_view(argv[i + 1]) == "1";
            Log::Info(fmt::format(
                "Setting fxaa to {}", settings.fxaa ? "true" : "false"
            ));
        }
        else if (arg == "-eyeSep" && argc > (i + 1)) {
            settings.eyeSeparation = static_cast<float>(atof(argv[i + 1]));
            Log::Info(fmt::format(
                "Setting eye separation to {}", settings.eyeSeparation
            ));
        }
        else if (arg == "-diameter" && argc > (i + 1)) {
            settings.domeDiameter = static_cast<float>(atof(argv[i + 1]));
            Log::Info(fmt::format(
                "Setting dome diameter to {}", settings.domeDiameter
            ));
        }
        else if (arg == "-msaa" && argc > (i + 1)) {
            settings.numberOfMSAASamples = atoi(argv[i + 1]);
            Log::Info(fmt::format(
                "Number of MSAA samples set to {}", settings.numberOfMSAASamples
            ));
        }
        else if (arg == "-res" && argc > (i + 1)) {
            settings.resolution = atoi(argv[i + 1]);
            Log::Info(fmt::format(
                "Resolution set to {}", settings.resolution
            ));
        }
        else if (arg == "-cubemap" && argc > (i + 1)) {
            settings.cubemapRes = atoi(argv[i + 1]);
            Log::Info(fmt::format(
                "Cubemap resolution set to {}", settings.cubemapRes
            ));
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
                    Log::Info("Unknown capturing format. Using PNG");
                    return Settings::CaptureFormat::PNG;
                }
            } (arg2);
            Settings::instance().setCaptureFormat(f);
            Log::Info(fmt::format("Format set to {}", argv[i + 1]));
        }
        else if (arg == "-path" && argc > (i + 1)) {
            Settings::instance().setCapturePath(argv[i + 1]);
            Log::Info(fmt::format("Left path set to {}", argv[i + 1]));
        }
        else if (arg == "-leftPath" || arg == "-rightPath") {
            Log::Warning("-leftPath and -rightPath are no longer supported; use -path");
        }
    }

    Engine::Callbacks callbacks;
    callbacks.preWindow = preWinInit;
    callbacks.initOpenGL = initOGL;
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.keyboard = keyboard;

    try {
        Engine::create(cluster, callbacks, config);
        Engine::instance().setClearColor(vec4{ 0.f, 0.f, 0.f, 1.f });
    }
    catch (const std::runtime_error& e) {
        Log::Error(e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance().render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
