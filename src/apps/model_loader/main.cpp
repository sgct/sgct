#include <sgct.h>
#include "objloader.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    bool hasUVs = false; //init to false and set to true when loading UVs
    bool hasNormals = false; //init to false and set to true when loading normals

    GLuint vboPositions = 0;
    GLuint vboUvs = 0;
    GLuint vboNormals = 0;
    GLsizei numberOfVertices = 0;

    //variables to share across cluster
    sgct::SharedDouble currentTime(0.0);

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

        if (!uvs.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, vboUvs);
            glBufferData(
                GL_ARRAY_BUFFER,
                uvs.size() * sizeof(glm::vec2),
                uvs.data(),
                GL_STATIC_DRAW
            );
            hasUVs = true;
        }
        else {
            sgct::MessageHandler::instance()->print(
                "Warning: Model is missing UV data\n"
            );
        }

        if (!normals.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
            glBufferData(
                GL_ARRAY_BUFFER,
                normals.size() * sizeof(glm::vec3),
                normals.data(),
                GL_STATIC_DRAW
            );
            hasNormals = true;
        }
        else {
            sgct::MessageHandler::instance()->print(
                "Warning: Model is missing normal data\n"
            );
        }

        glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);

        sgct::MessageHandler::instance()->print(
            "Model '%s' loaded successfully (%u vertices, VBOs: %u %u %u)\n",
            filename.c_str(), numberOfVertices, vboPositions, vboUvs, vboNormals
        );
    }
    else {
        sgct::MessageHandler::instance()->print(
            "Failed to load model '%s'\n", filename.c_str()
        );
    }
}

void drawFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    constexpr const double Speed = 0.44;

    // create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * Speed),
        glm::vec3(0.f, -1.f, 0.f)
    );

    glMultMatrixf(glm::value_ptr(scene));

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    
    glColor4f(1.f, 1.f, 1.f, 1.f);

    
    glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<void*>(0));

    if (hasUVs) {
        glClientActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));
        glBindBuffer(GL_ARRAY_BUFFER, vboUvs);

        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, reinterpret_cast<void*>(0));
    }
    
    if (hasNormals) {
        glBindBuffer(GL_ARRAY_BUFFER, vboNormals);

        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, reinterpret_cast<void*>(0));
    }
    
    glDrawArrays(GL_TRIANGLES, 0, numberOfVertices);

    glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);
    glPopClientAttrib();


    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(sgct::Engine::getTime());
    }
}

void initOGLFun() {
    TextureManager::instance()->setWarpingMode(GL_REPEAT, GL_REPEAT);
    TextureManager::instance()->setAnisotropicFilterSize(4.0f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture(
        "box",
        "../SharedResources/box.png",
        true
    );

    loadModel("../SharedResources/box.obj");
    
    glEnable( GL_TEXTURE_2D );
    glDisable(GL_LIGHTING);

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
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
    glDeleteBuffers(1, &vboPositions);
    vboPositions = 0;
    glDeleteBuffers(1, &vboUvs);
    vboUvs = 0;
    glDeleteBuffers(1, &vboNormals);
    vboNormals = 0;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setCleanUpFunction(cleanUpFun);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
