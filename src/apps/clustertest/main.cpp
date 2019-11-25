#include <sgct/action.h>
#include <sgct/clustermanager.h>
#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#include <sgct/keys.h>
#include <sgct/networkmanager.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/window.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    constexpr const int ExtendedSize = 10000;

    struct {
        GLuint vao;
        GLuint vbo;
        GLuint ibo;
    } geometry;
    int nVertices = 0;
    GLint matrixLocation = -1;

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

layout (location = 0) in vec3 vertPosition;
uniform mat4 matrix;

void main() { gl_Position = matrix * vec4(vertPosition, 1.0); }
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
        *text::FontManager::instance().getFont("SGCTFont", 24),
        text::TextAlignMode::TopLeft,
        100,
        500,
        glm::vec4(0.f, 1.f, 0.f, 1.f),
        "Time: %ls", sTimeOfDay.getVal().c_str()
    );
    if (extraPackages.getVal() && extraData.getSize() == ExtendedSize) {
        Window& win = Engine::instance().getCurrentWindow();
        float xp = win.getFramebufferResolution().x / 2.f - 150.f;
        text::print(
            *text::FontManager::instance().getFont("SGCTFont", 16),
            text::TextAlignMode::TopLeft,
            xp,
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
        if (Engine::instance().getCurrentFrameNumber() % 2 == 0) {
            // even
            if (Engine::instance().getCurrentFrustumMode() ==
                core::Frustum::Mode::StereoRightEye)
            {
                // left eye or mono since clear color is one step behind  -> red
                Engine::instance().setClearColor(glm::vec4(0.f, 0.f, 1.f, 1.f));
            }
            else {
                // right -> blue
                Engine::instance().setClearColor(glm::vec4(1.f, 0.f, 0.f, 1.f));
            }
        }
        else {
            // odd
            if (Engine::instance().getCurrentFrustumMode() ==
                core::Frustum::Mode::StereoRightEye)
            {
                // left eye or mono since clear color is one step behind
                Engine::instance().setClearColor(glm::vec4(0.5f, 0.5f, 0.5f, 1.f));
            }
            else {
                //right
                Engine::instance().setClearColor(glm::vec4(0.f, 1.f, 0.f, 1.f));
            }
        }
    }
    else {
        Engine::instance().setClearColor(glm::vec4(0.f, 0.f, 0.f, 0.f));
    }

    ShaderManager::instance().getShaderProgram("simple").bind();
    glm::mat4 matrix = Engine::instance().getCurrentModelViewProjectionMatrix();
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
    float pos = Engine::instance().getCurrentWindow().getFramebufferResolution().x / 2.f;

    if (Engine::instance().getCurrentFrustumMode() ==
        core::Frustum::Mode::StereoLeftEye)
    {
        text::print(
            *text::FontManager::instance().getFont("SGCTFont", 32),
            text::TextAlignMode::TopRight,
            pos,
            200,
            "Left"
        );
    }
    else if (Engine::instance().getCurrentFrustumMode() ==
             core::Frustum::Mode::StereoRightEye)
    {
        text::print(
            *text::FontManager::instance().getFont("SGCTFont", 32),
            text::TextAlignMode::TopLeft,
            pos,
            150,
            "Right"
        );
    }
    else if (Engine::instance().getCurrentFrustumMode() == core::Frustum::Mode::MonoEye)
    {
        text::print(
            *text::FontManager::instance().getFont("SGCTFont", 32),
            text::TextAlignMode::TopLeft,
            pos,
            200,
            "Mono"
        );
    }

    if (Engine::instance().getCurrentWindow().isUsingSwapGroups()) {
        text::print(
            *text::FontManager::instance().getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            pos - pos / 2.f,
            450,
            glm::vec4(1.f),
            glm::vec4(1.f, 0.f, 0.f, 0.5f),
            "Swap group: Active"
        );

        text::print(
            *text::FontManager::instance().getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            pos - pos / 2.f,
            500,
            glm::vec4(1.f),
            glm::vec4(1.f, 0.f, 0.f, 0.5f),
            "Press B to toggle barrier and R to reset counter"
        );

        if (Engine::instance().getCurrentWindow().isBarrierActive()) {
            text::print(
                *text::FontManager::instance().getFont("SGCTFont", 18),
                text::TextAlignMode::TopLeft,
                pos - pos / 2.f,
                400,
                glm::vec4(1.f),
                glm::vec4(1.f, 0.f, 0.f, 0.5f),
                "Swap barrier: Active"
            );
        }
        else {
            text::print(
                *text::FontManager::instance().getFont("SGCTFont", 18),
                text::TextAlignMode::TopLeft,
                pos - pos / 2.f,
                400,
                glm::vec4(1.f),
                glm::vec4(1.f, 0.f, 0.f, 0.5f),
                "Swap barrier: Inactive"
            );
        }

        if (Engine::instance().getCurrentWindow().isSwapGroupMaster()) {
            text::print(
                *text::FontManager::instance().getFont("SGCTFont", 18),
                text::TextAlignMode::TopLeft,
                pos - pos / 2.f,
                350,
                glm::vec4(1.f),
                glm::vec4(1.f, 0.f, 0.f, 0.5f),
                "Swap group master: True"
            );
        }
        else {
            text::print(
                *text::FontManager::instance().getFont("SGCTFont", 18),
                text::TextAlignMode::TopLeft,
                pos - pos / 2.f,
                350,
                glm::vec4(1.f),
                glm::vec4(1.f, 0.f, 0.f, 0.5f),
                "Swap group master: False"
            );
        }

        unsigned int f = Engine::instance().getCurrentWindow().getSwapGroupFrameNumber();
        text::print(
            *text::FontManager::instance().getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            pos - pos / 2.f,
            300,
            glm::vec4(1.f),
            glm::vec4(1.f, 0.f, 0.f, 0.5f),
            "Nvidia frame counter: %u", f
        );
        text::print(
            *text::FontManager::instance().getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            pos - pos / 2.f,
            250,
            glm::vec4(1.f),
            glm::vec4(1.f, 0.f, 0.f, 0.5f),
            "Framerate: %.3lf", 1.0 / Engine::instance().getDt()
        );
    }
    else {
        text::print(
            *text::FontManager::instance().getFont("SGCTFont", 18),
            text::TextAlignMode::TopLeft,
            pos - pos / 2.f,
            450,
            glm::vec4(1.f),
            glm::vec4(1.f, 0.f, 0.f, 0.5f),
            "Swap group: Inactive"
        );
    }
#endif // SGCT_HAS_TEXT
}

