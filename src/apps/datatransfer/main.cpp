#include <sgct/actions.h>
#include <sgct/clustermanager.h>
#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/keys.h>
#include <sgct/image.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#include <sgct/utils/box.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <memory>

namespace {
    std::unique_ptr<std::thread> loadThread;
    std::mutex imageMutex;
    GLFWwindow* hiddenWindow;
    GLFWwindow* sharedWindow;
    std::unique_ptr<sgct::Image> transImg;
    unsigned int textureId = 0;

    sgct::SharedBool stats(false);
    sgct::SharedInt32 texIndex(-1);

    sgct::SharedInt32 currentPackage(-1);
    sgct::SharedBool transfer(false);
    sgct::SharedBool serverUploadDone(false);
    sgct::SharedBool clientsUploadDone(false);
    sgct::SharedVector<std::string> imagePaths;
    sgct::SharedVector<GLuint> texIds;
    double sendTimer = 0.0;

    bool isRunning = true;

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


void drawFun(RenderData data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr const double Speed = 0.44;

    // create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.value() * Speed),
        glm::vec3(0.f, -1.f, 0.f)
    );
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.value() * (Speed / 2.0)),
        glm::vec3(1.f, 0.f, 0.f)
    );

    const glm::mat4 mvp = data.modelViewProjectionMatrix * scene;

    glActiveTexture(GL_TEXTURE0);
    
    if (texIndex.value() != -1) {
        glBindTexture(GL_TEXTURE_2D, texIds.valueAt(texIndex.value()));
    }
    else {
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    ShaderManager::instance().shaderProgram("xform").bind();
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    box->draw();
    ShaderManager::instance().shaderProgram("xform").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (Engine::instance().isMaster()) {
        currentTime.setValue(Engine::getTime());
        
        // if texture is uploaded then iterate the index
        if (serverUploadDone.value() && clientsUploadDone.value()) {
            texIndex.setValue(texIndex.value() + 1);
            serverUploadDone = false;
            clientsUploadDone = false;
        }
    }
}

void postSyncPreDrawFun() {
    Engine::instance().setStatsGraphVisibility(stats.value());
}

void initOGLFun() {
    textureId = TextureManager::instance().loadTexture("box.png", true, 8.f);
    box = std::make_unique<utils::Box>(2.f, utils::Box::TextureMappingMode::Regular);

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // our polygon winding is counter clockwise

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
}

void decodeFun() {
    SharedData::instance().readDouble(currentTime);
    SharedData::instance().readBool(stats);
    SharedData::instance().readInt32(texIndex);
}

void cleanUpFun() {
    box = nullptr;
    
    for (size_t i = 0; i < texIds.size(); i++) {
        GLuint tex = texIds.valueAt(i);
        if (tex) {
            glDeleteTextures(1, &tex);
        }
    }
    texIds.clear();

    if (hiddenWindow) {
        glfwDestroyWindow(hiddenWindow);
    }
}

void keyCallback(Key key, Modifier, Action action, int) {
    if (Engine::instance().isMaster() && (action == Action::Press)) {
        switch (key) {
            case Key::Esc:
                Engine::instance().terminate();
                break;
            case Key::S:
                stats.setValue(!stats.value());
                break;
            default:
                break;
        }
    }
}

void readImage(unsigned char* data, int len) {
    std::unique_lock lk(imageMutex);

    transImg = std::make_unique<sgct::Image>();

    try {
        transImg->load(reinterpret_cast<unsigned char*>(data), len);
    }
    catch (const std::runtime_error& e) {
        Logger::Error("%s", e.what());
        transImg = nullptr;
    }
}

void startDataTransfer() {
    // iterate
    int id = currentPackage.value();
    id++;
    currentPackage.setValue(id);

    // make sure to keep within bounds
    if (static_cast<int>(imagePaths.size()) <= id) {
        return;
    }

    sendTimer = sgct::Engine::getTime();

    // load from file
    const std::string& tmpPair = imagePaths.valueAt(static_cast<size_t>(id));

    std::ifstream file(tmpPair.c_str(), std::ios::binary);
    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size)) {
        const int s = static_cast<int>(size);
        Engine::instance().transferDataBetweenNodes(buffer.data(), s, id);

        // read the image on master
        readImage(reinterpret_cast<unsigned char*>(buffer.data()), s);
    }
}

void uploadTexture() {
    std::unique_lock lk(imageMutex);
    
    if (!transImg) {
        // if invalid load
        texIds.addValue(0);
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

    size_t bpc = transImg->bytesPerChannel();

    switch (transImg->channels()) {
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
        static_cast<GLsizei>(transImg->size().x),
        static_cast<GLsizei>(transImg->size().y)
    );
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        static_cast<GLsizei>(transImg->size().x),
        static_cast<GLsizei>(transImg->size().y),
        type,
        format,
        transImg->data()
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapLevels - 1);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    Logger::Info(
        "Texture id %d loaded (%dx%dx%d).",
        tex, transImg->size().x, transImg->size().y, transImg->channels()
    );

    texIds.addValue(tex);
    transImg = nullptr;

    glFinish();
    glfwMakeContextCurrent(nullptr);
}

void threadWorker() {
    while (isRunning) {
        // runs only on master
        if (transfer.value() && !serverUploadDone.value() &&
            !clientsUploadDone.value())
        {
            startDataTransfer();
            transfer.setValue(false);

            // load texture on master
            uploadTexture();
            serverUploadDone = true;

            if (sgct::ClusterManager::instance().numberOfNodes() == 1) {
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
        Logger::Info("Failed to create loader context");
    }

    glfwMakeContextCurrent(sharedWindow);

    if (Engine::instance().isMaster()) {
        loadThread = std::make_unique<std::thread>(threadWorker);
    }
}

void dataTransferDecoder(void* data, int length, int packageId, int clientIndex) {
    Logger::Info(
        "Decoding %d bytes in transfer id: %d on node %d", length, packageId, clientIndex
    );

    currentPackage.setValue(packageId);

    // read the image on master
    readImage(reinterpret_cast<unsigned char*>(data), length);
    uploadTexture();
}

void dataTransferStatus(bool connected, int clientIndex) {
    Logger::Info(
        "Transfer node %d is %s.", clientIndex, connected ? "connected" : "disconnected"
    );
}

void dataTransferAcknowledge(int packageId, int clientIndex) {
    Logger::Info(
        "Transfer id: %d is completed on node %d.", packageId, clientIndex
    );
    
    static int counter = 0;
    if (packageId == currentPackage.value()) {
        counter++;
        if (counter == (sgct::ClusterManager::instance().numberOfNodes() - 1)) {
            clientsUploadDone = true;
            counter = 0;

            Logger::Info(
                "Time to distribute and upload textures on cluster: %f ms", 
                (sgct::Engine::getTime() - sendTimer) * 1000.0
            );
        }
    }
}

void dropCallback(int, const char** paths) {
    if (!Engine::instance().isMaster()) {
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
        imagePaths.addValue(paths[0]);
        transfer.setValue(true);
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
        Logger::Error("%s", e.what());
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
