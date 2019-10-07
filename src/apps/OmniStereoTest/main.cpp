#include <sgct.h>
#include <sgct/image.h>
#include <sgct/window.h>
#include <sgct/user.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    constexpr const float Diameter = 14.8f;
    constexpr const float Tilt = glm::radians(30.f);

    sgct::Engine* gEngine;

    std::unique_ptr<sgct::utils::Box> box;
    std::unique_ptr<sgct::utils::DomeGrid> grid;
    GLint matrixLoc = -1;
    GLint gridMatrixLoc = -1;

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool takeScreenshot(true);

    struct OmniData {
        std::map<sgct::core::Frustum::Mode, glm::mat4> viewProjectionMatrix;
        bool enabled = false;
    };
    std::vector<std::vector<OmniData>> omniProjections;
    bool omniInited = false;

    // Parameters to control omni rendering
    bool maskOutSimilarities = false;
    int tileSize = 2;

    std::string turnMapSrc;
    std::string sepMapSrc;

    constexpr const char* baseVertexShader = R"(
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

   constexpr const char* baseFragmentShader = R"(
  #version 330 core

  uniform sampler2D tex;

  in vec2 uv;
  out vec4 color;

  void main() { color = texture(tex, uv); }
)";

   constexpr const char* gridVertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPositions;

  uniform mat4 mvp;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPositions, 1.0);
  })";

   constexpr const char* gridFragmentShader = R"(
  #version 330 core

  out vec4 color;

  void main() { color = vec4(1.0, 0.5, 0.0, 1.0); }
)";

} // namespace

using namespace sgct;

void renderGrid(glm::mat4 transform) {
    glUniformMatrix4fv(gridMatrixLoc, 1, GL_FALSE, glm::value_ptr(transform));
    grid->draw();
}

void initOmniStereo(bool mask) {
    double t0 = gEngine->getTime();

    if (gEngine->getNumberOfWindows() < 2) {
        MessageHandler::instance()->printError(
            "Failed to allocate omni stereo in secondary window\n"
        );
        return;
    }

    sgct::core::Image turnMap;
    const bool turnMapSuccess = turnMap.load(turnMapSrc);
    if (!turnMapSuccess) {
        MessageHandler::instance()->printWarning("Failed to load turn map\n");
    }

    sgct::core::Image sepMap;
    const bool sepMapSuccess = sepMap.load(sepMapSrc);
    if (!sepMapSuccess) {
        MessageHandler::instance()->printWarning("Failed to load separation map\n");
    }

    Window& win = gEngine->getWindow(1);
    const glm::ivec2 res = win.getFramebufferResolution() / tileSize;

    MessageHandler::instance()->printInfo(
        "Allocating: %d MB data\n", (sizeof(OmniData) * res.x * res.y) / (1024 * 1024)
    );
    omniProjections.resize(res.x);
    for (int i = 0; i < res.x; i++) {
        omniProjections[i].resize(res.y);
    }

    int VPCounter = 0;

    for (int eye = 0; eye <= 2; eye++) {
        const float eyeSep = gEngine->getDefaultUser().getEyeSeparation();

        core::Frustum::Mode fm;
        glm::vec3 eyePos;
        switch (eye) {
            case 0:
            default:
                fm = core::Frustum::Mode::MonoEye;
                eyePos = glm::vec3(0.f, 0.f, 0.f);
                break;
            case 1:
                fm = core::Frustum::Mode::StereoLeftEye;
                eyePos = glm::vec3(-eyeSep / 2.f, 0.f, 0.f);
                break;
            case 2:
                fm = core::Frustum::Mode::StereoRightEye;
                eyePos = glm::vec3(eyeSep / 2.f, 0.f, 0.f);
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

                constexpr const float fovInDegrees = 180.f;
                constexpr const float halfFov = glm::radians(fovInDegrees / 2.f);

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
                if (turnMap.getChannels() > 0) {
                    const glm::vec2 turnMapPos = {
                        (x / xResf) * static_cast<float>(turnMap.getWidth() - 1),
                        (y / yResf) * static_cast<float>(turnMap.getHeight() - 1)
                    };

                    // inverse gamma
                    const float headTurnMultiplier = pow(
                        turnMap.getInterpolatedSampleAt(
                            turnMapPos.x,
                            turnMapPos.y,
                            sgct::core::Image::Blue
                        ) / 255.f,
                        2.2f
                    );

                    if (headTurnMultiplier == 0.f) {
                        omniNeeded = false;
                    }

                    eyeRot *= headTurnMultiplier;
                }

                glm::vec3 newEyePos;
                if (sepMap.getChannels() > 0) {
                    const glm::vec2 sepMapPos = {
                        (x / xResf) * static_cast<float>(sepMap.getWidth() - 1),
                        (y / yResf) * static_cast<float>(sepMap.getHeight() - 1)
                    };

                    // inverse gamma 2.2
                    const float separationMultiplier = pow(
                        sepMap.getInterpolatedSampleAt(
                            sepMapPos.x,
                            sepMapPos.y,
                            sgct::core::Image::Blue
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
                        const float s = ((x + tc.x) / xResf - 0.5f) * 2.f;
                        const float t = ((y + tc.y) / yResf - 0.5f) * 2.f;

                        const float r2 = s * s + t * t;
                        // zenith - elevation (0 degrees in zenith, 90 degrees at the rim)
                        const float phi = sqrt(r2) * halfFov;
                        // azimuth (0 degrees at back of dome and 180 degrees at front)
                        const float theta = atan2(s, t);

                        constexpr const float radius = Diameter / 2.f;
                        glm::vec3 p = {
                            radius * sin(phi) * sin(theta),
                            radius * -sin(phi) * cos(theta),
                            radius * cos(phi)
                        };

                        const glm::mat4 rotMat = glm::rotate(
                            glm::mat4(1.f),
                            glm::radians(-90.f),
                            glm::vec3(1.f, 0.f, 0.f)
                        );
                        glm::vec3 convergencePos = glm::mat3(rotMat) * p;
                        return convergencePos;
                    };


                    sgct::core::ProjectionPlane projPlane;
                  
                    const glm::vec2 ll = glm::vec2(0.f, 0.f);
                    projPlane.setCoordinateLowerLeft(convertCoords(ll));

                    const glm::vec2 ul = glm::vec2(0.f, 1.f);
                    projPlane.setCoordinateUpperLeft(convertCoords(ul));

                    const glm::vec2 ur = glm::vec2(1.f, 1.f);
                    projPlane.setCoordinateUpperRight(convertCoords(ur));

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
                    sgct::core::Projection proj;
                    proj.calculateProjection(
                        tiltedEyePos,
                        projPlane,
                        gEngine->getNearClippingPlane(),
                        gEngine->getFarClippingPlane()
                    );

                    omniProjections[x][y].enabled = true;
                    omniProjections[x][y].viewProjectionMatrix[fm] =
                        proj.getViewProjectionMatrix();
                    VPCounter++;
                }
            }
        }
    }

    int percentage = (100 * VPCounter) / (res.x * res.y * 3);
    MessageHandler::instance()->printInfo(
        "Time to init viewports: %f s\n%d %% will be rendered.\n",
        gEngine->getTime() - t0, percentage
    );
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

void drawOmniStereo() {
    if (!omniInited) {
        return;
    }

    double t0 = gEngine->getTime();

    Window& win = gEngine->getWindow(1);
    glm::ivec2 res = win.getFramebufferResolution() / tileSize;

    ShaderManager::instance()->bindShaderProgram("xform");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));

    sgct::core::Frustum::Mode fm = gEngine->getCurrentFrustumMode();
    for (int x = 0; x < res.x; x++) {
        for (int y = 0; y < res.y; y++) {
            if (omniProjections[x][y].enabled) {
                glViewport(x * tileSize, y * tileSize, tileSize, tileSize);
                const glm::mat4 vp = omniProjections[x][y].viewProjectionMatrix[fm];

                renderBoxes(vp * gEngine->getModelMatrix());
            }
        }
    }

    ShaderManager::instance()->bindShaderProgram("grid");
    for (int x = 0; x < res.x; x++) {
        for (int y = 0; y < res.y; y++) {
            if (omniProjections[x][y].enabled) {
                glViewport(x * tileSize, y * tileSize, tileSize, tileSize);
                const glm::mat4 vp = omniProjections[x][y].viewProjectionMatrix[fm];

                renderGrid(vp);
            }
        }
    }

    const double t1 = gEngine->getTime();
    MessageHandler::instance()->printInfo("Time to draw frame: %f s\n", t1 - t0);
}

void drawFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if (gEngine->getCurrentWindowIndex() == 1) {
        drawOmniStereo();
    }
    else {
        glm::mat4 vp = gEngine->getCurrentViewProjectionMatrix();
        glm::mat4 model = gEngine->getModelMatrix();

        ShaderManager::instance()->bindShaderProgram("grid");
        renderGrid(vp);

        ShaderManager::instance()->bindShaderProgram("xform");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));
        renderBoxes(vp * model);
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
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

void postDrawFun() {
    // render a single frame and exit
    gEngine->terminate();
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::None);
    TextureManager::instance()->loadTexture("box", "box.png", true);

    box = std::make_unique<sgct::utils::Box>(
        0.5f,
        sgct::utils::Box::TextureMappingMode::Regular
    );
    grid = std::make_unique<sgct::utils::DomeGrid>(Diameter / 2.f, 180.f, 64, 32, 256);

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager& sm = *ShaderManager::instance();
    sm.addShaderProgram(
        "grid",
        gridVertexShader,
        gridFragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    sm.bindShaderProgram("grid");
    gridMatrixLoc = sm.getShaderProgram("grid").getUniformLocation("mvp");

    sm.addShaderProgram(
        "xform",
        baseVertexShader,
        baseFragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    sm.bindShaderProgram("xform");
    matrixLoc = sm.getShaderProgram("xform").getUniformLocation("mvp");
    GLint textureLoc = sm.getShaderProgram("xform").getUniformLocation("tex");
    glUniform1i(textureLoc, 0);

    sm.unBindShaderProgram();

    initOmniStereo(maskOutSimilarities);
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeBool(takeScreenshot);
}

void decodeFun() {
    sgct::SharedData::instance()->readDouble(currentTime);
    sgct::SharedData::instance()->readBool(takeScreenshot);
}

void cleanUpFun() {
    box = nullptr;
    grid = nullptr;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    for (int i = 0; i < argc; i++) {
        std::string_view argument = argv[i];

        if (argument == "-turnmap" && argc > i + 1) {
            turnMapSrc = argv[i + 1];
            MessageHandler::instance()->printInfo(
                "Setting turn map path to '%s'\n", turnMapSrc.c_str()
            );
        }
        if (argument == "-sepmap" && argc > i + 1) {
            sepMapSrc = argv[i + 1];
            MessageHandler::instance()->printInfo(
                "Setting separation map path to '%s'\n", sepMapSrc.c_str()
            );
        }
    }

    Settings::instance()->setSwapInterval(0);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setPostDrawFunction(postDrawFun);
    gEngine->setCleanUpFunction(cleanUpFun);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
