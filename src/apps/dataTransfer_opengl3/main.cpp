#include <algorithm>
#include <fstream>
#include <memory>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "sgct.h"
#include "sgct/ClusterManager.h"
#include "sgct/Image.h"

using namespace sgct;

namespace {
    constexpr const int headerSize = 1;
} // namespace

Engine* gEngine;

void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
void keyCallback(int key, int scancode, int action, int modifiers);
void contextCreationCallback(GLFWwindow* win);

void myDropCallback(int count, const char** paths);

void myDataTransferDecoder(void* receivedData, int receivedLength, int packageId,
    int clientIndex);
void myDataTransferStatus(bool connected, int clientIndex);
void myDataTransferAcknowledge(int packageId, int clientIndex);

void startDataTransfer();
void readImage(unsigned char* data, int len);
void uploadTexture();
void threadWorker();

std::unique_ptr<std::thread> loadThread;
std::mutex mutex;
GLFWwindow* hiddenWindow;
GLFWwindow* sharedWindow;
std::unique_ptr<sgct_core::Image> transImg;

SharedBool info(false);
SharedBool stats(false);
SharedInt32 texIndex(-1);

SharedInt32 currentPackage(-1);
SharedBool running(true);
SharedBool transfer(false);
SharedBool serverUploadDone(false);
SharedBool clientsUploadDone(false);
SharedVector<std::pair<std::string, int>> imagePaths;
SharedVector<GLuint> texIds;
double sendTimer = 0.0;

enum imageType { IM_JPEG, IM_PNG };
std::unique_ptr<sgct_utils::SGCTBox> myBox;
GLint Matrix_Loc = -1;

//variables to share across cluster
SharedDouble currentTime(0.0);

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);
  
    gEngine->setInitOGLFunction(myInitOGLFun);
    gEngine->setDrawFunction(myDrawFun);
    gEngine->setPreSyncFunction(myPreSyncFun);
    gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
    gEngine->setCleanUpFunction(myCleanUpFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setContextCreationCallback(contextCreationCallback);
    gEngine->setDropCallbackFunction(myDropCallback);

    if( !gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile)) {
        delete gEngine;
        return EXIT_FAILURE;
    }
    
    gEngine->setDataTransferCallback(myDataTransferDecoder);
    gEngine->setDataTransferStatusCallback(myDataTransferStatus);
    gEngine->setDataAcknowledgeCallback(myDataTransferAcknowledge);

    SharedData::instance()->setEncodeFunction(myEncodeFun);
    SharedData::instance()->setDecodeFunction(myDecodeFun);

    gEngine->render();

    running.setVal(false);

    if (loadThread) {
        loadThread->join();
        loadThread = nullptr;
    }

    delete gEngine;
    exit(EXIT_SUCCESS);
}

void myDrawFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    double speed = 0.44;

    // create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * speed),
        glm::vec3(0.f, -1.f, 0.f)
    );
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * (speed / 2.0)),
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

    ShaderManager::instance()->bindShaderProgram( "xform" );

    glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, glm::value_ptr(MVP));

    myBox->draw();

    ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void myPreSyncFun() {
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

void myPostSyncPreDrawFun() {
    gEngine->setDisplayInfoVisibility(info.getVal());
    gEngine->setStatsGraphVisibility(stats.getVal());
}

void myInitOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(
        sgct::TextureManager::CompressionMode::S3TC_DXT
    );
    TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    myBox = std::make_unique<sgct_utils::SGCTBox>(
        2.f,
        sgct_utils::SGCTBox::TextureMappingMode::Regular
    );

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // our polygon winding is counter clockwise

    ShaderManager::instance()->addShaderProgram("xform", "xform.vert", "xform.frag");
    ShaderManager::instance()->bindShaderProgram("xform");

    Matrix_Loc = ShaderManager::instance()->getShaderProgram("xform").getUniformLocation("MVP");
    glUniform1i(
        ShaderManager::instance()->getShaderProgram("xform").getUniformLocation("Tex"),
        0
    );

    ShaderManager::instance()->unBindShaderProgram();
}

void myEncodeFun() {
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeBool(info);
    SharedData::instance()->writeBool(stats);
    SharedData::instance()->writeInt32(texIndex);
}

void myDecodeFun() {
    SharedData::instance()->readDouble(currentTime);
    SharedData::instance()->readBool(info);
    SharedData::instance()->readBool(stats);
    SharedData::instance()->readInt32(texIndex);
}

void myCleanUpFun() {
    myBox = nullptr;
    
    for(size_t i = 0; i < texIds.getSize(); i++) {
        GLuint tex = texIds.getValAt(i);
        if (tex) {
            glDeleteTextures(1, &tex);
            texIds.setValAt(i, 0);
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
        case SGCT_KEY_S:
            if (action == SGCT_PRESS) {
                stats.setVal(!stats.getVal());
            }
            break;
        case SGCT_KEY_I:
            if (action == SGCT_PRESS) {
                info.setVal(!info.getVal());
            }
            break;
        }
    }
}

