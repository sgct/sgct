/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <sgct/utils/box.h>
#include <sgct/utils/domegrid.h>
#include <sgct/user.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    constexpr float Diameter = 14.8f;
    constexpr float Tilt = glm::radians(30.f);

    std::unique_ptr<sgct::utils::Box> box;
    std::unique_ptr<sgct::utils::DomeGrid> grid;
    GLint matrixLoc = -1;
    GLint gridMatrixLoc = -1;

    unsigned int textureId = 0;

    // variables to share across cluster
    double currentTime = 0.0;
    bool takeScreenshot = true;

    struct OmniData {
        std::map<sgct::Frustum::Mode, glm::mat4> viewProjectionMatrix;
        bool enabled = false;
    };
    std::vector<std::vector<OmniData>> omniProjections;
    bool omniInited = false;

    // Parameters to control omni rendering
    bool maskOutSimilarities = false;
    int tileSize = 2;

    std::string turnMapSrc;
    std::string sepMapSrc;

    constexpr std::string_view BaseVertexShader = R"(
  #version 330 core

  layout(location = 0) in vec2 texCoords;
  layout(location = 1) in vec3 normals;
  layout(location = 2) in vec3 vertPositions;

  uniform mat4 mvp;
  out vec2 uv;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPositions, 1.0);
    uv = texCoords;
  })";

   constexpr std::string_view BaseFragmentShader = R"(
  #version 330 core

  uniform sampler2D tex;

  in vec2 uv;
  out vec4 color;

  void main() { color = texture(tex, uv); }
)";

   constexpr std::string_view GridVertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPositions;

  uniform mat4 mvp;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPositions, 1.0);
  })";

   constexpr std::string_view GridFragmentShader = R"(
  #version 330 core

  out vec4 color;

  void main() { color = vec4(1.0, 0.5, 0.0, 1.0); }
)";

   unsigned char getSampleAt(const sgct::Image& img, int x, int y) {
       const int width = img.size().x;
       const size_t idx = (y * width + x) * img.channels() * img.bytesPerChannel();
       return img.data()[idx];
   }

   float getInterpolatedSampleAt(const sgct::Image& img, float x, float y) {
       int px = static_cast<int>(x); //floor x
       int py = static_cast<int>(y); //floor y

       // Calculate the weights for each pixel
       float fx = x - static_cast<float>(px);
       float fy = y - static_cast<float>(py);

       //if no need for interpolation
       if (fx == 0.f && fy == 0.f) {
           return static_cast<float>(getSampleAt(img, px, py));
       }

       float fx1 = 1.0f - fx;
       float fy1 = 1.0f - fy;

       float w0 = fx1 * fy1;
       float w1 = fx * fy1;
       float w2 = fx1 * fy;
       float w3 = fx * fy;

       float p0 = static_cast<float>(getSampleAt(img, px, py));
       float p1 = static_cast<float>(getSampleAt(img, px, py + 1));
       float p2 = static_cast<float>(getSampleAt(img, px + 1, py));
       float p3 = static_cast<float>(getSampleAt(img, px + 1, py + 1));

       return p0 * w0 + p1 * w1 + p2 * w2 + p3 * w3;
   }


} // namespace

using namespace sgct;

void renderGrid(glm::mat4 transform) {
    glUniformMatrix4fv(gridMatrixLoc, 1, GL_FALSE, glm::value_ptr(transform));
    grid->draw();
}

