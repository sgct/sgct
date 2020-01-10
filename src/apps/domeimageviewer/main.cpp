/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>

#include <sgct/utils/dome.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <fstream>

namespace {
    std::unique_ptr<std::thread> loadThread;
    std::mutex imageMutex;
    GLFWwindow* hiddenWindow;
    GLFWwindow* sharedWindow;
    std::vector<std::unique_ptr<sgct::Image>> transImages;

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
    std::unique_lock lk(imageMutex);

    std::unique_ptr<sgct::Image> img = std::make_unique<sgct::Image>();
    try {
        img->load(data, len);
        transImages.push_back(std::move(img));
    }
    catch (const std::runtime_error& e) {
        sgct::Log::Error("%s", e.what());
    }
}

void startDataTransfer() {
    int id = lastPackage.value();
    id++;

    // make sure to keep within bounds
    if (static_cast<int>(imagePaths.size()) <= id) {
        return;
    }
    sendTimer = sgct::Engine::getTime();

    const int imageCounter = static_cast<int32_t>(imagePaths.size());
    lastPackage.setValue(imageCounter - 1);

    for (int i = id; i < imageCounter; i++) {
        // load from file
        const std::string& p = imagePaths.valueAt(static_cast<size_t>(i));

        std::ifstream file(p.c_str(), std::ios::binary);
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size)) {
            const int s = static_cast<int>(buffer.size());
            NetworkManager::instance().transferData(buffer.data(), s, i);
            readImage(reinterpret_cast<unsigned char*>(buffer.data()), s);
        }
    }
}

