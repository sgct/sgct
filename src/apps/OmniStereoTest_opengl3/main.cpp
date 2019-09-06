#include <sgct.h>
#include <sgct/Image.h>
#include <sgct/SGCTUser.h>
#include <sgct/SGCTWindow.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    glm::mat4 levels[3];
    std::unique_ptr<sgct_utils::SGCTBox> box;
    std::unique_ptr<sgct_utils::SGCTDomeGrid> grid;
    GLint matrixLoc = -1;
    GLint gridMatrixLoc = -1;

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool takeScreenshot(true);

    //omni var
    struct OmniData {
        glm::mat4& projectionMatrix(sgct_core::Frustum::FrustumMode mode) {
            switch (mode) {
            case sgct_core::Frustum::FrustumMode::MonoEye:
                return mViewProjectionMatrixMono;
            case sgct_core::Frustum::FrustumMode::StereoLeftEye:
                return mViewProjectionMatrixStereoLeft;
            case sgct_core::Frustum::FrustumMode::StereoRightEye:
                return mViewProjectionMatrixStereoRight;
            }
        }

        const glm::mat4& projectionMatrix(sgct_core::Frustum::FrustumMode mode) const {
            switch (mode) {
                case sgct_core::Frustum::FrustumMode::MonoEye:
                    return mViewProjectionMatrixMono;
                case sgct_core::Frustum::FrustumMode::StereoLeftEye:
                    return mViewProjectionMatrixStereoLeft;
                case sgct_core::Frustum::FrustumMode::StereoRightEye:
                    return mViewProjectionMatrixStereoRight;
            }
        }

        glm::mat4 mViewProjectionMatrixMono;
        glm::mat4 mViewProjectionMatrixStereoLeft;
        glm::mat4 mViewProjectionMatrixStereoRight;
        bool enabled;
    };
    std::vector<std::vector<OmniData>> omniProjections;
    bool omniInited = false;

    // Parameters to control omni rendering
    bool maskOutSimilarities = false;
    int tileSize = 2;
    float domeDiameter = 14.8f;
    float domeTilt = 30.f;

    std::string turnMapSrc;
    std::string sepMapSrc;

} // namespace

using namespace sgct;

