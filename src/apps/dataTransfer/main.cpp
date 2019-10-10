#include <sgct.h>
#include <sgct/ClusterManager.h>
#include <sgct/Image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <memory>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<std::thread> loadThread;
    std::mutex mutex;
    GLFWwindow* hiddenWindow;
    GLFWwindow* sharedWindow;
    std::unique_ptr<sgct::core::Image> transImg;

    sgct::SharedBool info(false);
    sgct::SharedBool stats(false);
    sgct::SharedInt32 texIndex(-1);

    sgct::SharedInt32 currentPackage(-1);
    sgct::SharedBool running(true);
    sgct::SharedBool transfer(false);
    sgct::SharedBool serverUploadDone(false);
    sgct::SharedBool clientsUploadDone(false);
    sgct::SharedVector<std::string> imagePaths;
    sgct::SharedVector<GLuint> texIds;
    double sendTimer = 0.0;

    std::unique_ptr<sgct::utils::Box> box;
    GLint matrixLoc = -1;

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);

    constexpr const char* vertexShader = R"(
  #version 330 core

  layout(location = 0) in vec2 texCoords;
  layout(location = 1) in vec3 normals;
  layout(location = 2) in vec3 vertPositions;

  uniform mat4 mvp;
  out vec2 uv;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPositions, 1.0);
    uv = texCoords;
  })";
    
    constexpr const char* fragmentShader = R"(
  #version 330 core

  uniform sampler2D tex;

  in vec2 uv;
  out vec4 color;

  void main() { color = texture(tex, uv.st); }
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

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene;

    glActiveTexture(GL_TEXTURE0);
    
    if (texIndex.getVal() != -1) {
        glBindTexture(GL_TEXTURE_2D, texIds.getValAt(texIndex.getVal()));
    }
    else {
        glBindTexture(
            GL_TEXTURE_2D,
            TextureManager::instance()->getTextureId("box")
        );
    }

    ShaderManager::instance()->bindShaderProgram("xform");

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(MVP));

    box->draw();

    ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
        
        // if texture is uploaded then iterate the index
        if (serverUploadDone.getVal() && clientsUploadDone.getVal()) {
            texIndex.setVal(texIndex.getVal() + 1);
            serverUploadDone = false;
            clientsUploadDone = false;
        }
    }
}

void postSyncPreDrawFun() {
    gEngine->setDisplayInfoVisibility(info.getVal());
    gEngine->setStatsGraphVisibility(stats.getVal());
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "box.png", true);

    box = std::make_unique<sgct::utils::Box>(
        2.f,
        sgct::utils::Box::TextureMappingMode::Regular
    );

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // our polygon winding is counter clockwise

    ShaderManager::instance()->addShaderProgram(
        "xform",
        vertexShader,
        fragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    ShaderManager::instance()->bindShaderProgram("xform");

    const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("xform");
    matrixLoc = prog.getUniformLocation("mvp");
    glUniform1i(prog.getUniformLocation("tex"), 0 );
    ShaderManager::instance()->unBindShaderProgram();
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeBool(info);
    SharedData::instance()->writeBool(stats);
    SharedData::instance()->writeInt32(texIndex);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
    SharedData::instance()->readBool(info);
    SharedData::instance()->readBool(stats);
    SharedData::instance()->readInt32(texIndex);
}

void cleanUpFun() {
    box = nullptr;
    
    for (size_t i = 0; i < texIds.getSize(); i++) {
        GLuint tex = texIds.getValAt(i);
        if (tex) {
            glDeleteTextures(1, &tex);
        }
    }
    texIds.clear();
    
    
    if (hiddenWindow) {
        glfwDestroyWindow(hiddenWindow);
    }
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
        }
    }
}

void readImage(unsigned char* data, int len) {
    std::unique_lock lk(mutex);

    transImg = std::make_unique<sgct::core::Image>();

    const bool result = transImg->load(reinterpret_cast<unsigned char*>(data), len);
    if (!result) {
        transImg = nullptr;
    }
}

void startDataTransfer() {
    // iterate
    int id = currentPackage.getVal();
    id++;
    currentPackage.setVal(id);

    // make sure to keep within bounds
    if (static_cast<int>(imagePaths.getSize()) <= id) {
        return;
    }
        
    sendTimer = sgct::Engine::getTime();

    // load from file
    const std::string& tmpPair = imagePaths.getValAt(static_cast<size_t>(id));

    std::ifstream file(tmpPair.c_str(), std::ios::binary);
    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size)) {
        const int s = static_cast<int>(size);
        gEngine->transferDataBetweenNodes(buffer.data(), s, id);

        // read the image on master
        readImage(reinterpret_cast<unsigned char*>(buffer.data()), s);
    }
}

