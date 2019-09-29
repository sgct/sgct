#include <sgct.h>
#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/image.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <sgct/shareddata.h>
#include <sgct/shareddatatypes.h>
#include <sgct/texturemanager.h>
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
    } geometry;

    sgct::SharedBool showId(false);

    bool isTiltSet = false;
    float tilt = 0.f;
    float radius = 7.4f;

    struct Vertex {
        float x, y, z;
        float r, g, b, a;
    };
    int nVertices = 0;
    GLint tiltLocation = -1;
} // namespace

using namespace sgct;

void draw() {
    glDepthMask(GL_FALSE);

    ShaderManager::instance()->bindShaderProgram("simple");

    glBindVertexArray(geometry.vao);
    glDrawArrays(GL_LINE_LOOP, 0, nVertices);
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
    glGenVertexArrays(1, &geometry.vao);
    glGenBuffers(1, &geometry.vbo);

    glBindVertexArray(geometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);

    std::vector<Vertex> geoVertices;
    for (float e = 0.f; e <= 90.f; e += 2.25f) {
        Vertex vertex;

        const float elevation = glm::radians(e);

        if (static_cast<int>(e) % 9 == 0) {
            vertex.r = 1.f;
            vertex.g = 1.f;
            vertex.b = 0.f;
            vertex.a = 0.5f;
        }
        else {
            vertex.r = 1.f;
            vertex.g = 1.f;
            vertex.b = 1.f;
            vertex.a = 0.5f;
        }

        const float y = radius * sin(elevation);

        for (float a = 0.f; a < 360.f; a += 2.25f) {
            const float azimuth = glm::radians(a);
            const float x = radius * cos(elevation) * sin(azimuth);
            const float z = -radius * cos(elevation) * cos(azimuth);

            vertex.x = x;
            vertex.y = y;
            vertex.z = z;
            geoVertices.push_back(vertex);
        }
    }
    nVertices = static_cast<int>(geoVertices.size());
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        0
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, r))
    );

    glBufferData(
        GL_VERTEX_ARRAY,
        geoVertices.size() * sizeof(Vertex),
        geoVertices.data(),
        GL_STATIC_DRAW
    );
    glBindVertexArray(0);
    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);


    ShaderManager::instance()->addShaderProgram("simple", "simple.vert", "simple.frag");
    ShaderManager::instance()->bindShaderProgram("simple");
    const ShaderProgram& gProg = ShaderManager::instance()->getShaderProgram("simple");
    tiltLocation = gProg.getUniformLocation("tilt");
    ShaderManager::instance()->unBindShaderProgram();
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
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    MessageHandler::instance()->setNotifyLevel(MessageHandler::Level::NotifyAll);

    // parse arguments
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-tilt") == 0 && argc > (i + 1)) {
            tilt = static_cast<float>(atof(argv[i + 1]));
            isTiltSet = true;

            MessageHandler::instance()->print("Setting tilt to: %f\n", tilt);
        }
        else if (strcmp(argv[i], "-radius") == 0 && argc > (i + 1)) {
            radius = static_cast<float>(atof(argv[i + 1]));
            isTiltSet = true;

            MessageHandler::instance()->print("Setting radius to: %f\n", radius);
        }
    }

    Settings::instance()->setCaptureFromBackBuffer(true);

    gEngine->setDrawFunction(draw);
    gEngine->setInitOGLFunction(initGL);
    gEngine->setCleanUpFunction(cleanUp);
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
