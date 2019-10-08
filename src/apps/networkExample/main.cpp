#include <sgct.h>
#include <sgct/networkmanager.h>
#include <sgct/window.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string_view>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<std::thread> connectionThread;
    std::atomic_bool connected;
    std::atomic_bool running;

    std::unique_ptr<sgct::utils::Box> box;
    GLint matrixLoc = -1;

    sgct::SharedDouble currentTime(0.0);

    int port;
    std::string address;
    bool isServer = false;
    std::unique_ptr<sgct::core::Network> networkPtr;

    std::pair<double, int> timerData;

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

  void main() { color = texture(tex, uv); }
)";
} // namespace

void connect();

using namespace sgct;

void parseArguments(int& argc, char**& argv) {
    for (int i = 0; i < argc; i++) {
        std::string_view arg(argv[i]);
        if (arg == "-port" && argc > (i + 1)) {
            port = std::stoi(argv[i + 1]);
            MessageHandler::instance()->printInfo("Setting port to: %d", port);
        }
        else if (arg == "-address" && argc > (i + 1)) {
            address = argv[i + 1];
            MessageHandler::instance()->printInfo(
                "Setting address to: %s", address.c_str()
            );
        }
        else if (arg == "--server") {
            isServer = true;
            MessageHandler::instance()->printInfo(
                "This computer will host the connection"
            );
        }
    }
}

void networkConnectionUpdated(sgct::core::Network* conn) {
    if (conn->isServer()) {
        // wake up the connection handler thread on server if node disconnects to enable
        // reconnection
        conn->_startConnectionCond.notify_all();
    }

    connected = conn->isConnected();

    MessageHandler::instance()->printInfo(
        "Network is %s", conn->isConnected() ? "connected" : "disconneced"
    );
}

void networkAck(int packageId, int) {
    MessageHandler::instance()->printInfo(
        "Network package %d is received", packageId
    );

    if (timerData.second == packageId) {
        MessageHandler::instance()->printInfo(
            "Loop time: %lf ms", (sgct::Engine::getTime() - timerData.first) * 1000.0
        );
    }
}

void networkDecode(void* receivedData, int receivedLength, int packageId, int) {
    MessageHandler::instance()->printInfo("Network decoding package %d", packageId);
    std::string test(reinterpret_cast<char*>(receivedData), receivedLength);
    MessageHandler::instance()->printInfo("Message: \"%s\"", test.c_str());
}

void networkLoop() {
    connect();

    // if client try to connect to server even after disconnection
    if (!isServer) {
        while (running.load()) {
            if (connected.load() == false) {
                connect();
            }
            else {
                // just check if connected once per second
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
}

void connect() {
    if (!gEngine->isMaster()) {
        return;
    }

    // no need to specify the address on the host/server
    if (!isServer && address.empty()) {
        MessageHandler::instance()->printError("Network error: No address set");
        return;
    }

    networkPtr = std::make_unique<sgct::core::Network>();

    // init
    try {
        MessageHandler::instance()->printDebug(
            "Initiating network connection at port %d", port
        );

        networkPtr->setUpdateFunction(networkConnectionUpdated);
        networkPtr->setPackageDecodeFunction(networkDecode);
        networkPtr->setAcknowledgeFunction(networkAck);

        // must be initialized after binding
        networkPtr->init(
            port,
            address,
            isServer,
            sgct::core::Network::ConnectionType::DataTransfer
        );
    }
    catch (const std::runtime_error& err) {
        MessageHandler::instance()->printError("Network error: %s", err.what());
        networkPtr->initShutdown();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        networkPtr->closeNetwork(true);
        return;
    }

    connected = true;
}

void disconnect() {
    if (networkPtr) {
        networkPtr->initShutdown();

        // wait for all nodes callbacks to run
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        // wait for threads to die
        networkPtr->closeNetwork(false);
        networkPtr = nullptr;
    }
}

void sendData(const void* data, int length, int packageId) {
    if (networkPtr) {
        sgct::core::NetworkManager::instance()->transferData(
            data,
            length,
            packageId,
            networkPtr.get()
        );
        timerData.first = Engine::getTime();
        timerData.second = packageId;
    }
}

void sendTestMessage() {
    std::string test = "What's up?";
    static int counter = 0;
    sendData(test.data(), static_cast<int>(test.size()), counter);
    counter++;
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
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * (Speed / 2.0)),
        glm::vec3(1.f, 0.f, 0.f)
    );

    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix() * scene;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));

    ShaderManager::instance()->bindShaderProgram("xform");

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    box->draw();

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "box.png", true);

    box = std::make_unique<sgct::utils::Box>(
        2.f,
        sgct::utils::Box::TextureMappingMode::Regular
    );

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    sgct::ShaderManager::instance()->addShaderProgram(
        "xform",
        vertexShader,
        fragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    sgct::ShaderManager::instance()->bindShaderProgram("xform");

    const ShaderProgram& prg = sgct::ShaderManager::instance()->getShaderProgram("xform");
    matrixLoc = prg.getUniformLocation("mvp");
    GLint TexLoc = prg.getUniformLocation("tex");
    glUniform1i(TexLoc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();

    for (int i = 0; i < gEngine->getNumberOfWindows(); i++) {
        gEngine->getWindow(i).setWindowTitle(isServer ? "SERVER" : "CLIENT");
    }
}

void encodeFun() {
    sgct::SharedData::instance()->writeDouble(currentTime);
}

void decodeFun() {
    sgct::SharedData::instance()->readDouble(currentTime);
}

void cleanUpFun() {
    box = nullptr;
    running = false;

    if (connectionThread) {
        if (networkPtr) {
            networkPtr->initShutdown();
        }
        connectionThread->join();
        connectionThread = nullptr;
    }

    disconnect();
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster() && key == key::Space && action == action::Press) {
        sendTestMessage();
    }
}

int main(int argc, char* argv[]) {
    connected = false;
    running = true;

    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    parseArguments(argc, argv);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    connectionThread = std::make_unique<std::thread>(networkLoop);

    sgct::SharedData::instance()->setEncodeFunction(encodeFun);
    sgct::SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
