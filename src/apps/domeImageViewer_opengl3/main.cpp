#include <sgct.h>
#include <sgct/ClusterManager.h>
#include <sgct/Image.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>

namespace {
    constexpr const int HeaderSize = 1;
    enum class ImageType { JPEG, PNG };

    sgct::Engine* gEngine;

    std::unique_ptr<std::thread> loadThread;
    std::mutex mutex;
    GLFWwindow* hiddenWindow;
    GLFWwindow* sharedWindow;
    std::vector<std::unique_ptr<sgct::core::Image>> transImages;

    sgct::SharedBool info(false);
    sgct::SharedBool stats(false);
    sgct::SharedBool wireframe(false);
    sgct::SharedInt32 texIndex(-1);
    sgct::SharedInt32 incrIndex(1);
    sgct::SharedInt32 numSyncedTex(0);

    sgct::SharedInt32 lastPackage(-1);
    sgct::SharedBool running(true);
    sgct::SharedBool transfer(false);
    sgct::SharedBool serverUploadDone(false);
    sgct::SharedInt32 serverUploadCount(0);
    sgct::SharedBool clientsUploadDone(false);
    sgct::SharedVector<std::pair<std::string, int>> imagePaths;
    sgct::SharedVector<GLuint> texIds;
    double sendTimer = 0.0;

    std::unique_ptr<sgct::utils::Dome> dome;
    GLint matrixLoc = -1;

    sgct::SharedDouble currentTime(0.0);
} // namespace

using namespace sgct;

void readImage(unsigned char* data, int len) {
    std::unique_lock lk(mutex);

    std::unique_ptr<sgct::core::Image> img = std::make_unique<sgct::core::Image>();

    const char type = static_cast<char>(data[0]);
    assert(type == 0 || type == 1);
    ImageType t = static_cast<ImageType>(type);

    bool result = false;
    switch (t) {
        case ImageType::JPEG:
            result = img->loadJPEG(
                reinterpret_cast<unsigned char*>(data + HeaderSize),
                len - HeaderSize
            );
            break;
        case ImageType::PNG:
            result = img->loadPNG(
                reinterpret_cast<unsigned char*>(data + HeaderSize),
                len - HeaderSize
            );
            break;
    }

    if (result) {
        transImages.push_back(std::move(img));
    }
}

void startDataTransfer() {
    int id = lastPackage.getVal();
    id++;

    // make sure to keep within bounds
    if (static_cast<int>(imagePaths.getSize()) <= id) {
        return;
    }
    sendTimer = sgct::Engine::getTime();

    const int imageCounter = static_cast<int32_t>(imagePaths.getSize());
    lastPackage.setVal(imageCounter - 1);

    for (int i = id; i < imageCounter; i++) {
        // load from file
        using K = std::string;
        using V = int;
        const std::pair<K, V>& p = imagePaths.getValAt(static_cast<size_t>(i));

        std::ifstream file(p.first.c_str(), std::ios::binary);
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size + HeaderSize);
        int type = p.second;

        // write header (single unsigned char)
        buffer[0] = static_cast<char>(type);

        if (file.read(buffer.data() + HeaderSize, size)) {
            const int s = static_cast<int>(buffer.size());
            // transfer
            gEngine->transferDataBetweenNodes(buffer.data(), s, i );

            // read the image on master
            readImage(reinterpret_cast<unsigned char*>(buffer.data()), s);
        }
    }
}

void uploadTexture() {
    std::unique_lock lk(mutex);

    if (transImages.empty()) {
        return;
    }
    glfwMakeContextCurrent(hiddenWindow);

    for (size_t i = 0; i < transImages.size(); i++) {
        if (!transImages[i]) {
            // if invalid load
            texIds.addVal(GL_FALSE);
            continue;
        }

        // create texture
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const size_t bpc = transImages[i]->getBytesPerChannel();

        GLenum internalformat;
        GLenum type;
        switch (transImages[i]->getChannels()) {
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

        GLenum format = (bpc == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT);

        const GLsizei width = static_cast<GLsizei>(transImages[i]->getWidth());
        const GLsizei height = static_cast<GLsizei>(transImages[i]->getHeight());
        unsigned char* data = transImages[i]->getData();
        glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, width, height);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, type, format, data);

        // Disable mipmaps
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // unbind
        glBindTexture(GL_TEXTURE_2D, GL_FALSE);

        MessageHandler::instance()->print(
            "Texture id %d loaded (%dx%dx%d).\n",
            tex, transImages[i]->getWidth(), transImages[i]->getHeight(),
            transImages[i]->getChannels()
        );

        texIds.addVal(tex);

        transImages[i] = nullptr;
    }

    transImages.clear();
    glFinish();

    // restore
    glfwMakeContextCurrent(nullptr);
}

