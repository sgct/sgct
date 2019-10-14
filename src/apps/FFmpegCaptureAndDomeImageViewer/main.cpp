#include "capture.h"
#include <sgct.h>
#include <sgct/ClusterManager.h>
#include <sgct/Image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>

#ifdef SGCT_HAS_TEXT
#include <sgct/Font.h>
#include <sgct/FontManager.h>
#include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT

// abock (2019-09-08);  I fixed up and converted this file blind as I didn't have a way
// to test it properly.  So no guarantee that this is actually working

namespace {
    enum class ImageType { JPEG, PNG };
    
    constexpr const int HeaderSize = 1;

    sgct::Engine* gEngine;
    Capture gCapture;

    std::unique_ptr<sgct::utils::Plane> plane;
    std::unique_ptr<sgct::utils::Dome> dome;

    GLFWwindow* hiddenWindow;
    GLFWwindow* sharedWindow;

    // variables to share across cluster
    sgct::SharedDouble currTime(0.0);
    sgct::SharedBool info(false);
    sgct::SharedBool stats(false);
    sgct::SharedBool takeScreenshot(false);
    sgct::SharedBool wireframe(false);

    std::unique_ptr<std::thread> loadThread;
    std::mutex mutex;
    std::vector<sgct::core::Image*> transImages;

    sgct::SharedInt32 texIndex(-1);
    sgct::SharedInt32 incrIndex(1);
    sgct::SharedInt32 numSyncedTex(0);

    sgct::SharedBool running(true);
    sgct::SharedInt32 lastPackage(-1);
    sgct::SharedBool transfer(false);
    sgct::SharedBool serverUploadDone(false);
    sgct::SharedInt32 serverUploadCount(0);
    sgct::SharedBool clientsUploadDone(false);
    sgct::SharedVector<std::pair<std::string, ImageType>> imagePaths;
    sgct::SharedVector<GLuint> texIds;
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

    sgct::SharedBool captureRunning(true);
    sgct::SharedBool renderDome(fulldomeMode);
    sgct::SharedDouble captureRate(0.0);
    sgct::SharedInt32 domeCut(2);
    sgct::SharedBool chromaKey(false);
    sgct::SharedInt32 chromaKeyColorIdx(0);
    std::vector<glm::vec3> chromaKeyColors;

    constexpr const char* vertexShader = R"(
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

    constexpr const char* fragmentShader = R"(
  #version 330 core

  uniform sampler2D tex;
  uniform vec2 scaleUV;
  uniform vec2 offsetUV;

  in vec2 uv;
  out vec4 color;

  void main() { color = texture(tex, (uv.st * scaleUV) + offsetUV); }
)";

    constexpr const char* fragmentChromaKey = R"(
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
    vec4 texColor = texture(tex, (uv.st * scaleUV) + offsetUV);
   
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
    const int w = gCapture.getWidth();
    const int h = gCapture.getHeight();

    if (w * h <= 0) {
        MessageHandler::printError("Invalid texture size (%dx%d)", w, h);
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
    std::unique_lock lk(mutex);

    core::Image* img = new core::Image();

    char type = static_cast<char>(data[0]);
    assert(type == 0 || type == 1);
    ImageType t = ImageType(type);

    bool result = false;
    switch (t) {
        case ImageType::JPEG:
            result = img->loadJPEG(
                reinterpret_cast<unsigned char*>(data + HeaderSize),
                len - HeaderSize
            );
            break;
        case ImageType::PNG:
            result = img->loadPNG(
                reinterpret_cast<unsigned char*>(data + HeaderSize),
                len - HeaderSize
            );
            break;
    }

    if (!result) {
        // clear if failed
        delete img;
    }
    else {
        transImages.push_back(img);
    }
}

