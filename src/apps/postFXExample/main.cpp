#include <sgct.h>
#include <sgct/postfx.h>
#include <sgct/window.h>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct::utils::Box> box;
    sgct::SharedDouble currentTime(0.0);

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
        postFXTextureLocation.pass1 = sp.getUniformLocation("tex");
        originalTextureLocation = sp.getUniformLocation("texOrig");
        sp.unbind();
        gEngine->addPostFX(std::move(fx));
    }

    {
        PostFX fx;
        fx.init("HBlur", "blur_h.vert", "blur.frag");
        fx.setUpdateUniformsFunction(updatePass2);
        ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass2 = sp.getUniformLocation("tex");
        sizeLocation.pass2 = sp.getUniformLocation("size");
        sp.unbind();
        gEngine->addPostFX(std::move(fx));
    }

    {
        PostFX fx;
        fx.init("VBlur", "blur_v.vert", "blur.frag");
        fx.setUpdateUniformsFunction(updatePass3);
        ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass3 = sp.getUniformLocation("tex");
        sizeLocation.pass3 = sp.getUniformLocation("size");
        sp.unbind();
        gEngine->addPostFX(std::move(fx));
    }

    {
        PostFX fx;
        fx.init("Glow", "base.vert", "glow.frag");
        fx.setUpdateUniformsFunction(updatePass4);
        ShaderProgram& sp = fx.getShaderProgram();
        sp.bind();
        postFXTextureLocation.pass4 = sp.getUniformLocation("tex");
        originalTextureLocation = sp.getUniformLocation("texOrig");
        sp.unbind();
        gEngine->addPostFX(std::move(fx));
    }

    if (gEngine->getNumberOfWindows() > 1) {
        gEngine->getWindow(1).setUsePostFX(false);
    }
}

void drawFun() {
    constexpr const double Speed = 25.0;
    
    glTranslatef(0.f, 0.f, -3.f);
    glRotated(currentTime.getVal() * Speed, 0.0, -1.0, 0.0);
    glRotated(currentTime.getVal() * (Speed  / 2.0), 1.0, 0.0, 0.0);
    glColor3f(1.f, 1.f, 1.f);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));
    
    box->draw();
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "box.png", true);

    box = std::make_unique<sgct::utils::Box>(
        2.f,
        sgct::utils::Box::TextureMappingMode::Regular
    );

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

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
    gEngine = new Engine(config);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setCleanUpFunction(cleanUpFun);

    if (!gEngine->init(Engine::RunMode::Default_Mode, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