void contextCreationCallback(GLFWwindow* win) {
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    
    sharedWindow = win;
    hiddenWindow = glfwCreateWindow(1, 1, "Thread Window", nullptr, sharedWindow);
     
    if (!hiddenWindow) {
        MessageHandler::instance()->print("Failed to create loader context!\n");
    }
    
    glfwMakeContextCurrent(sharedWindow);
    
    if (gEngine->isMaster()) {
        loadThread = std::make_unique<std::thread>(threadWorker);
    }
}

void myDataTransferDecoder(void* receivedData, int receivedLength, int packageId,
                           int clientIndex)
{
    MessageHandler::instance()->print(
        "Decoding %d bytes in transfer id: %d on node %d\n",
        receivedLength, packageId, clientIndex
    );

    currentPackage.setVal(packageId);
    
    // read the image on master
    readImage(reinterpret_cast<unsigned char*>(receivedData), receivedLength);
    uploadTexture();
}

void myDataTransferStatus(bool connected, int clientIndex) {
    MessageHandler::instance()->print(
        "Transfer node %d is %s.\n", clientIndex, connected ? "connected" : "disconnected"
    );
}

void myDataTransferAcknowledge(int packageId, int clientIndex) {
    MessageHandler::instance()->print(
        "Transfer id: %d is completed on node %d.\n", packageId, clientIndex
    );
    
    static int counter = 0;
    if (packageId == currentPackage.getVal()) {
        counter++;
        if (counter == (sgct_core::ClusterManager::instance()->getNumberOfNodes() - 1)) {
            clientsUploadDone = true;
            counter = 0;
            
            MessageHandler::instance()->print(
                "Time to distribute and upload textures on cluster: %f ms\n", 
                (sgct::Engine::getTime() - sendTimer) * 1000.0
            );
        }
    }
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
            
            if (sgct_core::ClusterManager::instance()->getNumberOfNodes() == 1) {
                // no cluster
                clientsUploadDone = true;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void startDataTransfer() {
    // iterate
    int id = currentPackage.getVal();
    id++;
    currentPackage.setVal(id);
    
    // make sure to keep within bounds
    if (static_cast<int>(imagePaths.getSize()) > id) {
        sendTimer = sgct::Engine::getTime();
        
        // load from file
        const std::pair<std::string, int>& tmpPair =
            imagePaths.getValAt(static_cast<std::size_t>(id));
        
        std::ifstream file(tmpPair.first.c_str(), std::ios::binary);
        file.seekg(0, std::ios::end);
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<char> buffer(size + headerSize);
        char type = tmpPair.second;
        
        // write header (single unsigned char)
        buffer[0] = type;
        
        if (file.read(buffer.data() + headerSize, size)) {
            gEngine->transferDataBetweenNodes(
                buffer.data(),
                static_cast<int>(buffer.size()),
                id
            );
            
            // read the image on master
            readImage(
                reinterpret_cast<unsigned char*>(buffer.data()),
                static_cast<int>(buffer.size())
            );
        }
    }
}

void readImage(unsigned char* data, int len) {
    mutex.lock();
    
    transImg = std::make_unique<sgct_core::Image>();
    
    char type = static_cast<char>(data[0]);
    
    bool result = false;
    switch (type) {
        case IM_JPEG:
            result = transImg->loadJPEG(
                reinterpret_cast<unsigned char*>(data + headerSize),
                len-headerSize
            );
            break;
        case IM_PNG:
            result = transImg->loadPNG(
                reinterpret_cast<unsigned char*>(data + headerSize),
                len - headerSize
            );
            break;
    }
    
    if (!result) {
        transImg = nullptr;
    }

    mutex.unlock();
}

void uploadTexture() {
    mutex.lock();
    
    if (transImg) {
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
            static_cast<GLsizei>(transImg->getWidth()),
            static_cast<GLsizei>(transImg->getHeight())
        );
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            static_cast<GLsizei>(transImg->getWidth()),
            static_cast<GLsizei>(transImg->getHeight()),
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
        
        glBindTexture(GL_TEXTURE_2D, GL_FALSE);
        
        sgct::MessageHandler::instance()->print(
            "Texture id %d loaded (%dx%dx%d).\n",
            tex, transImg->getWidth(), transImg->getHeight(), transImg->getChannels()
        );
        
        texIds.addVal(tex);
        
        transImg = nullptr;
        
        glFinish();
        
        glfwMakeContextCurrent(nullptr);
    }
    else {
        // if invalid load
        texIds.addVal(0);
    }
    
    mutex.unlock();
}

void myDropCallback(int count, const char** paths) {
    if (gEngine->isMaster()) {
        size_t found;

        // simply pick the first path to transmit
        std::string tmpStr(paths[0]);

        // transform to lowercase
        std::transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);

        found = tmpStr.find(".jpg");
        if (found != std::string::npos) {
            imagePaths.addVal(std::pair<std::string, int>(paths[0], IM_JPEG));
            transfer.setVal(true);
            return;
        }

        found = tmpStr.find(".jpeg");
        if (found != std::string::npos) {
            imagePaths.addVal(std::pair<std::string, int>(paths[0], IM_JPEG));
            transfer.setVal(true);
            return;
        }

        found = tmpStr.find(".png");
        if (found != std::string::npos) {
            imagePaths.addVal(std::pair<std::string, int>(paths[0], IM_PNG));
            transfer.setVal(true);
            return;
        }
    }
}
