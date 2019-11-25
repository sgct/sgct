#include <sgct/action.h>
#include <sgct/clustermanager.h>
#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/keys.h>
#include <sgct/image.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/utils/dome.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>

namespace {
    std::unique_ptr<std::thread> loadThread;
    std::mutex mutex;
    GLFWwindow* hiddenWindow;
    GLFWwindow* sharedWindow;
    std::vector<std::unique_ptr<sgct::core::Image>> transImages;

    sgct::SharedBool info(false);
    sgct::SharedBool stats(false);
    sgct::SharedInt32 texIndex(-1);
    sgct::SharedInt32 incrIndex(1);
    sgct::SharedInt32 numSyncedTex(0);

    sgct::SharedInt32 lastPackage(-1);
    sgct::SharedBool transfer(false);
    sgct::SharedBool serverUploadDone(false);
    sgct::SharedInt32 serverUploadCount(0);
    sgct::SharedBool clientsUploadDone(false);
    sgct::SharedVector<std::string> imagePaths;
    sgct::SharedVector<GLuint> texIds;
    double sendTimer = 0.0;

    bool isRunning = true;

    std::unique_ptr<sgct::utils::Dome> dome;
    GLint matrixLoc = -1;

    sgct::SharedDouble currentTime(0.0);

    constexpr const char* vertexShader = R"(
  #version 330 core

  layout(location = 0) in vec2 texCoords;
  layout(location = 1) in vec3 normals;
  layout(location = 2) in vec3 vertPositions;

  uniform mat4 mvp;
  out vec2 uv;

  void main() {
    gl_Position =  mvp * vec4(vertPositions, 1.0);
    uv = texCoords;
  })";

    constexpr const char* fragmentShader = R"(
  #version 330 core

  uniform sampler2D tex;

  in vec2 uv;
  out vec4 color;

  void main() { color = texture(tex, uv); }
)";
} // namespace

using namespace sgct;

void readImage(unsigned char* data, int len) {
    std::unique_lock lk(mutex);

    std::unique_ptr<sgct::core::Image> img = std::make_unique<sgct::core::Image>();
    try {
        img->load(data, len);
        transImages.push_back(std::move(img));
    }
    catch (const std::runtime_error& e) {
        MessageHandler::printError("%s", e.what());
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
        const std::string& p = imagePaths.getValAt(static_cast<size_t>(i));

        std::ifstream file(p.c_str(), std::ios::binary);
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size)) {
            const int s = static_cast<int>(buffer.size());
            Engine::instance().transferDataBetweenNodes(buffer.data(), s, i);
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
            texIds.addVal(0);
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

        const GLsizei width = static_cast<GLsizei>(transImages[i]->getSize().x);
        const GLsizei height = static_cast<GLsizei>(transImages[i]->getSize().y);
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
        glBindTexture(GL_TEXTURE_2D, 0);

        MessageHandler::printInfo(
            "Texture id %d loaded (%dx%dx%d)",
            tex, transImages[i]->getSize().x, transImages[i]->getSize().y,
            transImages[i]->getChannels()
        );

        texIds.addVal(tex);
        transImages[i] = nullptr;
    }

    transImages.clear();
    glFinish();

    glfwMakeContextCurrent(nullptr);
}

