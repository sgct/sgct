#include <sgct/clustermanager.h>
#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/image.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/shareddatatypes.h>
#include <sgct/texturemanager.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <numeric>

#ifdef SGCT_HAS_TEXT
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT

namespace {
    struct {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint iboLine = 0;
        int nVertLine = 0;
        GLuint iboTriangle = 0;
        int nVertTriangle = 0;
    } geometry;

    sgct::SharedBool takeScreenshot(false);
    sgct::SharedBool captureBackbuffer(false);

    float radius = 7.4f;
    std::string texture;
    unsigned int textureId = 0;

    struct Vertex {
        float elevation, azimuth;
    };
    GLint matrixLocation = -1;

    constexpr const char* vertexShader = R"(
#version 330 core

layout(location = 0) in vec2 vertPosition;

out vec4 color;

uniform float radius;
uniform mat4 matrix;

const float PI = 3.1415926;
const float PI_HALF = PI / 2.0;

void main() {
  float elevation = vertPosition[0];
  float azimuth = vertPosition[1];

  vec3 p = vec3(
    radius * cos(elevation) * sin(azimuth),
    radius * sin(elevation),
    -radius * cos(elevation) * cos(azimuth)
  );
  gl_Position = matrix * vec4(p, 1.0);
  color = vec4(p, 1.0);
}
)";

    constexpr const char* fragmentShader = R"(
#version 330 core

in vec4 color;
out vec4 FragOut;

void main() { FragOut = color; }
)";
} // namespace

using namespace sgct;

void draw() {
    ShaderManager::instance().getShaderProgram("simple").bind();
    const glm::mat4 mvp = Engine::instance().getCurrentModelViewProjectionMatrix();
    glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(geometry.vao);
    if (textureId != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboTriangle);
        glDrawElements(GL_TRIANGLES, geometry.nVertTriangle, GL_UNSIGNED_SHORT, nullptr);
    }
    else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboLine);
        glDrawElements(GL_LINE_STRIP, geometry.nVertLine, GL_UNSIGNED_SHORT, nullptr);
    }
    glBindVertexArray(0);

    ShaderManager::instance().getShaderProgram("simple").unbind();
}

