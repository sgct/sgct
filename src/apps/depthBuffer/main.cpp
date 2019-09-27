
#include <sgct.h>
#include <sgct/clustermanager.h>
#include <sgct/postfx.h>
#include <sgct/user.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    int textureLocations[2];
    int currTimeLoc;
    bool mPause = false;
    GLuint terrainDisplayList = 0;

    // light data
    const glm::vec4 lightPosition = glm::vec4(-2.f, 5.f, 5.f, 1.f);
    const glm::vec4 lightAmbient = glm::vec4(0.1f, 0.1f, 0.1f, 1.f);
    const glm::vec4 lightDiffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.f);
    const glm::vec4 lightSpecular = glm::vec4(1.f, 1.f, 1.f, 1.f);

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool wireframe(false);
    sgct::SharedBool info(false);
    sgct::SharedBool stats(false);
    sgct::SharedBool takeScreenshot(false);
    sgct::SharedBool useTracking(false);
    sgct::SharedBool reloadShaders(false);

    sgct::PostFX fx;
    struct {
        int colorTex = -1;
        int depthTex = -1;
        int nearClip = -1;
        int farClip = -1;
    } fxLocation;

} // namespace

using namespace sgct;

#ifdef Test
sgct::utils::Sphere * sphere = NULL;
#endif

void updatePass() {
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gEngine->getCurrentDepthTexture());
    glUniform1i(fxLocation.colorTex, 0);
    glUniform1i(fxLocation.depthTex, 1);
    glUniform1f(fxLocation.nearClip, gEngine->getNearClippingPlane());
    glUniform1f(fxLocation.farClip, gEngine->getFarClippingPlane());
}

void setupPostFXs() {
    fx.init("Depth", "depth.vert", "depth.frag");
    fx.setUpdateUniformsFunction( updatePass );
    ShaderProgram& sp = fx.getShaderProgram();
    sp.bind();
    fxLocation.colorTex = sp.getUniformLocation("cTex");
    fxLocation.depthTex = sp.getUniformLocation("dTex");
    fxLocation.nearClip = sp.getUniformLocation("near");
    fxLocation.farClip = sp.getUniformLocation("far");
    sp.unbind();
    gEngine->addPostFX(fx);
}

/**
 * Will draw a flat surface that can be used for the heightmapped terrain.
 *
 * @param width Width of the surface
 * @param depth Depth of the surface
 * @param wRes Width resolution of the surface
 * @param dRes Depth resolution of the surface
 */
void drawTerrainGrid(float width, float depth, unsigned int wRes, unsigned int dRes) {
    const float wStart = -width * 0.5f;
    const float dStart = -depth * 0.5f;

    const float dW = width / static_cast<float>(wRes);
    const float dD = depth / static_cast<float>(dRes);

    for (unsigned int depthIndex = 0; depthIndex < dRes; ++depthIndex) {
        const float dPosLow = dStart + dD * static_cast<float>(depthIndex);
        const float dPosHigh = dStart + dD * static_cast<float>(depthIndex + 1);
        const float dTexCoordLow = depthIndex / static_cast<float>(dRes);
        const float dTexCoordHigh = (depthIndex + 1) / static_cast<float>(dRes);

        glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0.f, 1.f, 0.f);
        for (unsigned widthIndex = 0; widthIndex < wRes; ++widthIndex) {
            const float wPos = wStart + dW * static_cast<float>(widthIndex);
            const float wTexCoord = widthIndex / static_cast<float>(wRes);

            glMultiTexCoord2f(GL_TEXTURE0, wTexCoord, dTexCoordLow);
            glMultiTexCoord2f(GL_TEXTURE1, wTexCoord, dTexCoordLow);
            glVertex3f(wPos, 0.f, dPosLow);

            glMultiTexCoord2f(GL_TEXTURE0, wTexCoord, dTexCoordHigh);
            glMultiTexCoord2f(GL_TEXTURE1, wTexCoord, dTexCoordHigh);
            glVertex3f(wPos, 0.f, dPosHigh);
        }

        glEnd();
    }
}

void drawFun() {
#ifndef Test
    glLightfv(GL_LIGHT0, GL_POSITION, glm::value_ptr(lightPosition));
    
    glTranslatef(0.f, -0.15f, 2.5f);
    glRotatef(static_cast<float>(currentTime.getVal()) * 8.f, 0.f, 1.f, 0.f);

    glColor4f(1.f, 1.f, 1.f, 1.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("heightmap"));
    glEnable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("normalmap"));
    glEnable(GL_TEXTURE_2D);

    // set current shader program
    ShaderManager::instance()->bindShaderProgram("Heightmap");
    glUniform1f(currTimeLoc, static_cast<float>(currentTime.getVal()));
    glUniform1i(textureLocations[0], 0);
    glUniform1i(textureLocations[1], 1);

    glLineWidth(2.0);
    glCallList(terrainDisplayList);

    // unset current shader program
    ShaderManager::instance()->unBindShaderProgram();

    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
#else
    sphere->draw();
#endif
}

void preSyncFun() {
    if (gEngine->isMaster() && !mPause) {
        currentTime.setVal(currentTime.getVal() + gEngine->getAvgDt());
    }
}

