/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
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

    bool stats = false;
    int32_t texIndex = -1;
    int32_t incrIndex = 1;
    int32_t numSyncedTex = 0;

    int32_t lastPackage = -1;
    bool transfer = false;
    bool serverUploadDone = false;
    int32_t serverUploadCount(0);
    bool clientsUploadDone = false;
    std::vector<std::string> imagePaths;
    std::vector<GLuint> texIds;
    double sendTimer = 0.0;

    bool isRunning = true;

    std::unique_ptr<sgct::utils::Dome> dome;
    GLint matrixLoc = -1;

    double currentTime(0.0);

    constexpr std::string_view vertexShader = R"(
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

    constexpr std::string_view fragmentShader = R"(
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

    std::unique_ptr<Image> img = std::make_unique<Image>();
    try {
        img->load(data, len);
        transImages.push_back(std::move(img));
    }
    catch (const std::runtime_error& e) {
        Log::Error(e.what());
    }
}

void startDataTransfer() {
    int id = lastPackage;
    id++;

    // make sure to keep within bounds
    if (static_cast<int>(imagePaths.size()) <= id) {
        return;
    }
    sendTimer = time();

    const int imageCounter = static_cast<int32_t>(imagePaths.size());
    lastPackage = imageCounter - 1;

    for (int i = id; i < imageCounter; i++) {
        // load from file
        const std::string& p = imagePaths[static_cast<size_t>(i)];

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
            texIds.push_back(0);
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

        const GLsizei width = transImages[i]->size().x;
        const GLsizei height = transImages[i]->size().y;
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

        Log::Info(fmt::format(
            "Texture id %d loaded ({}x{}x{})",
            tex, transImages[i]->size().x, transImages[i]->size().y,
            transImages[i]->channels()
        ));

        texIds.push_back(tex);
        transImages[i] = nullptr;
    }

    transImages.clear();
    glFinish();

    glfwMakeContextCurrent(nullptr);
}

void threadWorker() {
    while (isRunning) {
        const bool trans = transfer;
        const bool serverDone = serverUploadDone;
        const bool clientDone = clientsUploadDone;
        // runs only on master
        if (trans && !serverDone && !clientDone) {
            startDataTransfer();
            transfer = false;

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


void draw(const RenderData& data) {
    if (texIndex == -1) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    const mat4& mvp = data.modelViewProjectionMatrix;

    glActiveTexture(GL_TEXTURE0);

    if ((static_cast<int>(texIds.size()) > (texIndex + 1)) &&
        data.frustumMode == Frustum::Mode::StereoRightEye)
    {
        glBindTexture(GL_TEXTURE_2D, texIds[texIndex + 1]);
    }
    else {
        glBindTexture(GL_TEXTURE_2D, texIds[texIndex]);
    }

    ShaderManager::instance().shaderProgram("xform").bind();
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, mvp.values);
    dome->draw();
    ShaderManager::instance().shaderProgram("xform").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSync() {
    if (Engine::instance().isMaster()) {
        currentTime = time();

        // if texture is uploaded then iterate the index
        if (serverUploadDone && clientsUploadDone) {
            numSyncedTex = static_cast<int32_t>(texIds.size());

            // only iterate up to the first new image, even if multiple images was added
            texIndex = numSyncedTex - serverUploadCount;

            serverUploadDone = false;
            clientsUploadDone = false;
        }
    }
}

void postSyncPreDraw() {
    Engine::instance().setStatsGraphVisibility(stats);
}

void initOGL(GLFWwindow* win) {
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

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, currentTime);
    serializeObject(data, stats);
    serializeObject(data, texIndex);
    serializeObject(data, incrIndex);
    return data;
}

void decode(const std::vector<std::byte>& data) {
    unsigned int pos = 0;
    deserializeObject(data, pos, currentTime);
    deserializeObject(data, pos, stats);
    deserializeObject(data, pos, texIndex);
    deserializeObject(data, pos, incrIndex);
}

void cleanup() {
    dome = nullptr;

    for (size_t i = 0; i < texIds.size(); i++) {
        GLuint tex = texIds[i];
        if (tex) {
            glDeleteTextures(1, &tex);
            texIds[i] = 0;
        }
    }
    texIds.clear();

    if (hiddenWindow) {
        glfwDestroyWindow(hiddenWindow);
    }
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (!Engine::instance().isMaster() || action != Action::Press) {
        return;
    }
    switch (key) {
        case Key::Esc:
            Engine::instance().terminate();
            break;
        case Key::S:
            stats = !stats;
            break;
        case Key::Key1:
            incrIndex = 1;
            break;
        case Key::Key2:
            incrIndex = 2;
            break;
        case Key::Left:
            if (numSyncedTex > 0) {
                if (texIndex > incrIndex - 1) {
                    texIndex = texIndex - incrIndex;
                }
                else {
                    texIndex = numSyncedTex - 1;
                }
            }
            break;
        case Key::Right:
            if (numSyncedTex > 0) {
                texIndex = (texIndex + incrIndex) % numSyncedTex;
            }
            break;
        default:
            break;
    }
}

void dataTransferDecoder(void* receivedData, int receivedLength, int packageId,
                         int clientIndex)
{
    Log::Info(fmt::format(
        "Decoding {} bytes in transfer id: {} on node {}",
        receivedLength, packageId, clientIndex
    ));

    lastPackage = packageId;

    // read the image on slave
    readImage(reinterpret_cast<unsigned char*>(receivedData), receivedLength);
    uploadTexture();
}

void dataTransferStatus(bool connected, int clientIndex) {
    Log::Info(fmt::format(
        "Transfer node {} is {}", clientIndex, connected ? "connected" : "disconnected"
    ));
}

void dataTransferAcknowledge(int packageId, int clientIndex) {
    Log::Info(fmt::format(
        "Transfer id: {} is completed on node {}", packageId, clientIndex
    ));

    if (packageId == lastPackage) {
        static int counter = 0;
        counter++;
        if (counter == (ClusterManager::instance().numberOfNodes() - 1)) {
            clientsUploadDone = true;
            counter = 0;

            Log::Info(fmt::format(
                "Time to distribute and upload textures on cluster: {} ms",
                (time() - sendTimer) * 1000.0
            ));
        }
    }
}

void drop(int count, const char** paths) {
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

        serverUploadCount = 0;

        // iterate all drop paths
        for (size_t i = 0; i < pathStrings.size(); i++) {
            std::string tmpStr = pathStrings[i];

            const size_t foundJpg = tmpStr.find(".jpg");
            const size_t foundJpeg = tmpStr.find(".jpeg");
            if (foundJpg != std::string::npos || foundJpeg != std::string::npos) {
                imagePaths.push_back(pathStrings[i]);
                transfer = true; // tell transfer thread to start processing data
                serverUploadCount++;
            }

            const size_t foundPng = tmpStr.find(".png");
            if (foundPng != std::string::npos) {
                imagePaths.push_back(pathStrings[i]);
                transfer = true; // tell transfer thread to start processing data
                serverUploadCount++;
            }
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

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGL;
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.cleanup = cleanup;
    callbacks.keyboard = keyboard;
    callbacks.drop = drop;
    callbacks.dataTransferDecode = dataTransferDecoder;
    callbacks.dataTransferStatus = dataTransferStatus;
    callbacks.dataTransferAcknowledge = dataTransferAcknowledge;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error& e) {
        Log::Error(e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance().exec();

    isRunning = false;

    if (loadThread) {
        loadThread->join();
        loadThread = nullptr;
    }

    Engine::destroy();
    exit(EXIT_SUCCESS);
}