void threadWorker() {
    while (isRunning) {
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

            if (core::ClusterManager::instance().getNumberOfNodes() == 1) {
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

    glm::mat4 MVP = Engine::instance().getCurrentModelViewProjectionMatrix();

    glActiveTexture(GL_TEXTURE0);

    if ((texIds.getSize() > (texIndex.getVal() + 1)) &&
        Engine::instance().getCurrentFrustumMode() ==
            sgct::core::Frustum::Mode::StereoRightEye)
    {
        glBindTexture(GL_TEXTURE_2D, texIds.getValAt(texIndex.getVal() + 1));
    }
    else {
        glBindTexture(GL_TEXTURE_2D, texIds.getValAt(texIndex.getVal()));
    }

    ShaderManager::instance().getShaderProgram("xform").bind();
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(MVP));
    dome->draw();
    ShaderManager::instance().getShaderProgram("xform").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (Engine::instance().isMaster()) {
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
    Engine::instance().setDisplayInfoVisibility(info.getVal());
    Engine::instance().setStatsGraphVisibility(stats.getVal());
}

void initOGLFun() {
    dome = std::make_unique<utils::Dome>(7.4f, 180.f, 256, 128);

    // Set up backface culling
    glCullFace(GL_BACK);
    // our polygon winding is clockwise since we are inside of the dome
    glFrontFace(GL_CW);

    ShaderManager::instance().addShaderProgram("xform", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().getShaderProgram("xform");
    prog.bind();
    matrixLoc = glGetUniformLocation(prog.getId(), "mvp");
    glUniform1i(glGetUniformLocation(prog.getId(), "tex"), 0);
    prog.unbind();
}

void encodeFun() {
    SharedData::instance().writeDouble(currentTime);
    SharedData::instance().writeBool(info);
    SharedData::instance().writeBool(stats);
    SharedData::instance().writeInt32(texIndex);
    SharedData::instance().writeInt32(incrIndex);
}

void decodeFun() {
    SharedData::instance().readDouble(currentTime);
    SharedData::instance().readBool(info);
    SharedData::instance().readBool(stats);
    SharedData::instance().readInt32(texIndex);
    SharedData::instance().readInt32(incrIndex);
}

void cleanUpFun() {
    dome = nullptr;

    for (size_t i = 0; i < texIds.getSize(); i++) {
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
    if (!Engine::instance().isMaster() || action != action::Press) {
        return;
    }
    switch (key) {
        case key::Esc:
            Engine::instance().terminate();
            break;
        case key::S:
            stats.setVal(!stats.getVal());
            break;
        case key::I:
            info.setVal(!info.getVal());
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
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    
    sharedWindow = win;
    hiddenWindow = glfwCreateWindow(1, 1, "Thread Window", nullptr, sharedWindow);
     
    if (!hiddenWindow) {
        MessageHandler::printInfo("Failed to create loader context");
    }
    
    // restore to normal
    glfwMakeContextCurrent(sharedWindow);
    
    if (Engine::instance().isMaster()) {
        loadThread = std::make_unique<std::thread>(threadWorker);
    }
}

void dataTransferDecoder(void* receivedData, int receivedLength, int packageId,
                         int clientIndex)
{
    MessageHandler::printInfo(
        "Decoding %d bytes in transfer id: %d on node %d",
        receivedLength, packageId, clientIndex
    );

    lastPackage.setVal(packageId);
    
    // read the image on slave
    readImage(reinterpret_cast<unsigned char*>(receivedData), receivedLength);
    uploadTexture();
}

void dataTransferStatus(bool connected, int clientIndex) {
    MessageHandler::printInfo(
        "Transfer node %d is %s.", clientIndex, connected ? "connected" : "disconnected"
    );
}

void dataTransferAcknowledge(int packageId, int clientIndex) {
    MessageHandler::printInfo(
        "Transfer id: %d is completed on node %d.", packageId, clientIndex
    );
    
    static int counter = 0;
    if (packageId == lastPackage.getVal()) {
        counter++;
        if (counter == (sgct::core::ClusterManager::instance().getNumberOfNodes() - 1)) {
            clientsUploadDone = true;
            counter = 0;
            
            MessageHandler::printInfo(
                "Time to distribute and upload textures on cluster: %f ms",
                (sgct::Engine::getTime() - sendTimer) * 1000.0
            );
        }
    }
}

void dropCallback(int count, const char** paths) {
    if (Engine::instance().isMaster()) {
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
                imagePaths.addVal(pathStrings[i]);
                transfer.setVal(true); // tell transfer thread to start processing data
                serverUploadCount.setVal(serverUploadCount.getVal() + 1);
            }

            const size_t foundPng = tmpStr.find(".png");
            if (foundPng != std::string::npos) {
                imagePaths.addVal(pathStrings[i]);
                transfer.setVal(true); // tell transfer thread to start processing data
                serverUploadCount.setVal(serverUploadCount.getVal() + 1);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);

    Engine::instance().setInitOGLFunction(initOGLFun);
    Engine::instance().setDrawFunction(drawFun);
    Engine::instance().setPreSyncFunction(preSyncFun);
    Engine::instance().setPostSyncPreDrawFunction(postSyncPreDrawFun);
    Engine::instance().setCleanUpFunction(cleanUpFun);
    Engine::instance().setKeyboardCallbackFunction(keyCallback);
    Engine::instance().setContextCreationCallback(contextCreationCallback);
    Engine::instance().setDropCallbackFunction(dropCallback);
    Engine::instance().setDataTransferCallback(dataTransferDecoder);
    Engine::instance().setDataTransferStatusCallback(dataTransferStatus);
    Engine::instance().setDataAcknowledgeCallback(dataTransferAcknowledge);
    Engine::instance().setEncodeFunction(encodeFun);
    Engine::instance().setDecodeFunction(decodeFun);

    try {
        Engine::instance().init(Engine::RunMode::Default_Mode, cluster);
    }
    catch (const std::runtime_error& e) {
        MessageHandler::printError("%s", e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance().render();

    isRunning = false;

    if (loadThread) {
        loadThread->join();
        loadThread = nullptr;
    }

    Engine::destroy();
    exit(EXIT_SUCCESS);
}
