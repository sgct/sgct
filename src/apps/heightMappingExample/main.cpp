#include <sgct/clustermanager.h>
#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#include <sgct/user.h>
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
    sgct::SharedBool info(false);
    sgct::SharedBool stats(false);
    sgct::SharedBool takeScreenshot(false);
    sgct::SharedBool useTracking(false);
    sgct::SharedObject<sgct::Window::StereoMode> stereoMode;

    struct Geometry {
        std::vector<float> vertPos;
        std::vector<float> texCoord;
        GLsizei numberOfVerts = 0;
    };

    constexpr const char* vertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPositions;
  layout(location = 1) in vec2 texCoords;

  uniform sampler2D hTex;
  uniform float currTime;
  uniform mat4 mvp;
  uniform mat4 mv;
  uniform mat4 mvLight;
  uniform vec4 lightPos;

  out vec2 uv; //texture coords
  out float vScale; // Height scaling
  out vec3 lightDir;
  out vec3 v;

  void main() {
    uv = texCoords;

    vScale = 0.2 + 0.10 * sin(currTime);
    float hVal = texture(hTex, uv).r;
    vec4 transformedVertex = vec4(vertPositions + vec3(0.0, hVal * vScale, 0.0), 1.0);

    // Transform a vertex to model space
    v = vec3(mv * transformedVertex);
    vec3 l = vec3(mvLight * lightPos);
    lightDir = normalize(l - v);
  
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * transformedVertex;
  })";

    constexpr const char* fragmentShader = R"(
  #version 330 core

  uniform sampler2D hTex;
  uniform sampler2D nTex;
  uniform vec4 lightAmbient;
  uniform vec4 lightDiffuse;
  uniform vec4 lightSpecular;
  uniform mat3 normalMatrix;

  in vec2 uv;
  in float vScale;
  in vec3 lightDir;
  in vec3 v;

  out vec4 color;

  // Computes the diffues shading by using the normal for
  // the fragment and direction from fragment to the light
  vec4 calcShading(vec3 N, vec3 L) {
    // Ambient contribution
    vec4 iamb = lightAmbient;

    // Diffuse contribution
    vec4 idiff = lightDiffuse * max(dot(N, L), 0.0);
    idiff = clamp(idiff, 0.0, 1.0);

    //Specular contribution
    vec3 E = normalize(-v);
    vec3 R = normalize(reflect(-L, N));
    const float specExp = 32.0;
    vec4 ispec = lightSpecular * pow(max(dot(R, E), 0.0), specExp);
    ispec = clamp(ispec, 0.0, 1.0);

    return iamb + idiff + ispec;
  }

  void main() {
    vec3 pixelVals = texture(nTex, uv).rgb;
    vec3 normal = vec3(
      (pixelVals.r * 2.0 - 1.0),
      (pixelVals.b * 2.0 - 1.0) / vScale,
      (pixelVals.g * 2.0 - 1.0)
    );
    if (vScale < 0) {
      normal = -normal;
    }

    // Set fragment color
    // This will result in a non-linear color temperature scale based on height value
    float hVal = texture(hTex, uv).x;
    float Pi = 3.14159265358979323846264;
    color.rgb = vec3(1.0 - cos(Pi * hVal), sin(Pi * hVal), cos(Pi * hVal));

    // multiply color with shading
    color.rgb *= calcShading(normalize(normalMatrix * normal), lightDir).rgb;
    color.a = 1.0;
  })";
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

    mSp.unbind();

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

    // Set up backface culling
    glCullFace(GL_BACK);
    // our polygon winding is counter clockwise
    glFrontFace(GL_CCW);
    glDepthFunc(GL_LESS);

    TextureManager::instance()->loadTexture("heightmap", "heightmap.png", true, 0);
    TextureManager::instance()->loadTexture("normalmap", "normalmap.png", true, 0);

    // setup shader
    ShaderManager::instance()->addShaderProgram(
        "Heightmap",
        vertexShader,
        fragmentShader
    );
    mSp = ShaderManager::instance()->getShaderProgram("Heightmap");

    mSp.bind();
    heightTextureLoc = mSp.getUniformLocation("hTex");
    normalTextureLoc = mSp.getUniformLocation("nTex");
    currTimeLoc = mSp.getUniformLocation("currTime");
    MVPLoc = mSp.getUniformLocation("mvp");
    MVLoc = mSp.getUniformLocation("mv");
    MVLightLoc = mSp.getUniformLocation("mvLight");
    NMLoc = mSp.getUniformLocation("normalMatrix");
    lightPosLoc = mSp.getUniformLocation("lightPos");
    lightAmbLoc = mSp.getUniformLocation("lightAmbient");
    lightDifLoc = mSp.getUniformLocation("lightDiffuse");
    lightSpeLoc = mSp.getUniformLocation("lightSpecular");
    glUniform1i(heightTextureLoc, 0);
    glUniform1i(normalTextureLoc, 1);
    glUniform4fv(lightPosLoc, 1, glm::value_ptr(lightPosition));
    glUniform4fv(lightAmbLoc, 1, glm::value_ptr(lightAmbient));
    glUniform4fv(lightDifLoc, 1, glm::value_ptr(lightDiffuse));
    glUniform4fv(lightSpeLoc, 1, glm::value_ptr(lightSpecular));
    mSp.unbind();

    Geometry geometry = generateTerrainGrid(1.f, 1.f, GridSize, GridSize);

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
    SharedData::instance()->writeBool(info);
    SharedData::instance()->writeBool(stats);
    SharedData::instance()->writeBool(takeScreenshot);
    SharedData::instance()->writeBool(useTracking);
    SharedData::instance()->writeObj(stereoMode);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
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
            case key::Q:
                gEngine->terminate();
                break;
            case key::T:
                useTracking.setVal(!useTracking.getVal());
                break;
            case key::E:
                sgct::core::ClusterManager::instance()->getDefaultUser().setTransform(
                    glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, 0.0, 4.0))
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

void cleanUpFun() {
    glDeleteBuffers(1, &vertexPositionBuffer);
    glDeleteBuffers(1, &texCoordBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);
    gEngine = Engine::instance();

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setEncodeFunction(encodeFun);
    gEngine->setDecodeFunction(decodeFun);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile, cluster)) {
        Engine::destroy();
        return EXIT_FAILURE;
    }

    gEngine->render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
