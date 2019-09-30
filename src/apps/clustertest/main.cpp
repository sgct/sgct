#include <sgct.h>
#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/networkmanager.h>
#include <sgct/window.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    constexpr const int ExtendedSize = 10000;

    sgct::Engine* gEngine;

    struct {
        GLuint vao;
        GLuint vbo;
        GLuint ibo;
    } geometry;
    struct Vertex {
        float x, y, z;
    };
    int nVertices = 0;
    GLint matrixLocation = -1;

    sgct::SharedDouble dt(0.0);
    sgct::SharedDouble currentTime(0.0);
    sgct::SharedWString sTimeOfDay;
    sgct::SharedBool showFPS(false);
    sgct::SharedBool extraPackages(false);
    sgct::SharedBool barrier(false);
    sgct::SharedBool resetCounter(false);
    sgct::SharedBool stats(false);
    sgct::SharedBool takeScreenshot(false);
    sgct::SharedBool slowRendering(false);
    sgct::SharedBool frametest(false);
    sgct::SharedFloat speed(5.f);
    sgct::SharedVector<float> extraData;

    constexpr const char* vertexShader = R"(
#version 330 core

layout(location = 0) in vec3 vertPosition;
uniform mat4 matrix;

void main() {
  gl_Position =  matrix * vec4(vertPosition, 1.0);
}
)";

    constexpr const char* fragmentShader = R"(
#version 330 core

out vec4 color;

void main() { color = vec4(1.0); }
)";
} // namespace

using namespace sgct;

void myDraw2DFun() {
#ifdef SGCT_HAS_TEXT
    text::print(
        *text::FontManager::instance()->getFont("SGCTFont", 24),
        text::TextAlignMode::TopLeft,
        50,
        700, 
        glm::vec4(1.f, 0.f, 0.f, 1.f),
        "Focused: %s", gEngine->getCurrentWindow().isFocused() ? "true" : "false"
    );
    text::print(
        *text::FontManager::instance()->getFont("SGCTFont", 24),
        text::TextAlignMode::TopLeft,
        100,
        500,
        glm::vec4(0.f, 1.f, 0.f, 1.f),
        L"Time: %ls", sTimeOfDay.getVal().c_str()
    );
    if (extraPackages.getVal() && extraData.getSize() == ExtendedSize) {
        float xPos =
            gEngine->getCurrentWindow().getFramebufferResolution().x / 2.f - 150.f;
        text::print(
            *text::FontManager::instance()->getFont("SGCTFont", 16),
            text::TextAlignMode::TopLeft,
            xPos,
            150.f,
            glm::vec4(0.f, 1.f, 0.5f, 1.f),
            "Vector val: %f, size: %u",
            extraData.getValAt(ExtendedSize / 2), extraData.getSize()
        );
    }
#endif // SGCT_HAS_TEXT
}

