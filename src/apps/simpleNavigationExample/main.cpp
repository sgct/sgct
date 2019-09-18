#include <sgct.h>
#include <sgct/user.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    sgct::Engine* gEngine;

    constexpr const float RotationSpeed = 0.0017f;
    constexpr const float WalkingSpeed = 2.5f;
    constexpr const int LandscapeSize = 50;
    constexpr const int NumberOfPyramids = 150;

    GLuint myLandscapeDisplayList = 0;

    bool buttonForward = false;
    bool buttonBackward = false;
    bool buttonLeft = false;
    bool buttonRight = false;

    //to check if left mouse button is pressed
    bool mouseLeftButton = false;
    // Holds the difference in position between when the left mouse button is pressed and
    // when the mouse button is held
    double mouseDx = 0.0;

    // Stores the positions that will be compared to measure the difference.
    double mouseXPos[] = { 0.0, 0.0 };

    glm::vec3 view(0.f, 0.f, 1.f);
    glm::vec3 up(0.f, 1.f, 0.f);
    glm::vec3 pos(0.f, 0.f, 0.f);

    sgct::SharedObject<glm::mat4> xform;
} // namespace

using namespace sgct;

void drawXZGrid(int size, float yPos) {
    glPolygonOffset(0.0f, 0.0f); // offset to avoid z-buffer fighting
    glPushMatrix();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glTranslatef(0.f, yPos, 0.f);

    glLineWidth(3.f);
    glColor4f(1.f, 1.f, 1.f, 0.8f);

    glBegin(GL_LINES);
    for (int x = -(size / 2); x < (size / 2); x++) {
        glVertex3i(x, 0, -(size / 2));
        glVertex3i(x, 0, (size / 2));
    }

    for (int z = -(size / 2); z < (size / 2); z++) {
        glVertex3i(-(size / 2), 0, z);
        glVertex3i((size / 2), 0, z);
    }
    glEnd();

    glDisable(GL_BLEND);
    glPopMatrix();
}

void drawPyramid(float width) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonOffset(1.f, 0.1f); // offset to avoid z-buffer fighting
    
    // enhance the pyramids with lines in the edges
    glLineWidth(2.f);
    glColor4f(1.0f, 0.0f, 0.5f, 0.8f);

    glBegin(GL_LINE_LOOP);
    glVertex3f(-width / 2.f, 0.f, -width / 2.f);
    glVertex3f(0.f, 2.f, 0.f);
    glVertex3f(-width / 2.f, 0.f, width / 2.f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-width / 2.f, 0.f, width / 2.f);
    glVertex3f(0.f, 2.f, 0.f);
    glVertex3f(width / 2.f, 0.f, width / 2.f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(width / 2.f, 0.f, width / 2.f);
    glVertex3f(0.f, 2.f, 0.f);
    glVertex3f(width / 2.f, 0.f, -width / 2.f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(width / 2.f, 0.f, -width / 2.f);
    glVertex3f(0.f, 2.f, 0.f);
    glVertex3f(-width / 2.f, 0.f, -width / 2.f);
    glEnd();

    glColor4f(1.f, 0.f, 0.5f, 0.3f);

    glPolygonOffset(0.f, 0.f); // offset to avoid z-buffer fighting
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.f, 2.f, 0.f);

    // draw sides
    glVertex3f(-width / 2.f, 0.f, -width / 2.f);
    glVertex3f(-width / 2.f, 0.f, width / 2.f);
    glVertex3f(width / 2.f, 0.f, width / 2.f);
    glVertex3f(width / 2.f, 0.f, -width / 2.f);
    glVertex3f(-width / 2.f, 0.f, -width / 2.f);

    glEnd();

    glDisable(GL_BLEND);
}

