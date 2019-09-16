#include <sgct.h>
#include <sgct/SGCTTracker.h>
#include <sgct/SGCTTrackingDevice.h>
#include <sgct/SGCTTrackingManager.h>
#include <sgct/SGCTWindow.h>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    // store each device's transform 4x4 matrix in a shared vector
    sgct::SharedVector<glm::mat4> sharedTransforms;
    sgct::SharedString sharedText;
    sgct::SharedObject<size_t> sharedHeadSensorIndex(0);
} // namespace

using namespace sgct;

void drawWireCube(float size) {
    // bottom
    glBegin(GL_LINE_STRIP);
    glVertex3f(-size, -size, -size);
    glVertex3f(size, -size, -size);
    glVertex3f(size, -size, size);
    glVertex3f(-size, -size, size);
    glVertex3f(-size, -size, -size);
    glEnd();

    // top
    glBegin(GL_LINE_STRIP);
    glVertex3f(-size, size, -size);
    glVertex3f(size, size, -size);
    glVertex3f(size, size, size);
    glVertex3f(-size, size, size);
    glVertex3f(-size, size, -size);
    glEnd();

    // sides
    glBegin(GL_LINES);
    glVertex3f(-size, -size, -size);
    glVertex3f(-size, size, -size);

    glVertex3f(size, -size, -size);
    glVertex3f(size, size, -size);

    glVertex3f(size, -size, size);
    glVertex3f(size, size, size);

    glVertex3f(-size, -size, size);
    glVertex3f(-size, size, size);
    glEnd();
}