void drawFun() {
    if (slowRendering.getVal()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // test quadbuffer
    if (frametest.getVal()) {
        if (gEngine->getCurrentFrameNumber() % 2 == 0) {
            // even
            if (gEngine->getCurrentFrustumMode() == core::Frustum::Mode::StereoRightEye) {
                // left eye or mono since clear color is one step behind  -> red
                gEngine->setClearColor(glm::vec4(0.f, 0.f, 1.f, 1.f));
            }
            else {
                // right -> blue
                gEngine->setClearColor(glm::vec4(1.f, 0.f, 0.f, 1.f));
            }
        }
        else {
            // odd
            if (gEngine->getCurrentFrustumMode() == core::Frustum::Mode::StereoRightEye) {
                // left eye or mono since clear color is one step behind
                gEngine->setClearColor(glm::vec4(0.5f, 0.5f, 0.5f, 1.f));
            }
            else {
                //right
                gEngine->setClearColor(glm::vec4(0.f, 1.f, 0.f, 1.f));
            }
        }
    }
    else {
        gEngine->setClearColor(glm::vec4(0.f, 0.f, 0.f, 0.f));
    }

    ShaderManager::instance()->bindShaderProgram("simple");
    glm::mat4 matrix = gEngine->getCurrentModelViewProjectionMatrix();
    matrix = glm::rotate(
        matrix,
        glm::radians(static_cast<float>(currentTime.getVal()) * speed.getVal()),
        glm::vec3(0.f, 1.f, 0.f)
    );
    matrix = glm::scale(matrix, glm::vec3(1.f, 0.5f, 1.f));
    glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, glm::value_ptr(matrix));

    glBindVertexArray(geometry.vao);
    glDrawElements(GL_LINE_STRIP, nVertices, GL_UNSIGNED_BYTE, nullptr);
    glBindVertexArray(0);

#ifdef SGCT_HAS_TEXT
    float xPos = gEngine->getCurrentWindow().getFramebufferResolution().x / 2.f;

    glColor3f(1.f, 1.f, 0.f);
    if (gEngine->getCurrentFrustumMode() == core::Frustum::Mode::StereoLeftEye) {
        text::print(
            *text::FontManager::instance()->getFont("SGCTFont", 32),
            text::TextAlignMode::TopRight,
            xPos,
            200,
            "Left"
        );
    }
    else if (gEngine->getCurrentFrustumMode() == core::Frustum::Mode::StereoRightEye) {
        text::print(
            *text::FontManager::instance()->getFont("SGCTFont", 32),
            text::TextAlignMode::TopLeft,
            xPos,
            150,
            "Right"
        );
    }
    else if (gEngine->getCurrentFrustumMode() == core::Frustum::Mode::MonoEye) {
        text::print(
            *text::FontManager::instance()->getFont("SGCTFont", 32),
            text::TextAlignMode::TopLeft,
            xPos,
            200,
            "Mono"
        );
    }

    wchar_t str0[] = L"åäö"; // Swedish string
    wchar_t str1[] = L"лдощдффыкйцн"; // Russian string
    wchar_t str2[] = L"かんじ"; // Japanese string
    wchar_t str3[] = L"汉字"; // Chinese string
    wchar_t str4[] = L"mương"; // Vietnamese string
    wchar_t str5[] = L"한자"; // Korean string
    wchar_t str6[] = L"बईबईसई"; // Hindi string

    text::FontManager::instance()->getFont("SGCTFont", 32)->setStrokeSize(2);
    text::FontManager::instance()->setStrokeColor(glm::vec4(1.f, 0.f, 0.f, 0.5f));
    
    // test
    glm::mat4 texMVP = glm::ortho(-1.f, 1.f, -1.f, 1.f);
    texMVP = glm::scale(texMVP, glm::vec3(0.1f));
    texMVP = glm::rotate(
        texMVP,
        static_cast<float>(currentTime.getVal()),
        glm::vec3(0.f, 1.f, 0.f)
    );

    text::print(
        *text::FontManager::instance()->getFont("SGCTFont", 32),
        text::TextAlignMode::TopRight,
        500,
        500,
        L"%ls\n%ls\n%ls\n%ls\n%ls\n%ls\n%ls",
        str0, str1, str2, str3, str4, str5, str6
    );

    if (gEngine->getCurrentWindow().isUsingSwapGroups()) {
        text::print(
            *text::FontManager::instance()->getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            xPos - xPos / 2.f,
            450,
            "Swap group: Active"
        );

        text::print(
            *text::FontManager::instance()->getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            xPos - xPos / 2.f,
            500,
            "Press B to toggle barrier and R to reset counter"
        );

        if (gEngine->getCurrentWindow().isBarrierActive()) {
            text::print(
                *text::FontManager::instance()->getFont("SGCTFont", 18),
                text::TextAlignMode::TopLeft,
                xPos - xPos / 2.f,
                400,
                "Swap barrier: Active"
            );
        }
        else {
            text::print(
                *text::FontManager::instance()->getFont("SGCTFont", 18),
                text::TextAlignMode::TopLeft,
                xPos - xPos / 2.f,
                400,
                "Swap barrier: Inactive"
            );
        }

        if (gEngine->getCurrentWindow().isSwapGroupMaster()) {
            text::print(
                *text::FontManager::instance()->getFont("SGCTFont", 18),
                text::TextAlignMode::TopLeft,
                xPos - xPos / 2.f,
                350,
                "Swap group master: True"
            );
        }
        else {
            text::print(
                *text::FontManager::instance()->getFont("SGCTFont", 18),
                text::TextAlignMode::TopLeft,
                xPos - xPos / 2.f,
                350,
                "Swap group master: False"
            );
        }

        unsigned int iFrame = gEngine->getCurrentWindow().getSwapGroupFrameNumber();
        text::print(
            *text::FontManager::instance()->getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            xPos - xPos / 2.f,
            300,
            "Nvidia frame counter: %u", iFrame
        );
        text::print(
            *text::FontManager::instance()->getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            xPos - xPos / 2.f,
            250,
            "Framerate: %.3lf", 1.0 / gEngine->getDt()
        );
    }
    else {
        text::print(
            *text::FontManager::instance()->getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            xPos - xPos / 2.f,
            450,
            "Swap group: Inactive"
        );
    }
#endif // SGCT_HAS_TEXT
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        dt.setVal(gEngine->getDt());
        currentTime.setVal(gEngine->getTime());

        time_t now = time(nullptr);
        constexpr const int TimeBufferSize = 256;
        char TimeBuffer[TimeBufferSize];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
        tm timeInfo;
        errno_t err = localtime_s(&timeInfo, &now);
        if (err == 0) {
            strftime(TimeBuffer, TimeBufferSize, "%X", &timeInfo);
        }
#else
        tm* timeInfoPtr;
        timeInfoPtr = localtime(&now);
        strftime(TimeBuffer, TIME_BUFFER_SIZE, "%X", timeInfoPtr);
#endif
        const std::string time = TimeBuffer;
        const std::wstring wTime(time.begin(), time.end());
        sTimeOfDay.setVal(wTime);
    }
}

