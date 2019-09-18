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

    bool buttonForward = false;
    bool buttonBackward = false;
    bool buttonLeft = false;
    bool buttonRight = false;

    // to check if left mouse button is pressed
    bool mouseLeftButton = false;
    // Holds the difference in position between when the left mouse button is pressed and
    // when the mouse button is held.
    double mouseDx = 0.0;
    // Stores the positions that will be compared to measure the difference.
    double mouseXPos[] = { 0.0, 0.0 };

    glm::vec3 view(0.f, 0.f, 1.f);
    glm::vec3 up(0.f, 1.f, 0.f);
    glm::vec3 pos(0.f, 0.f, 0.f);

    sgct::SharedObject<glm::mat4> xform;
    glm::mat4 pyramidTransforms[NumberOfPyramids];

    struct {
        GLuint vao = 0;
        GLuint vbo = 0;
        int nVerts = 0;
        GLint matrixLocation = -1;
    } pyramid;

    struct {
        GLuint vao = 0;
        GLuint vbo = 0;
        int nVerts = 0;
        GLint matrixLocation = -1;
    } grid;

    // shader locations
    GLint alphaLocation = -1;

    struct Vertex {
        float mX = 0.f;
        float mY = 0.f;
        float mZ = 0.f;
    };
} // namespace

using namespace sgct;


void createXZGrid(int size, float yPos) {
    grid.nVerts = size * 4;
    std::vector<Vertex> vertData(grid.nVerts);

    int i = 0;
    for (int x = -(size / 2); x < (size / 2); x++) {
        vertData[i].mX = static_cast<float>(x);
        vertData[i].mY = yPos;
        vertData[i].mZ = static_cast<float>(-(size / 2));

        vertData[i + 1].mX = static_cast<float>(x);
        vertData[i + 1].mY = yPos;
        vertData[i + 1].mZ = static_cast<float>(size / 2);

        i += 2;
    }

    for (int z = -(size / 2); z < (size / 2); z++) {
        vertData[i].mX = static_cast<float>(-(size / 2));
        vertData[i].mY = yPos;
        vertData[i].mZ = static_cast<float>(z);

        vertData[i + 1].mX = static_cast<float>(size / 2);
        vertData[i + 1].mY = yPos;
        vertData[i + 1].mZ = static_cast<float>(z);

        i += 2;
    }

    glBindVertexArray(grid.vao);
    glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);

    // upload data to GPU
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Vertex) * grid.nVerts,
        vertData.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    glBindVertexArray(GL_FALSE);
    glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);
}

void createPyramid(float width) {
    std::vector<Vertex> vertData;

    // enhance the pyramids with lines in the edges
    // -x
    vertData.push_back(Vertex{ -width / 2.f, 0.f,  width / 2.f });
    vertData.push_back(Vertex{ -width / 2.f, 0.f, -width / 2.f });
    vertData.push_back(Vertex{ 0.f, 2.f,          0.f });
    vertData.push_back(Vertex{ -width / 2.f, 0.f,  width / 2.f });
    // +x
    vertData.push_back(Vertex{ width / 2.f, 0.f, -width / 2.f });
    vertData.push_back(Vertex{ width / 2.f, 0.f,  width / 2.f });
    vertData.push_back(Vertex{ 0.f, 2.f,          0.f });
    vertData.push_back(Vertex{ width / 2.f, 0.f, -width / 2.f });
    // -z
    vertData.push_back(Vertex{ -width / 2.f, 0.f, -width / 2.f });
    vertData.push_back(Vertex{ width / 2.f, 0.f, -width / 2.f });
    vertData.push_back(Vertex{ 0.f, 2.f,          0.f });
    vertData.push_back(Vertex{ -width / 2.f, 0.f, -width / 2.f });
    // +z
    vertData.push_back(Vertex{ width / 2.f, 0.f,  width / 2.f });
    vertData.push_back(Vertex{ -width / 2.f, 0.f,  width / 2.f });
    vertData.push_back(Vertex{ 0.f, 2.f,          0.f });
    vertData.push_back(Vertex{ width / 2.f, 0.f,  width / 2.f });

    // triangles
    // -x
    vertData.push_back(Vertex{ -width / 2.f, 0.f, -width / 2.f });
    vertData.push_back(Vertex{ 0.f, 2.f,          0.f });
    vertData.push_back(Vertex{ -width / 2.f, 0.f,  width / 2.f });
    // +x
    vertData.push_back(Vertex{ width / 2.f, 0.f,  width / 2.f });
    vertData.push_back(Vertex{ 0.f, 2.f,         0.f });
    vertData.push_back(Vertex{ width / 2.f, 0.f, -width / 2.f });
    // -z
    vertData.push_back(Vertex{ width / 2.f, 0.f, -width / 2.f });
    vertData.push_back(Vertex{ 0.f, 2.f,          0.f });
    vertData.push_back(Vertex{ -width / 2.f, 0.f, -width / 2.f });
    // +z
    vertData.push_back(Vertex{ -width / 2.f, 0.f,  width / 2.f });
    vertData.push_back(Vertex{ 0.f, 2.f,          0.f });
    vertData.push_back(Vertex{ width / 2.f, 0.f,  width / 2.f });

    pyramid.nVerts = static_cast<int>(vertData.size());

    glBindVertexArray(pyramid.vao);
    glBindBuffer(GL_ARRAY_BUFFER, pyramid.vbo);

    // upload data to GPU
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Vertex) * pyramid.nVerts,
        vertData.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    glBindVertexArray(GL_FALSE);
    glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);

    vertData.clear();
}


