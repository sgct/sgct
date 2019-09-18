#include <sgct.h>
#include <sgct/postfx.h>
#include <sgct/window.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct_utils::Box> box;
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
} // namespace

using namespace sgct;

void updatePass1() {
    glUniform1i(postFXTextureLocation.pass1, 0);
}

void updatePass2() {
    glUniform1i(postFXTextureLocation.pass2, 0);
    glUniform1f(
        sizeLocation.pass2,
        static_cast<float>(gEngine->getCurrentResolution().x)
    );
}

void updatePass3() {
    glUniform1i(postFXTextureLocation.pass3, 0);
    glUniform1f(
        sizeLocation.pass3,
        static_cast<float>(gEngine->getCurrentResolution().y)
    );
}

void updatePass4() {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gEngine->getCurrentDrawTexture());
    glUniform1i(postFXTextureLocation.pass4, 0);
    glUniform1i(originalTextureLocation, 1);
}

void setupPostFXs() {
    {
        PostFX fx;
        fx.init("Threshold", "base.vert", "threshold.frag");
        fx.setUpdateUniformsFunction(updatePass1);
        ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass1 = sp.getUniformLocation("Tex");
        originalTextureLocation = sp.getUniformLocation("TexOrig");
        sp.unbind();
        gEngine->addPostFX(std::move(fx));
    }

    {
        PostFX fx;
        fx.init("HBlur", "blur_h.vert", "blur.frag");
        fx.setUpdateUniformsFunction(updatePass2);
        ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass2 = sp.getUniformLocation("Tex");
        sizeLocation.pass2 = sp.getUniformLocation("Size");
        sp.unbind();
        gEngine->addPostFX(std::move(fx));
    }

    {
        PostFX fx;
        fx.init("VBlur", "blur_v.vert", "blur.frag");
        fx.setUpdateUniformsFunction(updatePass3);
        ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass3 = sp.getUniformLocation("Tex");
        sizeLocation.pass3 = sp.getUniformLocation("Size");
        sp.unbind();
        gEngine->addPostFX(std::move(fx));
    }

    {
        PostFX fx;
        fx.init("Glow", "base.vert", "glow.frag");
        fx.setUpdateUniformsFunction(updatePass4);
        ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass4 = sp.getUniformLocation("Tex");
        originalTextureLocation = sp.getUniformLocation("TexOrig");
        sp.unbind();
        gEngine->addPostFX(std::move(fx));
    }

    if (gEngine->getNumberOfWindows() > 1) {
        gEngine->getWindow(1).setUsePostFX(false);
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

    ShaderManager::instance()->bindShaderProgram("xform");

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    box->draw();

    sgct::ShaderManager::instance()->unBindShaderProgram();

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
    TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    box = std::make_unique<sgct_utils::Box>(
        2.f,
        sgct_utils::Box::TextureMappingMode::Regular
    );

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance()->addShaderProgram(
        "xform",
        "SimpleVertexShader.vertexshader",
        "SimpleFragmentShader.fragmentshader"
    );

    ShaderManager::instance()->bindShaderProgram("xform");

    const ShaderProgram& prog = ShaderManager::instance()->getShaderProgram("xform");
    matrixLoc = prog.getUniformLocation("MVP");
    GLint textureLocation = prog.getUniformLocation("Tex");
    glUniform1i(textureLocation, 0);

    sgct::ShaderManager::instance()->unBindShaderProgram();

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
    gEngine = new Engine(arg);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
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
