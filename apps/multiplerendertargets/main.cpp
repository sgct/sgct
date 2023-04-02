/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>

#include <sgct/utils/box.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    std::unique_ptr<sgct::utils::Box> box;

    // variables to share across cluster
    double currentTime = 0.0;
    bool takeScreenshot = false;

    // shader locs
    int textureLoc = -1;
    int mvpMatrixLoc = -1;
    int worldMatrixTransposeLoc = -1;
    int normalMatrixLoc = -1;

    unsigned int textureId = 0;

    constexpr std::string_view VertexShader = R"(
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

    constexpr std::string_view FragmentShader = R"(
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

void draw(const RenderData& data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr double Speed = 0.44;

    // create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime * Speed),
        glm::vec3(0.f, -1.f, 0.f)
    );
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime * (Speed / 2.0)),
        glm::vec3(1.f, 0.f, 0.f)
    );

    const glm::mat4 mvp = glm::make_mat4(data.modelViewProjectionMatrix.values) * scene;
    const glm::mat4 mv = glm::make_mat4(data.viewMatrix.values) *
        glm::make_mat4(data.modelMatrix.values) * scene;
    const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(mv));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    ShaderManager::instance().shaderProgram("MRT").bind();
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(worldMatrixTransposeLoc, 1, GL_TRUE, glm::value_ptr(mv));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    glUniform1i(textureLoc, 0);

    box->draw();

    ShaderManager::instance().shaderProgram("MRT").unbind();

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

void initOGL(GLFWwindow*) {
    ShaderManager::instance().addShaderProgram("MRT", VertexShader, FragmentShader);
    const ShaderProgram& prg = ShaderManager::instance().shaderProgram("MRT");
    prg.bind();
    textureLoc = glGetUniformLocation(prg.id(), "tDiffuse");
    worldMatrixTransposeLoc = glGetUniformLocation(prg.id(), "worldMatrixTranspose");
    mvpMatrixLoc = glGetUniformLocation(prg.id(), "mvpMatrix");
    normalMatrixLoc = glGetUniformLocation(prg.id(), "normalMatrix");

    prg.bind();
    textureId = TextureManager::instance().loadTexture("box.png", true, 8.f);

    box = std::make_unique<utils::Box>(2.f, utils::Box::TextureMappingMode::Regular);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
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
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (Engine::instance().isMaster() && (action == Action::Press)) {
        if (key == Key::Esc) {
            Engine::instance().terminate();
        }
        else if (key == Key::P) {
            takeScreenshot = true;
        }
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    if (!cluster.success) {
        return -1;
    }

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

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGL;
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
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
