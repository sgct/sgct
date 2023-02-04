/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include "capture.h"

#include <sgct/clustermanager.h>
#include <sgct/image.h>
#include <sgct/sgct.h>
#include <sgct/utils/dome.h>
#include <sgct/utils/plane.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <mutex>

namespace {
    enum class ImageType { JPEG, PNG };

    constexpr int HeaderSize = 1;

    Capture capture;

    std::unique_ptr<sgct::utils::Plane> plane;
    std::unique_ptr<sgct::utils::Dome> dome;

    GLFWwindow* hiddenWindow;

    // variables to share across cluster
    bool info = false;
    bool stats = false;
    bool takeScreenshot = false;
    bool wireframe = false;

    std::unique_ptr<std::thread> loadThread;
    std::mutex imageMutex;
    std::vector<sgct::Image*> transImages;

    int32_t texIndex = -1;
    int32_t incrIndex = 1;
    int32_t numSyncedTex = 0;

    int32_t lastPackage = -1;
    bool transfer = false;
    bool serverUploadDone = false;
    int32_t serverUploadCount = 0;
    bool clientsUploadDone = false;
    std::vector<std::pair<std::string, ImageType>> imagePaths;
    std::vector<GLuint> texIds;
    double sendTimer = 0.0;

    GLint matrixLoc = -1;
    GLint uvScaleLoc = -1;
    GLint uvOffsetLoc = -1;
    GLint chromaKeyMatrixLoc = -1;
    GLint chromaKeyUvScaleLoc = -1;
    GLint chromaKeyUvOffsetLoc = -1;
    GLint chromaKeyColorLoc = -1;
    GLuint texId = 0;

    std::unique_ptr<std::thread> captureThread;
    bool flipFrame = false;
    bool fulldomeMode = false;
    float planeAzimuth = 0.f;
    float planeElevation = 33.f;
    float planeRoll = 0.f;

    bool renderDome(fulldomeMode);
    double captureRate(0.0);
    int32_t domeCut(2);
    bool chromaKey(false);
    int32_t chromaKeyColorIdx(0);
    std::vector<glm::vec3> chromaKeyColors;

    bool captureRunning = true;
    bool isRunning = true;

    constexpr std::string_view VertexShader = R"(
  #version 330 core

  layout(location = 0) in vec2 texCoords;
  layout(location = 1) in vec3 normals;
  layout(location = 2) in vec3 vertPositions;

  uniform mat4 mvp;
  out vec2 uv;

  void main() {
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

  void main() { color = texture(tex, (uv * scaleUV) + offsetUV); }
)";

    constexpr std::string_view FragmentChromaKey = R"(
  #version 330 core

  uniform sampler2D tex;
  uniform vec2 scaleUV;
  uniform vec2 offsetUV;
  uniform float thresholdSensitivity;
  uniform float smoothing;
  uniform vec3 chromaKeyColor;

  in vec2 uv;
  out vec4 color;

  void main() {
    vec4 texColor = texture(tex, (uv * scaleUV) + offsetUV);

    float maskY = 0.2989 * chromaKeyColor.r + 0.5866 * chromaKeyColor.g +
                  0.1145 * chromaKeyColor.b;
    float maskCr = 0.7132 * (chromaKeyColor.r - maskY);
    float maskCb = 0.5647 * (chromaKeyColor.b - maskY);

    float Y = 0.2989 * texColor.r + 0.5866 * texColor.g + 0.1145 * texColor.b;
    float Cr = 0.7132 * (texColor.r - Y);
    float Cb = 0.5647 * (texColor.b - Y);

    float blendValue = smoothstep(
      thresholdSensitivity,
      thresholdSensitivity + smoothing,
      distance(vec2(Cr, Cb), vec2(maskCr, maskCb))
    );
    if (blendValue > 0.1) {
      color = vec4(texColor.rgb, texColor.a * blendValue);
    }
    else {
      discard;
    }
})";
} // namespace

using namespace sgct;

