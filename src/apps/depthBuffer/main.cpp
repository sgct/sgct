#include <stdlib.h>
#include <stdio.h>

#include <glm/gtc/matrix_transform.hpp>

#include "sgct.h"
#include "sgct/ClusterManager.h"
#include "sgct/PostFX.h"
#include "sgct/SGCTUser.h"

using namespace sgct;

Engine* gEngine;

void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

void keyCallback(int key, int scancode, int action, int modifiers);
void drawTerrainGrid(float width, float height, unsigned int wRes, unsigned int dRes);

int myTextureLocations[2];
int currTimeLoc;
bool mPause = false;
GLuint myTerrainDisplayList = 0;

//light data
GLfloat lightPosition[] = { -2.0f, 5.0f, 5.0f, 1.0f };
GLfloat lightAmbient[]= { 0.1f, 0.1f, 0.1f, 1.0f };
GLfloat lightDiffuse[]= { 0.8f, 0.8f, 0.8f, 1.0f };
GLfloat lightSpecular[]= { 1.0f, 1.0f, 1.0f, 1.0f };

//variables to share across cluster
SharedDouble currentTime(0.0);
SharedBool wireframe(false);
SharedBool info(false);
SharedBool stats(false);
SharedBool takeScreenshot(false);
SharedBool useTracking(false);
SharedBool reloadShaders(false);

PostFX fx;
int fxCTexLoc = -1;
int fxDTexLoc = -1;
int fxNearLoc = -1;
int fxFarLoc = -1;

#ifdef Test
sgct_utils::SGCTSphere * mySphere = NULL;
#endif

void updatePass() {
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gEngine->getCurrentDepthTexture());
    glUniform1i(fxCTexLoc, 0);
    glUniform1i(fxDTexLoc, 1);
    glUniform1f(fxNearLoc, gEngine->getNearClippingPlane());
    glUniform1f(fxFarLoc, gEngine->getFarClippingPlane());
}

void setupPostFXs() {
    fx.init("Depth", "depth.vert", "depth.frag");
    fx.setUpdateUniformsFunction( updatePass );
    ShaderProgram& sp = fx.getShaderProgram();
    sp.bind();
    fxCTexLoc = sp.getUniformLocation("cTex");
    fxDTexLoc = sp.getUniformLocation("dTex");
    fxNearLoc = sp.getUniformLocation("near");
    fxFarLoc = sp.getUniformLocation("far");
    sp.unbind();
    gEngine->addPostFX(fx);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    gEngine->setInitOGLFunction(myInitOGLFun);
    gEngine->setDrawFunction(myDrawFun);
    gEngine->setPreSyncFunction(myPreSyncFun);
    gEngine->setCleanUpFunction(myCleanUpFun);
    gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    
    SGCTSettings::instance()->setUseDepthTexture(true);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
    sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}

void myDrawFun() {
#ifndef Test
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    
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
    glUniform1i(myTextureLocations[0], 0);
    glUniform1i(myTextureLocations[1], 1);

    glLineWidth(2.0);
    glCallList(myTerrainDisplayList);

    // unset current shader program
    ShaderManager::instance()->unBindShaderProgram();

    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
#else
    mySphere->draw();
#endif
}

void myPreSyncFun() {
    if (gEngine->isMaster() && !mPause) {
        currentTime.setVal(currentTime.getVal() + gEngine->getAvgDt());
    }
}

