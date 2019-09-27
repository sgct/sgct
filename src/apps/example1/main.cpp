#include <sgct.h>

namespace {
    sgct::Engine* gEngine;
    sgct::SharedDouble currentTime(0.0);
} // namespace

using namespace sgct;

void drawFun() {
    float speed = 50.f;
    glRotatef(static_cast<float>(currentTime.getVal()) * speed, 0.f, 1.f, 0.f);

    // render a single triangle
    glBegin(GL_TRIANGLES);
    glColor3f(1.f, 0.f, 0.f);
    glVertex3f(-0.5f, -0.5f, 0.f);

    glColor3f(0.f, 1.f, 0.f);
    glVertex3f(0.f, 0.5f, 0.f);

    glColor3f(0.f, 0.f, 1.f);
    glVertex3f(0.5f, -0.5f, 0.f);
    glEnd();
}

void preSyncFun() {
    // set the time only on the master
    if (gEngine->isMaster()) {
        // get the time in seconds
        currentTime.setVal(Engine::getTime());
    }
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    gEngine = new Engine(config);

    // Bind your functions
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
