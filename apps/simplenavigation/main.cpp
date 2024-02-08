/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>
#include <sgct/opengl.h>
#include <sgct/user.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    constexpr float RotationSpeed = 0.0017f;
    constexpr float WalkingSpeed = 2.5f;

    constexpr int LandscapeSize = 50;
    constexpr int NumberOfPyramids = 150;

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

    glm::mat4 xform;
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

    constexpr std::string_view GridVertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPosition;

  uniform mat4 mvp;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPosition, 1.0);
  })";

    constexpr std::string_view GridFragmentShader = R"(
  #version 330 core
  out vec4 color;
  void main() { color = vec4(1.0, 1.0, 1.0, 0.8); }
)";

    constexpr std::string_view PyramidVertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPosition;

  uniform mat4 mvp;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPosition, 1.0);
  })";

    constexpr std::string_view PyramidFragmentShader = R"(
  #version 330 core
  uniform float alpha;
  out vec4 color;
  void main() { color = vec4(1.0, 0.0, 0.5, alpha); }
)";
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    vertData.clear();
}


void drawPyramid(glm::mat4 mvp, int index) {
    const glm::mat4 proj = mvp * xform * pyramidTransforms[index];

    ShaderManager::instance().shaderProgram("pyramidShader").bind();

    glUniformMatrix4fv(pyramid.matrixLocation, 1, GL_FALSE, glm::value_ptr(proj));

    glBindVertexArray(pyramid.vao);

    // draw lines
    glLineWidth(2.f);
    glPolygonOffset(1.f, 0.1f); // offset to avoid z-buffer fighting
    glUniform1f(alphaLocation, 0.8f);
    glDrawArrays(GL_LINES, 0, 16);
    // draw triangles
    glPolygonOffset(0.f, 0.f); // offset to avoid z-buffer fighting
    glUniform1f(alphaLocation, 0.3f);
    glDrawArrays(GL_TRIANGLES, 16, 12);

    glBindVertexArray(0);
    ShaderManager::instance().shaderProgram("pyramidShader").unbind();
}

void drawXZGrid(glm::mat4 mvp) {
    const glm::mat4 proj = mvp * xform;

    ShaderManager::instance().shaderProgram("gridShader").bind();
    glUniformMatrix4fv(grid.matrixLocation, 1, GL_FALSE, glm::value_ptr(proj));
    glBindVertexArray(grid.vao);
    glLineWidth(3.f);
    glPolygonOffset(0.f, 0.f); // offset to avoid z-buffer fighting
    glDrawArrays(GL_LINES, 0, grid.nVerts);

    glBindVertexArray(0);
    ShaderManager::instance().shaderProgram("gridShader").unbind();
}

void cleanup() {
    glDeleteBuffers(1, &pyramid.vbo);
    glDeleteBuffers(1, &grid.vbo);

    glDeleteVertexArrays(1, &pyramid.vao);
    glDeleteVertexArrays(1, &grid.vao);
}

void initOGL(GLFWwindow*) {
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

    ShaderManager::instance().addShaderProgram(
        "gridShader",
        GridVertexShader,
        GridFragmentShader
    );
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("gridShader");
    prog.bind();
    grid.matrixLocation = glGetUniformLocation(prog.id(), "mvp");
    prog.unbind();

    ShaderManager::instance().addShaderProgram(
        "pyramidShader",
        PyramidVertexShader,
        PyramidFragmentShader
    );
    const ShaderProgram& pyramidProg =
        ShaderManager::instance().shaderProgram("pyramidShader");
    pyramidProg.bind();
    pyramid.matrixLocation = glGetUniformLocation(pyramidProg.id(), "mvp");
    alphaLocation = glGetUniformLocation(pyramidProg.id(), "alpha");
    pyramidProg.unbind();
}

void preSync() {
    if (Engine::instance().isMaster()) {
        const Window* wnd = Engine::instance().focusedWindow();
        if (mouseLeftButton && wnd) {
            double yPos;
            glfwGetCursorPos(wnd->windowHandle(), &mouseXPos[0], &yPos);
            mouseDx = mouseXPos[0] - mouseXPos[1];
        }
        else {
            mouseDx = 0.0;
        }

        static float panRot = 0.f;
        panRot += static_cast<float>(
            mouseDx * RotationSpeed * Engine::instance().statistics().dt()
        );

        //rotation around the y-axis
        const glm::mat4 viewRotateX = glm::rotate(
            glm::mat4(1.f),
            panRot,
            glm::vec3(0.f, 1.f, 0.f)
        );

        view = glm::inverse(glm::mat3(viewRotateX)) * glm::vec3(0.f, 0.f, 1.f);

        const glm::vec3 right = glm::cross(view, up);
        const float dt = static_cast<float>(Engine::instance().statistics().dt());

        if (buttonForward) {
            pos += (WalkingSpeed * dt * view);
        }
        if (buttonBackward) {
            pos -= (WalkingSpeed * dt * view);
        }
        if (buttonLeft) {
            pos -= (WalkingSpeed * dt * right);
        }
        if (buttonRight) {
            pos += (WalkingSpeed * dt * right);
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

        // 4. transform user back to original position
        vec3 mono = Engine::defaultUser().posMono();
        xform = glm::translate(glm::mat4(1.f), glm::vec3(mono.x, mono.y, mono.z));
        // 3. apply view rotation
        xform *= viewRotateX;
        // 2. apply navigation translation
        xform *= glm::translate(glm::mat4(1.f), pos);
        // 1. transform user to coordinate system origin
        xform *= glm::translate(glm::mat4(1.f), -glm::vec3(mono.x, mono.y, mono.z));
    }
}

void draw(const RenderData& data) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    drawXZGrid(glm::make_mat4(data.modelViewProjectionMatrix.values));

    for (int i = 0; i < NumberOfPyramids; i++) {
        drawPyramid(glm::make_mat4(data.modelViewProjectionMatrix.values), i);
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, xform);
    return data;
}

void decode(const std::vector<std::byte>& data) {
    unsigned int startPos = 0;
    deserializeObject(data, startPos, xform);
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (Engine::instance().isMaster()) {
        switch (key) {
            case Key::Esc:
                Engine::instance().terminate();
                break;
            case Key::Up:
            case Key::W:
                buttonForward = (action == Action::Repeat || action == Action::Press);
                break;
            case Key::Down:
            case Key::S:
                buttonBackward = (action == Action::Repeat || action == Action::Press);
                break;
            case Key::Left:
            case Key::A:
                buttonLeft = (action == Action::Repeat || action == Action::Press);
                break;
            case Key::Right:
            case Key::D:
                buttonRight = (action == Action::Repeat || action == Action::Press);
                break;
            default:
                break;
        }
    }
}

void mouseButton(MouseButton button, Modifier, Action action, Window*) {
    const Window* wnd = Engine::instance().focusedWindow();
    if (Engine::instance().isMaster() && button == MouseButton::ButtonLeft && wnd) {
        mouseLeftButton = (action == Action::Press);
        double yPos;
        glfwGetCursorPos(wnd->windowHandle(), &mouseXPos[1], &yPos);
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    if (!cluster.success) {
        return -1;
    }

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGL;
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.draw = draw;
    callbacks.cleanup = cleanup;
    callbacks.keyboard = keyboard;
    callbacks.mouseButton = mouseButton;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error& e) {
        Log::Error(e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance().exec();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}