void postSyncPreDrawFun() {
    gEngine->setWireframe(wireframe.getVal());
    gEngine->setDisplayInfoVisibility(info.getVal());
    gEngine->setStatsGraphVisibility(stats.getVal());
    sgct::core::ClusterManager::instance()->getTrackingManager().setEnabled(
        useTracking.getVal()
    );

    if (takeScreenshot.getVal()) {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }

    if (reloadShaders.getVal()) {
        fx.getShaderProgram().reload();
        reloadShaders.setVal(false);
        sgct::ShaderManager::instance()->reloadShaderProgram("Heightmap");
    }
}

void initOGLFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);

    // Set up light 0
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, glm::value_ptr(lightAmbient));
    glLightfv(GL_LIGHT0, GL_DIFFUSE, glm::value_ptr(lightDiffuse));
    glLightfv(GL_LIGHT0, GL_SPECULAR, glm::value_ptr(lightSpecular));

    // create and compile display list
    terrainDisplayList = glGenLists(1);
    glNewList(terrainDisplayList, GL_COMPILE);
    // draw the terrain once to add it to the display list
    drawTerrainGrid(1.f, 1.f, 256, 256);
    glEndList();

    TextureManager::instance()->loadTexture(
        "heightmap",
        "heightmap.png",
        true,
        0
    );
    TextureManager::instance()->loadTexture(
        "normalmap",
        "normalmap.png",
        true,
        0
    );

    ShaderManager::instance()->addShaderProgram(
        "Heightmap",
        "heightmap.vert",
        "heightmap.frag"
    );

    ShaderManager::instance()->bindShaderProgram("Heightmap");
    const ShaderProgram& heightMapProg = ShaderManager::instance()->getShaderProgram(
        "Heightmap"
    );
    textureLocations[0] = -1;
    textureLocations[1] = -1;
    currTimeLoc = -1;
    textureLocations[0] = heightMapProg.getUniformLocation("hTex");
    textureLocations[1] = heightMapProg.getUniformLocation("nTex");
    currTimeLoc = heightMapProg.getUniformLocation("currTime");

    glUniform1i(textureLocations[0], 0);
    glUniform1i(textureLocations[1], 1);
    sgct::ShaderManager::instance()->unBindShaderProgram();

    setupPostFXs();

#ifdef Test
    sphere = new sgct::utils::Sphere(2.0f, 512);
    gEngine->setNearAndFarClippingPlanes(1.0f, 3.0f);
#else
    gEngine->setNearAndFarClippingPlanes(0.1f, 5.0f);
#endif
}

void encodeFun() {
    sgct::SharedData::instance()->writeDouble(currentTime);
    sgct::SharedData::instance()->writeBool(wireframe);
    sgct::SharedData::instance()->writeBool(info);
    sgct::SharedData::instance()->writeBool(stats);
    sgct::SharedData::instance()->writeBool(takeScreenshot);
    sgct::SharedData::instance()->writeBool(useTracking);
    sgct::SharedData::instance()->writeBool(reloadShaders);
}

void decodeFun() {
    sgct::SharedData::instance()->readDouble(currentTime);
    sgct::SharedData::instance()->readBool(wireframe);
    sgct::SharedData::instance()->readBool(info);
    sgct::SharedData::instance()->readBool(stats);
    sgct::SharedData::instance()->readBool(takeScreenshot);
    sgct::SharedData::instance()->readBool(useTracking);
    sgct::SharedData::instance()->readBool(reloadShaders);
}

void cleanUpFun() {
#ifdef Test
    delete sphere;
#endif
    glDeleteLists(terrainDisplayList, 1);
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster()) {
        switch (key) {
            case key::S:
                if (action == action::Press) {
                    stats.setVal(!stats.getVal());
                }
                break;
            case key::I:
                if (action == action::Press) {
                    info.setVal(!info.getVal());
                }
                break;
            case key::W:
                if (action == action::Press) {
                    wireframe.setVal(!wireframe.getVal());
                }
                break;
            case key::Q:
                if (action == action::Press) {
                    gEngine->terminate();
                }
                break;
            case key::T:
                if (action == action::Press) {
                    useTracking.setVal(!useTracking.getVal());
                }
                break;
            case key::E:
                if (action == action::Press) {
                    glm::dmat4 xform = glm::translate(
                        glm::dmat4(1.0),
                        glm::dvec3(0.0, 0.0, 4.0)
                    );
                    sgct::core::ClusterManager::instance()->getDefaultUser().setTransform(
                        xform
                    );
                }
                break;
            case key::Space:
                if (action == action::Press) {
                    mPause = !mPause;
                }
                break;
            case key::F:
                if (action == action::Press) {
                    for (int i = 0; i < gEngine->getNumberOfWindows(); i++) {
                        gEngine->getWindow(i).setUseFXAA(
                            !gEngine->getWindow(i).useFXAA()
                        );
                    }
                }
                break;
            case key::P:
            case key::F10:
                if (action == action::Press) {
                    takeScreenshot.setVal(true);
                }
                break;
            case key::R:
                if (action == action::Press) {
                    reloadShaders.setVal(true);
                }
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    gEngine = new Engine(config);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);

    Settings::instance()->setUseDepthTexture(true);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction(encodeFun);
    sgct::SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
