#include <sgct.h>
#include <sgct/ClusterManager.h>
#include <sgct/SGCTUser.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    constexpr const int GridSize = 256;

    sgct::Engine* gEngine;

    // shader data
    sgct::ShaderProgram mSp;
    GLint heightTextureLoc = -1;
    GLint normalTextureLoc = -1;
    GLint currTimeLoc = -1;
    GLint MVPLoc = -1;
    GLint MVLoc = -1;
    GLint MVLightLoc = -1;
    GLint NMLoc = -1;
    GLint lightPosLoc = -1;
    GLint lightAmbLoc = -1;
    GLint lightDifLoc = -1;
    GLint lightSpeLoc = -1;

    // opengl objects
    GLuint vertexArray = 0;
    GLuint vertexPositionBuffer = 0;
    GLuint texCoordBuffer = 0;

    // light data
    const glm::vec4 lightPosition(-2.f, 5.f, 5.f, 1.f);
    const glm::vec4 lightAmbient(0.1f, 0.1f, 0.1f, 1.f);
    const glm::vec4 lightDiffuse(0.8f, 0.8f, 0.8f, 1.f);
    const glm::vec4 lightSpecular(1.f, 1.f, 1.f, 1.f);

    bool mPause = false;

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool wireframe(false);
    sgct::SharedBool info(false);
    sgct::SharedBool stats(false);
    sgct::SharedBool takeScreenshot(false);
    sgct::SharedBool useTracking(false);
    sgct::SharedObject<sgct::SGCTWindow::StereoMode> stereoMode;

    struct Geometry {
        std::vector<float> vertPos;
        std::vector<float> texCoord;
        GLsizei numberOfVerts = 0;
    };
} // namespace

using namespace sgct;

/**
 *
 * Will draw a flat surface that can be used for the heightmapped terrain.
 *
 * \param width Width of the surface
 * \param depth Depth of the surface
 * \param wRes Width resolution of the surface
 * \param dRes Depth resolution of the surface
 */
Geometry generateTerrainGrid(float width, float depth, unsigned int wRes, unsigned int
                             dRes)
{
    const float wStart = -width * 0.5f;
    const float dStart = -depth * 0.5f;

    const float dW = width / static_cast<float>(wRes);
    const float dD = depth / static_cast<float>(dRes);

    Geometry res;

    for (unsigned int depthIndex = 0; depthIndex < dRes; ++depthIndex) {
        const float dPosLow = dStart + dD * static_cast<float>(depthIndex);
        const float dPosHigh = dStart + dD * static_cast<float>(depthIndex + 1);
        const float dTexCoordLow = depthIndex / static_cast<float>(dRes);
        const float dTexCoordHigh = (depthIndex + 1) / static_cast<float>(dRes);

        for (unsigned widthIndex = 0; widthIndex < wRes; ++widthIndex) {
            const float wPos = wStart + dW * static_cast<float>(widthIndex);
            const float wTexCoord = widthIndex / static_cast<float>(wRes);

            // p0
            res.vertPos.push_back(wPos);
            res.vertPos.push_back(0.f);
            res.vertPos.push_back(dPosLow);

            // p1
            res.vertPos.push_back(wPos);
            res.vertPos.push_back(0.f);
            res.vertPos.push_back(dPosHigh);

            // tex0
            res.texCoord.push_back(wTexCoord);
            res.texCoord.push_back(dTexCoordLow);

            // tex1
            res.texCoord.push_back(wTexCoord);
            res.texCoord.push_back(dTexCoordHigh);
        }
    }

    // each vertex has three componets (x, y & z)
    res.numberOfVerts = static_cast<GLsizei>(res.vertPos.size() / 3);

    return res;
}