void postSyncPreDrawFun() {
    gEngine->setDisplayInfoVisibility(showFPS.getVal());

    // barrier is set by swap group not window both windows has the same HDC
    Window::setBarrier(barrier.getVal());
    if (resetCounter.getVal()) {
        Window::resetSwapGroupFrameNumber();
    }
    gEngine->setStatsGraphVisibility(stats.getVal());

    if (takeScreenshot.getVal()) {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void postDrawFun() {
    if (gEngine->isMaster()) {
        resetCounter.setVal(false);
    }
}

void initOGLFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);

    size_t numberOfActiveViewports = 0;
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        for (int j = 0; j < thisNode->getWindow(i).getNumberOfViewports(); j++) {
            if (thisNode->getWindow(i).getViewport(j).isEnabled()) {
                numberOfActiveViewports++;
            }
        }
    }

    MessageHandler::instance()->print(
        "Number of active viewports: %d\n", numberOfActiveViewports
    );

    constexpr const uint8_t RestartIndex = std::numeric_limits<uint8_t>::max();

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RestartIndex);

    glGenVertexArrays(1, &geometry.vao);
    glGenBuffers(1, &geometry.vbo);
    glGenBuffers(1, &geometry.ibo);

    glBindVertexArray(geometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.ibo);

    std::vector<Vertex> vertices;
    std::vector<uint8_t> indices;

    vertices.push_back({ -1.f, -1.f, -1.f });     // 0: ---
    vertices.push_back({  1.f, -1.f, -1.f });     // 1: +--
    vertices.push_back({  1.f, -1.f,  1.f });     // 2: +-+
    vertices.push_back({ -1.f, -1.f,  1.f });     // 3: --+
    vertices.push_back({ -1.f,  1.f, -1.f });     // 4: -+-
    vertices.push_back({  1.f,  1.f, -1.f });     // 5: ++-
    vertices.push_back({  1.f,  1.f,  1.f });     // 6: +++
    vertices.push_back({ -1.f,  1.f,  1.f });     // 7: -++


    // bottom
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(3);
    indices.push_back(0);
    indices.push_back(RestartIndex);

    // top
    indices.push_back(4);
    indices.push_back(5);
    indices.push_back(6);
    indices.push_back(7);
    indices.push_back(4);
    indices.push_back(RestartIndex);

    // sides
    indices.push_back(0);
    indices.push_back(4);
    indices.push_back(RestartIndex);
    indices.push_back(1);
    indices.push_back(5);
    indices.push_back(RestartIndex);
    indices.push_back(2);
    indices.push_back(6);
    indices.push_back(RestartIndex);
    indices.push_back(3);
    indices.push_back(7);
    indices.push_back(RestartIndex);

    nVertices = static_cast<int>(indices.size());
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
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(uint8_t),
        indices.data(),
        GL_STATIC_DRAW
    );
    glBindVertexArray(0);

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

