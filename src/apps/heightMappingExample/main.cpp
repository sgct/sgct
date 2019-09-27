#include <sgct.h>
#include <sgct/clustermanager.h>
#include <sgct/user.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    sgct::Engine* gEngine;

    int textureLocations[2];
    int currTimeLoc;
    bool mPause = false;
    GLuint myTerrainDisplayList = 0;

    // light data
    glm::vec4 lightPosition = glm::vec4(-2.f, 5.f, 5.f, 1.f);
    glm::vec4 lightAmbient = glm::vec4(0.1f, 0.1f, 0.1f, 1.f);
    glm::vec4 lightDiffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.f);
    glm::vec4 lightSpecular = glm::vec4(1.f, 1.f, 1.f, 1.f);

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool wireframe(false);
    sgct::SharedBool info(false);
    sgct::SharedBool stats(false);
    sgct::SharedBool takeScreenshot(false);
    sgct::SharedBool useTracking(false);
    sgct::SharedObject<sgct::Window::StereoMode> stereoMode;
} // namespace

using namespace sgct;

/**
 * Will draw a flat surface that can be used for the heightmapped terrain.
 *
 * \param width Width of the surface
 * \param depth Depth of the surface
 * \param wRes Width resolution of the surface
 * \param dRes Depth resolution of the surface
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
    sgct::ShaderManager::instance()->bindShaderProgram("Heightmap");
    glUniform1f(currTimeLoc, static_cast<float>(currentTime.getVal()));

    glLineWidth(2.0); // for wireframe
    glCallList(myTerrainDisplayList);

    // unset current shader program
    ShaderManager::instance()->unBindShaderProgram();

    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
}

void preSyncFun() {
    if (gEngine->isMaster() && !mPause) {
        currentTime.setVal( currentTime.getVal() + gEngine->getAvgDt());
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
}

void initOGLFun() {
    stereoMode.setVal(gEngine->getWindow(0).getStereoMode());

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE );
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);

    // Set up light 0
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, glm::value_ptr(lightAmbient));
    glLightfv(GL_LIGHT0, GL_DIFFUSE, glm::value_ptr(lightDiffuse));
    glLightfv(GL_LIGHT0, GL_SPECULAR, glm::value_ptr(lightSpecular));

    // create and compile display list
    myTerrainDisplayList = glGenLists(1);
    glNewList(myTerrainDisplayList, GL_COMPILE);

    // draw the terrain once to add it to the display list
    drawTerrainGrid(1.f, 1.f, 256, 256);
    glEndList();

    TextureManager::instance()->loadTexture("heightmap", "heightmap.png", true, 0);
    TextureManager::instance()->loadTexture("normalmap", "normalmap.png", true, 0);

    ShaderManager::instance()->addShaderProgram(
        "Heightmap",
        "heightmap.vert",
        "heightmap.frag"
    );

    ShaderManager::instance()->bindShaderProgram("Heightmap");
    const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("Heightmap");

    textureLocations[0] = prog.getUniformLocation("hTex");
    textureLocations[1] = prog.getUniformLocation("nTex");
    currTimeLoc = prog.getUniformLocation("currTime");

    glUniform1i(textureLocations[0], 0);
    glUniform1i(textureLocations[1], 1);
    ShaderManager::instance()->unBindShaderProgram();
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeBool(wireframe);
    SharedData::instance()->writeBool(info);
    SharedData::instance()->writeBool(stats);
    SharedData::instance()->writeBool(takeScreenshot);
    SharedData::instance()->writeBool(useTracking);
    SharedData::instance()->writeObj(stereoMode);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
    SharedData::instance()->readBool(wireframe);
    SharedData::instance()->readBool(info);
    SharedData::instance()->readBool(stats);
    SharedData::instance()->readBool(takeScreenshot);
    SharedData::instance()->readBool(useTracking);
    SharedData::instance()->readObj(stereoMode);
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster() && action == action::Press) {
        switch (key) {
            case key::S:
                stats.setVal(!stats.getVal());
                break;
            case key::I:
                info.setVal(!info.getVal());
                break;
            case key::W:
                wireframe.setVal(!wireframe.getVal());
                break;
            case key::Q:
                gEngine->terminate();
                break;
            case key::T:
                useTracking.setVal(!useTracking.getVal());
                break;
            case key::E:
                sgct::core::ClusterManager::instance()->getDefaultUser().setTransform(
                    glm::translate(glm::dmat4(1.f), glm::dvec3(0.f, 0.f, 4.f))
                );
                break;
            case key::Space:
                mPause = !mPause;
                break;
            case key::F:
                for (int i = 0; i < gEngine->getNumberOfWindows(); i++) {
                    gEngine->getWindow(i).setUseFXAA(!gEngine->getWindow(i).useFXAA());
                }
                break;
            case key::P:
            case key::F10:
                takeScreenshot.setVal(true);
                break;
            case key::R:
                sgct::core::ClusterManager::instance()->getThisNode()->showAllWindows();
                break;
            case key::Left:
                if (static_cast<int>(stereoMode.getVal()) > 0) {
                    const int v = static_cast<int>(stereoMode.getVal()) - 1;
                    Window::StereoMode m = static_cast<Window::StereoMode>(v);
                    stereoMode.setVal(m);
                }
                break;
            case key::Right:
                const int v = static_cast<int>(stereoMode.getVal()) + 1;
                Window::StereoMode m = static_cast<Window::StereoMode>(v);
                stereoMode.setVal(m);
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
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();

    glDeleteLists(myTerrainDisplayList, 1);
    delete gEngine;
    exit(EXIT_SUCCESS);
}
