/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>

#include <numeric>

namespace {
    struct {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint iboLine = 0;
        int nVertLine = 0;
    } gridGeometry;

    bool takeScreenshot = false;
    bool captureBackbuffer = false;
    bool renderGrid = true;
    bool renderTestBox = false;

    float radius = 7.4f;

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

void initGL(GLFWwindow*) {
    constexpr const uint16_t RestartIndex = std::numeric_limits<uint16_t>::max();
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RestartIndex);

    glGenVertexArrays(1, &gridGeometry.vao);
    glGenBuffers(1, &gridGeometry.vbo);
    glGenBuffers(1, &gridGeometry.iboLine);

    glBindVertexArray(gridGeometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gridGeometry.vbo);

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

        gridGeometry.nVertLine = static_cast<int>(indices.size());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridGeometry.iboLine);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(uint16_t),
            indices.data(),
            GL_STATIC_DRAW
        );
    }
    glBindVertexArray(0);

    //if (!texture.empty()) {
    //    textureId = TextureManager::instance().loadTexture(texture, true, 0);
    //}

    ShaderManager::instance().addShaderProgram("simple", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("simple");
    prog.bind();
    matrixLocation = glGetUniformLocation(prog.id(), "matrix");
    glUniform1f(glGetUniformLocation(prog.id(), "radius"), radius);

    prog.unbind();
}

void postSyncPreDraw() {
    Settings::instance().setCaptureFromBackBuffer(captureBackbuffer);
    if (takeScreenshot) {
        Engine::instance().takeScreenshot();
        takeScreenshot = false;
    }
}

void drawGrid() {
    glBindVertexArray(gridGeometry.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridGeometry.iboLine);
    glDrawElements(GL_LINE_STRIP, gridGeometry.nVertLine, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

void draw(const RenderData& data) {
    ShaderManager::instance().shaderProgram("simple").bind();
    const glm::mat4 mvp = data.modelViewProjectionMatrix;
    glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, glm::value_ptr(mvp));

    drawGrid();

    ShaderManager::instance().shaderProgram("simple").unbind();
}

void postDraw() {
    //if (Engine::instance().isMaster()) {
    //    if (Engine::instance().currentFrameNumber() == 10) {
    //        Log::Info("Taking first screenshot");
    //        takeScreenshot = true;
    //    }

    //    if (Engine::instance().currentFrameNumber() == 15) {
    //        Log::Info("Capturing from Back buffer");
    //        captureBackbuffer = true;
    //    }

    //    if (Engine::instance().currentFrameNumber() == 20) {
    //        Log::Info("Taking second screenshot");
    //        takeScreenshot = true;
    //    }

    //    if (Engine::instance().currentFrameNumber() == 25) {
    //        Engine::instance().terminate();
    //    }
    //}
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, takeScreenshot);
    serializeObject(data, captureBackbuffer);
    serializeObject(data, renderGrid);
    serializeObject(data, renderTestBox);
    return data;
}

void decode(const std::vector<std::byte>& data, unsigned int pos) {
    deserializeObject(data, pos, takeScreenshot);
    deserializeObject(data, pos, captureBackbuffer);
    deserializeObject(data, pos, renderGrid);
    deserializeObject(data, pos, renderTestBox);
}

void cleanUp() {
    glDeleteVertexArrays(1, &gridGeometry.vao);
    glDeleteBuffers(1, &gridGeometry.vbo);
    glDeleteBuffers(1, &gridGeometry.iboLine);
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    Engine::Callbacks callbacks;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.postDraw = postDraw;
    callbacks.initOpenGL = initGL;
    callbacks.cleanUp = cleanUp;
    callbacks.encode = encode;
    callbacks.decode = decode;

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