void renderBoxes(glm::mat4 transform) {
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

void renderGrid(glm::mat4 transform) {
    glUniformMatrix4fv(gridMatrixLoc, 1, GL_FALSE, &transform[0][0]);
    grid->draw();
}

void initOmniStereo(float diameter, float tilt, bool mask) {
    double t0 = gEngine->getTime();

    if (gEngine->getNumberOfWindows() < 2) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to allocate omni stereo in secondary window\n"
        );
        return;
    }

    sgct_core::Image turnMap;
    if (!(!turnMapSrc.empty() && turnMap.load(turnMapSrc))) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Failed to load turn map\n"
        );
    }

    sgct_core::Image sepMap;
    if (!(!sepMapSrc.empty() && sepMap.load(sepMapSrc))) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Failed to load separation map\n"
        );
    }

    const sgct::SGCTWindow& win = gEngine->getWindow(1);
    const glm::ivec2 res = win.getFramebufferResolution() / tileSize;

    MessageHandler::instance()->print(
        "Allocating: %d MB data\n",
        (sizeof(OmniData) * res.x * res.y) / (1024 * 1024)
    );

    omniProjections.resize(res.x);
    for (int i = 0; i < res.x; i++) {
        omniProjections[i].resize(res.y);
    }

    int VPCounter = 0;

    auto handleEye = [&](sgct_core::Frustum::FrustumMode fm, const glm::vec3& eyePos) {
        for (int y = 0; y < res.y; y++) {
            for (int x = 0; x < res.x; x++) {
                // scale to [-1, 1)
                // Center of each pixel
                const float xResf = static_cast<float>(res.x);
                const float yResf = static_cast<float>(res.y);
                const glm::vec2 texCoord = {
                    ((static_cast<float>(x) + 0.5f) / xResf - 0.5f) * 2.f,
                    ((static_cast<float>(y) + 0.5f) / yResf - 0.5f) * 2.f
                };
                const float r2 = texCoord.s * texCoord.s + texCoord.t * texCoord.t;

                constexpr const float fovInDegrees = 180.f;
                constexpr const float halfFov = glm::radians(fovInDegrees / 2.f);
                const float phi = sqrt(r2) * halfFov;
                const float theta = atan2(texCoord.s, -texCoord.t);

                const glm::vec3 normalPos = {
                    sin(phi) * sin(theta),
                    -sin(phi) * cos(theta),
                    cos(phi)
                };

                const float tmpY = normalPos.y * cos(glm::radians(tilt)) -
                                   normalPos.z * sin(glm::radians(tilt));
                float eyeRot = atan2(normalPos.x, -tmpY);

                //get corresponding map positions
                bool omni_needed = true;
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
                            sgct_core::Image::Blue
                        ) / 255.0f,
                        2.2f
                    );

                    if (headTurnMultiplier == 0.f) {
                        omni_needed = false;
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
                            sgct_core::Image::Blue
                        ) / 255.f,
                        2.2f
                    );

                    if (separationMultiplier == 0.f) {
                        omni_needed = false;
                    }

                    //get values at positions
                    newEyePos = eyePos * separationMultiplier;
                }
                else {
                    newEyePos = eyePos;
                }

                // IF NOT VALID
                if (r2 > 1.1f || (!omni_needed && mask)) {
                    omniProjections[x][y].enabled = false;
                    continue;
                }

                const glm::mat3 rotMat = glm::mat3(
                    glm::rotate(
                        glm::mat4(1.f),
                        glm::radians(-90.f),
                        glm::vec3(1.f, 0.f, 0.f)
                    )
                );

                sgct_core::SGCTProjectionPlane projPlane;

                auto convertPosition = [&](glm::vec2 corner) {
                    // scale to [-1, 1)
                    const glm::vec2 tc = glm::vec2(0.f, 0.f);
                    float s = ((static_cast<float>(x) + tc.x) / xResf - 0.5f) * 2.f;
                    float t = ((static_cast<float>(y) + tc.y) / yResf - 0.5f) * 2.f;

                    const float r2 = s * s + t * t;
                    // zenith - elevation (0 degrees in zenith, 90 degrees at the rim)
                    const float phi = sqrt(r2) * halfFov;
                    // azimuth (0 degrees at back of dome and 180 degrees at front)
                    const float theta = atan2(s, t);

                    const float radius = diameter / 2.f;
                    const glm::vec3 pLowerLeft = {
                        radius * sin(phi) * sin(theta),
                        radius * -sin(phi) * cos(theta),
                        radius * cos(phi)
                    };
                    const glm::vec3 convergencePos = rotMat * pLowerLeft;
                    return convergencePos;
                };

                const glm::vec3 lowerLeft = convertPosition(glm::vec2(0.f, 0.f));
                projPlane.setCoordinateLowerLeft(lowerLeft);

                const glm::vec3 upperLeft = convertPosition(glm::vec2(0.f, 1.f));
                projPlane.setCoordinateUpperLeft(upperLeft);

                const glm::vec3 upperRight = convertPosition(glm::vec2(1.f, 1.f));
                projPlane.setCoordinateUpperRight(upperRight);

                const glm::mat3 rotEyeMat = glm::mat3(
                    glm::rotate(glm::mat4(1.f), eyeRot, glm::vec3(0.f, -1.f, 0.f))
                );
                const glm::vec3 rotatedEyePos = rotEyeMat * newEyePos;

                // tilt
                const glm::mat3 tiltEyeMat = glm::mat3(
                    glm::rotate(
                        glm::mat4(1.f),
                        glm::radians(tilt),
                        glm::vec3(1.f, 0.f, 0.f)
                    )
                );
                const glm::vec3 tiltedEyePos = tiltEyeMat * rotatedEyePos;

                // calc projection
                sgct_core::SGCTProjection proj;
                proj.calculateProjection(
                    tiltedEyePos,
                    projPlane,
                    gEngine->getNearClippingPlane(),
                    gEngine->getFarClippingPlane()
                );

                omniProjections[x][y].enabled = true;
                const glm::mat4& p = proj.getViewProjectionMatrix();
                omniProjections[x][y].projectionMatrix(fm) = p;
                VPCounter++;
            }
        }
    };

    const glm::vec3 eyePosMono = glm::vec3(0.f);
    handleEye(sgct_core::Frustum::FrustumMode::MonoEye, eyePosMono);

    const float eyeSep = gEngine->getDefaultUser().getEyeSeparation();
    const glm::vec3 eyePosStereoLeft = glm::vec3(-eyeSep / 2.f, 0.f, 0.f);
    handleEye(sgct_core::Frustum::FrustumMode::StereoLeftEye, eyePosStereoLeft);

    const glm::vec3 eyePosStereoRight = glm::vec3(eyeSep / 2.f, 0.f, 0.f);
    handleEye(sgct_core::Frustum::FrustumMode::StereoRightEye, eyePosStereoRight);

    int percentage = (100 * VPCounter) / (res.x * res.y * 3);
    MessageHandler::instance()->print(
        "Time to init viewports: %f s\n%d %% will be rendered.\n",
        gEngine->getTime() - t0, percentage
    );
    omniInited = true;
}

