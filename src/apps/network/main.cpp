#include <sgct/actions.h>
#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/keys.h>
#include <sgct/networkmanager.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#include <sgct/window.h>
#include <sgct/utils/box.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string_view>

namespace {
    std::unique_ptr<std::thread> connectionThread;
    std::atomic_bool connected;
    std::atomic_bool running;

    unsigned int textureId = 0;

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

void networkConnectionUpdated(sgct::core::Network* conn) {
    if (conn->isServer()) {
        // wake up the connection handler thread on server if node disconnects to enable
        // reconnection
        conn->startConnectionConditionVar().notify_all();
    }

    connected = conn->isConnected();

    Logger::Info(
        "Network is %s", conn->isConnected() ? "connected" : "disconneced"
    );
}

void networkAck(int packageId, int) {
    Logger::Info("Network package %d is received", packageId);

    if (timerData.second == packageId) {
        Logger::Info(
            "Loop time: %lf ms", (sgct::Engine::getTime() - timerData.first) * 1000.0
        );
    }
}

void networkDecode(void* receivedData, int receivedLength, int packageId, int) {
    Logger::Info("Network decoding package %d", packageId);
    std::string test(reinterpret_cast<char*>(receivedData), receivedLength);
    Logger::Info("Message: \"%s\"", test.c_str());
}

void connect() {
    if (!Engine::instance().isMaster()) {
        return;
    }

    // no need to specify the address on the host/server
    if (!isServer && address.empty()) {
        Logger::Error("Network error: No address set");
        return;
    }

    networkPtr = std::make_unique<sgct::core::Network>(
        port,
        address,
        isServer,
        sgct::core::Network::ConnectionType::DataTransfer
    );

    // init
    try {
        Logger::Debug("Initiating network connection at port %d", port);

        networkPtr->setUpdateFunction(networkConnectionUpdated);
        networkPtr->setPackageDecodeFunction(networkDecode);
        networkPtr->setAcknowledgeFunction(networkAck);
        networkPtr->init();
    }
    catch (const std::runtime_error& err) {
        Logger::Error("Network error: %s", err.what());
        networkPtr->initShutdown();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        networkPtr->closeNetwork(true);
        return;
    }

    connected = true;
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

void sendData(const void* data, int length, int id) {
    if (networkPtr) {
        core::NetworkManager::instance().transferData(data, length, id, *networkPtr);
        timerData.first = Engine::getTime();
        timerData.second = id;
    }
}

void sendTestMessage() {
    std::string test = "What's up?";
    static int counter = 0;
    sendData(test.data(), static_cast<int>(test.size()), counter);
    counter++;
}

void drawFun(RenderData data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr const double Speed = 0.44;

    //create scene transform (animation)
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
    glBindTexture(GL_TEXTURE_2D, textureId);

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
    }
}

void initOGLFun() {
    textureId = TextureManager::instance().loadTexture("box.png", true, 8.f);
    box = std::make_unique<utils::Box>(2.f, utils::Box::TextureMappingMode::Regular);

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    sgct::ShaderManager::instance().addShaderProgram(
        "xform",
        vertexShader,
        fragmentShader
    );
    const ShaderProgram& prg = sgct::ShaderManager::instance().shaderProgram("xform");
    prg.bind();
    matrixLoc = glGetUniformLocation(prg.id(), "mvp");
    glUniform1i(glGetUniformLocation(prg.id(), "tex"), 0 );
    prg.unbind();

    for (const std::unique_ptr<Window>& win : Engine::instance().windows()) {
        win->setWindowTitle(isServer ? "SERVER" : "CLIENT");
    }
}

void encodeFun() {
    sgct::SharedData::instance().writeDouble(currentTime);
}

void decodeFun() {
    sgct::SharedData::instance().readDouble(currentTime);
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

void keyCallback(Key key, Modifier, Action action, int) {
    if (Engine::instance().isMaster() && action == Action::Press) {
        if (key == Key::Esc) {
            Engine::instance().terminate();
        }
        else if (key == Key::Space) {
            sendTestMessage();
        }
    }
}

int main(int argc, char* argv[]) {
    connected = false;
    running = true;

    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    for (int i = 0; i < argc; i++) {
        std::string_view v(argv[i]);
        if (v == "-port" && argc > (i + 1)) {
            port = std::stoi(argv[i + 1]);
            Logger::Info("Setting port to: %d", port);
        }
        else if (v == "-address" && argc > (i + 1)) {
            address = argv[i + 1];
            Logger::Info("Setting address to: %s", address.c_str());
        }
        else if (v == "--server") {
            isServer = true;
            Logger::Info("This computer will host the connection");
        }
    }

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGLFun;
    callbacks.draw = drawFun;
    callbacks.preSync = preSyncFun;
    callbacks.cleanUp = cleanUpFun;
    callbacks.keyboard = keyCallback;
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

    connectionThread = std::make_unique<std::thread>(networkLoop);

    Engine::instance().render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