void initOmniStereo(bool mask) {
    double t0 = time();

    if (Engine::instance().windows().size() < 2) {
        Log::Error("Failed to allocate omni stereo in secondary window");
        return;
    }

    Image turnMap;
    turnMap.load(turnMapSrc);

    Image sepMap;
    sepMap.load(sepMapSrc);

    Window& win = *Engine::instance().windows()[1];
    const ivec2 res = ivec2{
        win.framebufferResolution().x / tileSize,
        win.framebufferResolution().y / tileSize
    };

    Log::Info(fmt::format(
        "Allocating: {} MB data", (sizeof(OmniData) * res.x * res.y) / (1024 * 1024)
    ));
    omniProjections.resize(res.x);
    for (int i = 0; i < res.x; i++) {
        omniProjections[i].resize(res.y);
    }

    int VPCounter = 0;

    for (int eye = 0; eye <= 2; eye++) {
        float eyeSep = Engine::instance().defaultUser().eyeSeparation();

        Frustum::Mode fm;
        glm::vec3 eyePos;
        switch (eye) {
            case 0:
            default:
                fm = Frustum::Mode::MonoEye;
                eyePos = glm::vec3{ 0.f, 0.f, 0.f };
                break;
            case 1:
                fm = Frustum::Mode::StereoLeftEye;
                eyePos = glm::vec3{ -eyeSep / 2.f, 0.f, 0.f };
                break;
            case 2:
                fm = Frustum::Mode::StereoRightEye;
                eyePos = glm::vec3{ eyeSep / 2.f, 0.f, 0.f };
                break;
        }

        for (int y = 0; y < res.y; y++) {
            for (int x = 0; x < res.x; x++) {
                // scale to [-1, 1)
                // Center of each pixel
                const float xResf = static_cast<float>(res.x);
                const float yResf = static_cast<float>(res.y);
                const float s = ((static_cast<float>(x) + 0.5f) / xResf - 0.5f) * 2.f;
                const float t = ((static_cast<float>(y) + 0.5f) / yResf - 0.5f) * 2.f;
                const float r2 = s * s + t * t;

                constexpr float fovInDegrees = 180.f;
                constexpr float halfFov = glm::radians(fovInDegrees / 2.f);

                const float phi = sqrt(r2) * halfFov;
                const float theta = atan2(s, -t);

                const glm::vec3 normalPosition = {
                    sin(phi) * sin(theta),
                    -sin(phi) * cos(theta),
                    cos(phi)
                };

                float tmpY = normalPosition.y * cos(Tilt) - normalPosition.z * sin(Tilt);
                float eyeRot = atan2(normalPosition.x, -tmpY);

                // get corresponding map positions
                bool omniNeeded = true;
                if (turnMap.channels() > 0) {
                    const glm::vec2 turnMapPos = {
                        (x / xResf) * static_cast<float>(turnMap.size().x - 1),
                        (y / yResf) * static_cast<float>(turnMap.size().y - 1)
                    };

                    // inverse gamma
                    const float headTurnMultiplier = pow(
                        getInterpolatedSampleAt(
                            turnMap,
                            turnMapPos.x,
                            turnMapPos.y
                        ) / 255.f,
                        2.2f
                    );

                    if (headTurnMultiplier == 0.f) {
                        omniNeeded = false;
                    }

                    eyeRot *= headTurnMultiplier;
                }

                glm::vec3 newEyePos;
                if (sepMap.channels() > 0) {
                    const glm::vec2 sepMapPos = {
                        (x / xResf) * static_cast<float>(sepMap.size().x - 1),
                        (y / yResf) * static_cast<float>(sepMap.size().y - 1)
                    };

                    // inverse gamma 2.2
                    const float separationMultiplier = pow(
                        getInterpolatedSampleAt(
                            sepMap,
                            sepMapPos.x,
                            sepMapPos.y
                        ) / 255.f,
                        2.2f
                    );

                    if (separationMultiplier == 0.f) {
                        omniNeeded = false;
                    }

                    // get values at positions
                    newEyePos = eyePos * separationMultiplier;
                }
                else {
                    newEyePos = eyePos;
                }

                // IF VALID
                if (r2 <= 1.1f && (omniNeeded || !mask)) {
                    auto convertCoords = [&](glm::vec2 tc) {
                        //scale to [-1, 1)
                        const float ss = ((x + tc.x) / xResf - 0.5f) * 2.f;
                        const float tt = ((y + tc.y) / yResf - 0.5f) * 2.f;

                        const float rr2 = ss * ss + tt * tt;
                        // zenith - elevation (0 degrees in zenith, 90 degrees at the rim)
                        const float phi2 = sqrt(rr2) * halfFov;
                        // azimuth (0 degrees at back of dome and 180 degrees at front)
                        const float theta2 = atan2(ss, tt);

                        constexpr float radius = Diameter / 2.f;
                        glm::vec3 p = {
                            radius * sin(phi2) * sin(theta2),
                            radius * -sin(phi2) * cos(theta2),
                            radius * cos(phi2)
                        };

                        const glm::mat4 rotMat = glm::rotate(
                            glm::mat4(1.f),
                            glm::radians(-90.f),
                            glm::vec3(1.f, 0.f, 0.f)
                        );
                        glm::vec3 convPos = glm::mat3(rotMat) * p;
                        return vec3{ convPos.x, convPos.y, convPos.z };
                    };


                    ProjectionPlane projPlane;

                    projPlane.setCoordinates(
                        convertCoords(glm::vec2(0.f, 0.f)),
                        convertCoords(glm::vec2(0.f, 1.f)),
                        convertCoords(glm::vec2(1.f, 1.f))
                    );

                    const glm::mat4 rotEyeMat = glm::rotate(
                        glm::mat4(1.f),
                        eyeRot,
                        glm::vec3(0.f, -1.f, 0.f)
                    );
                    const glm::vec3 rotatedEyePos = glm::mat3(rotEyeMat) * newEyePos;

                    // tilt
                    const glm::mat4 tiltEyeMat = glm::rotate(
                        glm::mat4(1.f),
                        Tilt,
                        glm::vec3(1.f, 0.f, 0.f)
                    );

                    const glm::vec3 tiltedEyePos = glm::mat3(tiltEyeMat) * rotatedEyePos;

                    // calc projection
                    Projection proj;
                    proj.calculateProjection(
                        vec3{ tiltedEyePos.x, tiltedEyePos.y, tiltedEyePos.z },
                        projPlane,
                        Engine::instance().nearClipPlane(),
                        Engine::instance().farClipPlane()
                    );

                    omniProjections[x][y].enabled = true;
                    omniProjections[x][y].viewProjectionMatrix[fm] = glm::make_mat4(
                        proj.viewProjectionMatrix().values
                    );
                    VPCounter++;
                }
            }
        }
    }

    int percentage = (100 * VPCounter) / (res.x * res.y * 3);
    Log::Info(fmt::format(
        "Time to init viewports: {} s\n{} %% will be rendered",
        time() - t0, percentage
    ));
    omniInited = true;
}

