#include <sgct/actions.h>
#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/keys.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#include <sgct/window.h>
#include <sgct/utils/box.h>
#include <GLFW/glfw3.h>
#include <SpoutLibrary.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    struct {
        GLuint vao;
        GLuint vbo;
    } geometry;

    bool shouldTakeScreenshot = false;

    SPOUTHANDLE receiver = nullptr;
    std::string sender;
    unsigned int width;
    unsigned int height;
    bool initialized = false;

    constexpr const char* vertexShader = R"(
  #version 330 core

  layout(location = 0) in vec2 vertPositions;

  out vec2 uv;

  void main() {
    gl_Position = vec4(vertPositions, 0.0, 1.0);
    uv = (vertPositions + vec2(1.0)) / vec2(2.0);
    uv.y = 1.0 - uv.y;
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
 
bool bindSpout() {
    const bool creationSuccess = receiver->CreateReceiver(sender.data(), width, height);
    if (!initialized && creationSuccess) {
        Logger::Info(
            "Spout: Initing %ux%u texture from '%s'", width, height, sender.c_str()
        );
        initialized = true;
    }

    if (initialized) {
        const bool receiveSucess = receiver->ReceiveTexture(sender.data(), width, height);
        if (receiveSucess) {
            return receiver->BindSharedTexture();
        }
        else {
            Logger::Info("Spout disconnected");

            // reset if disconnected
            initialized = false;
            sender = '\0';
            receiver->ReleaseReceiver();
        }

    }

    return false;
}

void drawFun() {
    const ShaderProgram& prog = ShaderManager::instance().getShaderProgram("xform");
    prog.bind();

    glActiveTexture(GL_TEXTURE0);
    bindSpout();

    glBindVertexArray(geometry.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    prog.unbind();
    receiver->UnBindSharedTexture();

    if (shouldTakeScreenshot) {
        Engine::instance().takeScreenshot();
        shouldTakeScreenshot = false;
    }
}

void initOGLFun() {
    glGenVertexArrays(1, &geometry.vao);
    glGenBuffers(1, &geometry.vbo);

    glBindVertexArray(geometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);

    struct Vertex { float x, y; };

    std::array<Vertex, 6> vertices;
    vertices[0] = { -1.f, -1.f };
    vertices[1] = { 1.f,  1.f };
    vertices[2] = {-1.f, 1.f };
    vertices[3] = { -1.f, -1.f };
    vertices[4] = { 1.f, -1.f };
    vertices[5] = { 1.f,  1.f };

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    // setup spout
    sender = '\0';
    receiver = GetSpout();
    
    // set background
    Engine::instance().setClearColor(glm::vec4(0.3f, 0.3f, 0.3f, 0.f));

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance().addShaderProgram("xform", vertexShader, fragmentShader);

    const ShaderProgram& prog = ShaderManager::instance().getShaderProgram("xform");
    prog.bind();

    glUniform1i(glGetUniformLocation(prog.getId(), "tex"), 0);

    prog.unbind();
}

void cleanUpFun() {
    glDeleteVertexArrays(1, &geometry.vao);
    glDeleteBuffers(1, &geometry.vbo);

    if (receiver) {
        receiver->ReleaseReceiver();
        receiver->Release();
    }
}

void keyboardCallback(Key key, Modifier, Action action, int) {
    if (action == Action::Press) {
        switch (key) {
            case Key::Esc:
                Engine::instance().terminate();
                break;
            case Key::Key1:
                sender = "Right\0";
                Logger::Info("Settings receiver to 'Right'");
                break;
            case Key::Key2:
                sender = "zLeft\0";
                Logger::Info("Settings receiver to 'zLeft'");
                break;
            case Key::Key3:
                sender = "Bottom\0";
                Logger::Info("Settings receiver to 'Bottom'");
                break;
            case Key::Key4:
                sender = "Top\0";
                Logger::Info("Settings receiver to 'Top'");
                break;
            case Key::Key5:
                sender = "Left\0";
                Logger::Info("Settings receiver to 'Left'");
                break;
            case Key::Key6:
                sender = "zRight\0";
                Logger::Info("Settings receiver to 'zRight'");
                break;
            case Key::Space:
                shouldTakeScreenshot = true;
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);

    Engine::instance().setInitOGLFunction(initOGLFun);
    Engine::instance().setDrawFunction(drawFun);
    Engine::instance().setCleanUpFunction(cleanUpFun);
    Engine::instance().setKeyboardCallbackFunction(keyboardCallback);

    try {
        Engine::instance().init(cluster);
        Engine::instance().render();
    }
    catch (const std::runtime_error & e) {
        Logger::Error("%s", e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