void drawPyramid(int index) {
    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix() *
        xform.getVal() * pyramidTransforms[index];

    ShaderManager::instance()->bindShaderProgram("pyramidShader");

    glUniformMatrix4fv(pyramid.matrixLocation, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(pyramid.vao);

    // draw lines
    glLineWidth(2.0f);
    glPolygonOffset(1.f, 0.1f); // offset to avoid z-buffer fighting
    glUniform1f(alphaLocation, 0.8f);
    glDrawArrays(GL_LINES, 0, 16);
    // draw triangles
    glPolygonOffset(0.f, 0.f); // offset to avoid z-buffer fighting
    glUniform1f(alphaLocation, 0.3f);
    glDrawArrays(GL_TRIANGLES, 16, 12);

    glBindVertexArray(0);
    sgct::ShaderManager::instance()->unBindShaderProgram();
}

void drawXZGrid() {
    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix() * xform.getVal();

    ShaderManager::instance()->bindShaderProgram("gridShader");

    glUniformMatrix4fv(grid.matrixLocation, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(grid.vao);

    glLineWidth(3.f);
    glPolygonOffset(0.f, 0.f); // offset to avoid z-buffer fighting
    glDrawArrays(GL_LINES, 0, grid.nVerts);

    glBindVertexArray(0);
    ShaderManager::instance()->unBindShaderProgram();
}

void cleanUpFun() {
    glDeleteBuffers(1, &pyramid.vbo);
    glDeleteBuffers(1, &grid.vbo);

    glDeleteVertexArrays(1, &pyramid.vao);
    glDeleteVertexArrays(1, &grid.vao);
}

void initOGLFun() {
    glGenVertexArrays(1, &pyramid.vao);
    glGenVertexArrays(1, &grid.vao);

    glGenBuffers(1, &pyramid.vbo);
    glGenBuffers(1, &grid.vbo);

    createXZGrid(LandscapeSize, -1.5f);
    createPyramid(0.6f);

    // pick a seed for the random function (must be same on all nodes)
    srand(9745);
    for (int i = 0; i < NumberOfPyramids; i++) {
        const float x = static_cast<float>(rand() % LandscapeSize - LandscapeSize / 2);
        const float z = static_cast<float>(rand() % LandscapeSize - LandscapeSize / 2);

        pyramidTransforms[i] = glm::translate(glm::mat4(1.f), glm::vec3(x, -1.5f, z));
    }

    ShaderManager::instance()->addShaderProgram(
        "gridShader",
        "gridShader.vert",
        "gridShader.frag"
    );
    ShaderManager::instance()->bindShaderProgram("gridShader");
    const ShaderProgram& gProg= ShaderManager::instance()->getShaderProgram("gridShader");
    grid.matrixLocation = gProg.getUniformLocation("MVP");
    ShaderManager::instance()->unBindShaderProgram();

    ShaderManager::instance()->addShaderProgram(
        "pyramidShader",
        "pyramidShader.vert",
        "pyramidShader.frag"
    );
    ShaderManager::instance()->bindShaderProgram("pyramidShader");
    const ShaderProgram& pyramidProg =
        ShaderManager::instance()->getShaderProgram("pyramidShader");
    pyramid.matrixLocation = pyramidProg.getUniformLocation("MVP");
    alphaLocation = pyramidProg.getUniformLocation("alpha");
    sgct::ShaderManager::instance()->unBindShaderProgram();
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        if (mouseLeftButton) {
            double yPos;
            Engine::getMousePos(gEngine->getFocusedWindowIndex(), &mouseXPos[0], &yPos);
            mouseDx = mouseXPos[0] - mouseXPos[1];
        }
        else {
            mouseDx = 0.0;
        }

        static float panRot = 0.f;
        panRot += static_cast<float>(mouseDx * RotationSpeed * gEngine->getDt());

        //rotation around the y-axis
        const glm::mat4 viewRotateX = glm::rotate(
            glm::mat4(1.f),
            panRot,
            glm::vec3(0.f, 1.f, 0.f)
        
        );

        view = glm::inverse(glm::mat3(viewRotateX)) * glm::vec3(0.f, 0.f, 1.f);

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

        /**
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

        glm::mat4 result;
        // 4. transform user back to original position
        result = glm::translate(glm::mat4(1.f), Engine::getDefaultUser().getPosMono());
        // 3. apply view rotation
        result *= viewRotateX;
        // 2. apply navigation translation
        result *= glm::translate(glm::mat4(1.f), pos);
        // 1. transform user to coordinate system origin
        result *= glm::translate(glm::mat4(1.f), -Engine::getDefaultUser().getPosMono());

        xform.setVal(result);
    }
}

void drawFun() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    drawXZGrid();

    for (int i = 0; i < NumberOfPyramids; i++) {
        drawPyramid(i);
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
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
            case key::Up:
            case key::W:
                buttonForward = (action == action::Repeat || action == action::Press);
                break;
            case key::Down:
            case key::S:
                buttonBackward = (action == action::Repeat || action == action::Press);
                break;
            case key::Left:
            case key::A:
                buttonLeft = (action == action::Repeat || action == action::Press);
                break;
            case key::Right:
            case key::D:
                buttonRight = (action == action::Repeat || action == action::Press);
                break;
        }
    }
}

void mouseButtonCallback(int button, int action, int) {
    if (gEngine->isMaster() && button == mouse::ButtonLeft) {
        mouseLeftButton = (action == action::Press);
        double yPos;
        Engine::getMousePos(gEngine->getFocusedWindowIndex(), &mouseXPos[1], &yPos);
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setMouseButtonCallbackFunction(mouseButtonCallback);
    gEngine->setClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    gEngine->setCleanUpFunction(cleanUpFun);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}