void myPostSyncPreDrawFun() {
    gEngine->setWireframe(wireframe.getVal());
    gEngine->setDisplayInfoVisibility(info.getVal());
    gEngine->setStatsGraphVisibility(stats.getVal());
    sgct_core::ClusterManager::instance()->getTrackingManager().setEnabled(
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

void myInitOGLFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);

    // Set up light 0
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    // create and compile display list
    myTerrainDisplayList = glGenLists(1);
    glNewList(myTerrainDisplayList, GL_COMPILE);
    // draw the terrain once to add it to the display list
    drawTerrainGrid(1.f, 1.f, 256, 256);
    glEndList();

    //sgct::TextureManager::Instance()->setAnisotropicFilterSize(4.0f);
    TextureManager::instance()->loadTexture(
        "heightmap",
        "../SharedResources/heightmap.png",
        true,
        0
    );
    TextureManager::instance()->loadTexture(
        "normalmap",
        "../SharedResources/normalmap.png",
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
    myTextureLocations[0] = -1;
    myTextureLocations[1] = -1;
    currTimeLoc = -1;
    myTextureLocations[0] = heightMapProg.getUniformLocation("hTex");
    myTextureLocations[1] = heightMapProg.getUniformLocation("nTex");
    currTimeLoc = heightMapProg.getUniformLocation("curr_time");

    glUniform1i(myTextureLocations[0], 0);
    glUniform1i(myTextureLocations[1], 1);
    sgct::ShaderManager::instance()->unBindShaderProgram();

    setupPostFXs();

#ifdef Test
    mySphere = new sgct_utils::SGCTSphere(2.0f, 512);
    gEngine->setNearAndFarClippingPlanes(1.0f, 3.0f);
#else
    gEngine->setNearAndFarClippingPlanes(0.1f, 5.0f);
#endif
}

void myEncodeFun() {
    sgct::SharedData::instance()->writeDouble(currentTime);
    sgct::SharedData::instance()->writeBool(wireframe);
    sgct::SharedData::instance()->writeBool(info);
    sgct::SharedData::instance()->writeBool(stats);
    sgct::SharedData::instance()->writeBool(takeScreenshot);
    sgct::SharedData::instance()->writeBool(useTracking);
    sgct::SharedData::instance()->writeBool(reloadShaders);
}

void myDecodeFun() {
    sgct::SharedData::instance()->readDouble(currentTime);
    sgct::SharedData::instance()->readBool(wireframe);
    sgct::SharedData::instance()->readBool(info);
    sgct::SharedData::instance()->readBool(stats);
    sgct::SharedData::instance()->readBool(takeScreenshot);
    sgct::SharedData::instance()->readBool(useTracking);
    sgct::SharedData::instance()->readBool(reloadShaders);
}

void myCleanUpFun() {
#ifdef Test
    delete mySphere;
#endif
    glDeleteLists(myTerrainDisplayList, 1);
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
        const float dTexCoordHigh = (depthIndex+1) / static_cast<float>(dRes);

        glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0.f, 1.f, 0.f);
        for (unsigned widthIndex = 0; widthIndex < wRes; ++widthIndex ) {
            const float wPos = wStart + dW * static_cast<float>( widthIndex );
            const float wTexCoord = widthIndex / static_cast<float>( wRes );

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

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster()) {
        switch (key) {
        case 'S':
            if (action == SGCT_PRESS) {
                stats.setVal(!stats.getVal());
            }
            break;
        case 'I':
            if (action == SGCT_PRESS) {
                info.setVal(!info.getVal());
            }
            break;
        case 'W':
            if (action == SGCT_PRESS) {
                wireframe.setVal(!wireframe.getVal());
            }
            break;
        case 'Q':
            if (action == SGCT_PRESS) {
                gEngine->terminate();
            }
            break;
        case 'T':
            if (action == SGCT_PRESS) {
                useTracking.setVal(!useTracking.getVal());
            }
            break;
        case 'E':
            if (action == SGCT_PRESS) {
                glm::dmat4 xform = glm::translate(
                    glm::dmat4(1.0),
                    glm::dvec3(0.0, 0.0, 4.0)
                );
                sgct_core::ClusterManager::instance()->getDefaultUser().setTransform(
                    xform
                );
            }
            break;
        case SGCT_KEY_SPACE:
            if (action == SGCT_PRESS) {
                mPause = !mPause;
            }
            break;
        case 'F':
            if (action == SGCT_PRESS) {
                for (size_t i = 0; i < gEngine->getNumberOfWindows(); i++) {
                    gEngine->getWindow(i).setUseFXAA(!gEngine->getWindow(i).useFXAA());
                }
            }
            break;
        case 'P':
        case SGCT_KEY_F10:
            if (action == SGCT_PRESS) {
                takeScreenshot.setVal(true);
            }
            break;
        case 'R':
            if (action == SGCT_PRESS) {
                reloadShaders.setVal(true);
            }
            break;
        }
    }
}