void threadWorker() {
    while (running.getVal()) {
        const bool trans = transfer.getVal();
        const bool serverDone = serverUploadDone.getVal();
        const bool clientDone = clientsUploadDone.getVal();
        // runs only on master
        if (trans && !serverDone && !clientDone) {
            startDataTransfer();
            transfer.setVal(false);

            // load textures on master
            uploadTexture();
            serverUploadDone = true;

            if (sgct::core::ClusterManager::instance()->getNumberOfNodes() == 1) {
                clientsUploadDone = true;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


void drawFun() {    
    if (texIndex.getVal() == -1) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix();
        
    glActiveTexture(GL_TEXTURE0);

    if ((texIds.getSize() > (texIndex.getVal() + 1)) &&
        gEngine->getCurrentFrustumMode() == sgct::core::Frustum::Mode::StereoRightEye)
    {
        glBindTexture(GL_TEXTURE_2D, texIds.getValAt(texIndex.getVal() + 1));
    }
    else {
        glBindTexture(GL_TEXTURE_2D, texIds.getValAt(texIndex.getVal()));
    }

    ShaderManager::instance()->bindShaderProgram("xform");
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(MVP));

    // draw the box
    dome->draw();

    ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
        
        // if texture is uploaded then iterate the index
        if (serverUploadDone.getVal() && clientsUploadDone.getVal()) {
            numSyncedTex = static_cast<int32_t>(texIds.getSize());
            
            // only iterate up to the first new image, even if multiple images was added
            texIndex = numSyncedTex.getVal() - serverUploadCount.getVal();

            serverUploadDone = false;
            clientsUploadDone = false;
        }
    }
}

void postSyncPreDrawFun() {
    gEngine->setDisplayInfoVisibility(info.getVal());
    gEngine->setStatsGraphVisibility(stats.getVal());
    gEngine->setWireframe(wireframe.getVal());
}

void initOGLFun() {
    dome = std::make_unique<sgct::utils::Dome>(7.4f, 180.f, 256, 128);

    // Set up backface culling
    glCullFace(GL_BACK);
    // our polygon winding is clockwise since we are inside of the dome
    glFrontFace(GL_CW);

    ShaderManager::instance()->addShaderProgram("xform", "xform.vert", "xform.frag");

    ShaderManager::instance()->bindShaderProgram("xform");

    const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("xform");
    matrixLoc = prog.getUniformLocation("mvp");
    GLint texLoc = prog.getUniformLocation("tex");
    glUniform1i(texLoc, 0);

    ShaderManager::instance()->unBindShaderProgram();
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeBool(info);
    SharedData::instance()->writeBool(stats);
    SharedData::instance()->writeBool(wireframe);
    SharedData::instance()->writeInt32(texIndex);
    SharedData::instance()->writeInt32(incrIndex);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
    SharedData::instance()->readBool(info);
    SharedData::instance()->readBool(stats);
    SharedData::instance()->readBool(wireframe);
    SharedData::instance()->readInt32(texIndex);
    SharedData::instance()->readInt32(incrIndex);
}

void cleanUpFun() {
    dome = nullptr;
    
    for (size_t i = 0; i < texIds.getSize(); i++) {
        GLuint tex = texIds.getValAt(i);
        if (tex) {
            glDeleteTextures(1, &tex);
            texIds.setValAt(i, GL_FALSE);
        }
    }
    texIds.clear();
    
    
    if (hiddenWindow) {
        glfwDestroyWindow(hiddenWindow);
    }
}

void keyCallback(int key, int, int action, int) {
    if (!gEngine->isMaster() || action != action::Press) {
        return;
    }
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
        case key::F:
            wireframe.setVal(!wireframe.getVal());
            break;
        case key::Key1:
            incrIndex.setVal(1);
            break;
        case key::Key2:
            incrIndex.setVal(2);
            break;
        case key::Left:
            if (numSyncedTex.getVal() > 0) {
                if (texIndex.getVal() > incrIndex.getVal() - 1) {
                    texIndex.setVal(texIndex.getVal() - incrIndex.getVal());
                }
                else {
                    texIndex.setVal(numSyncedTex.getVal() - 1);
                }
            }
            break;
        case key::Right:
            if (numSyncedTex.getVal() > 0) {
                texIndex.setVal(
                    (texIndex.getVal() + incrIndex.getVal()) % numSyncedTex.getVal()
                );
            }
            break;
    }
}

void contextCreationCallback(GLFWwindow* win) {
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    
    sharedWindow = win;
    hiddenWindow = glfwCreateWindow(1, 1, "Thread Window", nullptr, sharedWindow);
     
    if (!hiddenWindow) {
        MessageHandler::instance()->print("Failed to create loader context!\n");
    }
    
    // restore to normal
    glfwMakeContextCurrent(sharedWindow);
    
    if (gEngine->isMaster()) {
        loadThread = std::make_unique<std::thread>(threadWorker);
    }
}

void dataTransferDecoder(void* receivedData, int receivedLength, int packageId,
                         int clientIndex)
{
    MessageHandler::instance()->print(
        "Decoding %d bytes in transfer id: %d on node %d\n",
        receivedLength, packageId, clientIndex
    );

    lastPackage.setVal(packageId);
    
    // read the image on slave
    readImage(reinterpret_cast<unsigned char*>(receivedData), receivedLength);
    uploadTexture();
}

void dataTransferStatus(bool connected, int clientIndex) {
    MessageHandler::instance()->print(
        "Transfer node %d is %s.\n", clientIndex, connected ? "connected" : "disconnected"
    );
}

void dataTransferAcknowledge(int packageId, int clientIndex) {
    MessageHandler::instance()->print(
        "Transfer id: %d is completed on node %d.\n", packageId, clientIndex
    );
    
    static int counter = 0;
    if (packageId == lastPackage.getVal()) {
        counter++;
        if (counter == (sgct::core::ClusterManager::instance()->getNumberOfNodes() - 1)) {
            clientsUploadDone = true;
            counter = 0;
            
            MessageHandler::instance()->print(
                "Time to distribute and upload textures on cluster: %f ms\n",
                (sgct::Engine::getTime() - sendTimer) * 1000.0
            );
        }
    }
}

void dropCallback(int count, const char** paths) {
    if (gEngine->isMaster()) {
        std::vector<std::string> pathStrings;
        for (int i = 0; i < count; i++) {
            // simply pick the first path to transmit
            std::string tmpStr(paths[i]);

            // transform to lowercase
            std::transform(
                tmpStr.begin(),
                tmpStr.end(),
                tmpStr.begin(),
                [](char c) { return static_cast<char>(tolower(c)); }
            );

            pathStrings.push_back(tmpStr);
        }

        // sort in alphabetical order
        std::sort(pathStrings.begin(), pathStrings.end());

        serverUploadCount.setVal(0);

        // iterate all drop paths
        for (size_t i = 0; i < pathStrings.size(); i++) {
            std::string tmpStr = pathStrings[i];

            const size_t foundJpg = tmpStr.find(".jpg");
            const size_t foundJpeg = tmpStr.find(".jpeg");
            if (foundJpg != std::string::npos || foundJpeg != std::string::npos) {
                imagePaths.addVal(std::pair<std::string, int>(
                    pathStrings[i],
                    static_cast<int>(ImageType::JPEG)
                ));
                transfer.setVal(true); // tell transfer thread to start processing data
                serverUploadCount.setVal(serverUploadCount.getVal() + 1);
            }

            const size_t foundPng = tmpStr.find(".png");
            if (foundPng != std::string::npos) {
                imagePaths.addVal(std::pair<std::string, int>(
                    pathStrings[i],
                    static_cast<int>(ImageType::PNG)
                ));
                transfer.setVal(true); // tell transfer thread to start processing data
                serverUploadCount.setVal(serverUploadCount.getVal() + 1);
            }
        }
    }
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
    gEngine->setContextCreationCallback(contextCreationCallback);
    gEngine->setDropCallbackFunction(dropCallback);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile)) {
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