void encodeFun() {
    unsigned char flags = 0;
    flags = showFPS.getVal()        ? flags | 1   : flags & ~1;   // bit 1
    flags = extraPackages.getVal()  ? flags | 2   : flags & ~2;   // bit 2
    flags = barrier.getVal()        ? flags | 4   : flags & ~4;   // bit 3
    flags = resetCounter.getVal()   ? flags | 8   : flags & ~8;   // bit 4
    flags = stats.getVal()          ? flags | 16  : flags & ~16;  // bit 5
    flags = takeScreenshot.getVal() ? flags | 32  : flags & ~32;  // bit 6
    flags = slowRendering.getVal()  ? flags | 64  : flags & ~64;  // bit 7
    flags = frametest.getVal()      ? flags | 128 : flags & ~128; // bit 8

    SharedUChar sf(flags);

    SharedData::instance()->writeDouble(dt);
    SharedData::instance()->writeDouble(currentTime);
    SharedData::instance()->writeFloat(speed);
    SharedData::instance()->writeUChar(sf);
    SharedData::instance()->writeWString(sTimeOfDay);

    if (extraPackages.getVal()) {
        SharedData::instance()->writeVector(extraData);
    }
}

void decodeFun() {
    SharedUChar sf;
    SharedData::instance()->readDouble(dt);
    SharedData::instance()->readDouble(currentTime);
    SharedData::instance()->readFloat(speed);
    SharedData::instance()->readUChar(sf);
    SharedData::instance()->readWString(sTimeOfDay);

    unsigned char flags = sf.getVal();
    showFPS.setVal(flags & 1);
    extraPackages.setVal(flags & 2);
    barrier.setVal(flags & 4);
    resetCounter.setVal(flags & 8);
    stats.setVal(flags & 16);
    takeScreenshot.setVal(flags & 32);
    slowRendering.setVal(flags & 64);
    frametest.setVal(flags & 128);

    if (extraPackages.getVal()) {
        SharedData::instance()->readVector(extraData);
    }
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster()) {
        static bool mousePointer = true;

        switch (key) {
            case key::C:
                if (action == action::Press) {
                    static bool useCompress = false;
                    useCompress = !useCompress;
                    SharedData::instance()->setCompression(useCompress);
                }
                break;
            case key::F:
                if (action == action::Press) {
                    frametest.setVal(!frametest.getVal());
                }
                break;
            case key::I:
                if (action == action::Press) {
                    showFPS.setVal(!showFPS.getVal());
                }
                break;
            case key::E:
                if (action == action::Press) {
                    extraPackages.setVal(!extraPackages.getVal());
                }
                break;
            case key::B:
                if (action == action::Press) {
                    barrier.setVal(!barrier.getVal());
                }
                break;
            case key::R:
                if (action == action::Press) {
                    resetCounter.setVal(!resetCounter.getVal());
                }
                break;
            case key::S:
                if (action == action::Press) {
                    stats.setVal(!stats.getVal());
                }
                break;
            case key::G:
                if (action == action::Press) {
                    gEngine->sendMessageToExternalControl("Testing!!\r\n");
                }
                break;
            case key::M:
                if (action == action::Press) {
                    mousePointer = !mousePointer;

                    for (int i = 0; i < gEngine->getNumberOfWindows(); i++) {
                        sgct::Engine::setMouseCursorVisibility(i, mousePointer);
                    }
                }
                break;
            case key::F9:
                if (action == action::Press) {
                    slowRendering.setVal(!slowRendering.getVal());
                }
                break;
            case key::F10:
                if (action == action::Press) {
                    takeScreenshot.setVal(true);
                }
                break;
            case key::Up:
                speed.setVal(speed.getVal() * 1.1f);
                break;
            case key::Down:
                speed.setVal(speed.getVal() / 1.1f);
                break;
        }
    }
}

void externalControlCallback(const char* receivedChars, int size) {
    if (gEngine->isMaster()) {
        std::string_view data(receivedChars, size);
        if (data == "info") {
            showFPS.setVal(!showFPS.getVal());
        }
        else if (data == "size") {
            gEngine->setExternalControlBufferSize(4096);
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    gEngine->setClearColor(glm::vec4(0.f, 0.f, 0.f, 0.f));
    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setExternalControlCallback(externalControlCallback);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setDraw2DFunction(myDraw2DFun);

    if (!gEngine->init(Engine::RunMode::Default_Mode, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    if (gEngine->isMaster()) {
        for (int i = 0; i < ExtendedSize; i++) {
            extraData.addVal(static_cast<float>(rand() % 500) / 500.f);
        }
    }

    SharedData::instance()->setEncodeFunction(encodeFun);
    SharedData::instance()->setDecodeFunction(decodeFun);

    gEngine->setDrawFunction(drawFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setPostDrawFunction(postDrawFun);

    const std::vector<std::string>& addresses =
        core::NetworkManager::instance()->getLocalAddresses();
    for (unsigned int i = 0; i < addresses.size(); i++) {
        fprintf(stderr, "Address %u: %s\n", i, addresses[i].c_str());
    }

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
