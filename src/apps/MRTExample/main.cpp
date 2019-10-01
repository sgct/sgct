#include <sgct.h>
#include <sgct/readconfig.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct::utils::Box> box;

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool takeScreenshot(false);

    // shader locs
    int textureID = -1;
    int mvpMatrixId = -1;
    int worldMatrixTransposeId = -1;
    int normalMatrixId = -1;

    constexpr const char* vertexShader = R"(
  #version 330 core

  layout(location = 0) in vec2 texCoords;
  layout(location = 1) in vec3 normals;
  layout(location = 2) in vec3 vertPositions;

  uniform mat4 mvpMatrix;
  uniform mat4 worldMatrixTranspose;
  uniform mat3 normalMatrix;

  out vec2 uv;
  out vec3 n;
  out vec4 p;

  void main() {
    // Move the normals back from the camera space to the world space
    mat3 worldRotationInverse = mat3(worldMatrixTranspose);

    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvpMatrix * vec4(vertPositions, 1.0);
    uv = texCoords;
    n  = normalize(worldRotationInverse * normalMatrix * normals);
    p  = gl_Position;
  })";

    constexpr const char* fragmentShader = R"(
  #version 330 core

  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
  layout(location = 2) out vec3 position;

  uniform sampler2D tDiffuse;

  in vec2 uv;
  in vec3 n;
  in vec4 p;

  void main() {
    diffuse = texture(tDiffuse, uv);
    normal = n;
    position = p.xyz;
  }
)";
} // namespace

using namespace sgct;

void drawFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr double Speed = 0.44;

    // create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * Speed),
        glm::vec3(0.f, -1.f, 0.f)
    );
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * (Speed / 2.0)),
        glm::vec3(1.f, 0.f, 0.f)
    );

    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix() * scene;
    const glm::mat4 mv = gEngine->getCurrentModelViewMatrix() * scene;
    const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(mv));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));

    ShaderManager::instance()->bindShaderProgram("MRT");

    glUniformMatrix4fv(mvpMatrixId, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(worldMatrixTransposeId, 1, GL_TRUE, glm::value_ptr(mv));
    glUniformMatrix3fv(normalMatrixId, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    glUniform1i(textureID, 0);

    box->draw();

    ShaderManager::instance()->unBindShaderProgram();

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

void initOGLFun() {
    ShaderManager::instance()->addShaderProgram(
        "MRT",
        vertexShader,
        fragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    ShaderManager::instance()->bindShaderProgram("MRT");

    const ShaderProgram& prg = ShaderManager::instance()->getShaderProgram("MRT");

    textureID = prg.getUniformLocation("tDiffuse");
    worldMatrixTransposeId = prg.getUniformLocation("worldMatrixTranspose");
    mvpMatrixId = prg.getUniformLocation("mvpMatrix");
    normalMatrixId = prg.getUniformLocation("normalMatrix");

    ShaderManager::instance()->unBindShaderProgram();
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "box.png", true);

    // test
    int sizeX;
    int sizeY;
    int sizeC;
    std::string path = TextureManager::instance()->getTexturePath("box");
    TextureManager::instance()->getDimensions("box", sizeX, sizeY, sizeC);
    MessageHandler::instance()->print(
        "Texture info, x=%d, y=%d, c=%d, path=%s\n",
        sizeX, sizeY, sizeC, path.c_str()
    );

    box = std::make_unique<sgct::utils::Box>(
        2.f,
        sgct::utils::Box::TextureMappingMode::Regular
        );

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

void encodeFun() {
    sgct::SharedData::instance()->writeDouble(currentTime);
    sgct::SharedData::instance()->writeBool(takeScreenshot);
}

void decodeFun() {
    sgct::SharedData::instance()->readDouble(currentTime);
    sgct::SharedData::instance()->readBool(takeScreenshot);
}

void cleanUpFun() {
    box = nullptr;
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster()) {
        switch (key) {
            case key::P:
            case key::F10:
                if (action == action::Press) {
                    takeScreenshot.setVal(true);
                }
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);

    sgct::Settings::instance()->setUseNormalTexture(true);
    sgct::Settings::instance()->setUsePositionTexture(true);

    if (!gEngine->init(sgct::Engine::RunMode::OpenGL_3_3_Core_Profile, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction(encodeFun);
    sgct::SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();

    delete gEngine;

    exit(EXIT_SUCCESS);
}
