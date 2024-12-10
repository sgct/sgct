/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include "capture.h"

#include <sgct/sgct.h>
#include <sgct/utils/dome.h>
#include <sgct/utils/plane.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdlib.h>
#include <stdio.h>

namespace {
    Capture capture;

    std::unique_ptr<sgct::utils::Plane> plane;
    std::unique_ptr<sgct::utils::Dome> dome;

    GLint matrixLoc = -1;
    GLint scaleUvLoc = -1;
    GLint offsetUVLoc = -1;
    GLuint texId = 0;

    std::thread* workerThread;
    GLFWwindow* hiddenWindow;
    bool flipFrame = false;
    bool fulldomeMode = false;
    float planeAzimuth = 0.f;
    float planeElevation = 33.f;
    float planeRoll = 0.f;

    // variables to share across cluster
    double currTime = 0.0;
    bool info = false;
    bool stats = false;
    bool takeScreenshot = false;

    // variables to share between threads
    bool workerRunning = true;
    bool renderDome = fulldomeMode;
    double captureRate = 0.0;
    int32_t domeCut = 2;


    constexpr std::string_view VertexShader = R"(
  #version 330 core

  layout(location = 0) in vec2 texCoords;
  layout(location = 1) in vec3 normals;
  layout(location = 2) in vec3 vertPositions;

  uniform mat4 mvp;

  out vec2 uv;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPositions, 1.0);
    uv = texCoords;
  })";

    constexpr std::string_view FragmentShader = R"(
  #version 330 core

  uniform sampler2D tex;
  uniform vec2 scaleUV;
  uniform vec2 offsetUV;

  in vec2 uv;
  out vec4 color;

  void main() { color = texture(tex, (uv.st * scaleUV) + offsetUV); }
)";
} // namespace

using namespace sgct;

void captureLoop() {
    glfwMakeContextCurrent(hiddenWindow);

    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    const int dataSize = capture.width() * capture.height() * 3;
    glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);

    while (workerRunning) {
        capture.poll();
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glDeleteBuffers(1, &pbo);

    glfwMakeContextCurrent(nullptr); // detach context
}

void calculateStats() {
    double timeStamp = Engine::getTime();
    static double previousTimeStamp = timeStamp;
    static double numberOfSamples = 0.0;
    static double duration = 0.0;

    timeStamp = Engine::getTime();
    duration += timeStamp - previousTimeStamp;
    previousTimeStamp = timeStamp;
    numberOfSamples++;

    if (duration >= 1.0) {
        captureRate = numberOfSamples / duration;
        duration = 0.0;
        numberOfSamples = 0.0;
    }
}

void allocateTexture() {
    const int w = capture.width();
    const int h = capture.height();

    if (w * h <= 0) {
        sgct::Log::Error("Invalid texture size (%dx%d)", w, h);
        return;
    }

    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, w, h);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void uploadData(const uint8_t** data, int w, int h) {
    // At least two textures and GLSync objects should be used to control that the
    // uploaded texture is the same for all viewports to prevent any tearing and maintain
    // frame sync

    if (texId == 0) {
        return;
    }

    unsigned char* gpuMemory = reinterpret_cast<unsigned char*>(
        glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY)
        );
    if (gpuMemory) {
        int dataOffset = 0;
        const int stride = w * 3;

        if (flipFrame) {
            for (int row = 0; row < h; row++) {
                std::memcpy(gpuMemory + dataOffset, data[0] + row * stride, stride);
                dataOffset += stride;
            }
        }
        else {
            for (int row = h - 1; row > -1; row--) {
                std::memcpy(gpuMemory + dataOffset, data[0] + row * stride, stride);
                dataOffset += stride;
            }
        }

        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, 0);
    }

    calculateStats();
}

void draw(const RenderData& data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glm::mat4 MVP = data.modelViewProjectionMatrix;

    ShaderManager::instance().shaderProgram("xform").bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    glm::vec2 texSize = glm::vec2(
        static_cast<float>(capture.width()),
        static_cast<float>(capture.height())
    );

    if (fulldomeMode) {
        // TextureCut 2 equals showing only the middle square of a capturing a input
        if (domeCut == 2) {
            glUniform2f(scaleUvLoc, texSize.y / texSize.x, 1.f);
            glUniform2f(offsetUVLoc, ((texSize.x - texSize.y)*0.5f) / texSize.x, 0.f);
        }
        else {
            glUniform2f(scaleUvLoc, 1.f, 1.f);
            glUniform2f(offsetUVLoc, 0.f, 0.f);
        }

        glCullFace(GL_FRONT); //camera on the inside of the dome

        glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(MVP));
        dome->draw();
    }
    else {
        // plane mode
        glUniform2f(scaleUvLoc, 1.f, 1.f);
        glUniform2f(offsetUVLoc, 0.f, 0.f);

        glCullFace(GL_BACK);

        //transform and draw plane
        glm::mat4 planeTransform = glm::mat4(1.f);
        // azimuth
        planeTransform = glm::rotate(
            planeTransform,
            glm::radians(planeAzimuth),
            glm::vec3(0.f, -1.f, 0.f)
        );
        // elevation
        planeTransform = glm::rotate(
            planeTransform,
            glm::radians(planeElevation),
            glm::vec3(1.f, 0.f, 0.f)
        );
        // roll
        planeTransform = glm::rotate(
            planeTransform,
            glm::radians(planeRoll),
            glm::vec3(0.f, 0.f, 1.f)
        );
        // distance
        planeTransform = glm::translate(planeTransform, glm::vec3(0.f, 0.f, -5.f));

        planeTransform = MVP * planeTransform;
        glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(planeTransform));
        plane->draw();
    }

    ShaderManager::instance().shaderProgram("xform").unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void draw2D(const RenderData& data) {
    if (info) {
        unsigned int size = static_cast<unsigned int>(9.f * data.window.scale().x);
        text::Font* font = text::FontManager::instance().font("SGCTFont", size);
        float padding = 10.f;

        text::print(
            data.window,
            data.viewport,
            *font,
            text::Alignment::TopLeft,
            padding,
            static_cast<float>(data.window.framebufferResolution().y - size) - padding,
            glm::vec4(1.f),
            "Format: %s\nResolution: %d x %d\nRate: %.2lf Hz",
            capture.format().c_str(),
            capture.width(),
            capture.height(),
            captureRate
        );
    }
}