void uploadTexture() {
    std::unique_lock lk(imageMutex);

    if (transImages.empty()) {
        return;
    }
    glfwMakeContextCurrent(hiddenWindow);

    for (size_t i = 0; i < transImages.size(); i++) {
        if (!transImages[i]) {
            // if invalid load
            texIds.addValue(0);
            continue;
        }

        // create texture
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const size_t bpc = transImages[i]->bytesPerChannel();

        GLenum internalformat;
        GLenum type;
        switch (transImages[i]->channels()) {
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

        const GLsizei width = static_cast<GLsizei>(transImages[i]->size().x);
        const GLsizei height = static_cast<GLsizei>(transImages[i]->size().y);
        unsigned char* data = transImages[i]->data();
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

        sgct::Log::Info(
            "Texture id %d loaded (%dx%dx%d)",
            tex, transImages[i]->size().x, transImages[i]->size().y,
            transImages[i]->channels()
        );

        texIds.addValue(tex);
        transImages[i] = nullptr;
    }

    transImages.clear();
    glFinish();

    glfwMakeContextCurrent(nullptr);
}

void threadWorker() {
    while (isRunning) {
        const bool trans = transfer.value();
        const bool serverDone = serverUploadDone.value();
        const bool clientDone = clientsUploadDone.value();
        // runs only on master
        if (trans && !serverDone && !clientDone) {
            startDataTransfer();
            transfer.setValue(false);

            // load textures on master
            uploadTexture();
            serverUploadDone = true;

            if (ClusterManager::instance().numberOfNodes() == 1) {
                clientsUploadDone = true;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


void drawFun(RenderData data) {
    if (texIndex.value() == -1) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    const glm::mat4 mvp = data.modelViewProjectionMatrix;

    glActiveTexture(GL_TEXTURE0);

    if ((texIds.size() > (texIndex.value() + 1)) &&
        data.frustumMode == sgct::Frustum::Mode::StereoRightEye)
    {
        glBindTexture(GL_TEXTURE_2D, texIds.valueAt(texIndex.value() + 1));
    }
    else {
        glBindTexture(GL_TEXTURE_2D, texIds.valueAt(texIndex.value()));
    }

    ShaderManager::instance().shaderProgram("xform").bind();
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    dome->draw();
    ShaderManager::instance().shaderProgram("xform").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (Engine::instance().isMaster()) {
        currentTime.setValue(Engine::getTime());

        // if texture is uploaded then iterate the index
        if (serverUploadDone.value() && clientsUploadDone.value()) {
            numSyncedTex = static_cast<int32_t>(texIds.size());
            
            // only iterate up to the first new image, even if multiple images was added
            texIndex = numSyncedTex.value() - serverUploadCount.value();

            serverUploadDone = false;
            clientsUploadDone = false;
        }
    }
}

void postSyncPreDrawFun() {
    Engine::instance().setStatsGraphVisibility(stats.value());
}

void initOGLFun() {
    dome = std::make_unique<utils::Dome>(7.4f, 180.f, 256, 128);

    // Set up backface culling
    glCullFace(GL_BACK);
    // our polygon winding is clockwise since we are inside of the dome
    glFrontFace(GL_CW);

    ShaderManager::instance().addShaderProgram("xform", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();
    matrixLoc = glGetUniformLocation(prog.id(), "mvp");
    glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);
    prog.unbind();
}

void encodeFun() {
    SharedData::instance().writeDouble(currentTime);
    SharedData::instance().writeBool(stats);
    SharedData::instance().writeInt32(texIndex);
    SharedData::instance().writeInt32(incrIndex);
}

void decodeFun() {
    SharedData::instance().readDouble(currentTime);
    SharedData::instance().readBool(stats);
    SharedData::instance().readInt32(texIndex);
    SharedData::instance().readInt32(incrIndex);
}

void cleanUpFun() {
    dome = nullptr;

    for (size_t i = 0; i < texIds.size(); i++) {
        GLuint tex = texIds.valueAt(i);
        if (tex) {
            glDeleteTextures(1, &tex);
            texIds.setValueAt(i, 0);
        }
    }
    texIds.clear();

    if (hiddenWindow) {
        glfwDestroyWindow(hiddenWindow);
    }
}

void keyCallback(Key key, Modifier, Action action, int) {
    if (!Engine::instance().isMaster() || action != Action::Press) {
        return;
    }
    switch (key) {
        case Key::Esc:
            Engine::instance().terminate();
            break;
        case Key::S:
            stats.setValue(!stats.value());
            break;
        case Key::Key1:
            incrIndex.setValue(1);
            break;
        case Key::Key2:
            incrIndex.setValue(2);
            break;
        case Key::Left:
            if (numSyncedTex.value() > 0) {
                if (texIndex.value() > incrIndex.value() - 1) {
                    texIndex.setValue(texIndex.value() - incrIndex.value());
                }
                else {
                    texIndex.setValue(numSyncedTex.value() - 1);
                }
            }
            break;
        case Key::Right:
            if (numSyncedTex.value() > 0) {
                texIndex.setValue(
                    (texIndex.value() + incrIndex.value()) % numSyncedTex.value()
                );
            }
            break;
        default:
            break;
    }
}

void contextCreationCallback(GLFWwindow* win) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    
    sharedWindow = win;
    hiddenWindow = glfwCreateWindow(1, 1, "Thread Window", nullptr, sharedWindow);
     
    if (!hiddenWindow) {
        Log::Info("Failed to create loader context");
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
    Log::Info(
        "Decoding %d bytes in transfer id: %d on node %d",
        receivedLength, packageId, clientIndex
    );

    lastPackage.setValue(packageId);
    
    // read the image on slave
    readImage(reinterpret_cast<unsigned char*>(receivedData), receivedLength);
    uploadTexture();
}

void dataTransferStatus(bool connected, int clientIndex) {
    Log::Info(
        "Transfer node %d is %s.", clientIndex, connected ? "connected" : "disconnected"
    );
}

void dataTransferAcknowledge(int packageId, int clientIndex) {
    Log::Info(
        "Transfer id: %d is completed on node %d.", packageId, clientIndex
    );
    
    static int counter = 0;
    if (packageId == lastPackage.value()) {
        counter++;
        if (counter == (sgct::ClusterManager::instance().numberOfNodes() - 1)) {
            clientsUploadDone = true;
            counter = 0;
            
            Log::Info(
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

        serverUploadCount.setValue(0);

        // iterate all drop paths
        for (size_t i = 0; i < pathStrings.size(); i++) {
            std::string tmpStr = pathStrings[i];

            const size_t foundJpg = tmpStr.find(".jpg");
            const size_t foundJpeg = tmpStr.find(".jpeg");
            if (foundJpg != std::string::npos || foundJpeg != std::string::npos) {
                imagePaths.addValue(pathStrings[i]);
                transfer.setValue(true); // tell transfer thread to start processing data
                serverUploadCount.setValue(serverUploadCount.value() + 1);
            }

            const size_t foundPng = tmpStr.find(".png");
            if (foundPng != std::string::npos) {
                imagePaths.addValue(pathStrings[i]);
                transfer.setValue(true); // tell transfer thread to start processing data
                serverUploadCount.setValue(serverUploadCount.value() + 1);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGLFun;
    callbacks.draw = drawFun;
    callbacks.preSync = preSyncFun;
    callbacks.postSyncPreDraw = postSyncPreDrawFun;
    callbacks.cleanUp = cleanUpFun;
    callbacks.keyboard = keyCallback;
    callbacks.contextCreation = contextCreationCallback;
    callbacks.drop = dropCallback;
    callbacks.dataTransferDecode = dataTransferDecoder;
    callbacks.dataTransferStatus = dataTransferStatus;
    callbacks.dataTransferAcknowledge = dataTransferAcknowledge;
    callbacks.encode = encodeFun;
    callbacks.decode = decodeFun;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error& e) {
        Log::Error("%s", e.what());
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