void drawFun() {    
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    // for wireframe
    glLineWidth(1.0);

    constexpr double Speed = 0.14;

    //create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, -0.15f, 2.5f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * Speed),
        glm::vec3(0.f, 1.f, 0.f)
    );

    const glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene;
    const glm::mat4 MV = gEngine->getCurrentModelViewMatrix() * scene;
    const glm::mat4 MV_light = gEngine->getCurrentModelViewMatrix();
    const glm::mat3 NM = glm::inverseTranspose(glm::mat3(MV));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("heightmap"));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("normalmap"));

    mSp.bind();

    glUniformMatrix4fv(MVPLoc, 1, GL_FALSE, glm::value_ptr(MVP));
    glUniformMatrix4fv(MVLoc, 1, GL_FALSE, glm::value_ptr(MV));
    glUniformMatrix4fv(MVLightLoc, 1, GL_FALSE, glm::value_ptr(MV_light));
    glUniformMatrix3fv(NMLoc, 1, GL_FALSE, glm::value_ptr(NM));
    glUniform1f(currTimeLoc, static_cast<float>(currentTime.getVal()));

    glBindVertexArray(vertexArray);

    // Draw the triangles
    for (unsigned int i = 0; i < GridSize; i++) {
        glDrawArrays(GL_TRIANGLE_STRIP, i * GridSize * 2, GridSize * 2);
    }

    glBindVertexArray(0);

    ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
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
    sgct_core::ClusterManager::instance()->getTrackingManager().setEnabled(
        useTracking.getVal()
    );

    if (takeScreenshot.getVal()) {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void initOGLFun() {
    stereoMode.setVal(gEngine->getWindow(0).getStereoMode());

    // Set up backface culling
    glCullFace(GL_BACK);
    // our polygon winding is counter clockwise
    glFrontFace(GL_CCW);
    glDepthFunc(GL_LESS);

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

    // setup shader
    ShaderManager::instance()->addShaderProgram(
        mSp,
        "Heightmap",
        "heightmap.vert",
        "heightmap.frag"
    );

    mSp.bind();
    heightTextureLoc = mSp.getUniformLocation("hTex");
    normalTextureLoc = mSp.getUniformLocation("nTex");
    currTimeLoc = mSp.getUniformLocation("curr_time");
    MVPLoc = mSp.getUniformLocation("MVP");
    MVLoc = mSp.getUniformLocation("MV");
    MVLightLoc = mSp.getUniformLocation("MV_light");
    NMLoc = mSp.getUniformLocation("normalMatrix");
    lightPosLoc = mSp.getUniformLocation("lightPos");
    lightAmbLoc = mSp.getUniformLocation("light_ambient");
    lightDifLoc = mSp.getUniformLocation("light_diffuse");
    lightSpeLoc = mSp.getUniformLocation("light_specular");
    glUniform1i(heightTextureLoc, 0);
    glUniform1i(normalTextureLoc, 1);
    glUniform4fv(lightPosLoc, 1, glm::value_ptr(lightPosition));
    glUniform4fv(lightAmbLoc, 1, glm::value_ptr(lightAmbient));
    glUniform4fv(lightDifLoc, 1, glm::value_ptr(lightDiffuse));
    glUniform4fv(lightSpeLoc, 1, glm::value_ptr(lightSpecular));
    ShaderManager::instance()->unBindShaderProgram();

    Geometry geometry = generateTerrainGrid(1.0f, 1.0f, GridSize, GridSize);

    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    glGenBuffers(1, &vertexPositionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * geometry.vertPos.size(),
        geometry.vertPos.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    // generate texture coord buffer
    glGenBuffers(1, &texCoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * geometry.texCoord.size(),
        geometry.texCoord.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    if (gEngine->isMaster() && action == SGCT_PRESS) {
        switch (key) {
            case 'S':
                stats.setVal(!stats.getVal());
                break;
            case 'I':
                info.setVal(!info.getVal());
                break;
            case 'W':
                wireframe.setVal(!wireframe.getVal());
                break;
            case 'Q':
                gEngine->terminate();
                break;
            case 'T':
                useTracking.setVal(!useTracking.getVal());
                break;
            case 'E':
                sgct_core::ClusterManager::instance()->getDefaultUser().setTransform(
                    glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, 0.0, 4.0))
                );
                break;
            case SGCT_KEY_SPACE:
                mPause = !mPause;
                break;
            case 'F':
                for (size_t i = 0; i < gEngine->getNumberOfWindows(); i++) {
                    gEngine->getWindow(i).setUseFXAA(!gEngine->getWindow(i).useFXAA());
                }
                break;
            case 'P':
            case SGCT_KEY_F10:
                takeScreenshot.setVal(true);
                break;
            case SGCT_KEY_LEFT:
                if (static_cast<int>(stereoMode.getVal()) > 0) {
                    const int v = static_cast<int>(stereoMode.getVal()) - 1;
                    SGCTWindow::StereoMode m = static_cast<SGCTWindow::StereoMode>(v);
                    stereoMode.setVal(m);
                }
                break;
            case SGCT_KEY_RIGHT:
                const int v = static_cast<int>(stereoMode.getVal()) + 1;
                SGCTWindow::StereoMode m = static_cast<SGCTWindow::StereoMode>(v);
                stereoMode.setVal(m);
                break;
        }
    }
}

void cleanUpFun() {
    glDeleteBuffers(1, &vertexPositionBuffer);
    glDeleteBuffers(1, &texCoordBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
