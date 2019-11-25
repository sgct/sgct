#include <sgct/actions.h>
#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/keys.h>
#include <sgct/shareddata.h>

namespace {
    sgct::SharedDouble currentTime(0.0);

    sgct::SharedBool showStats(false);
    sgct::SharedBool showGraph(false);
    sgct::SharedFloat sizeFactor(0.5f);
} // namespace

using namespace sgct;

void drawFun() {
    constexpr const float Speed = 50.0f;
    glRotatef(static_cast<float>(currentTime.getVal()) * Speed, 0.f, 1.f, 0.f);

    const float size = sizeFactor.getVal();

    glBegin(GL_TRIANGLES);
    glColor3f(1.f, 0.f, 0.f);
    glVertex3f(-0.5f * size, -0.5f * size, 0.f);

    glColor3f(0.f, 1.f, 0.f);
    glVertex3f(0.f, 0.5f * size, 0.f);

    glColor3f(0.f, 0.f, 1.f);
    glVertex3f(0.5f * size, -0.5f * size, 0.f);
    glEnd();
}

void preSyncFun() {
    // set the time only on the master
    if (Engine::instance().isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void postSyncPreDrawFun() {
    Engine::instance().setDisplayInfoVisibility(showStats.getVal());
    Engine::instance().setStatsGraphVisibility(showGraph.getVal());
}

void encodeFun() {
    SharedData::instance().writeDouble(currentTime);
    SharedData::instance().writeFloat(sizeFactor);
    SharedData::instance().writeBool(showStats);
    SharedData::instance().writeBool(showGraph);
}

void decodeFun() {
    SharedData::instance().readDouble(currentTime);
    SharedData::instance().readFloat(sizeFactor);
    SharedData::instance().readBool(showStats);
    SharedData::instance().readBool(showGraph);
}

void externalControlMessageCallback(const char* receivedChars, int size) {
    if (Engine::instance().isMaster()) {
        std::string_view msg(receivedChars, size);
        if (size == 7 && msg.substr(0, 5) == "stats") {
            showStats.setVal(msg.substr(6, 1) == "1");
        }
        else if (size == 7 && msg.substr(0, 5) == "graph") {
            showGraph.setVal(msg.substr(6, 1) == "1");
        }
        else if (size >= 6 && msg.substr(0, 4) == "size") {
            // parse string to int
            int tmpVal = std::stoi(std::string(msg.substr(5)));
            // recalc percent to float
            sizeFactor.setVal(static_cast<float>(tmpVal) / 100.f);
        }

        MessageHandler::printInfo("Message: '%s', size: %d", receivedChars, size);
    }
}

void externalControlStatusCallback(bool connected) {
    if (connected) {
        MessageHandler::printInfo("External control connected");
    }
    else {
        MessageHandler::printInfo("External control disconnected");
    }
}

void keyCallback(Key key, int, Action action, Modifier) {
    if (key == Key::Esc && action == Action::Press) {
        Engine::instance().terminate();
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);

    Engine::instance().setDrawFunction(drawFun);
    Engine::instance().setPreSyncFunction(preSyncFun);
    Engine::instance().setKeyboardCallbackFunction(keyCallback);
    Engine::instance().setPostSyncPreDrawFunction(postSyncPreDrawFun);
    Engine::instance().setExternalControlCallback(externalControlMessageCallback);
    Engine::instance().setExternalControlStatusCallback(externalControlStatusCallback);
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
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
