#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string_view>

namespace {
    GLuint vao = 0;
    GLuint vbo = 0;

    GLsizei numberOfVertices = 0;

    unsigned int textureId = 0;

    // shader locations
    GLint mvpLoc = -1;

    // variables to share across cluster
    sgct::SharedDouble currentTime(0.0);

    constexpr const char* vertexShader = R"(
  #version 330 core

  layout (location = 0) in vec3 vertPositions;
  layout (location = 1) in vec2 texCoords;

  out vec2 uv;

  uniform mat4 mvp;

  void main() {
    gl_Position = mvp * vec4(vertPositions, 1.0);
    uv = texCoords;
  })";

    constexpr const char* fragmentShader = R"(
  #version 330 core

  in vec2 uv;
  out vec4 color;

  uniform sampler2D tex;

  void main() { color = texture(tex, uv); }
)";

    struct Vertex {
        float x, y, z;
        float s, t;
    };

    std::vector<Vertex> loadOBJ(const std::string& path) {
        sgct::MessageHandler::printInfo("Loading OBJ file %s", path.c_str());


        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        std::vector<int> vertexIndices;
        std::vector<int> uvIndices;

        FILE* file = fopen(path.c_str(), "r");
        if (file == nullptr) {
            throw std::runtime_error("Cannot open file");
        }

        constexpr const int LineHeaderStrSize = 128;
        char lineHeader[LineHeaderStrSize];

        while (true) {
            // read the first word of the line
            int res = fscanf(file, "%s", lineHeader);
            // EOF = End Of File. Quit the loop.
            if (res == EOF) {
                break; 
            }

            std::string_view line = lineHeader;
            if (line == "v") {
                glm::vec3 vertex;
                fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
                positions.push_back(vertex);
            }
            else if (line == "vt") {
                glm::vec2 uv;
                fscanf(file, "%f %f\n", &uv.x, &uv.y);
                uvs.push_back(uv);
            }
            else if (line == "f") {
                unsigned int vIdx0;
                unsigned int vIdx1;
                unsigned int vIdx2;

                unsigned int uvIdx0;
                unsigned int uvIdx1;
                unsigned int uvIdx2;

                unsigned int dummy;

                constexpr const int FaceCoordSize = 64;
                char face0[FaceCoordSize];
                char face1[FaceCoordSize];
                char face2[FaceCoordSize];

                int matches = fscanf(file, "%s %s %s\n", face0, face1, face2);
                if (matches != 3) {
                    throw std::runtime_error(
                        "Parser can only read simple triangle meshes are supported"
                    );
                }
                // vertex + uv + normal valid
                if (sscanf(face0, "%u/%u/%u", &vIdx0, &uvIdx0, &dummy) == 3) {
                    if (sscanf(face1, "%u/%u/%u", &vIdx1, &uvIdx1, &dummy) != 3) {
                        throw std::runtime_error("Parser can't read 2nd face index");
                    }
                    
                    if (sscanf(face2, "%u/%u/%u", &vIdx2, &uvIdx2, &dummy) != 3) {
                        throw std::runtime_error("Parser can't read 3rd face index");
                    }

                    vertexIndices.push_back(vIdx0);
                    vertexIndices.push_back(vIdx1);
                    vertexIndices.push_back(vIdx2);
                    uvIndices.push_back(uvIdx0);
                    uvIndices.push_back(uvIdx1);
                    uvIndices.push_back(uvIdx2);
                }
                else if (sscanf(face0, "%u/%u//", &vIdx0, &uvIdx0) == 2) {
                    if (sscanf(face1, "%u/%u//", &vIdx1, &uvIdx1) != 2) {
                        throw std::runtime_error("Parser can't read 2nd face index");
                    }
                    
                    if (sscanf(face2, "%u/%u//", &vIdx2, &uvIdx2) != 2) {
                        throw std::runtime_error("Parser can't read 3rd face index");
                    }

                    vertexIndices.push_back(vIdx0);
                    vertexIndices.push_back(vIdx1);
                    vertexIndices.push_back(vIdx2);
                    uvIndices.push_back(uvIdx0);
                    uvIndices.push_back(uvIdx1);
                    uvIndices.push_back(uvIdx2);
                }

                // vertex only
                else if (sscanf(face0, "%u///", &vIdx0) == 1) {
                    if (sscanf(face1, "%u///", &vIdx1) != 1) {
                        throw std::runtime_error("Parser can't read 2nd face index");
                    }
                    
                    if (sscanf(face2, "%u///", &vIdx2) != 1) {
                        throw std::runtime_error("Parser can't read 3rd face index");
                    }

                    vertexIndices.push_back(vIdx0);
                    vertexIndices.push_back(vIdx1);
                    vertexIndices.push_back(vIdx2);
                }
                else  {
                    throw std::runtime_error("Error; unknown face format");
                }
            }
            else {
                // Probably a comment, eat up the rest of the line
                char DummyBuffer[1000];
                fgets(DummyBuffer, 1000, file);
            }
        }

        // For each vertex of each triangle
        assert(vertexIndices.size() == uvIndices.size());
        std::vector<Vertex> vertices;
        for (unsigned int i = 0; i < vertexIndices.size(); i++) {
            Vertex v;
            v.x = positions[vertexIndices[i] - 1].x;
            v.y = positions[vertexIndices[i] - 1].y;
            v.z = positions[vertexIndices[i] - 1].z;
            v.s = uvs[uvIndices[i] - 1].s;
            v.t = uvs[uvIndices[i] - 1].t;
            vertices.push_back(v);
        }

        return vertices;
    }

    /// Loads obj model and uploads to the GPU 
    void loadModel(const std::string& filename) {
        std::vector<Vertex> vertices = loadOBJ(filename.c_str());
        if (vertices.empty()) {
            sgct::MessageHandler::printInfo("Failed to load model %s", filename.c_str());
        }
        // store the number of triangles
        numberOfVertices = static_cast<GLsizei>(vertices.size());

        // create VAO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            vertices.size() * sizeof(Vertex),
            vertices.data(),
            GL_STATIC_DRAW
        );

        // first attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(3 * sizeof(float))
        );

        glBindVertexArray(0);

        sgct::MessageHandler::printInfo(
            "Model '%s' loaded successfully (%u vertices, VAO: %u, VBOs: %u %u).",
            filename.c_str(), numberOfVertices, vao, vbo
        );
    }
} // namespace