void renderBoxes(glm::mat4 transform) {
    // create scene transform
    const glm::mat4 levels[3] = {
        glm::translate(glm::mat4(1.f), glm::vec3(0.f, -0.5f, -3.f)),
        glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, -2.75f)),
        glm::translate(glm::mat4(1.f), glm::vec3(0.f, 2.5f, -1.25f))
    };

    glm::mat4 boxTrans;
    for (unsigned int l = 0; l < 3; l++) {
        for (float a = 0.f; a < 360.f; a += (15.f * static_cast<float>(l + 1))) {
            const glm::mat4 rot = glm::rotate(
                glm::mat4(1.f),
                glm::radians(a),
                glm::vec3(0.f, 1.f, 0.f)
            );

            boxTrans = transform * rot * levels[l];
            glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(boxTrans));

            box->draw();
        }
    }
}

void drawOmniStereo(const RenderData& renderData) {
    if (!omniInited) {
        return;
    }

    double t0 = time();

    Window& win = *Engine::instance().windows()[1];
    ivec2 res = ivec2{
        win.framebufferResolution().x / tileSize,
        win.framebufferResolution().y / tileSize
    };

    ShaderManager::instance().shaderProgram("xform").bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    Frustum::Mode fm = renderData.frustumMode;
    for (int x = 0; x < res.x; x++) {
        for (int y = 0; y < res.y; y++) {
            if (omniProjections[x][y].enabled) {
                glViewport(x * tileSize, y * tileSize, tileSize, tileSize);
                const glm::mat4 vp = omniProjections[x][y].viewProjectionMatrix[fm];
                renderBoxes(vp * glm::make_mat4(renderData.modelMatrix.values));
            }
        }
    }

    ShaderManager::instance().shaderProgram("grid").bind();
    for (int x = 0; x < res.x; x++) {
        for (int y = 0; y < res.y; y++) {
            if (omniProjections[x][y].enabled) {
                glViewport(x * tileSize, y * tileSize, tileSize, tileSize);
                const glm::mat4 vp = omniProjections[x][y].viewProjectionMatrix[fm];
                renderGrid(vp);
            }
        }
    }

    Log::Info(fmt::format("Time to draw frame: {}s", time() - t0));
}