void initGL() {
    constexpr const uint16_t RestartIndex = std::numeric_limits<uint16_t>::max();
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RestartIndex);

    glGenVertexArrays(1, &geometry.vao);
    glGenBuffers(1, &geometry.vbo);
    glGenBuffers(1, &geometry.iboLine);
    glGenBuffers(1, &geometry.iboTriangle);

    glBindVertexArray(geometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);

    constexpr const int ElevationSteps = 40;
    constexpr const int AzimuthSteps = 160;

    std::vector<Vertex> vertices;
    // (abock, 2019-10-09) We generate the vertices ring-wise;  first iterating over the
    // elevation and then the azimuth will lead to the bottom most ring be filled first,
    // before going to the upper rings.  That also means that two vertices on top of each
    // other should be separated in the vertices list by 'AzimuthSteps' positions
    for (int e = 0; e <= ElevationSteps; ++e) {
        for (int a = 0; a < AzimuthSteps; ++a) {
            Vertex vertex;
            float ev = static_cast<float>(e) / static_cast<float>(ElevationSteps - 1);
            float av = static_cast<float>(a) / static_cast<float>(AzimuthSteps - 1);
            vertex.elevation = glm::radians(ev * 90.f);
            vertex.azimuth = glm::radians(av * 360.f);
            vertices.push_back(vertex);
        }
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    {
        // Line representation
        // (abock, 2019-10-09) It's possible to compute a better size of how many indices
        // we'll need, but I can't be asked (and we are seriously not performance limited)
        std::vector<uint16_t> indices;
        indices.reserve(2 * ElevationSteps * AzimuthSteps);

        // First the horizontal, azimuth lines
        for (int e = 0; e < ElevationSteps; ++e ) {
            std::vector<uint16_t> t(AzimuthSteps);
            std::iota(t.begin(), t.end(), static_cast<uint16_t>(e * AzimuthSteps));
            t.push_back(static_cast<uint16_t>(e * AzimuthSteps)); // close the ring
            t.push_back(RestartIndex); // restart for the next ring
            indices.insert(indices.end(), t.begin(), t.end());
        }
        indices.push_back(RestartIndex);
        // Then the vertical lines; see above; every vertical vertex is separated by
        // exactly 'AzimuthSteps' positions in the vertex array
        for (int a = 0; a < AzimuthSteps; ++a) {
            for (int e = 0; e < ElevationSteps; ++e) {
                indices.push_back(static_cast<uint16_t>(a + e * AzimuthSteps));
            }
            indices.push_back(RestartIndex);
        }

        geometry.nVertLine = static_cast<int>(indices.size());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboLine);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(uint16_t),
            indices.data(),
            GL_STATIC_DRAW
        );
    }
    {
        // Triangle representation
        std::vector<uint16_t> indices;
        for (int e = 0; e < ElevationSteps; ++e) {
            for (int a = 0; a < AzimuthSteps; ++a) {
                const uint16_t base = static_cast<uint16_t>(e * AzimuthSteps + a);
                // first triangle
                indices.push_back(base);
                indices.push_back(base + 1);
                indices.push_back(base + AzimuthSteps);

                //// second triangle
                indices.push_back(base + 1);
                indices.push_back(base + AzimuthSteps + 1);
                indices.push_back(base + AzimuthSteps);
            }
        }
        geometry.nVertTriangle = static_cast<int>(indices.size());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.iboTriangle);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(uint16_t),
            indices.data(),
            GL_STATIC_DRAW
        );
    }
    glBindVertexArray(0);

    if (!texture.empty()) {
        textureId = TextureManager::instance().loadTexture(texture, true, 0);
    }

    ShaderManager::instance().addShaderProgram("simple", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().getShaderProgram("simple");
    prog.bind();
    matrixLocation = prog.getUniformLocation("matrix");

    glUniform1f(prog.getUniformLocation("radius"), radius);

    prog.unbind();
}

void postSyncPreDraw() {
    Settings::instance().setCaptureFromBackBuffer(captureBackbuffer.getVal());
    if (takeScreenshot.getVal()) {
        Engine::instance().takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void postDraw() {
    if (Engine::instance().isMaster()) {
        if (Engine::instance().getCurrentFrameNumber() == 10) {
            MessageHandler::printInfo("Taking first screenshot");
            takeScreenshot.setVal(true);
        }

        if (Engine::instance().getCurrentFrameNumber() == 15) {
            MessageHandler::printInfo("Capturing from Back buffer");
            captureBackbuffer.setVal(true);
        }

        if (Engine::instance().getCurrentFrameNumber() == 20) {
            MessageHandler::printInfo("Taking second screenshot");
            takeScreenshot.setVal(true);
        }

        if (Engine::instance().getCurrentFrameNumber() == 25) {
            Engine::instance().terminate();
        }
    }
}

void encode() {
    SharedData::instance().writeBool(takeScreenshot);
    SharedData::instance().writeBool(captureBackbuffer);
}

void decode() {
    SharedData::instance().readBool(takeScreenshot);
    SharedData::instance().readBool(captureBackbuffer);
}

void cleanUp() {
    glDeleteVertexArrays(1, &geometry.vao);
    glDeleteBuffers(1, &geometry.vbo);
    glDeleteBuffers(1, &geometry.iboLine);
    glDeleteBuffers(1, &geometry.iboTriangle);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);

    Engine::instance().setPostSyncPreDrawFunction(postSyncPreDraw);
    Engine::instance().setDrawFunction(draw);
    Engine::instance().setPostDrawFunction(postDraw);
    Engine::instance().setInitOGLFunction(initGL);
    Engine::instance().setCleanUpFunction(cleanUp);
    Engine::instance().setEncodeFunction(encode);
    Engine::instance().setDecodeFunction(decode);

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