void allocateTexture() {
    const int w = capture.width();
    const int h = capture.height();

    if (w * h <= 0) {
        Log::Error("Invalid texture size (%dx%d)", w, h);
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

void readImage(unsigned char* data, int len) {
    std::unique_lock lk(imageMutex);

    Image* img = new Image();

    char type = static_cast<char>(data[0]);
    assert(type == 0 || type == 1);
    ImageType t = ImageType(type);

    bool result = false;
    switch (t) {
        case ImageType::JPEG:
            img->load(
                reinterpret_cast<unsigned char*>(data + HeaderSize),
                len - HeaderSize
            );
            break;
        case ImageType::PNG:
            img->load(
                reinterpret_cast<unsigned char*>(data + HeaderSize),
                len - HeaderSize
            );
            break;
    }

    if (!result) {
        delete img;
    }
    else {
        transImages.push_back(img);
    }
}

void startDataTransfer() {
    // iterate
    int id = lastPackage;
    id++;

    // make sure to keep within bounds
    if (static_cast<int>(imagePaths.size()) > id) {
        sendTimer = Engine::getTime();

        int imageCounter = static_cast<int32_t>(imagePaths.size());
        lastPackage = imageCounter - 1;

        for (int i = id; i < imageCounter; i++) {
            // load from file
            std::pair<std::string, ImageType> tmpPair =
                imagePaths[static_cast<size_t>(i)];

            std::ifstream file(tmpPair.first.c_str(), std::ios::binary);
            file.seekg(0, std::ios::end);
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<char> buffer(size + HeaderSize);
            char type = static_cast<char>(tmpPair.second);

            // write header (single unsigned char)
            buffer[0] = type;

            if (file.read(buffer.data() + HeaderSize, size)) {
                // transfer
                NetworkManager::instance().transferData(
                    buffer.data(),
                    static_cast<int>(buffer.size()),
                    i
                );

                // read the image on master
                readImage(
                    reinterpret_cast<unsigned char*>(buffer.data()),
                    static_cast<int>(buffer.size())
                );
            }
        }
    }
}

void uploadTexture() {
    std::unique_lock lk(imageMutex);

    if (transImages.empty()) {
        return;
    }

    glfwMakeContextCurrent(hiddenWindow);

    for (size_t i = 0; i < transImages.size(); i++) {
        if (!transImages[i]) {
            texIds.push_back(0);
            continue;
        }

        // create texture
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        GLenum internalformat;
        GLenum type;
        const unsigned int bpc = transImages[i]->bytesPerChannel();

        switch (transImages[i]->channels()) {
            case 1:
                internalformat = (bpc == 1 ? GL_R8 : GL_R16);
                type = GL_RED;
                break;
            case 2:
                internalformat = (bpc == 1 ? GL_RG8 : GL_RG16);
                type = GL_RG;
                break;
            case 3:
            default:
                internalformat = (bpc == 1 ? GL_RGB8 : GL_RGB16);
                type = GL_BGR;
                break;
            case 4:
                internalformat = (bpc == 1 ? GL_RGBA8 : GL_RGBA16);
                type = GL_BGRA;
                break;
        }

        GLenum format = (bpc == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT);

        glTexStorage2D(
            GL_TEXTURE_2D,
            1,
            internalformat,
            transImages[i]->size().x,
            transImages[i]->size().y
        );
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            transImages[i]->size().x,
            transImages[i]->size().y,
            type,
            format,
            transImages[i]->data()
        );

        // Disable mipmaps
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        Log::Info(
            "Texture id %d loaded (%dx%dx%d)",
            tex, transImages[i]->size().x, transImages[i]->size().y,
            transImages[i]->channels()
        );

        texIds.push_back(tex);

        delete transImages[i];
        transImages[i] = nullptr;
    }

    transImages.clear();
    glFinish();

    glfwMakeContextCurrent(nullptr);
}

void captureLoop() {
    glfwMakeContextCurrent(hiddenWindow);

    const int dataSize = capture.width() * capture.height() * 3;
    GLuint PBO;
    glGenBuffers(1, &PBO);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);

    while (captureRunning) {
        capture.poll();

        if (Engine::instance().isMaster() && transfer && !serverUploadDone &&
            !clientsUploadDone)
        {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

            startDataTransfer();
            transfer = false;

            // load textures on master
            uploadTexture();
            serverUploadDone = true;

            if (ClusterManager::instance().numberOfNodes() == 1) {
                // no cluster
                clientsUploadDone = true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // restore for capture
            glfwMakeContextCurrent(hiddenWindow);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
        }
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glDeleteBuffers(1, &PBO);

    glfwMakeContextCurrent(nullptr);
}

void calculateStats() {
    double timeStamp = Engine::getTime();
    static double previousTimeStamp = timeStamp;
    static double numberOfSamples = 0.0;
    static double duration = 0.0;

    timeStamp = sgct::Engine::getTime();
    duration += timeStamp - previousTimeStamp;
    previousTimeStamp = timeStamp;
    numberOfSamples++;

    if (duration >= 1.0) {
        captureRate = numberOfSamples / duration;
        duration = 0.0;
        numberOfSamples = 0.0;
    }
}

void threadWorker() {
    while (isRunning) {
        // runs only on master
        if (transfer && !serverUploadDone && !clientsUploadDone) {
            startDataTransfer();
            transfer = false;

            // load textures on master
            uploadTexture();
            serverUploadDone = true;

            if (ClusterManager::instance().numberOfNodes() == 1) {
                // no cluster
                clientsUploadDone = true;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void uploadData(const uint8_t** data, int width, int height) {
    // At least two textures and GLSync objects
    // should be used to control that the uploaded texture is the same
    // for all viewports to prevent any tearing and maintain frame sync

    if (!texId) {
        return;
    }

    unsigned char* gpuMemory = reinterpret_cast<unsigned char*>(
        glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY)
    );
    if (gpuMemory) {
        int dataOffset = 0;
        const int stride = width * 3;

        if (flipFrame) {
            for (int row = 0; row < height; row++) {
                memcpy(gpuMemory + dataOffset, data[0] + row * stride, stride);
                dataOffset += stride;
            }
        }
        else {
            for (int row = height - 1; row > -1; row--) {
                memcpy(gpuMemory + dataOffset, data[0] + row * stride, stride);
                dataOffset += stride;
            }
        }

        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            width,
            height,
            GL_BGR,
            GL_UNSIGNED_BYTE,
            0
        );
    }

    calculateStats();
}

void draw3DFun(const RenderData& data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    const glm::mat4 mvp = data.modelViewProjectionMatrix;

    // Set up backface culling
    glCullFace(GL_BACK);

    if (texIndex != -1) {
        ShaderManager::instance().shaderProgram("xform").bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texIds[texIndex]);
        glUniform2f(uvScaleLoc, 1.f, 1.f);
        glUniform2f(uvOffsetLoc, 0.f, 0.f);
        glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        glFrontFace(GL_CW);

        dome->draw();
        ShaderManager::instance().shaderProgram("xform").unbind();
    }

    GLint ScaleUV_L = uvScaleLoc;
    GLint OffsetUV_L = uvOffsetLoc;
    GLint Matrix_L = matrixLoc;
    if (chromaKey) {
        ShaderManager::instance().shaderProgram("chromakey").bind();
        glUniform3fv(
            chromaKeyColorLoc,
            1,
            glm::value_ptr(chromaKeyColors[chromaKeyColorIdx])
        );
        ScaleUV_L = chromaKeyUvScaleLoc;
        OffsetUV_L = chromaKeyUvOffsetLoc;
        Matrix_L = chromaKeyMatrixLoc;
    }
    else {
        ShaderManager::instance().shaderProgram("xform").bind();
    }

    glFrontFace(GL_CCW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    const glm::vec2 texSize = glm::vec2(
        static_cast<float>(capture.width()),
        static_cast<float>(capture.height())
    );

    if (fulldomeMode) {
        // TextureCut 2 equals showing only the middle square of a capturing a
        // widescreen input
        if (domeCut == 2) {
            glUniform2f(ScaleUV_L, texSize.y / texSize.x, 1.f);
            glUniform2f(OffsetUV_L, ((texSize.x - texSize.y) * 0.5f) / texSize.x, 0.f);
        }
        else {
            glUniform2f(ScaleUV_L, 1.f, 1.f);
            glUniform2f(OffsetUV_L, 0.f, 0.f);
        }

        glCullFace(GL_FRONT); // camera on the inside of the dome

        glUniformMatrix4fv(Matrix_L, 1, GL_FALSE, glm::value_ptr(mvp));
        dome->draw();
    }
    else {
        // plane mode
        glUniform2f(ScaleUV_L, 1.f, 1.f);
        glUniform2f(OffsetUV_L, 0.f, 0.f);

        glCullFace(GL_BACK);

        // transform and draw plane
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

        planeTransform = mvp * planeTransform;
        glUniformMatrix4fv(Matrix_L, 1, GL_FALSE, glm::value_ptr(planeTransform));
        plane->draw();
    }

    ShaderProgram::unbind();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void draw2DFun(const RenderData& data) {
    if (info) {
        unsigned int fontSize = static_cast<unsigned int>(9.f * data.window.scale().x);
        text::Font* font = text::FontManager::instance().font("SGCTFont", fontSize);
        constexpr float Padding = 10.f;

        const float resY = static_cast<float>(data.window.framebufferResolution().y);
        text::print(
            data.window,
            data.viewport,
            *font,
            text::Alignment::TopLeft,
            Padding,
            static_cast<float>(resY - fontSize) - Padding,
            glm::vec4(1.f, 1.f, 1.f, 1.f), // color
            "Format: %s\nResolution: %d x %d\nRate: %.2lf Hz",
            capture.format().c_str(), capture.width(), capture.height(), captureRate
        );
    }
}

void preSyncFun() {
    if (Engine::instance().isMaster()) {
        // if texture is uploaded then iterate the index
        if (serverUploadDone && clientsUploadDone) {
            numSyncedTex = static_cast<int32_t>(texIds.size());

            // only iterate up to the first new image, even if multiple images was added
            texIndex = numSyncedTex - serverUploadCount;

            serverUploadDone = false;
            clientsUploadDone = false;
        }
    }
}

void postSyncPreDrawFun() {
    Engine::instance().setStatsGraphVisibility(stats);

    if (takeScreenshot) {
        Engine::instance().takeScreenshot();
        takeScreenshot = false;
    }

    // set the flag frame synchronized for all viewports
    fulldomeMode = renderDome;
}

void initOGLFun(GLFWwindow* win) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    hiddenWindow = glfwCreateWindow(1, 1, "Thread Window", nullptr, win);

    if (!hiddenWindow) {
        Log::Info("Failed to create loader context");
    }

    // restore to normal
    glfwMakeContextCurrent(win);

    capture.initialize();

    allocateTexture();

    // start capture thread if host or load thread if master and not host
    const Node& thisNode = ClusterManager::instance().thisNode();
    if (thisNode.address() == capture.videoHost()) {
        captureThread = std::make_unique<std::thread>(captureLoop);
    }
    else if (Engine::instance().isMaster()) {
        loadThread = std::make_unique<std::thread>(threadWorker);
    }

    capture.setVideoDecoderCallback(uploadData);

    // chroma key color
    chromaKeyColors.push_back(glm::vec3(0.f, 0.f, 0.f));
    chromaKeyColors.push_back(glm::vec3(0.f, 1.f, 0.f));
    chromaKeyColors.push_back(glm::vec3(0.f, 0.f, 1.f));
    chromaKeyColors.push_back(glm::vec3(0.f, 177.f / 255.f, 64.f / 255.f));

    // create plane
    constexpr float PlaneWidth = 8.f;
    const float h = static_cast<float>(capture.height());
    const float w = static_cast<float>(capture.width());
    float planeHeight = PlaneWidth * h / w;
    plane = std::make_unique<utils::Plane>(PlaneWidth, planeHeight);

    // create dome
    dome = std::make_unique<utils::Dome>(7.4f, 180.f, 256, 128);

    ShaderManager::instance().addShaderProgram("xform", vertexShader, fragmentShader);
    {
        const ShaderProgram& prog = ShaderManager::instance().shaderProgram("xform");
        prog.bind();
        matrixLoc = glGetUniformLocation(prog.id(), "mvp");
        uvScaleLoc = glGetUniformLocation(prog.id(), "scaleUV");
        uvOffsetLoc = glGetUniformLocation(prog.id(), "offsetUV");
        glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);
        prog.unbind();
    }

    ShaderManager::instance().addShaderProgram(
        "chromakey",
        vertexShader,
        fragmentChromaKey
    );
    {
        const ShaderProgram& prog = ShaderManager::instance().shaderProgram("chromakey");
        prog.bind();
        chromaKeyMatrixLoc = glGetUniformLocation(prog.id(), "mvp");
        chromaKeyUvScaleLoc = glGetUniformLocation(prog.id(), "scaleUV");
        chromaKeyUvOffsetLoc = glGetUniformLocation(prog.id(), "offsetUV");
        chromaKeyColorLoc = glGetUniformLocation(prog.id(), "chromaKeyColor");
        glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);
        prog.unbind();
    }
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, info);
    serializeObject(data, stats);
    serializeObject(data, wireframe);
    serializeObject(data, texIndex);
    serializeObject(data, incrIndex);
    serializeObject(data, takeScreenshot);
    serializeObject(data, renderDome);
    serializeObject(data, domeCut);
    serializeObject(data, chromaKey);
    serializeObject(data, chromaKeyColorIdx);
    return data;
}

void decode(const std::vector<std::byte>& data) {
    unsigned int pos = 0;
    deserializeObject(data, pos, info);
    deserializeObject(data, pos, stats);
    deserializeObject(data, pos, wireframe);
    deserializeObject(data, pos, texIndex);
    deserializeObject(data, pos, incrIndex);
    deserializeObject(data, pos, takeScreenshot);
    deserializeObject(data, pos, renderDome);
    deserializeObject(data, pos, domeCut);
    deserializeObject(data, pos, chromaKey);
    deserializeObject(data, pos, chromaKeyColorIdx);
}

void cleanUpFun() {
    dome = nullptr;
    plane = nullptr;

    glDeleteTextures(1, &texId);
    texId = 0;

    glDeleteTextures(static_cast<GLsizei>(texIds.size()), texIds.data());
    texIds.clear();

    if (hiddenWindow) {
        glfwDestroyWindow(hiddenWindow);
    }
}

void keyCallback(Key key, Modifier, Action action, int) {
    if (Engine::instance().isMaster() && (action == Action::Press)) {
        switch (key) {
            case Key::C:
                chromaKey = !chromaKey;
                break;
            case Key::D:
                renderDome = true;
                break;
            case Key::S:
                stats = !stats;
                break;
            case Key::I:
                info = !info;
                break;
            case Key::F:
            case Key::W:
                wireframe = !wireframe;
                break;
            case Key::Key1:
                domeCut = 1;
                break;
            case Key::Key2:
                domeCut = 2;
                break;
            case Key::P:
                // plane mode
                renderDome = false;
                break;
            case Key::Left:
                if (numSyncedTex > 0) {
                    if (texIndex > incrIndex - 1) {
                        texIndex = texIndex - incrIndex;
                    }
                    else {
                        texIndex = numSyncedTex - 1;
                    }
                }
                break;
            case Key::Right:
                if (numSyncedTex > 0) {
                    texIndex = (texIndex + incrIndex) % numSyncedTex;
                }
                break;
            case Key::Up:
                if (chromaKeyColorIdx < chromaKeyColors.size()) {
                    chromaKeyColorIdx = chromaKeyColorIdx + 1;
                }
                break;
            case Key::Down:
                if (chromaKeyColorIdx > 0) {
                    chromaKeyColorIdx = chromaKeyColorIdx - 1;
                }
                break;
        }
    }
}

void dataTransferDecoder(void* data, int length, int packageId, int clientIndex) {
    Log::Info(
        "Decoding %d bytes in transfer id: %d on node %d", length, packageId, clientIndex
    );

    lastPackage = packageId;

    // read the image on slave
    readImage(reinterpret_cast<unsigned char*>(data), length);
    uploadTexture();
}

void dataTransferStatus(bool connected, int clientIndex) {
    Log::Info(
        "Transfer node %d is %s", clientIndex, connected ? "connected" : "disconnected"
    );
}

void dataTransferAcknowledge(int packageId, int clientIndex) {
    Log::Info("Transfer id: %d is completed on node %d", packageId, clientIndex);

    if (packageId == lastPackage) {
        static int counter = 0;
        counter++;
        if (counter == (ClusterManager::instance().numberOfNodes() - 1)) {
            clientsUploadDone = true;
            counter = 0;

            Log::Info(
                "Time to distribute and upload textures on cluster: %f ms",
                (sgct::Engine::getTime() - sendTimer) * 1000.0
            );
        }
    }
}

void dropCallback(int count, const char** paths) {
    if (Engine::instance().isMaster()) {
        std::vector<std::string> pathStrings;
        for (int i = 0; i < count; i++) {
            std::string tmpStr(paths[i]);

            // transform to lowercase
            std::transform(
                tmpStr.begin(), tmpStr.end(),
                tmpStr.begin(),
                [](char c) { return static_cast<char>(::tolower(c)); }
            );

            pathStrings.push_back(tmpStr);
        }

        // sort in alphabetical order
        std::sort(pathStrings.begin(), pathStrings.end());

        serverUploadCount = 0;

        // iterate all drop paths
        for (int i = 0; i < pathStrings.size(); i++) {
            std::string tmpStr = pathStrings[i];

            // find file type
            const size_t foundJpg = tmpStr.find(".jpg");
            const size_t foundJpeg = tmpStr.find(".jpeg");
            const size_t foundPng = tmpStr.find(".png");
            if (foundJpg != std::string::npos || foundJpeg != std::string::npos) {
                imagePaths.emplace_back(pathStrings[i], ImageType::JPEG);
                transfer = true; // tell transfer thread to start processing data
                serverUploadCount = serverUploadCount + 1;
            }
            else if (foundPng != std::string::npos) {
                imagePaths.emplace_back(pathStrings[i], ImageType::PNG);
                // tell transfer thread to start processing data
                transfer = true;
                serverUploadCount = serverUploadCount + 1;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    // arguments:
    //   -host <host which should capture>
    //   -video <device name>
    //   -option <key> <val>
    //   -flip
    //   -plane <azimuth> <elevation> <roll>
    //
    // to obtain video device names in windows use:
    //   ffmpeg -list_devices true -f dshow -i dummy
    // for mac:
    //   ffmpeg -f avfoundation -list_devices true -i ""
    //
    // to obtain device properties in windows use:
    //   ffmpeg -f dshow -list_options true -i video=<device name>
    //
    // For options look at: http://ffmpeg.org/ffmpeg-devices.html

    int i = 0;
    while (i < argc) {
        std::string_view a = argv[i];
        if ((a == "-host") && argc > (i + 1)) {
            capture.setVideoHost(argv[i + 1]);
        }
        else if ((a == "-video") && argc > (i + 1)) {
            capture.setVideoDevice(argv[i + 1]);
        }
        else if ((a == "-option") && argc > (i + 2)) {
            capture.addOption(
                std::make_pair<std::string, std::string>(argv[i + 1], argv[i + 2])
            );
        }
        else if (a == "-flip") {
            flipFrame = true;
        }
        else if ((a == "-plane") && argc > (i + 3)) {
            planeAzimuth = static_cast<float>(atof(argv[i + 1]));
            planeElevation = static_cast<float>(atof(argv[i + 2]));
            planeRoll = static_cast<float>(atof(argv[i + 3]));
        }

        i++; //iterate
    }

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initOGLFun;
    callbacks.draw = draw3DFun;
    callbacks.draw2D = draw2DFun;
    callbacks.preSync = preSyncFun;
    callbacks.postSyncPreDraw = postSyncPreDrawFun;
    callbacks.cleanUp = cleanUpFun;
    callbacks.keyboard = keyCallback;
    callbacks.drop = dropCallback;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.dataTransferDecode = dataTransferDecoder;
    callbacks.dataTransferStatus = dataTransferStatus;
    callbacks.dataTransferAcknowledge = dataTransferAcknowledge;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error & e) {
        Log::Error("%s", e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance().render();

    captureRunning = false;
    if (captureThread) {
        captureThread->join();
        captureThread = nullptr;
    }

    isRunning = false;
    if (loadThread) {
        loadThread->join();
        loadThread = nullptr;
    }

    Engine::destroy();
    exit(EXIT_SUCCESS);
}