void draw(const RenderData& data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if (data.window.id() == 1) {
        drawOmniStereo(data);
    }
    else {
        const glm::mat4 vp = glm::make_mat4(data.projectionMatrix.values) *
            glm::make_mat4(data.viewMatrix.values);

        ShaderManager::instance().shaderProgram("grid").bind();
        renderGrid(vp);

        ShaderManager::instance().shaderProgram("xform").bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        renderBoxes(vp * glm::make_mat4(data.modelMatrix.values));
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSync() {
    if (Engine::instance().isMaster()) {
        currentTime = time();
    }
}

void postSyncPreDraw() {
    if (takeScreenshot) {
        Engine::instance().takeScreenshot();
        takeScreenshot = false;
    }
}

void postDraw() {
    // render a single frame and exit
    Engine::instance().terminate();
}

void initOGL(GLFWwindow*) {
    textureId = TextureManager::instance().loadTexture("box.png", true, 8.f);

    box = std::make_unique<utils::Box>(0.5f, utils::Box::TextureMappingMode::Regular);
    grid = std::make_unique<utils::DomeGrid>(Diameter / 2.f, 180.f, 64, 32, 256);

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager& sm = ShaderManager::instance();
    sm.addShaderProgram("grid", GridVertexShader, GridFragmentShader);
    const ShaderProgram& gridProg = sm.shaderProgram("grid");
    gridProg.bind();
    gridMatrixLoc = glGetUniformLocation(gridProg.id(), "mvp");
    gridProg.unbind();

    sm.addShaderProgram("xform", BaseVertexShader, BaseFragmentShader);
    const ShaderProgram& xformProg = sm.shaderProgram("xform");
    xformProg.bind();
    matrixLoc = glGetUniformLocation(xformProg.id(), "mvp");
    GLint textureLoc = glGetUniformLocation(xformProg.id(), "tex");
    glUniform1i(textureLoc, 0);
    xformProg.unbind();

    initOmniStereo(maskOutSimilarities);
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, currentTime);
    serializeObject(data, takeScreenshot);
    return data;
}

void decode(const std::vector<std::byte>& data) {
    unsigned int pos = 0;
    deserializeObject(data, pos, currentTime);
    deserializeObject(data, pos, takeScreenshot);
}

void cleanup() {
    box = nullptr;
    grid = nullptr;
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (key == Key::Esc && action == Action::Press) {
        Engine::instance().terminate();
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    if (!cluster.success) {
        return -1;
    }

    if (cluster.settings.has_value()) {
        if (cluster.settings->display.has_value()) {
            cluster.settings->display->swapInterval = 0;
        }
        else {
            config::Settings::Display display;
            display.swapInterval = 0;
            cluster.settings->display = display;
        }
    }
    else {
        config::Settings::Display display;
        display.swapInterval = 0;

        config::Settings settings;
        settings.display = display;

        cluster.settings = settings;
    }


    for (int i = 0; i < argc; i++) {
        std::string_view argument = argv[i];

        if (argument == "-turnmap" && argc > i + 1) {
            turnMapSrc = argv[i + 1];
            Log::Info(fmt::format("Setting turn map path to {}", turnMapSrc));
        }
        if (argument == "-sepmap" && argc > i + 1) {
            sepMapSrc = argv[i + 1];
            Log::Info(fmt::format("Setting separation map path to '{}'", sepMapSrc));
        }
    }

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGL;
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.postDraw = postDraw;
    callbacks.cleanup = cleanup;
    callbacks.keyboard = keyboard;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error& e) {
        Log::Error(e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance().exec();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