void drawOmniStereo() {
    if (!omniInited) {
        return;
    }
    double t0 = gEngine->getTime();

    const SGCTWindow& win = gEngine->getWindow(1);
    glm::ivec2 res = win.getFramebufferResolution() / tileSize;

    ShaderManager::instance()->bindShaderProgram("xform");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));

    for (int x = 0; x < res.x; x++) {
        for (int y = 0; y < res.y; y++) {
            if (omniProjections[x][y].enabled) {
                glViewport(x * tileSize, y * tileSize, tileSize, tileSize);
                sgct_core::Frustum::FrustumMode fm = gEngine->getCurrentFrustumMode();
                glm::mat4 vp = omniProjections[x][y].projectionMatrix(fm);
                renderBoxes(vp * gEngine->getModelMatrix());
            }
        }
    }

    ShaderManager::instance()->bindShaderProgram("grid");
    for (int x = 0; x < res.x; x++) {
        for (int y = 0; y < res.y; y++) {
            if (omniProjections[x][y].enabled) {
                glViewport(x * tileSize, y * tileSize, tileSize, tileSize);
                sgct_core::Frustum::FrustumMode fm = gEngine->getCurrentFrustumMode();
                glm::mat4 vp = omniProjections[x][y].projectionMatrix(fm);
                renderGrid(vp);
            }
        }
    }

    MessageHandler::instance()->print(
        "Time to draw frame: %f s\n", gEngine->getTime() - t0
    );
}

void drawFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if (gEngine->getCurrentWindowIndex() == 1) {
        drawOmniStereo();
    }
    else {
        const glm::mat4 vp = gEngine->getCurrentViewProjectionMatrix();
        const glm::mat4 model = gEngine->getModelMatrix();

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
        currentTime.setVal(sgct::Engine::getTime());
    }
}

void postSyncPreDrawFun() {
    if (takeScreenshot.getVal()) {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void myPostDrawFun() {
    // render a single frame and exit
    gEngine->terminate();
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::None);
    TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    box = std::make_unique<sgct_utils::SGCTBox>(
        0.5f,
        sgct_utils::SGCTBox::TextureMappingMode::Regular
        );
    grid = std::make_unique<sgct_utils::SGCTDomeGrid>(
        domeDiameter / 2.f,
        180.f,
        64,
        32,
        256
    );

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance()->addShaderProgram("grid", "grid.vert", "grid.frag");
    ShaderManager::instance()->bindShaderProgram("grid");

    const ShaderProgram& gridProg = ShaderManager::instance()->getShaderProgram("grid");
    gridMatrixLoc = gridProg.getUniformLocation("MVP");

    ShaderManager::instance()->addShaderProgram("xform", "base.vert", "base.frag");
    sgct::ShaderManager::instance()->bindShaderProgram("xform");

    const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("xform");
    matrixLoc = prog.getUniformLocation("MVP");
    GLint textureLocation = prog.getUniformLocation("Tex");
    glUniform1i(textureLocation, 0);

    ShaderManager::instance()->unBindShaderProgram();

    // create scene transform
    levels[0] = glm::translate(glm::mat4(1.f), glm::vec3(0.f, -0.5f, -3.f));
    levels[1] = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, -2.75f));
    levels[2] = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 2.5f, -1.25f));

    initOmniStereo(domeDiameter, domeTilt, maskOutSimilarities);
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
    grid = nullptr;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new sgct::Engine(arg);

    for (int n = 0; n < argc; n++) {
        std::string_view arg(argv[n]);
        if (arg == "-turnmap" && argc > n + 1) {
            turnMapSrc = argv[n + 1];
            MessageHandler::instance()->print(
                "Setting turn map path to '%s'\n", turnMapSrc.c_str()
            );
        }
        if (arg == "-sepmap" && argc > n + 1) {
            sepMapSrc = argv[n + 1];
            MessageHandler::instance()->print(
                "Setting separation map path to '%s'\n", sepMapSrc.c_str()
            );
        }
    }

    SGCTSettings::instance()->setSwapInterval(0);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setPostDrawFunction(myPostDrawFun);
    gEngine->setCleanUpFunction(cleanUpFun);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction(encodeFun);
    sgct::SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
