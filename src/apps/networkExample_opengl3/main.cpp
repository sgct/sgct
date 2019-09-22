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

} // namespace

void connect();

using namespace sgct;

void parseArguments(int& argc, char**& argv) {
    for (int i = 0; i < argc; i++) {
        std::string_view arg(argv[i]);
        if (arg == "-port" && argc > (i + 1)) {
            port = std::stoi(argv[i + 1]);
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Setting port to: %d\n", port
            );
        }
        else if (arg == "-address" && argc > (i + 1)) {
            address = argv[i + 1];
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Setting address to: %s\n", address.c_str()
            );
        }
        else if (arg == "--server") {
            isServer = true;
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "This computer will host the connection\n"
            );
        }
    }
}

void networkConnectionUpdated(sgct::core::Network* conn) {
    if (conn->isServer()) {
        // wake up the connection handler thread on server if node disconnects to enable
        // reconnection
        conn->mStartConnectionCond.notify_all();
    }

    connected = conn->isConnected();

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Network is %s\n", conn->isConnected() ? "connected" : "disconneced"
    );
}

void networkAck(int packageId, int clientId) {
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Network package %d is received\n", packageId
    );

    if (timerData.second == packageId) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Loop time: %lf ms\n", (sgct::Engine::getTime() - timerData.first) * 1000.0
        );
    }
}

void networkDecode(void* receivedData, int receivedLength, int packageId, int clientId) {
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Network decoding package %d...\n", packageId
    );

    std::string test(reinterpret_cast<char*>(receivedData), receivedLength);

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Message: \"%s\"\n", test.c_str()
    );
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Network error: No address set\n"
        );
        return;
    }

    networkPtr = std::make_unique<sgct::core::Network>();

    // init
    try {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "Initiating network connection at port %d\n", port
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Network error: %s\n", err.what()
        );
        networkPtr->initShutdown();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        networkPtr->closeNetwork(true);
        return;
    }
    catch (const char* err) {
        // @TODO (abock, 2019-09-05);  I think i removed all instances of throwing raw
        // char* from the networking API, but I'm not 100% sure that won't happen, so I'll
        // leave this right here
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Network error: %s\n", err
        );
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

    sgct::ShaderManager::instance()->addShaderProgram("xform", "box.vert", "box.frag");

    sgct::ShaderManager::instance()->bindShaderProgram("xform");

    const ShaderProgram& prg = sgct::ShaderManager::instance()->getShaderProgram("xform");
    matrixLoc = prg.getUniformLocation("mvp");
    GLint TexLoc = prg.getUniformLocation("tex");
    glUniform1i(TexLoc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();

    for (size_t i = 0; i < gEngine->getNumberOfWindows(); i++) {
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
    gEngine = new Engine(arg);

    parseArguments(argc, argv);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile)) {
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
