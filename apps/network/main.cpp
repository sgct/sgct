/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <sgct/utils/box.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    std::unique_ptr<std::thread> connectionThread;
    std::atomic_bool connected = false;
    std::atomic_bool running = true;

    unsigned int textureId = 0;

    std::unique_ptr<sgct::utils::Box> box;
    GLint matrixLoc = -1;

    double currentTime(0.0);

    int port;
    std::string address;
    bool isServer = false;
    std::unique_ptr<sgct::Network> networkPtr;

    std::pair<double, int> timerData;

    constexpr std::string_view VertexShader = R"(
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

    constexpr std::string_view FragmentShader = R"(
  #version 330 core

  uniform sampler2D tex;

  in vec2 uv;
  out vec4 color;

  void main() { color = texture(tex, uv); }
)";
} // namespace

using namespace sgct;

void networkConnectionUpdated(Network* conn) {
    if (conn->isServer()) {
        // wake up the connection handler thread on server if node disconnects to enable
        // reconnection
        conn->startConnectionConditionVar().notify_all();
    }

    connected = conn->isConnected();

    Log::Info(fmt::format(
        "Network is {}", conn->isConnected() ? "connected" : "disconneced"
    ));
}

void networkAck(int packageId, int) {
    Log::Info(fmt::format("Network package {} is received", packageId));

    if (timerData.second == packageId) {
        Log::Info(fmt::format(
            "Loop time: {} ms", (time() - timerData.first) * 1000.0
        ));
    }
}

void networkDecode(void* receivedData, int receivedLength, int packageId, int) {
    Log::Info(fmt::format("Network decoding package {}", packageId));
    std::string test(reinterpret_cast<char*>(receivedData), receivedLength);
    Log::Info(fmt::format("Message: \"{}\"", test));
}

void connect() {
    if (!Engine::instance().isMaster()) {
        return;
    }

    // no need to specify the address on the host/server
    if (!isServer && address.empty()) {
        Log::Error("Network error: No address set");
        return;
    }

    networkPtr = std::make_unique<Network>(
        port,
        address,
        isServer,
        Network::ConnectionType::DataTransfer
    );

    // init
    try {
        Log::Debug(fmt::format("Initiating network connection at port {}", port));

        networkPtr->setUpdateFunction(networkConnectionUpdated);
        networkPtr->setPackageDecodeFunction(networkDecode);
        networkPtr->setAcknowledgeFunction(networkAck);
        networkPtr->initialize();
    }
    catch (const std::runtime_error& err) {
        Log::Error(fmt::format("Network error: {}", err.what()));
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
        NetworkManager::instance().transferData(data, length, id, *networkPtr);
        timerData.first = time();
        timerData.second = id;
    }
}

void sendTestMessage() {
    std::string test = "What's up?";
    static int counter = 0;
    sendData(test.data(), static_cast<int>(test.size()), counter);
    counter++;
}

void draw(const RenderData& data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr double Speed = 0.44;

    //create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime * Speed),
        glm::vec3(0.f, -1.f, 0.f)
    );
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime * (Speed / 2.0)),
        glm::vec3(1.f, 0.f, 0.f)
    );

    const glm::mat4 mvp = glm::make_mat4(data.modelViewProjectionMatrix.values) * scene;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    ShaderManager::instance().shaderProgram("xform").bind();
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    box->draw();
    ShaderManager::instance().shaderProgram("xform").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSync() {
    if (Engine::instance().isMaster()) {
        currentTime = time();
    }
}

void initOGL(GLFWwindow*) {
    textureId = TextureManager::instance().loadTexture("box.png", true, 8.f);
    box = std::make_unique<utils::Box>(2.f, utils::Box::TextureMappingMode::Regular);

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance().addShaderProgram("xform", VertexShader, FragmentShader);
    const ShaderProgram& prg = ShaderManager::instance().shaderProgram("xform");
    prg.bind();
    matrixLoc = glGetUniformLocation(prg.id(), "mvp");
    glUniform1i(glGetUniformLocation(prg.id(), "tex"), 0);
    prg.unbind();

    for (const std::unique_ptr<Window>& win : Engine::instance().windows()) {
        win->setWindowTitle(isServer ? "SERVER" : "CLIENT");
    }
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, currentTime);
    return data;
}

void decode(const std::vector<std::byte>& data) {
    unsigned int pos = 0;
    deserializeObject(data, pos, currentTime);
}

void cleanup() {
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

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (Engine::instance().isMaster() && action == Action::Press) {
        if (key == Key::Esc) {
            Engine::instance().terminate();
        }
        else if (key == Key::Space) {
            sendTestMessage();
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

    for (int i = 0; i < argc; i++) {
        std::string_view v(argv[i]);
        if (v == "-port" && argc > (i + 1)) {
            port = std::stoi(argv[i + 1]);
            Log::Info(fmt::format("Setting port to: {}", port));
        }
        else if (v == "-address" && argc > (i + 1)) {
            address = argv[i + 1];
            Log::Info(fmt::format("Setting address to: {}", address));
        }
        else if (v == "--server") {
            isServer = true;
            Log::Info("This computer will host the connection");
        }
    }

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGL;
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.draw = draw;
    callbacks.cleanup = cleanup;
    callbacks.keyboard = keyboard;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error& e) {
        Log::Error(e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    connectionThread = std::make_unique<std::thread>(networkLoop);

    Engine::instance().exec();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
