/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>

namespace {
    double currentTime = 0.0;

    bool showGraph = false;
    float sizeFactor = 0.5f;
} // namespace

using namespace sgct;

void drawFun(const RenderData&) {
    constexpr const float Speed = 50.f;
    glRotatef(static_cast<float>(currentTime) * Speed, 0.f, 1.f, 0.f);

    const float size = sizeFactor;

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
        currentTime = Engine::getTime();
    }
}

void postSyncPreDrawFun() {
    Engine::instance().setStatsGraphVisibility(showGraph);
}

std::vector<std::byte> encodeFun() {
    std::vector<std::byte> data;
    serializeObject(data, currentTime);
    serializeObject(data, sizeFactor);
    serializeObject(data, showGraph);
    return data;
}

void decodeFun(const std::vector<std::byte>& data) {
    unsigned int pos = 0;
    deserializeObject(data, pos, currentTime);
    deserializeObject(data, pos, sizeFactor);
    deserializeObject(data, pos, showGraph);
}

void externalControlMessageCallback(const char* receivedChars, int size) {
    if (Engine::instance().isMaster()) {
        std::string_view msg(receivedChars, size);
        if (size == 7 && msg.substr(0, 5) == "graph") {
            showGraph = msg.substr(6, 1) == "1";
        }
        else if (size >= 6 && msg.substr(0, 4) == "size") {
            // parse string to int
            int tmpVal = std::stoi(std::string(msg.substr(5)));
            // recalc percent to float
            sizeFactor = static_cast<float>(tmpVal) / 100.f;
        }

        Log::Info("Message: '%s', size: %d", receivedChars, size);
    }
}

void externalControlStatusCallback(bool connected) {
    if (connected) {
        Log::Info("External control connected");
    }
    else {
        Log::Info("External control disconnected");
    }
}

void keyCallback(Key key, Modifier, Action action, int) {
    if (key == Key::Esc && action == Action::Press) {
        Engine::instance().terminate();
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    Engine::Callbacks callbacks;
    callbacks.draw = drawFun;
    callbacks.preSync = preSyncFun;
    callbacks.keyboard = keyCallback;
    callbacks.postSyncPreDraw = postSyncPreDrawFun;
    callbacks.externalDecode = externalControlMessageCallback;
    callbacks.externalStatus = externalControlStatusCallback;
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
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
