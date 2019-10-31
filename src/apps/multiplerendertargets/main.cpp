#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/readconfig.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#include <sgct/utils/box.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    std::unique_ptr<sgct::utils::Box> box;

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedBool takeScreenshot(false);

    // shader locs
    int textureLoc = -1;
    int mvpMatrixLoc = -1;
    int worldMatrixTransposeLoc = -1;
    int normalMatrixLoc = -1;

    unsigned int textureId = 0;

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
    mat3 worldRotationInverse = mat3(worldMatrixTranspose);

    gl_Position =  mvpMatrix * vec4(vertPositions, 1.0);
    uv = texCoords;
    n  = normalize(worldRotationInverse * normalMatrix * normals);
    p  = gl_Position;
  })";

    constexpr const char* fragmentShader = R"(
  #version 330 core

  in vec2 uv;
  in vec3 n;
  in vec4 p;

  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
  layout(location = 2) out vec3 position;

  uniform sampler2D tDiffuse;

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

    constexpr const double Speed = 0.44;

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

    const glm::mat4 mvp = Engine::instance()->getCurrentModelViewProjectionMatrix() * scene;
    const glm::mat4 mv = Engine::instance()->getCurrentModelViewMatrix() * scene;
    const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(mv));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    ShaderManager::instance()->getShaderProgram("MRT").bind();
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(worldMatrixTransposeLoc, 1, GL_TRUE, glm::value_ptr(mv));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    glUniform1i(textureLoc, 0);

    box->draw();

    ShaderManager::instance()->getShaderProgram("MRT").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (Engine::instance()->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void postSyncPreDrawFun() {
    if (takeScreenshot.getVal()) {
        Engine::instance()->takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void initOGLFun() {
    ShaderManager::instance()->addShaderProgram("MRT", vertexShader, fragmentShader);
    const ShaderProgram& prg = ShaderManager::instance()->getShaderProgram("MRT");
    prg.bind();
    textureLoc = prg.getUniformLocation("tDiffuse");
    worldMatrixTransposeLoc = prg.getUniformLocation("worldMatrixTranspose");
    mvpMatrixLoc = prg.getUniformLocation("mvpMatrix");
    normalMatrixLoc = prg.getUniformLocation("normalMatrix");

    prg.bind();
    textureId = TextureManager::instance()->loadTexture("box.png", true, 8.f);

    box = std::make_unique<utils::Box>(2.f, utils::Box::TextureMappingMode::Regular);

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
    if (Engine::instance()->isMaster() && (action == action::Press) && (key == key::P)) {
        takeScreenshot.setVal(true);
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    if (cluster.settings) {
        cluster.settings->useNormalTexture = true;
        cluster.settings->usePositionTexture = true;
    }
    else {
        config::Settings settings;
        settings.useNormalTexture = true;
        settings.usePositionTexture = true;
        cluster.settings = settings;
    }

    Engine::create(config);

    Engine::instance()->setInitOGLFunction(initOGLFun);
    Engine::instance()->setDrawFunction(drawFun);
    Engine::instance()->setPreSyncFunction(preSyncFun);
    Engine::instance()->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    Engine::instance()->setCleanUpFunction(cleanUpFun);
    Engine::instance()->setKeyboardCallbackFunction(keyCallback);
    Engine::instance()->setEncodeFunction(encodeFun);
    Engine::instance()->setDecodeFunction(decodeFun);

    try {
        Engine::instance()->init(Engine::RunMode::Default_Mode, cluster);
    }
    catch (const std::runtime_error&) {
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance()->render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
