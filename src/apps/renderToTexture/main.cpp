#include <sgct.h>
#include <sgct/SGCTWindow.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine * gEngine;

    struct FramebufferData {
        unsigned int texture;
        unsigned int fbo;
        unsigned int renderBuffer;
        unsigned int depthBuffer;
        glm::ivec2 size;
    };

    std::vector<FramebufferData> buffers;
    std::unique_ptr<sgct_utils::SGCTBox> box;

    //variables to share across cluster
    sgct::SharedDouble currentTime(0.0);

} // namespace

using namespace sgct;

void drawScene() {
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    constexpr const double Speed = 25.0;

    glPushMatrix();
    glTranslatef(0.f, 0.f, -3.f);
    glRotated(currentTime.getVal() * Speed, 0.0, -1.0, 0.0);
    glRotated(currentTime.getVal() * (Speed / 2.0), 1.0, 0.0, 0.0);
    glColor3f(1.f, 1.f, 1.f);
    glBindTexture(GL_TEXTURE_2D, TextureManager::instance()->getTextureId("box"));

    box->draw();
    glPopMatrix();
    glPopAttrib();
}

void clearBuffers() {
    for (unsigned int i = 0; i < buffers.size(); i++) {
        glDeleteFramebuffers(1, &(buffers[i].fbo));
        glDeleteRenderbuffers(1, &(buffers[i].renderBuffer));
        glGenRenderbuffers(1, &(buffers[i].depthBuffer));

        glDeleteTextures(1, &(buffers[i].texture));

        buffers[i].fbo = 0;
        buffers[i].renderBuffer = 0;
        buffers[i].depthBuffer = 0;
        buffers[i].texture = 0;
        buffers[i].size = glm::ivec2(1, 1);
    }
}

void createTextures() {
    glEnable(GL_TEXTURE_2D);

    for (unsigned int i = 0; i < buffers.size(); i++) {
        glGenTextures(1, &buffers[i].texture);
        glBindTexture(GL_TEXTURE_2D, buffers[i].texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            buffers[i].size.x,
            buffers[i].size.y,
            0,
            GL_BGRA,
            GL_UNSIGNED_BYTE,
            nullptr
        );
    }

    gEngine->checkForOGLErrors();
    glDisable(GL_TEXTURE_2D);
}

void createFBOs() {
    for (size_t i = 0 ; i < buffers.size(); ++i) {
        buffers[i].fbo = 0;
        buffers[i].renderBuffer = 0;
        buffers[i].depthBuffer = 0;
        buffers[i].texture = 0;
        buffers[i].size = gEngine->getDrawBufferSize(i);
    }

    createTextures();

    for (FramebufferData& data : buffers) {
        glGenFramebuffers(1, &data.fbo);
        glGenRenderbuffers(1, &data.renderBuffer);
        glGenRenderbuffers(1, &data.depthBuffer);

        // setup color buffer
        glBindFramebuffer(GL_FRAMEBUFFER, data.fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, data.renderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, data.size.x, data.size.y);
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_RENDERBUFFER,
            data.renderBuffer
        );

        //setup depth buffer
        glBindFramebuffer(GL_FRAMEBUFFER, data.fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, data.depthBuffer);
        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH_COMPONENT32,
            data.size.x,
            data.size.y
        );
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER,
            data.depthBuffer
        );

        // Does the GPU support current FBO configuration?
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            MessageHandler::instance()->print("Something went wrong creating FBO\n");
        }

        // unbind
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void resizeFBOs() {
    MessageHandler::instance()->print("Re-sizing buffers\n");
    clearBuffers();
    createFBOs();
}

void drawFun() {
    sgct_core::OffScreenBuffer* fbo = gEngine->getCurrentFBO();
    size_t drawIndex = gEngine->getCurrentDrawBufferIndex();
    
    // get viewport data and set the viewport
    glViewport(0, 0, buffers[drawIndex].size.x, buffers[drawIndex].size.y);

    // bind fbo
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, buffers[drawIndex].fbo);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        buffers[drawIndex].texture,
        0
    );

    glMatrixMode(GL_PROJECTION);

    glLoadMatrixf(glm::value_ptr(gEngine->getCurrentViewProjectionMatrix()));

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(gEngine->getModelMatrix()));

    // clear
    glClearColor(0.f, 0.f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw scene to texture target
    drawScene();

    if (fbo) {
        fbo->bind();
    }

    // render a quad in ortho/2D mode with target texture
    // set viewport
    glm::ivec4 coords = gEngine->getCurrentViewportPixelCoords();
    glViewport(coords.x, coords.y, coords.z, coords.z);
    
    // enter ortho mode (2D projection)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glPushMatrix();
    glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 2.0);
    glMatrixMode(GL_MODELVIEW);

    ShaderManager::instance()->bindShaderProgram("InvertColor");
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glLoadIdentity();

    glBindTexture(GL_TEXTURE_2D, buffers[drawIndex].texture);

    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f);
    glVertex2f(0.f, 0.f);
    glTexCoord2f(1.f, 0.f);
    glVertex2f(1.f, 0.f);
    glTexCoord2f(1.f, 1.f);
    glVertex2f(1.f, 1.f);
    glTexCoord2f(0.f, 1.f);
    glVertex2f(0.f, 1.f);
    glEnd();

    glPopAttrib();
    sgct::ShaderManager::instance()->unBindShaderProgram();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currentTime.setVal(Engine::getTime());
    }
}

void postSyncPreDrawFun() {
    // Fisheye cubemaps are constant size
    for (unsigned int i = 0; i < gEngine->getNumberOfWindows(); i++) {
        if (gEngine->getWindow(i).isWindowResized()) {
            resizeFBOs();
            break;
        }
    }
}

void initOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    box = std::make_unique<sgct_utils::SGCTBox>(
        1.f,
        sgct_utils::SGCTBox::TextureMappingMode::Regular
    );

    // set up shader
    ShaderManager::instance()->addShaderProgram(
        "InvertColor",
        "simple.vert",
        "simple.frag"
    );
    ShaderManager::instance()->bindShaderProgram("InvertColor");
    ShaderManager::instance()->unBindShaderProgram();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    size_t numberOfBuffers = gEngine->getNumberOfDrawBuffers();
    for (size_t i = 0; i < numberOfBuffers; i++) {
        FramebufferData tmpBuffer;
        buffers.push_back(tmpBuffer);
    }

    createFBOs();
}

void cleanUpFun() {
    box = nullptr;
    
    clearBuffers();
    buffers.clear();
}

void encodeFun() {
    SharedData::instance()->writeDouble(currentTime);
}

void decodeFun() {
    SharedData::instance()->readDouble(currentTime);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setCleanUpFunction(cleanUpFun);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