void uploadTexture() {
    std::unique_lock lk(mutex);
    
    if (!transImg) {
        // if invalid load
        texIds.addVal(0);
        return;
    }

    glfwMakeContextCurrent(hiddenWindow);
        
    // create texture
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
    GLenum internalformat;
    GLenum type;

    size_t bpc = transImg->getBytesPerChannel();

    switch (transImg->getChannels()) {
        case 1:
            internalformat = (bpc == 1 ? GL_R8 : GL_R16);
            type = GL_RED;
            break;
        case 2:
            internalformat = (bpc == 1 ? GL_RG8 : GL_RG16);
            type = GL_RG;
            break;
        case 3:
        default:
            internalformat = (bpc == 1 ? GL_RGB8 : GL_RGB16);
            type = GL_BGR;
            break;
        case 4:
            internalformat = (bpc == 1 ? GL_RGBA8 : GL_RGBA16);
            type = GL_BGRA;
            break;
    }
        
    int mipMapLevels = 8;
    GLenum format = (bpc == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT);

    glTexStorage2D(
        GL_TEXTURE_2D,
        mipMapLevels,
        internalformat,
        static_cast<GLsizei>(transImg->getSize().x),
        static_cast<GLsizei>(transImg->getSize().y)
    );
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        static_cast<GLsizei>(transImg->getSize().x),
        static_cast<GLsizei>(transImg->getSize().y),
        type,
        format,
        transImg->getData()
    );
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapLevels - 1);
        
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
    glBindTexture(GL_TEXTURE_2D, 0);
        
    sgct::MessageHandler::instance()->printInfo(
        "Texture id %d loaded (%dx%dx%d).",
        tex, transImg->getSize().x, transImg->getSize().y, transImg->getChannels()
    );
        
    texIds.addVal(tex);
        
    transImg = nullptr;
        
    glFinish();
        
    glfwMakeContextCurrent(nullptr);
}


void threadWorker() {
    while (running.getVal()) {
        // runs only on master
        if (transfer.getVal() && !serverUploadDone.getVal() &&
            !clientsUploadDone.getVal())
        {
            startDataTransfer();
            transfer.setVal(false);

            // load texture on master
            uploadTexture();
            serverUploadDone = true;

            if (sgct::core::ClusterManager::instance()->getNumberOfNodes() == 1) {
                // no cluster
                clientsUploadDone = true;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void contextCreationCallback(GLFWwindow* win) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    
    sharedWindow = win;
    hiddenWindow = glfwCreateWindow(1, 1, "Thread Window", nullptr, sharedWindow);
     
    if (!hiddenWindow) {
        MessageHandler::instance()->printInfo("Failed to create loader context!");
    }
    
    glfwMakeContextCurrent(sharedWindow);
    
    if (gEngine->isMaster()) {
        loadThread = std::make_unique<std::thread>(threadWorker);
    }
}

void dataTransferDecoder(void* receivedData, int receivedLength, int packageId,
                           int clientIndex)
{
    MessageHandler::instance()->printInfo(
        "Decoding %d bytes in transfer id: %d on node %d",
        receivedLength, packageId, clientIndex
    );

    currentPackage.setVal(packageId);
    
    // read the image on master
    readImage(reinterpret_cast<unsigned char*>(receivedData), receivedLength);
    uploadTexture();
}

void dataTransferStatus(bool connected, int clientIndex) {
    MessageHandler::instance()->printInfo(
        "Transfer node %d is %s.", clientIndex, connected ? "connected" : "disconnected"
    );
}

void dataTransferAcknowledge(int packageId, int clientIndex) {
    MessageHandler::instance()->printInfo(
        "Transfer id: %d is completed on node %d.", packageId, clientIndex
    );
    
    static int counter = 0;
    if (packageId == currentPackage.getVal()) {
        counter++;
        if (counter == (sgct::core::ClusterManager::instance()->getNumberOfNodes() - 1)) {
            clientsUploadDone = true;
            counter = 0;
            
            MessageHandler::instance()->printInfo(
                "Time to distribute and upload textures on cluster: %f ms", 
                (sgct::Engine::getTime() - sendTimer) * 1000.0
            );
        }
    }
}

void dropCallback(int, const char** paths) {
    if (!gEngine->isMaster()) {
        return;
    }

    // simply pick the first path to transmit
    std::string tmpStr(paths[0]);

    // transform to lowercase
    std::transform(
        tmpStr.begin(),
        tmpStr.end(),
        tmpStr.begin(),
        [](char c) { return static_cast<char>(c); }
    );

    const bool foundJpg = tmpStr.find(".jpg") != std::string::npos;
    const bool foundJpeg = tmpStr.find(".jpeg") != std::string::npos;
    const bool foundPng = tmpStr.find(".png") != std::string::npos;
    if (foundJpg || foundJpeg || foundPng) {
        imagePaths.addVal(paths[0]);
        transfer.setVal(true);
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
    gEngine->setContextCreationCallback(contextCreationCallback);
    gEngine->setDropCallbackFunction(dropCallback);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->setDataTransferCallback(dataTransferDecoder);
    gEngine->setDataTransferStatusCallback(dataTransferStatus);
    gEngine->setDataAcknowledgeCallback(dataTransferAcknowledge);

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();

    running.setVal(false);

    if (loadThread) {
        loadThread->join();
        loadThread = nullptr;
    }

    delete gEngine;
    exit(EXIT_SUCCESS);
}