void startDataTransfer() {
    // iterate
    int id = lastPackage.getVal();
    id++;

    // make sure to keep within bounds
    if (static_cast<int>(imagePaths.getSize()) > id) {
        sendTimer = Engine::getTime();

        int imageCounter = static_cast<int32_t>(imagePaths.getSize());
        lastPackage.setVal(imageCounter - 1);

        for (int i = id; i < imageCounter; i++) {
            // load from file
            std::pair<std::string, ImageType> tmpPair =
                imagePaths.getValAt(static_cast<size_t>(i));

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
                gEngine->transferDataBetweenNodes(
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
    std::unique_lock lk(mutex);

    if (transImages.empty()) {
        return;
    }

    glfwMakeContextCurrent(hiddenWindow);

    for (size_t i = 0; i < transImages.size(); i++) {
        if (!transImages[i]) {
            texIds.addVal(0);
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
        const unsigned int bpc = transImages[i]->getBytesPerChannel();

        switch (transImages[i]->getChannels()) {
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
            transImages[i]->getWidth(),
            transImages[i]->getHeight()
        );
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            transImages[i]->getWidth(),
            transImages[i]->getHeight(),
            type,
            format,
            transImages[i]->getData()
        );

        // Disable mipmaps
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // unbind
        glBindTexture(GL_TEXTURE_2D, 0);

        MessageHandler::printInfo(
            "Texture id %d loaded (%dx%dx%d)",
            tex, transImages[i]->getWidth(), transImages[i]->getHeight(),
            transImages[i]->getChannels()
        );

        texIds.addVal(tex);

        delete transImages[i];
        transImages[i] = NULL;
    }

    transImages.clear();
    glFinish();

    // restore
    glfwMakeContextCurrent(NULL);
}

void captureLoop() {
    glfwMakeContextCurrent(hiddenWindow);

    const int dataSize = gCapture.getWidth() * gCapture.getHeight() * 3;
    GLuint PBO;
    glGenBuffers(1, &PBO);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);

    while (captureRunning.getVal()) {
        gCapture.poll();

        if (gEngine->isMaster() && transfer.getVal() && !serverUploadDone.getVal() &&
            !clientsUploadDone.getVal())
        {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

            startDataTransfer();
            transfer.setVal(false);

            // load textures on master
            uploadTexture();
            serverUploadDone = true;

            if (core::ClusterManager::instance()->getNumberOfNodes() == 1) {
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
        captureRate.setVal(numberOfSamples / duration);
        duration = 0.0;
        numberOfSamples = 0.0;
    }
}

void threadWorker() {
    while (running.getVal()) {
        // runs only on master
        if (transfer.getVal() && !serverUploadDone.getVal() &&
            !clientsUploadDone.getVal())
        {
            startDataTransfer();
            transfer.setVal(false);

            // load textures on master
            uploadTexture();
            serverUploadDone = true;

            if (core::ClusterManager::instance()->getNumberOfNodes() == 1) {
                // no cluster
                clientsUploadDone = true;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void uploadData(uint8_t** data, int width, int height) {
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

void draw3DFun() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    const glm::mat4 mvp = gEngine->getCurrentModelViewProjectionMatrix();

    // Set up backface culling
    glCullFace(GL_BACK);

    if (texIndex.getVal() != -1) {
        ShaderManager::instance()->bindShaderProgram("xform");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texIds.getValAt(texIndex.getVal()));
        glUniform2f(uvScaleLoc, 1.f, 1.f);
        glUniform2f(uvOffsetLoc, 0.f, 0.f);
        glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        glFrontFace(GL_CW);

        dome->draw();
        ShaderManager::instance()->unBindShaderProgram();
    }

    GLint ScaleUV_L = uvScaleLoc;
    GLint OffsetUV_L = uvOffsetLoc;
    GLint Matrix_L = matrixLoc;
    if (chromaKey.getVal()) {
        ShaderManager::instance()->bindShaderProgram("chromakey");
        glUniform3fv(
            chromaKeyColorLoc,
            1,
            glm::value_ptr(chromaKeyColors[chromaKeyColorIdx.getVal()])
        );
        ScaleUV_L = chromaKeyUvScaleLoc;
        OffsetUV_L = chromaKeyUvOffsetLoc;
        Matrix_L = chromaKeyMatrixLoc;
    }
    else {
        ShaderManager::instance()->bindShaderProgram("xform");
    }

    glFrontFace(GL_CCW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    const glm::vec2 texSize = glm::vec2(
        static_cast<float>(gCapture.getWidth()),
        static_cast<float>(gCapture.getHeight())
    );

    if (fulldomeMode) {
        // TextureCut 2 equals showing only the middle square of a capturing a
        // widescreen input
        if (domeCut.getVal() == 2) {
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

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void draw2DFun() {
    if (info.getVal()) {
        const unsigned int fontSize = static_cast<unsigned int>(
            9.f * gEngine->getCurrentWindow().getScale().x
        );
        text::Font* font = text::FontManager::instance()->getFont(
            "SGCTFont",
            fontSize
        );
        constexpr const float padding = 10.0f;

        const float resY = gEngine->getCurrentWindow().getFramebufferResolution().y;
        text::print(
            *font,
            text::TextAlignMode::TopLeft,
            padding,
            static_cast<float>(resY - fontSize) - padding,
            glm::vec4(1.f, 1.f, 1.f, 1.f), // color
            "Format: %s\nResolution: %d x %d\nRate: %.2lf Hz",
            gCapture.getFormat(), gCapture.getWidth(), gCapture.getHeight(),
            captureRate.getVal());
    }
}

void preSyncFun() {
    if (gEngine->isMaster()) {
        currTime.setVal(Engine::getTime());
        
        // if texture is uploaded then iterate the index
        if (serverUploadDone.getVal() && clientsUploadDone.getVal()) {
            numSyncedTex = static_cast<int32_t>(texIds.getSize());
            
            // only iterate up to the first new image, even if multiple images was added
            texIndex = numSyncedTex.getVal() - serverUploadCount.getVal();

            serverUploadDone = false;
            clientsUploadDone = false;
        }
    }
}

void postSyncPreDrawFun() {
    gEngine->setDisplayInfoVisibility(info.getVal());
    gEngine->setStatsGraphVisibility(stats.getVal());

    if (takeScreenshot.getVal()) {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }

    // set the flag frame synchronized for all viewports
    fulldomeMode = renderDome.getVal();
}

void initOGLFun() {
    gCapture.init();

    allocateTexture();

    // start capture thread if host or load thread if master and not host
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    if (thisNode->getAddress() == gCapture.getVideoHost()) {
        captureThread = std::make_unique<std::thread>(captureLoop);
    }
    else if (gEngine->isMaster()) {
        loadThread = std::make_unique<std::thread>(threadWorker);
    }

    gCapture.setVideoDecoderCallback(uploadData);

    // chroma key color
    chromaKeyColors.push_back(glm::vec3(0.f, 0.f, 0.f));
    chromaKeyColors.push_back(glm::vec3(0.f, 1.f, 0.f));
    chromaKeyColors.push_back(glm::vec3(0.f, 0.f, 1.f));
    chromaKeyColors.push_back(glm::vec3(0.f, 177.f / 255.f, 64.f / 255.f));

    // create plane
    constexpr const float planeWidth = 8.f;
    const float h = static_cast<float>(gCapture.getHeight());
    const float w = static_cast<float>(gCapture.getWidth());
    float planeHeight = planeWidth * h / w;
    plane = std::make_unique<utils::Plane>(planeWidth, planeHeight);

    // create dome
    dome = std::make_unique<utils::Dome>(7.4f, 180.f, 256, 128);

    ShaderManager& sm = *ShaderManager::instance();
    sm.addShaderProgram(
        "xform",
        vertexShader,
        fragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    sm.bindShaderProgram("xform");

    matrixLoc = sm.getShaderProgram("xform").getUniformLocation("mvp");
    uvScaleLoc = sm.getShaderProgram("xform").getUniformLocation("scaleUV");
    uvOffsetLoc = sm.getShaderProgram("xform").getUniformLocation("offsetUV");
    GLint textureLocation = sm.getShaderProgram("xform").getUniformLocation("tex");
    glUniform1i(textureLocation, 0);
    sm.unBindShaderProgram();

    sm.addShaderProgram(
        "chromakey",
        vertexShader,
        fragmentChromaKey,
        ShaderProgram::ShaderSourceType::String
    );
    sm.bindShaderProgram("chromakey");

    chromaKeyMatrixLoc = sm.getShaderProgram("chromakey").getUniformLocation("mvp");
    chromaKeyUvScaleLoc = sm.getShaderProgram("chromakey").getUniformLocation("scaleUV");
    chromaKeyUvOffsetLoc = sm.getShaderProgram("chromakey").getUniformLocation("offsetUV");
    chromaKeyColorLoc = sm.getShaderProgram("chromakey").getUniformLocation("chromaKeyColor");
    GLint textureLocationCk = sm.getShaderProgram("chromakey").getUniformLocation("tex");
    glUniform1i(textureLocationCk, 0);

    sm.unBindShaderProgram();

    Engine::checkForOGLErrors();
}

void encode() {
    SharedData::instance()->writeDouble(currTime);
    SharedData::instance()->writeBool(info);
    SharedData::instance()->writeBool(stats);
    SharedData::instance()->writeBool(wireframe);
    SharedData::instance()->writeInt32(texIndex);
    SharedData::instance()->writeInt32(incrIndex);
    SharedData::instance()->writeBool(takeScreenshot);
    SharedData::instance()->writeBool(renderDome);
    SharedData::instance()->writeInt32(domeCut);
    SharedData::instance()->writeBool(chromaKey);
    SharedData::instance()->writeInt32(chromaKeyColorIdx);
}

void decode() {
    SharedData::instance()->readDouble(currTime);
    SharedData::instance()->readBool(info);
    SharedData::instance()->readBool(stats);
    SharedData::instance()->readBool(wireframe);
    SharedData::instance()->readInt32(texIndex);
    SharedData::instance()->readInt32(incrIndex);
    SharedData::instance()->readBool(takeScreenshot);
    SharedData::instance()->readBool(renderDome);
    SharedData::instance()->readInt32(domeCut);
    SharedData::instance()->readBool(chromaKey);
    SharedData::instance()->readInt32(chromaKeyColorIdx);
}

void cleanUpFun() {
    dome = nullptr;
    plane = nullptr;

    glDeleteTextures(1, &texId);
    texId = 0;
    
    for (size_t i=0; i < texIds.getSize(); i++) {
        GLuint tex = texIds.getValAt(i);
        glDeleteTextures(1, &tex);
    }
    texIds.clear();
    
    if (hiddenWindow) {
        glfwDestroyWindow(hiddenWindow);
    }
}

void keyCallback(int key, int, int action, int) {
    if (gEngine->isMaster() && (action == action::Press)) {
        switch (key) {
            case key::C:
                chromaKey.setVal(!chromaKey.getVal());
                break;
            case key::D:
                renderDome.setVal(true);
                break;
            case key::S:
                stats.setVal(!stats.getVal());
                break;
            case key::I:
                info.setVal(!info.getVal());
                break;
            case key::F:
            case key::W:
                wireframe.setVal(!wireframe.getVal());
                break;
            case key::Key1:
                domeCut.setVal(1);
                break;
            case key::Key2:
                domeCut.setVal(2);
                break;
            case key::P:
                // plane mode
                renderDome.setVal(false);
                break;
            case key::Left:
                if (numSyncedTex.getVal() > 0) {
                    if (texIndex.getVal() > incrIndex.getVal() - 1) {
                        texIndex.setVal(texIndex.getVal() - incrIndex.getVal());
                    }
                    else {
                        texIndex.setVal(numSyncedTex.getVal() - 1);
                    }
                }
                break;
            case key::Right:
                if (numSyncedTex.getVal() > 0) {
                    texIndex.setVal(
                        (texIndex.getVal() + incrIndex.getVal()) % numSyncedTex.getVal()
                    );
                }
                break;
            case key::Up:
                if (chromaKeyColorIdx.getVal() < chromaKeyColors.size()) {
                    chromaKeyColorIdx.setVal(chromaKeyColorIdx.getVal() + 1);
                }
                break;
            case key::Down:
                if (chromaKeyColorIdx.getVal() > 0) {
                    chromaKeyColorIdx.setVal(chromaKeyColorIdx.getVal() - 1);
                }
                break;
        }
    }
}

void contextCreationCallback(GLFWwindow* win) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    
    sharedWindow = win;
    hiddenWindow = glfwCreateWindow(1, 1, "Thread Window", nullptr, sharedWindow);
     
    if (!hiddenWindow) {
        MessageHandler::printInfo("Failed to create loader context");
    }
    
    // restore to normal
    glfwMakeContextCurrent(sharedWindow);
}

void dataTransferDecoder(void* receivedData, int receivedLength, int packageId,
                           int clientIndex)
{
    MessageHandler::printInfo(
        "Decoding %d bytes in transfer id: %d on node %d",
        receivedLength, packageId, clientIndex
    );

    lastPackage.setVal(packageId);
    
    // read the image on slave
    readImage(reinterpret_cast<unsigned char*>(receivedData), receivedLength);
    uploadTexture();
}

void dataTransferStatus(bool connected, int clientIndex) {
    MessageHandler::printInfo(
        "Transfer node %d is %s", clientIndex, connected ? "connected" : "disconnected"
    );
}

void dataTransferAcknowledge(int packageId, int clientIndex) {
    MessageHandler::printInfo(
        "Transfer id: %d is completed on node %d", packageId, clientIndex
    );
    
    static int counter = 0;
    if (packageId == lastPackage.getVal()) {
        counter++;
        if (counter == (core::ClusterManager::instance()->getNumberOfNodes() - 1)) {
            clientsUploadDone = true;
            counter = 0;
            
            MessageHandler::printInfo(
                "Time to distribute and upload textures on cluster: %f ms",
                (sgct::Engine::getTime() - sendTimer) * 1000.0
            );
        }
    }
}

void dropCallback(int count, const char** paths) {
    if (gEngine->isMaster()) {
        std::vector<std::string> pathStrings;
        for (int i = 0; i < count; i++) {
            std::string tmpStr(paths[i]);

            // transform to lowercase
            std::transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);

            pathStrings.push_back(tmpStr);
        }

        // sort in alphabetical order
        std::sort(pathStrings.begin(), pathStrings.end());

        serverUploadCount.setVal(0);

        // iterate all drop paths
        for (int i = 0; i < pathStrings.size(); i++) {
            std::string tmpStr = pathStrings[i];

            // find file type
            const size_t foundJpg = tmpStr.find(".jpg");
            const size_t foundJpeg = tmpStr.find(".jpeg");
            const size_t foundPng = tmpStr.find(".png");
            if (foundJpg != std::string::npos || foundJpeg != std::string::npos) {
                imagePaths.addVal(std::pair<std::string, ImageType>(pathStrings[i], ImageType::JPEG));
                transfer.setVal(true); //tell transfer thread to start processing data
                serverUploadCount.setVal(serverUploadCount.getVal() + 1 );
            }
            else if (foundPng != std::string::npos) {
                imagePaths.addVal(std::pair<std::string, ImageType>(pathStrings[i], ImageType::PNG));
                // tell transfer thread to start processing data
                transfer.setVal(true);
                serverUploadCount.setVal(serverUploadCount.getVal() + 1);
            }
        }
    }
}

void parseArguments(int& argc, char**& argv) {
    int i = 0;
    while (i < argc) {
        std::string_view arg = argv[i];
        if ((arg == "-host") && argc > (i + 1)) {
            gCapture.setVideoHost(argv[i + 1]);
        }
        else if ((arg == "-video") && argc > (i + 1)) {
            gCapture.setVideoDevice(argv[i + 1]);
        }
        else if ((arg == "-option") && argc > (i + 2)) {
            gCapture.addOption(
                std::make_pair<std::string, std::string>(argv[i + 1], argv[i + 2])
            );
        }
        else if (arg == "-flip") {
            flipFrame = true;
        }
        else if ((arg == "-plane") && argc > (i + 3)) {
            planeAzimuth = static_cast<float>(atof(argv[i + 1]));
            planeElevation = static_cast<float>(atof(argv[i + 2]));
            planeRoll = static_cast<float>(atof(argv[i + 3]));
        }

        i++; //iterate
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

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

    parseArguments(argc, argv);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(draw3DFun);
    gEngine->setDraw2DFunction(draw2DFun);
    gEngine->setPreSyncFunction(preSyncFun);
    gEngine->setPostSyncPreDrawFunction(postSyncPreDrawFun);
    gEngine->setCleanUpFunction(cleanUpFun);
    gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setContextCreationCallback(contextCreationCallback);
    gEngine->setDropCallbackFunction(dropCallback);

    if (!gEngine->init(Engine::RunMode::Default_Mode, cluster)){
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->setDataTransferCallback(dataTransferDecoder);
    gEngine->setDataTransferStatusCallback(dataTransferStatus);
    gEngine->setDataAcknowledgeCallback(dataTransferAcknowledge);

    sgct::SharedData::instance()->setEncodeFunction(encode);
    sgct::SharedData::instance()->setDecodeFunction(decode);

    gEngine->render();

    captureRunning.setVal(false);
    if (captureThread) {
        captureThread->join();
        captureThread = nullptr;
    }

    running.setVal(false);
    if (loadThread) {
        loadThread->join();
        loadThread = nullptr;
    }

    gCapture.cleanup();
    delete gEngine;

    exit(EXIT_SUCCESS);
}
