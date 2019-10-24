#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#include <sgct/postfx.h>
#include <sgct/window.h>
#include <sgct/utils/box.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct::utils::Box> box;
    sgct::SharedDouble currentTime(0.0);

    GLint matrixLoc = -1;

    struct {
        GLint pass1 = -1;
        GLint pass2 = -1;
        GLint pass3 = -1;
        GLint pass4 = -1;
    } postFXTextureLocation;
    GLint originalTextureLocation = -1;
    struct {
        GLint pass2 = -1;
        GLint pass3 = -1;
    } sizeLocation;

    std::string loadFile(const std::string& filename) {
        std::ifstream vertexFile(filename);
        std::string vertexString(
            (std::istreambuf_iterator<char>(vertexFile)),
            std::istreambuf_iterator<char>()
        );
        return vertexString;
    }
} // namespace

using namespace sgct;

void setupPostFXs() {
    {
        PostFX fx(
            "Threshold",
            loadFile("base.vert"),
            loadFile("threshold.frag"),
            [](){ glUniform1i(postFXTextureLocation.pass1, 0); }
        );
        const ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass1 = sp.getUniformLocation("tex");
        originalTextureLocation = sp.getUniformLocation("texOrig");
        sp.unbind();
        gEngine->getCurrentWindow().addPostFX(std::move(fx));
    }

    {
        PostFX fx(
            "HBlur",
            loadFile("blur_h.vert"),
            loadFile("blur.frag"),
            [](){
                glUniform1i(postFXTextureLocation.pass2, 0);
                glUniform1f(
                    sizeLocation.pass2,
                    static_cast<float>(gEngine->getCurrentResolution().x)
                );
            }
        );
        const ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass2 = sp.getUniformLocation("tex");
        sizeLocation.pass2 = sp.getUniformLocation("size");
        sp.unbind();
        gEngine->getCurrentWindow().addPostFX(std::move(fx));
    }

    {
        PostFX fx(
            "VBlur",
            loadFile("blur_v.vert"),
            loadFile("blur.frag"),
            [](){
                glUniform1i(postFXTextureLocation.pass3, 0);
                glUniform1f(
                    sizeLocation.pass3,
                    static_cast<float>(gEngine->getCurrentResolution().y)
                );
            }
        );
        const ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass3 = sp.getUniformLocation("tex");
        sizeLocation.pass3 = sp.getUniformLocation("size");
        sp.unbind();
        gEngine->getCurrentWindow().addPostFX(std::move(fx));
    }

    {
        PostFX fx(
            "Glow",
            loadFile("base.vert"),
            loadFile("glow.frag"),
            [](){
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, gEngine->getCurrentDrawTexture());
                glUniform1i(postFXTextureLocation.pass4, 0);
                glUniform1i(originalTextureLocation, 1);
            }
        );
        const ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass4 = sp.getUniformLocation("tex");
        originalTextureLocation = sp.getUniformLocation("texOrig");
        sp.unbind();
        gEngine->getCurrentWindow().addPostFX(std::move(fx));
    }
}

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
    scene = glm::rotate(
        scene,
        static_cast<float>(currentTime.getVal() * (Speed / 2.0)),
        glm::vec3(1.f, 0.f, 0.f)
    );

    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix() * scene;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));

    ShaderManager::instance()->getShaderProgram("xform").bind();

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    box->draw();
    ShaderManager::instance()->getShaderProgram("xform").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "box.png", true);

    box = std::make_unique<utils::Box>(2.f, utils::Box::TextureMappingMode::Regular);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance()->addShaderProgram(
        "xform",
        loadFile("simple.vert"),
        loadFile("simple.frag")
    );
    const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("xform");
    prog.bind();
    matrixLoc = prog.getUniformLocation("mvp");
    GLint textureLocation = prog.getUniformLocation("tex");
    glUniform1i(textureLocation, 0);
    prog.unbind();

    setupPostFXs();
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
}

void cleanUpFun() {
    box = nullptr;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);
    gEngine = Engine::instance();

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setEncodeFunction(encodeFun);
    gEngine->setDecodeFunction(decodeFun);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile, cluster)) {
        Engine::destroy();
        return EXIT_FAILURE;
    }

    gEngine->render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
