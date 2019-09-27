#include <sgct.h>
#include <sgct/engine.h>
#include <sgct/window.h>
#include <stdlib.h>
#include <stdio.h>
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

namespace {
    sgct::Engine* gEngine;

    std::unique_ptr<sgct::utils::Box> box;
    GLint matrixLoc = -1;

    sgct::SharedDouble currTime(0.0);
    sgct::SharedFloat sharedSpeed(0.44f);
    sgct::SharedBool sharedTextureOnOff(true);
    sgct::SharedObject<glm::vec3> sharedClearColor(glm::vec3(60.0f));

    bool useTexture = true;
    float speed = 0.44f;
    ImVec4 clearColor = ImColor(60, 60, 60);
    bool showSettingsWindow = true;
    bool showTestWindow = false;
} // namespace

using namespace sgct;

void myDrawFun() {

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    gEngine->setClearColor(clearColor.x, clearColor.y, clearColor.z, 1.f);

    // create scene transform (animation)
    glm::mat4 scene = glm::translate(glm::mat4(1.f), glm::vec3( 0.f, 0.f, -3.f));
    scene = glm::rotate(
        scene,
        static_cast<float>(currTime.getVal() * speed), glm::vec3(0.f, -1.f, 0.f)
    );
    scene = glm::rotate(
        scene,
        static_cast<float>(currTime.getVal() * (speed / 2.f)), glm::vec3(1.f, 0.f, 0.f)
    );

    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix() * scene;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
        GL_TEXTURE_2D,
        (useTexture ? TextureManager::instance()->getTextureId("box") : 0)
    );

    ShaderManager::instance()->bindShaderProgram("xform");

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    box->draw();
    ShaderManager::instance()->unBindShaderProgram();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void myDraw2DFun() {
    if (gEngine->isMaster()) {
        const glm::ivec2 res = gEngine->getCurrentWindow().getFramebufferResolution();
        ImGui_ImplGlfwGL3_NewFrame(res.x, res.y);

        // Show a settings window custom made for this application
        // Toggle this windows with the 'W' key.
        if (showSettingsWindow) {
            ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Settings");
            ImGui::SliderFloat("Rotation Speed", &speed, 0.f, 1.f);
            ImGui::Checkbox("Texture On/Off", &useTexture);
            ImGui::ColorEdit3("Clear Color", static_cast<float*>(&clearColor));
            if (ImGui::Button("Toggle Test Window")) {
                showTestWindow ^= 1;
            }
            ImGui::End();
        }

        // Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (showTestWindow) {
            ImGui::SetNextWindowPos(ImVec2(100, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&showTestWindow);
        }

        ImGui::Render();
    }
}

void myPreSyncFun() {
    if (gEngine->isMaster()) {
        currTime.setVal(Engine::getTime());
    }
}

void myInitOGLFun() {
    TextureManager::instance()->setAnisotropicFilterSize(8.f);
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::S3TC_DXT);
    TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    box = std::make_unique<utils::Box>(2.f, utils::Box::TextureMappingMode::Regular);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance()->addShaderProgram("xform", "simple.vert", "simple.frag");
    ShaderManager::instance()->bindShaderProgram("xform");

    const sgct::ShaderProgram& prg = ShaderManager::instance()->getShaderProgram("xform");
    matrixLoc = prg.getUniformLocation("MVP");
    GLint textureLocation = prg.getUniformLocation("Tex");
    glUniform1i(textureLocation, 0);

    ShaderManager::instance()->unBindShaderProgram();
    
    // Setup ImGui binding
    if (gEngine->isMaster()) {
        ImGui_ImplGlfwGL3_Init(gEngine->getCurrentWindow().getWindowHandle());
    }
}

void myEncodeFun() {
    SharedData::instance()->writeDouble(currTime);
    sharedSpeed.setVal(speed);
    SharedData::instance()->writeFloat(sharedSpeed);
    sharedTextureOnOff.setVal(useTexture);
    SharedData::instance()->writeBool(sharedTextureOnOff);
    sharedClearColor.setVal(glm::vec3(clearColor.x, clearColor.y, clearColor.z));
    SharedData::instance()->writeObj(sharedClearColor);
}

void myDecodeFun() {
    SharedData::instance()->readDouble(currTime);
    SharedData::instance()->readFloat(sharedSpeed);
    speed = sharedSpeed.getVal();
    SharedData::instance()->readBool(sharedTextureOnOff);
    useTexture = sharedTextureOnOff.getVal();
    SharedData::instance()->readObj(sharedClearColor);
    clearColor.x = sharedClearColor.getVal().x;
    clearColor.y = sharedClearColor.getVal().y;
    clearColor.z = sharedClearColor.getVal().z;
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster()) {
        if ((key == sgct::key::W) && (action == sgct::action::Press)) {
            showSettingsWindow = !showSettingsWindow;
        }
        ImGui_ImplGlfwGL3_KeyCallback(
            gEngine->getCurrentWindow().getWindowHandle(),
            key,
            0,
            action,
            0
        );
    }
}

void charCallback(unsigned int c, int) {
    if (gEngine->isMaster()) {
        ImGui_ImplGlfwGL3_CharCallback(gEngine->getCurrentWindow().getWindowHandle(), c);
    }
}

void mouseButtonCallback(int button, int action, int mods) {
    if (gEngine->isMaster()) {
        ImGui_ImplGlfwGL3_MouseButtonCallback(
            gEngine->getCurrentWindow().getWindowHandle(),
            button,
            action,
            0
        );
    }
}

void mouseScrollCallback(double x, double y) {
    if (gEngine->isMaster()) {
        ImGui_ImplGlfwGL3_ScrollCallback(
            gEngine->getCurrentWindow().getWindowHandle(),
            x,
            y
        );
    }
}

void myCleanUpFun() {
    box = nullptr;
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    gEngine->setInitOGLFunction(myInitOGLFun);
    gEngine->setDraw2DFunction(myDraw2DFun);
    gEngine->setDrawFunction(myDrawFun);
    gEngine->setPreSyncFunction(myPreSyncFun);
    gEngine->setCleanUpFunction(myCleanUpFun);

    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setCharCallbackFunction(charCallback);
    gEngine->setMouseButtonCallbackFunction(mouseButtonCallback);
    gEngine->setMouseScrollCallbackFunction(mouseScrollCallback);

    if (!gEngine->init(Engine::RunMode::OpenGL_3_3_Core_Profile)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    SharedData::instance()->setEncodeFunction(myEncodeFun);
    SharedData::instance()->setDecodeFunction(myDecodeFun);

    gEngine->render();

    if (gEngine->isMaster()) {
        ImGui_ImplGlfwGL3_Shutdown();
    }

    delete gEngine;

    exit(EXIT_SUCCESS);
}