using namespace sgct;

void drawFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    constexpr const double Speed = 0.44;

    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * Speed),
        glm::vec3(0.f, -1.f, 0.f)
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    ShaderManager::instance().getShaderProgram("xform").bind();

    const glm::mat4 mvp = Engine::instance().getCurrentModelViewProjectionMatrix() * scene;
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, numberOfVertices);
    glBindVertexArray(0);

    ShaderManager::instance().getShaderProgram("xform").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (Engine::instance().isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void initOGLFun() {
    textureId = TextureManager::instance().loadTexture("box.png", true, 4.f);
    loadModel("box.obj");

    ShaderManager::instance().addShaderProgram("xform", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().getShaderProgram("xform");
    prog.bind();
    mvpLoc = glGetUniformLocation(prog.getId(), "mvp");
    glUniform1i(glGetUniformLocation(prog.getId(), "tex"), 0);
    prog.unbind();
}

void encodeFun() {
    SharedData::instance().writeDouble(currentTime);
}

void decodeFun() {
    SharedData::instance().readDouble(currentTime);
}

void cleanUpFun() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);

    Engine::instance().setInitOGLFunction(initOGLFun);
    Engine::instance().setDrawFunction(drawFun);
    Engine::instance().setPreSyncFunction(preSyncFun);
    Engine::instance().setCleanUpFunction(cleanUpFun);
    Engine::instance().setEncodeFunction(encodeFun);
    Engine::instance().setDecodeFunction(decodeFun);

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
