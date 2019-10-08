#include <sgct.h>
#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/image.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <sgct/shareddata.h>
#include <sgct/shareddatatypes.h>
#include <sgct/texturemanager.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#ifdef SGCT_HAS_TEXT
#include <sgct/Font.h>
#include <sgct/FontManager.h>
#include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT

namespace {
    sgct::Engine* gEngine;

    struct {
        GLuint vao;
        GLuint vbo;
        GLuint ibo;
    } geometry;

    sgct::SharedBool showId(false);

    float tilt = 0.f;
    float radius = 7.4f;

    struct Vertex {
        float x, y, z;
    };
    int nVertices = 0;
    GLint matrixLocation = -1;

    constexpr const char* vertexShader = R"(
#version 330 core

layout(location = 0) in vec3 vertPosition;
uniform mat4 matrix;
out vec4 trans_color;

void main() {
  gl_Position =  matrix * vec4(vertPosition, 1.0);
  trans_color = vec4(vertPosition, 1.0);
}
)";

    constexpr const char* fragmentShader = R"(
#version 330 core

in vec4 trans_color;
out vec4 color;

void main() { color = trans_color; }
)";
} // namespace

using namespace sgct;

void draw() {
    glDepthMask(GL_FALSE);

    ShaderManager::instance()->bindShaderProgram("simple");
    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix();

    // Inverting the tilt to keep the backwards compatibility with the previous
    // implementation
    const glm::mat4 mat = glm::rotate(mvp, -glm::radians(tilt), glm::vec3(1.f, 0.f, 0.f));

    glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, glm::value_ptr(mat));

    glBindVertexArray(geometry.vao);
    glDrawElements(GL_LINE_STRIP, nVertices, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);

    ShaderManager::instance()->unBindShaderProgram();

#ifdef SGCT_HAS_TEXT
    if (showId.getVal()) {
        Window& win = gEngine->getCurrentWindow();
        core::BaseViewport* vp = win.getCurrentViewport();
        const float w = static_cast<float>(win.getResolution().x) * vp->getSize().x;
        const float h = static_cast<float>(win.getResolution().y) * vp->getSize().y;
        
        const float offset = w / 2.f - w / 7.f;
        
        const float s1 = h / 8.f;
        text::Font* f1 = text::FontManager::instance()->getFont(
            "SGCTFont",
            static_cast<unsigned int>(s1)
        );

        text::print(
            *f1,
            sgct::text::TextAlignMode::TopLeft,
            offset,
            h / 2.f - s1,
            glm::vec4(0.f, 0.f, 1.f, 1.f),
            "%d",
            sgct::core::ClusterManager::instance()->getThisNodeId()
        );

        const float s2 = h / 20.f;
        text::Font* f2 = text::FontManager::instance()->getFont(
            "SGCTFont",
            static_cast<unsigned int>(s2)
        );
        text::print(
            *f2,
            text::TextAlignMode::TopLeft,
            offset,
            h / 2.f - (s1 + s2) * 1.2f,
            glm::vec4(0.f, 0.f, 1.f, 1.f),
            "%s",
            core::ClusterManager::instance()->getThisNode()->getAddress().c_str()
        );
    }
#endif // SGCT_HAS_TEXT
    glDepthMask(GL_TRUE);
}

void initGL() {
    constexpr const uint16_t RestartIndex = std::numeric_limits<uint16_t>::max();

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RestartIndex);

    glGenVertexArrays(1, &geometry.vao);
    glGenBuffers(1, &geometry.vbo);
    glGenBuffers(1, &geometry.ibo);

    glBindVertexArray(geometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.ibo);

    std::vector<Vertex> geoVertices;
    std::vector<uint16_t> geoIndices;
    uint16_t idx = 0;
    // First the horizontal lines
    for (float e = 0.f; e <= 90.f; e += 2.25f) {
        Vertex vertex;

        const float elevation = glm::radians(e);

        const float y = radius * sin(elevation);

        uint16_t firstIndex = idx;
        for (float a = 0.f; a < 360.f; a += 2.25f) {
            const float azimuth = glm::radians(a);
            const float x = radius * cos(elevation) * sin(azimuth);
            const float z = -radius * cos(elevation) * cos(azimuth);

            vertex.x = x;
            vertex.y = y;
            vertex.z = z;
            geoVertices.push_back(vertex);
            geoIndices.push_back(idx);
            ++idx;
        }
        geoIndices.push_back(firstIndex);
        geoIndices.push_back(RestartIndex);
    }
    geoIndices.push_back(RestartIndex);

    // Then the vertical lines
    for (float a = 0.f; a < 360.f; a += 2.25f) {
        Vertex vertex;

        const float azimuth = glm::radians(a);
        for (float e = 0.f; e <= 90.f; e += 2.25f) {
            const float elevation = glm::radians(e);
            const float x = radius * cos(elevation) * sin(azimuth);
            const float y = radius * sin(elevation);
            const float z = -radius * cos(elevation) * cos(azimuth);

            vertex.x = x;
            vertex.y = y;
            vertex.z = z;
            geoVertices.push_back(vertex);
            geoIndices.push_back(idx);
            idx++;
        }
        geoIndices.push_back(RestartIndex);
    }


    nVertices = static_cast<int>(geoIndices.size());
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        nullptr
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        geoVertices.size() * sizeof(Vertex),
        geoVertices.data(),
        GL_STATIC_DRAW
    );
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        geoIndices.size() * sizeof(uint16_t),
        geoIndices.data(),
        GL_STATIC_DRAW
    );

    glBindVertexArray(0);
    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);


    ShaderManager::instance()->addShaderProgram(
        "simple",
        vertexShader,
        fragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    ShaderManager::instance()->bindShaderProgram("simple");
    const ShaderProgram& gProg = ShaderManager::instance()->getShaderProgram("simple");
    matrixLocation = gProg.getUniformLocation("matrix");
    ShaderManager::instance()->unBindShaderProgram();
}

void keyboardCallback(int key, int, int action, int) {
    if (key == key::I && action == action::Press) {
        showId.setVal(!showId.getVal());
    }
}

void encode() {
    SharedData::instance()->writeBool(showId);
}

void decode() {
    SharedData::instance()->readBool(showId);
}

void cleanUp() {
    glDeleteVertexArrays(1, &geometry.vao);
    glDeleteBuffers(1, &geometry.vbo);
    glDeleteBuffers(1, &geometry.ibo);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    MessageHandler::instance()->setNotifyLevel(MessageHandler::Level::NotifyAll);

    // parse arguments
    for (int i = 0; i < argc; i++) {
        std::string_view argg = argv[i];
        if (argg == "-tilt" && argc > (i + 1)) {
            tilt = static_cast<float>(atof(argv[i + 1]));
            MessageHandler::instance()->printInfo("Setting tilt to: %f", tilt);
        }
        else if (argg == "-radius" && argc > (i + 1)) {
            radius = static_cast<float>(atof(argv[i + 1]));
            MessageHandler::instance()->printInfo("Setting radius to: %f", radius);
        }
    }

    Settings::instance()->setCaptureFromBackBuffer(true);

    gEngine->setDrawFunction(draw);
    gEngine->setInitOGLFunction(initGL);
    gEngine->setCleanUpFunction(cleanUp);
    gEngine->setKeyboardCallbackFunction(keyboardCallback);
    SharedData::instance()->setEncodeFunction(encode);
    SharedData::instance()->setDecodeFunction(decode);

    // Init the engine
    if (!gEngine->init(Engine::RunMode::Default_Mode, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