void initOGLFun() {
    // create and compile display list
    myLandscapeDisplayList = glGenLists(1);
    glNewList(myLandscapeDisplayList, GL_COMPILE);

    drawXZGrid(LandscapeSize, -1.5f);

    // pick a seed for the random function (must be same on all nodes)
    srand(9745);
    for (int i = 0; i < NumberOfPyramids; i++) {
        const float xPos = static_cast<float>(rand() % LandscapeSize - LandscapeSize / 2);
        const float zPos = static_cast<float>(rand() % LandscapeSize - LandscapeSize / 2);

        glPushMatrix();
        glTranslatef(xPos, -1.5f, zPos);
        drawPyramid(0.6f);
        glPopMatrix();
    }

    glEndList();
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        if (mouseLeftButton) {
            double yPos;
            // get the mouse pos from first window
            Engine::getMousePos(gEngine->getFocusedWindowIndex(), &mouseXPos[0], &yPos);
            mouseDx = mouseXPos[0] - mouseXPos[1];
        }
        else {
            mouseDx = 0.0;
        }

        static float panRot = 0.0f;
        panRot += (static_cast<float>(mouseDx) * RotationSpeed *
                   static_cast<float>(gEngine->getDt()));

        // rotation around the y-axis
        const glm::mat4 viewRotateX = glm::rotate(
            glm::mat4(1.f),
            panRot,
            glm::vec3(0.f, 1.f, 0.f)
        );

        view = glm::inverse(glm::mat3(viewRotateX)) * glm::vec3(0.0f, 0.0f, 1.0f);

        const glm::vec3 right = glm::cross(view, up);

        if (buttonForward) {
            pos += (WalkingSpeed * static_cast<float>(gEngine->getDt()) * view);
        }
        if (buttonBackward) {
            pos -= (WalkingSpeed * static_cast<float>(gEngine->getDt()) * view);
        }
        if (buttonLeft) {
            pos -= (WalkingSpeed * static_cast<float>(gEngine->getDt()) * right);
        }
        if (buttonRight) {
            pos += (WalkingSpeed * static_cast<float>(gEngine->getDt()) * right);
        }

        /*
         * To get a first person camera, the world needs to be transformed around the
         * users head.
         *
         * This is done by:
         *   1. Transform the user to coordinate system origin
         *   2. Apply navigation
         *   3. Apply rotation
         *   4. Transform the user back to original position
         *
         * However, mathwise this process need to be reversed due to the matrix
         * multiplication order.
         */

        //4. transform user back to original position
         glm::mat4 result = glm::translate(
             glm::mat4(1.f),
             Engine::getDefaultUser().getPosMono()
         );
        //3. apply view rotation
        result *= viewRotateX;
        //2. apply navigation translation
        result *= glm::translate(glm::mat4(1.f), pos);
        //1. transform user to coordinate system origin
        result *= glm::translate(glm::mat4(1.f), -Engine::getDefaultUser().getPosMono());

        xform.setVal(result);
    }
}

void drawFun() {
    glDisable(GL_DEPTH_TEST);
    
    glMultMatrixf(glm::value_ptr(xform.getVal()));
    glCallList(myLandscapeDisplayList);

    glEnable(GL_DEPTH_TEST);
}

void encodeFun() {
    SharedData::instance()->writeObj(xform);
}

void decodeFun() {
    SharedData::instance()->readObj(xform);
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster()) {
        switch (key) {
            case SGCT_KEY_UP:
            case SGCT_KEY_W:
                buttonForward = (action == SGCT_REPEAT || action == SGCT_PRESS);
                break;
            case SGCT_KEY_DOWN:
            case SGCT_KEY_S:
                buttonBackward = (action == SGCT_REPEAT || action == SGCT_PRESS);
                break;
            case SGCT_KEY_LEFT:
            case SGCT_KEY_A:
                buttonLeft = (action == SGCT_REPEAT || action == SGCT_PRESS);
                break;
            case SGCT_KEY_RIGHT:
            case SGCT_KEY_D:
                buttonRight = (action == SGCT_REPEAT || action == SGCT_PRESS);
                break;
        }
    }
}

void mouseButtonCallback(int button, int action, int) {
    if (gEngine->isMaster() && button == SGCT_MOUSE_BUTTON_LEFT) {
        mouseLeftButton = (action == SGCT_PRESS);
        double yPos;
        Engine::getMousePos(gEngine->getFocusedWindowIndex(), &mouseXPos[1], &yPos);
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new sgct::Engine(arg);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setMouseButtonCallbackFunction(mouseButtonCallback);
    gEngine->setClearColor(0.1f, 0.1f, 0.1f, 1.f);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    glDeleteLists(myLandscapeDisplayList, 1);
    delete gEngine;
    exit(EXIT_SUCCESS);
}