void drawAxes(float size) {
    glLineWidth(2.0);
    glBegin(GL_LINES);

    // x-axis
    glColor3f(1.f, 0.f, 0.f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(size, 0.f, 0.f);

    // y-axis
    glColor3f(0.f, 1.f, 0.f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, size, 0.f);

    // z-axis
    glColor3f(0.f, 0.f, 1.f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, 0.f, size);

    glEnd();
}

void initOGLFun() {
    glEnable(GL_DEPTH_TEST);

    // only store the tracking data on the master node
    if (!gEngine->isMaster()) {
        return;
    }

    size_t index = 0;
        
    for (size_t i = 0; i < Engine::getTrackingManager().getNumberOfTrackers(); i++) {
        sgct::SGCTTracker* trackerPtr = sgct::Engine::getTrackingManager().getTracker(i);
            
        // init the shared vector with identity matrixes
        for (size_t j= 0; j < trackerPtr->getNumberOfDevices(); j++) {
            sgct::SGCTTrackingDevice* devicePtr = trackerPtr->getDevice(j);
            
            if (devicePtr->hasSensor()) {
                sharedTransforms.addVal(glm::mat4(1.f));
                    
                // find the head sensor
                if (Engine::getTrackingManager().getHeadDevice() == devicePtr) {
                    sharedHeadSensorIndex.setVal(index);
                }

                index++;
            }
        }
    }
}

// This callback is called once per render loop iteration.
void preSyncFun() {
    /*
     *
     * Store all transforms in the array by looping through all trackers and all devices.
     *
     * Storing values from the tracker in the pre-sync callback will guarantee that the
     * values are equal for all draw calls within the same frame. This prevents the
     * application from getting different tracked data for left and right draws using a
     * stereoscopic display. Remember to get all sensor, button and analog data that will
     * affect the rendering in this stage.
     */

    // only store the tracking data on the master node
    if (!gEngine->isMaster()) {
        return;
    }

    size_t index = 0;
    std::stringstream ss;
        
    // Loop through trackers (like intersense IS-900, Microsoft Kinect, PhaseSpace etc.)
    for (size_t i = 0; i < Engine::getTrackingManager().getNumberOfTrackers(); i++) {
        sgct::SGCTTracker* trackerPtr = Engine::getTrackingManager().getTracker(i);
        
        // Loop trough all tracking devices (like headtracker, wand, stylus etc.)
        for (size_t j = 0; j < trackerPtr->getNumberOfDevices(); j++) {
            sgct::SGCTTrackingDevice* devicePtr = trackerPtr->getDevice(j);
                
            ss << "Device " << i << '-' << j << ": " << devicePtr->getName() << '\n';
                
            if (devicePtr->hasSensor()) {
                sharedTransforms.setValAt(index, devicePtr->getWorldTransform());
                index++;

                const double time = devicePtr->getTrackerDeltaTime();
                ss << "     Sensor id: " << devicePtr->getSensorId()
                   << " freq: " << (time <= 0.0 ? 0.0 : 1.0 / time) << " Hz\n";

                ss << "\n     Pos\n"
                   << "          x=" << devicePtr->getPosition().x << '\n'
                   << "          y=" << devicePtr->getPosition().y << '\n'
                   << "          z=" << devicePtr->getPosition().z << '\n';

                ss << "\n     Rot\n"
                   << "          rx=" << devicePtr->getEulerAngles().x << '\n'
                   << "          ry=" << devicePtr->getEulerAngles().y << '\n'
                   << "          rz=" << devicePtr->getEulerAngles().z << '\n';
            }

            if (devicePtr->hasButtons()) {
                ss << "\n     Buttons\n";
                    
                for (size_t k = 0; k < devicePtr->getNumberOfButtons(); k++) {
                    ss << "          Button " << k << ": "
                       << (devicePtr->getButton(k) ? "pressed" : "released") << '\n';
                }
            }

            if (devicePtr->hasAnalogs()) {
                ss << "\n     Analog axes\n";
                    
                for (size_t k = 0; k < devicePtr->getNumberOfAxes(); k++) {
                    ss << "          Axis " << k << ": "
                       << devicePtr->getAnalog(k) << '\n';
                }
            }

            ss << '\n';
        }
    }

    // store the string stream into the shared string
    sharedText.setVal(ss.str());
}

/*
 * This callback can be called several times per render loop iteration. Using a single
 * viewport in stereo (3D) usually results in refresh rate of 120 Hz.
 */
void drawFun() {
    // draw some yellow cubes in space
    for (float i = -0.5f; i <= 0.5f; i += 0.2f) {
        for (float j = -0.5f; j <= 0.5f; j += 0.2f) {
            glPushMatrix();
            glTranslatef(i, j, 0.f);
            glColor3f(1.f, 1.f, 0.f);
            drawWireCube(0.04f);
            glPopMatrix();
        }
    }

    // draw a cube and axes around each wand
    for (size_t i = 0; i < sharedTransforms.getSize(); i++) {
        if (i != sharedHeadSensorIndex.getVal())  {
            glLineWidth(2.0);
            glPushMatrix();
            glMultMatrixf(glm::value_ptr(sharedTransforms.getValAt(i)));

            glColor3f(0.5f, 0.5f, 0.5f);
            drawWireCube(0.1f);

            drawAxes(0.1f);

            // draw pointer line
            glBegin(GL_LINES);
            glColor3f(1.f, 1.f, 0.f);
            glVertex3f(0.f, 0.f, 0.f);
            glVertex3f(0.f, 0.f, -5.f);
            glEnd();

            glPopMatrix();
        }
    }

#ifdef SGCT_HAS_TEXT
    // draw text
    const float textVerticalPos = static_cast<float>(
        gEngine->getCurrentWindow().getResolution().y
    ) - 100.0f;
    constexpr const int fontSize = 12;
    
    glColor3f(1.f, 1.f, 1.f);
    sgct_text::print(
        sgct_text::FontManager::instance()->getFont("SGCTFont", fontSize),
        sgct_text::TextAlignMode::TopLeft,
        120.f,
        textVerticalPos,
        sharedText.getVal().c_str()
    );
#endif // SGCT_HAS_TEXT
}

void encodeFun() {
    SharedData::instance()->writeVector(sharedTransforms);
    SharedData::instance()->writeString(sharedText);
    SharedData::instance()->writeObj(sharedHeadSensorIndex);
}

void decodeFun() {
    SharedData::instance()->readVector(sharedTransforms);
    SharedData::instance()->readString(sharedText);
    SharedData::instance()->readObj(sharedHeadSensorIndex);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    // Bind your functions
    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setDrawFunction(drawFun);

    // Init the engine
    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