void preSync() {
    if (Engine::instance().isMaster()) {
        currTime = Engine::getTime();
    }
}

void postSyncPreDraw() {
    Engine::instance().setStatsGraphVisibility(stats);

    if (takeScreenshot) {
        Engine::instance().takeScreenshot();
        takeScreenshot = false;
    }

    fulldomeMode = renderDome; // set the flag frame synchronized for all viewports
}

void myInitOGLFun(GLFWwindow* win) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    hiddenWindow = glfwCreateWindow(1, 1, "Thread Window", nullptr, win);
    if (!hiddenWindow) {
        sgct::Log::Info("Failed to create loader context");
    }

    // restore to normal
    glfwMakeContextCurrent(win);

    capture.initialize();

    allocateTexture();

    // start capture thread
    const sgct::Node& thisNode = sgct::ClusterManager::instance().thisNode();
    if (thisNode.address() == capture.videoHost()) {
        workerThread = new std::thread(captureLoop);
    }

    capture.setVideoDecoderCallback(uploadData);

    // create plane
    float planeWidth = 8.f;
    float planeHeight =
        planeWidth *
        (static_cast<float>(capture.height()) / static_cast<float>(capture.width()));
    plane = std::make_unique<utils::Plane>(planeWidth, planeHeight);

    // create dome
    dome = std::make_unique<utils::Dome>(7.4f, 180.f, 256, 128);

    // Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ShaderManager::instance().addShaderProgram("xform", vertexShader, fragmentShader);
    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
    prog.bind();

    matrixLoc = glGetUniformLocation(prog.id(), "mvp");
    scaleUvLoc = glGetUniformLocation(prog.id(), "scaleUV");
    offsetUVLoc = glGetUniformLocation(prog.id(), "offsetUV");
    glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);

    prog.unbind();
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, currTime);
    serializeObject(data, info);
    serializeObject(data, stats);
    serializeObject(data, takeScreenshot);
    serializeObject(data, renderDome);
    serializeObject(data, domeCut);
    return data;
}

void decode(const std::vector<std::byte>& data) {
    unsigned int pos = 0;
    deserializeObject(data, pos, currTime);
    deserializeObject(data, pos, info);
    deserializeObject(data, pos, stats);
    deserializeObject(data, pos, takeScreenshot);
    deserializeObject(data, pos, renderDome);
    deserializeObject(data, pos, domeCut);
}

void cleanUp() {
    plane = nullptr;
    dome = nullptr;

    glDeleteTextures(1, &texId);
    texId = 0;
}

void keyCallback(Key key, Modifier, Action action, int) {
    if (Engine::instance().isMaster() && (action == Action::Press)) {
        switch (key) {
            //dome mode
            case Key::D:
                renderDome = true;
                break;
            case Key::S:
                stats = !stats;
                break;
            case Key::I:
                info = !info;
                break;
            case Key::Key1:
                domeCut = 1;
                break;
            case Key::Key2:
                domeCut = 2;
                break;

            // plane mode
            case Key::P:
                renderDome = false;
                break;
            case Key::PrintScreen:
            case Key::F10:
                takeScreenshot = true;
                break;
        }
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = myInitOGLFun;
    callbacks.draw = draw;
    callbacks.draw2D = draw2D;
    callbacks.preSync = preSync;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.cleanUp = cleanUp;
    callbacks.keyboard = keyCallback;
    callbacks.encode = encode;
    callbacks.decode = decode;

    // arguments:
    // -host <host which should capture>
    // -video <device name>
    // -option <key> <val>
    // -flip
    // -plane <azimuth> <elevation> <roll>
    //
    // to obtain video device names in windows use:
    // ffmpeg -list_devices true -f dshow -i dummy
    // for mac:
    // ffmpeg -f avfoundation -list_devices true -i ""
    //
    // to obtain device properties in windows use:
    // ffmpeg -f dshow -list_options true -i video=<device name>
    //
    // For options look at: http://ffmpeg.org/ffmpeg-devices.html

    int i = 0;
    while (i < argc) {
        std::string_view a = std::string_view(argv[i]);
        if (a == "-host" && argc > (i + 1)) {
            capture.setVideoHost(std::string(argv[i + 1]));
        }
        else if (a == "-video" && argc > (i + 1)) {
            capture.setVideoDevice(std::string(argv[i + 1]));
        }
        else if (a == "-option" && argc > (i + 2)) {
            capture.addOption(
                std::pair(std::string(argv[i + 1]), std::string(argv[i + 2])));
        }
        else if (a == "-flip") {
            flipFrame = true;
        }
        else if (a == "-plane" && argc > (i + 3)) {
            planeAzimuth = static_cast<float>(atof(argv[i + 1]));
            planeElevation = static_cast<float>(atof(argv[i + 2]));
            planeRoll = static_cast<float>(atof(argv[i + 3]));
        }

        i++;
    }

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error & e) {
        Log::Error("%s", e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    // Main loop
    Engine::instance().render();

    // kill worker thread
    workerRunning = false;
    if (workerThread) {
        workerThread->join();
        delete workerThread;
    }

    // Exit program
    exit(EXIT_SUCCESS);
}