void preSyncFun() {
    if (Engine::instance().isMaster()) {
        currentTime.setVal(Engine::instance().getTime());

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
        strftime(TimeBuffer, TimeBufferSize, "%X", timeInfoPtr);
#endif
        const std::string time = TimeBuffer;
        const std::wstring wTime(time.begin(), time.end());
        sTimeOfDay.setVal(wTime);
    }
}

void postSyncPreDrawFun() {
    Engine::instance().setDisplayInfoVisibility(showFPS.getVal());

    // barrier is set by swap group not window both windows has the same HDC
    Window::setBarrier(barrier.getVal());
    if (resetCounter.getVal()) {
        Window::resetSwapGroupFrameNumber();
    }
    Engine::instance().setStatsGraphVisibility(stats.getVal());

    if (takeScreenshot.getVal()) {
        Engine::instance().takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void postDrawFun() {
    if (Engine::instance().isMaster()) {
        resetCounter.setVal(false);
    }
}

void initOGLFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);

    size_t numberOfActiveViewports = 0;
    const core::Node& thisNode = core::ClusterManager::instance().getThisNode();
    for (int i = 0; i < thisNode.getNumberOfWindows(); i++) {
        for (int j = 0; j < thisNode.getWindow(i).getNumberOfViewports(); j++) {
            if (thisNode.getWindow(i).getViewport(j).isEnabled()) {
                numberOfActiveViewports++;
            }
        }
    }

    MessageHandler::printInfo("Number of active viewports: %d", numberOfActiveViewports);

    constexpr const uint8_t RestartIndex = std::numeric_limits<uint8_t>::max();

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RestartIndex);

    glGenVertexArrays(1, &geometry.vao);
    glGenBuffers(1, &geometry.vbo);
    glGenBuffers(1, &geometry.ibo);

    glBindVertexArray(geometry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.ibo);

    struct Vertex {
        float x, y, z;
    };
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

    ShaderManager::instance().addShaderProgram("simple", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().getShaderProgram("simple");
    prog.bind();
    matrixLocation = glGetUniformLocation(prog.getId(), "matrix");
    prog.unbind();
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

    SharedData::instance().writeDouble(currentTime);
    SharedData::instance().writeFloat(speed);
    SharedData::instance().writeUChar(sf);
    SharedData::instance().writeWString(sTimeOfDay);

    if (extraPackages.getVal()) {
        SharedData::instance().writeVector(extraData);
    }
}

void decodeFun() {
    SharedUChar sf;
    SharedData::instance().readDouble(currentTime);
    SharedData::instance().readFloat(speed);
    SharedData::instance().readUChar(sf);
    SharedData::instance().readWString(sTimeOfDay);

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
        SharedData::instance().readVector(extraData);
    }
}

void keyCallback(int key, int, int action, int) {
    if (Engine::instance().isMaster()) {
        switch (key) {
            case key::Esc:
                if (action == action::Press) {
                    Engine::instance().terminate();
                }
                break;
            case key::C:
                if (action == action::Press) {
                    static bool useCompress = false;
                    useCompress = !useCompress;
                    SharedData::instance().setCompression(useCompress);
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
                    Engine::instance().sendMessageToExternalControl("Testing!!\r\n");
                }
                break;
            case key::M:
                if (action == action::Press) {
                    static bool mousePointer = true;
                    mousePointer = !mousePointer;

                    for (int i = 0; i < Engine::instance().getNumberOfWindows(); i++) {
                        Engine::setMouseCursorVisibility(i, mousePointer);
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
    if (Engine::instance().isMaster()) {
        std::string_view data(receivedChars, size);
        if (data == "info") {
            showFPS.setVal(!showFPS.getVal());
        }
        else if (data == "size") {
            Engine::instance().setExternalControlBufferSize(4096);
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);

    Engine::instance().setClearColor(glm::vec4(0.f, 0.f, 0.f, 0.f));
    Engine::instance().setInitOGLFunction(initOGLFun);
    Engine::instance().setExternalControlCallback(externalControlCallback);
    Engine::instance().setKeyboardCallbackFunction(keyCallback);
    Engine::instance().setDraw2DFunction(myDraw2DFun);
    Engine::instance().setEncodeFunction(encodeFun);
    Engine::instance().setDecodeFunction(decodeFun);
    Engine::instance().setDrawFunction(drawFun);
    Engine::instance().setPreSyncFunction(preSyncFun);
    Engine::instance().setPostSyncPreDrawFunction(postSyncPreDrawFun);
    Engine::instance().setPostDrawFunction(postDrawFun);

    try {
        Engine::instance().init(Engine::RunMode::Default_Mode, cluster);
    }
    catch (const std::runtime_error& e) {
        MessageHandler::printError("%s", e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    if (Engine::instance().isMaster()) {
        for (int i = 0; i < ExtendedSize; i++) {
            extraData.addVal(static_cast<float>(rand() % 500) / 500.f);
        }
    }

    Engine::instance().render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
