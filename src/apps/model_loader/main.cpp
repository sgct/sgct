#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#include "objloader.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    GLuint vboPositions = 0;
    GLuint vboUvs = 0;
    GLuint vboNormals = 0;
    GLuint VertexArrayID = 0;
    GLsizei numberOfVertices = 0;

    unsigned int textureId = 0;

    // shader locations
    GLint MVPLoc = -1;
    GLint NMLoc = -1;

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);

    constexpr const char* vertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPositions;
  layout(location = 1) in vec2 texCoords;
  layout(location = 2) in vec3 normals;

  uniform mat4 mvp;
  uniform mat3 nm; //Normal Matrix

  out vec2 uv;
  out vec3 tnormals; //transformed normals

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position = mvp * vec4(vertPositions, 1.0);
    uv = texCoords;
    tnormals = normalize(nm * normals);
  })";

    constexpr const char* fragmentShader = R"(
  #version 330 core

  uniform sampler2D tex;

  in vec2 uv;
  in vec3 tnormals;
  out vec4 color;

  void main() { color = texture(tex, uv); }
)";
} // namespace

using namespace sgct;

/// Loads obj model and uploads to the GPU 
void loadModel(std::string filename) {
    // Read our .obj file
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    const bool success = loadOBJ(filename.c_str(), positions, uvs, normals);
    if (success) {
        // store the number of triangles
        numberOfVertices = static_cast<GLsizei>(positions.size());

        // create VAO
        glGenVertexArrays(1, &VertexArrayID);
        glBindVertexArray(VertexArrayID);

        glGenBuffers(1, &vboPositions);
        glGenBuffers(1, &vboUvs);
        glGenBuffers(1, &vboNormals);

        glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
        glBufferData(
            GL_ARRAY_BUFFER,
            positions.size() * sizeof(glm::vec3),
            positions.data(),
            GL_STATIC_DRAW
        );

        // first attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        if (!uvs.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, vboUvs);
            glBufferData(
                GL_ARRAY_BUFFER,
                uvs.size() * sizeof(glm::vec2),
                uvs.data(),
                GL_STATIC_DRAW
            );

            // second attribute buffer : UVs
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        }
        else {
            MessageHandler::printInfo("Warning: Model is missing UV data");
        }

        if (!normals.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
            glBufferData(
                GL_ARRAY_BUFFER,
                normals.size() * sizeof(glm::vec3),
                normals.data(),
                GL_STATIC_DRAW
            );
            // third attribute buffer : Normals
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        }
        else {
            MessageHandler::printInfo("Warning: Model is missing normal data");
        }

        glBindVertexArray(0);

        // clear vertex data that is uploaded on GPU
        positions.clear();
        uvs.clear();
        normals.clear();

        //print some usefull info
        sgct::MessageHandler::printInfo(
            "Model '%s' loaded successfully (%u vertices, VAO: %u, VBOs: %u %u %u).",
            filename.c_str(), numberOfVertices, VertexArrayID,
            vboPositions, vboUvs, vboNormals
        );
    }
    else {
        sgct::MessageHandler::printInfo("Failed to load model '%s'", filename.c_str());
    }
}

void drawFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr const double Speed = 0.44;

    //create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * Speed),
        glm::vec3(0.f, -1.f, 0.f)
    );

    const glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    ShaderManager::instance()->getShaderProgram("xform").bind();

    glUniformMatrix4fv(MVPLoc, 1, GL_FALSE, glm::value_ptr(MVP));
    glUniformMatrix3fv(NMLoc, 1, GL_FALSE, glm::value_ptr(MVP));

    glBindVertexArray(VertexArrayID);
    glDrawArrays(GL_TRIANGLES, 0, numberOfVertices);
    glBindVertexArray(0);

    ShaderManager::instance()->getShaderProgram("xform").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void initOGLFun() {
    textureId = TextureManager::instance()->loadTexture("box.png", true, 4.f);
    loadModel("box.obj");

    // Set up backface culling
    glCullFace(GL_BACK);
    // our polygon winding is counter clockwise
    glFrontFace(GL_CCW);

    ShaderManager::instance()->addShaderProgram("xform", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("xform");
    prog.bind();
    MVPLoc = prog.getUniformLocation("mvp");
    NMLoc = prog.getUniformLocation("nm");
    GLint textureLocation = prog.getUniformLocation("tex");
    glUniform1i(textureLocation, 0);
    prog.unbind();
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
}

/**
 * De-allocate data from GPU. Textures are deleted automatically when using texture
 * manager. Shaders are deleted automatically when using shader manager
*/
void cleanUpFun() {
    glDeleteVertexArrays(1, &VertexArrayID);
    VertexArrayID = 0;

    glDeleteBuffers(1, &vboPositions);
    vboPositions = 0;
    glDeleteBuffers(1, &vboUvs);
    vboUvs = 0;
    glDeleteBuffers(1, &vboNormals);
    vboNormals = 0;
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
    gEngine->setCleanUpFunction(cleanUpFun);
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
