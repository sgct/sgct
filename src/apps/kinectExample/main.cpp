#include <sgct.h>
#include <sgct/tracker.h>
#include <sgct/trackingdevice.h>
#include <sgct/trackingmanager.h>

namespace {
    sgct::Engine* gEngine;

    sgct::SharedDouble currentTime(0.0);
    sgct::SharedFloat sizeFactor(0.5f);

    sgct::TrackingDevice* leftHand = nullptr;
    sgct::TrackingDevice* rightHand = nullptr;
} // namespace

using namespace sgct;

void initOGLFun() {
    glEnable(GL_DEPTH_TEST);
 
    // connect only to VRPN on the master
    if (gEngine->isMaster()) {
        // get the tracking pointers
        Tracker* tracker = Engine::getTrackingManager().getTracker("Kinect0");
        if (tracker) {
            leftHand = tracker->getDevice("Left Hand");
            rightHand = tracker->getDevice("Right Hand");
        }

        if (leftHand == nullptr || rightHand == nullptr) {
            MessageHandler::instance()->print(
                "Failed to get pointers to hand trackers!\n"
            );
        }
    }
}

void drawFun() {
    if (leftHand == nullptr || rightHand == nullptr) {
        return;
    }

    const float Speed = 50.f;
    glRotatef(static_cast<float>(currentTime.getVal() ) * Speed, 0.f, 1.f, 0.f);

    const float size = sizeFactor.getVal();

    // render a single triangle
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
    if (leftHand == nullptr || rightHand == nullptr) {
        return;
    }

    // set the time only on the master
    if (gEngine->isMaster()) {
        // get the time in seconds
        currentTime.setVal(sgct::Engine::getTime());

        const glm::vec3 leftPos = leftHand->getPosition();
        const glm::vec3 rightPos = rightHand->getPosition();
        const float dist = glm::length(leftPos - rightPos);
        sizeFactor.setVal((dist < 2.f && dist > 0.2f) ? dist : 0.5f);
    }
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeFloat(sizeFactor);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
    SharedData::instance()->readFloat(sizeFactor);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    gEngine = new Engine(config);

    gEngine->setInitOGLFunction(initOGLFun);
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
