#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <algorithm> //used for transform string to lowercase
#include <sgct.h>
#include "Capture.hpp"

sgct::Engine * gEngine;
Capture * gCapture = NULL;

//sgct callbacks
void myDraw3DFun();
void myDraw2DFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
void myKeyCallback(int key, int action);
void myContextCreationCallback(GLFWwindow * win);

sgct_utils::SGCTPlane * plane = NULL;
sgct_utils::SGCTDome * dome = NULL;

GLFWwindow * hiddenWindow;
GLFWwindow * sharedWindow;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool info(false);
sgct::SharedBool stats(false);
sgct::SharedBool takeScreenshot(false);
sgct::SharedBool wireframe(false);

//DomeImageViewer
void myDropCallback(int count, const char** paths);
void myDataTransferDecoder(void * receivedData, int receivedlength, int packageId, int clientIndex);
void myDataTransferStatus(bool connected, int clientIndex);
void myDataTransferAcknowledge(int packageId, int clientIndex);
void startDataTransfer();
void readImage(unsigned char * data, int len);
void uploadTexture();
void threadWorker(void *arg);

tthread::thread * loadThread;
tthread::mutex mutex;
std::vector<sgct_core::Image *> transImages;

sgct::SharedInt32 texIndex(-1);
sgct::SharedInt32 incrIndex(1);
sgct::SharedInt32 numSyncedTex(0);

sgct::SharedBool running(true);
sgct::SharedInt32 lastPackage(-1);
sgct::SharedBool transfer(false);
sgct::SharedBool serverUploadDone(false);
sgct::SharedInt32 serverUploadCount(0);
sgct::SharedBool clientsUploadDone(false);
sgct::SharedVector<std::pair<std::string, int>> imagePaths;
sgct::SharedVector<GLuint> texIds;
double sendTimer = 0.0;

enum imageType { IM_JPEG, IM_PNG };
const int headerSize = 1;

//FFmpegCapture
void uploadData(uint8_t ** data, int width, int height);
void parseArguments(int& argc, char**& argv);
void allocateTexture();
void captureLoop(void *arg);
void calculateStats();

GLint Matrix_Loc = -1;
GLint ScaleUV_Loc = -1;
GLint OffsetUV_Loc = -1;
GLint Matrix_Loc_CK = -1;
GLint ScaleUV_Loc_CK = -1;
GLint OffsetUV_Loc_CK = -1;
GLint ChromaKeyColor_Loc_CK = -1;
GLuint texId = GL_FALSE;

tthread::thread * captureThread;
bool flipFrame = false;
bool fulldomeMode = false;
float planeAzimuth = 0.0f;
float planeElevation = 33.0f;
float planeRoll = 0.0f;

sgct::SharedBool captureRunning(true);
sgct::SharedBool renderDome(fulldomeMode);
sgct::SharedDouble captureRate(0.0);
sgct::SharedInt32 domeCut(2);
sgct::SharedBool chromaKey(false);
sgct::SharedInt32 chromaKeyColorIdx(0);
std::vector<glm::vec3> chromaKeyColors;

int main( int argc, char* argv[] )
{
    //sgct::MessageHandler::instance()->setNotifyLevel(sgct::MessageHandler::NOTIFY_ALL);
    
    gEngine = new sgct::Engine( argc, argv );
    gCapture = new Capture();

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

    parseArguments(argc, argv);
    
    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDrawFunction( myDraw3DFun );
    gEngine->setDraw2DFunction( myDraw2DFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
    gEngine->setCleanUpFunction( myCleanUpFun );
    gEngine->setKeyboardCallbackFunction(myKeyCallback);
    gEngine->setContextCreationCallback(myContextCreationCallback);
    gEngine->setDropCallbackFunction(myDropCallback);

    if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
    {
        delete gEngine;
        return EXIT_FAILURE;
    }
    
    gEngine->setDataTransferCallback(myDataTransferDecoder);
    gEngine->setDataTransferStatusCallback(myDataTransferStatus);
    gEngine->setDataAcknowledgeCallback(myDataTransferAcknowledge);
    //gEngine->setDataTransferCompression(true, 6);

    //sgct::SharedData::instance()->setCompression(true);
    sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
    sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

    // Main loop
    gEngine->render();

    //kill capture thread
    captureRunning.setVal(false);
    if (captureThread)
    {
        captureThread->join();
        delete captureThread;
    }

    running.setVal(false);
    if (loadThread)
    {
        loadThread->join();
        delete loadThread;
    }

    // Clean up
    delete gCapture;
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void myDraw3DFun()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix();

    //Set up backface culling
    glCullFace(GL_BACK);

    if (texIndex.getVal() != -1)
    {
        sgct::ShaderManager::instance()->bindShaderProgram("xform");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texIds.getValAt(texIndex.getVal()));
        glUniform2f(ScaleUV_Loc, 1.f, 1.f);
        glUniform2f(OffsetUV_Loc, 0.f, 0.f);
        glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);

        glFrontFace(GL_CW);

        dome->draw();
        sgct::ShaderManager::instance()->unBindShaderProgram();

    }

    GLint ScaleUV_L = ScaleUV_Loc;
    GLint OffsetUV_L = OffsetUV_Loc;
    GLint Matrix_L = Matrix_Loc;
    if (chromaKey.getVal())
    {
        sgct::ShaderManager::instance()->bindShaderProgram("chromakey");
        glUniform3f(ChromaKeyColor_Loc_CK
            , chromaKeyColors[chromaKeyColorIdx.getVal()].r
            , chromaKeyColors[chromaKeyColorIdx.getVal()].g
            , chromaKeyColors[chromaKeyColorIdx.getVal()].b);
        ScaleUV_L = ScaleUV_Loc_CK;
        OffsetUV_L = OffsetUV_Loc_CK;
        Matrix_L = Matrix_Loc_CK;
    }
    else
    {
        sgct::ShaderManager::instance()->bindShaderProgram("xform");
    }

    glFrontFace(GL_CCW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    glm::vec2 texSize = glm::vec2(static_cast<float>(gCapture->getWidth()), static_cast<float>(gCapture->getHeight()));

    if (fulldomeMode)
    {
        // TextureCut 2 equals showing only the middle square of a capturing a widescreen input
        if (domeCut.getVal() == 2){
            glUniform2f(ScaleUV_L, texSize.y / texSize.x, 1.f);
            glUniform2f(OffsetUV_L, ((texSize.x - texSize.y)*0.5f) / texSize.x, 0.f);
        }
        else{
            glUniform2f(ScaleUV_L, 1.f, 1.f);
            glUniform2f(OffsetUV_L, 0.f, 0.f);
        }

        glCullFace(GL_FRONT); //camera on the inside of the dome

        glUniformMatrix4fv(Matrix_L, 1, GL_FALSE, &MVP[0][0]);
        dome->draw();
    }
    else //plane mode
    {
        glUniform2f(ScaleUV_L, 1.f, 1.f);
        glUniform2f(OffsetUV_L, 0.f, 0.f);

        glCullFace(GL_BACK);

        //transform and draw plane
        glm::mat4 planeTransform = glm::mat4(1.0f);
        planeTransform = glm::rotate(planeTransform, glm::radians(planeAzimuth), glm::vec3(0.0f, -1.0f, 0.0f)); //azimuth
        planeTransform = glm::rotate(planeTransform, glm::radians(planeElevation), glm::vec3(1.0f, 0.0f, 0.0f)); //elevation
        planeTransform = glm::rotate(planeTransform, glm::radians(planeRoll), glm::vec3(0.0f, 0.0f, 1.0f)); //roll
        planeTransform = glm::translate(planeTransform, glm::vec3(0.0f, 0.0f, -5.0f)); //distance

        planeTransform = MVP * planeTransform;
        glUniformMatrix4fv(Matrix_L, 1, GL_FALSE, &planeTransform[0][0]);
        plane->draw();
    }

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void myDraw2DFun()
{
    if (info.getVal())
    {
        unsigned int font_size = static_cast<unsigned int>(9.0f*gEngine->getCurrentWindowPtr()->getXScale());
        const sgct_text::Font * font = sgct_text::FontManager::instance()->getFont("SGCTFont", font_size);
        float padding = 10.0f;

        sgct_text::print(font,
            padding, static_cast<float>(gEngine->getCurrentWindowPtr()->getYFramebufferResolution() - font_size) - padding, //x and y pos
            glm::vec4(1.0, 1.0, 1.0, 1.0), //color
            "Format: %s\nResolution: %d x %d\nRate: %.2lf Hz",
            gCapture->getFormat(),
            gCapture->getWidth(),
            gCapture->getHeight(),
            captureRate.getVal());
    }
}

void myPreSyncFun()
{
    if( gEngine->isMaster() )
    {
        curr_time.setVal( sgct::Engine::getTime() );
        
        //if texture is uploaded then iterate the index
        if (serverUploadDone.getVal() && clientsUploadDone.getVal())
        {
            numSyncedTex = static_cast<int32_t>(texIds.getSize());
            
            //only iterate up to the first new image, even if multiple images was added
            texIndex = numSyncedTex - serverUploadCount.getVal();

            serverUploadDone = false;
            clientsUploadDone = false;
        }
    }
}

void myPostSyncPreDrawFun()
{
    gEngine->setDisplayInfoVisibility(info.getVal());
    gEngine->setStatsGraphVisibility(stats.getVal());
    //gEngine->setWireframe(wireframe.getVal());

    if (takeScreenshot.getVal())
    {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }

    fulldomeMode = renderDome.getVal(); //set the flag frame synchronized for all viewports
}

void myInitOGLFun()
{
    gCapture->init();

    //allocate texture
    allocateTexture();

    //start capture thread if host or load thread if master and not host
    sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();
    if (thisNode->getAddress() == gCapture->getVideoHost())
        captureThread = new (std::nothrow) tthread::thread(captureLoop, NULL);
    else if (gEngine->isMaster())
        loadThread = new (std::nothrow) tthread::thread(threadWorker, NULL);

    std::function<void(uint8_t ** data, int width, int height)> callback = uploadData;
    gCapture->setVideoDecoderCallback(callback);

    //chroma key color
    chromaKeyColors.push_back(glm::vec3(0.0, 0.0, 0.0));
    chromaKeyColors.push_back(glm::vec3(0.0, 1.0, 0.0));
    chromaKeyColors.push_back(glm::vec3(0.0, 0.0, 1.0));
    chromaKeyColors.push_back(glm::vec3(0.0, 177.0 / 255.0, 64.0 / 255.0));

    //create plane
    float planeWidth = 8.0f;
    float planeHeight = planeWidth * (static_cast<float>(gCapture->getHeight()) / static_cast<float>(gCapture->getWidth()));
    plane = new sgct_utils::SGCTPlane(planeWidth, planeHeight);

    //create dome
    dome = new sgct_utils::SGCTDome(7.4f, 180.0f, 256, 128);

    sgct::ShaderManager::instance()->addShaderProgram( "xform",
            "xform.vert",
            "xform.frag" );

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
    ScaleUV_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation("scaleUV");
    OffsetUV_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation("offsetUV");
    GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
    glUniform1i( Tex_Loc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();

    sgct::ShaderManager::instance()->addShaderProgram("chromakey",
        "xform.vert",
        "chromakey.frag");

    sgct::ShaderManager::instance()->bindShaderProgram("chromakey");

    Matrix_Loc_CK = sgct::ShaderManager::instance()->getShaderProgram("chromakey").getUniformLocation("MVP");
    ScaleUV_Loc_CK = sgct::ShaderManager::instance()->getShaderProgram("chromakey").getUniformLocation("scaleUV");
    OffsetUV_Loc_CK = sgct::ShaderManager::instance()->getShaderProgram("chromakey").getUniformLocation("offsetUV");
    ChromaKeyColor_Loc_CK = sgct::ShaderManager::instance()->getShaderProgram("chromakey").getUniformLocation("chromaKeyColor");
    GLint Tex_Loc_CK = sgct::ShaderManager::instance()->getShaderProgram("chromakey").getUniformLocation("Tex");
    glUniform1i(Tex_Loc_CK, 0);

    sgct::ShaderManager::instance()->unBindShaderProgram();

    sgct::Engine::checkForOGLErrors();
}

void myEncodeFun()
{
    sgct::SharedData::instance()->writeDouble(&curr_time);
    sgct::SharedData::instance()->writeBool(&info);
    sgct::SharedData::instance()->writeBool(&stats);
    sgct::SharedData::instance()->writeBool(&wireframe);
    sgct::SharedData::instance()->writeInt32(&texIndex);
    sgct::SharedData::instance()->writeInt32(&incrIndex);
    sgct::SharedData::instance()->writeBool(&takeScreenshot);
    sgct::SharedData::instance()->writeBool(&renderDome);
    sgct::SharedData::instance()->writeInt32(&domeCut);
    sgct::SharedData::instance()->writeBool(&chromaKey);
    sgct::SharedData::instance()->writeInt32(&chromaKeyColorIdx);
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readDouble(&curr_time);
    sgct::SharedData::instance()->readBool(&info);
    sgct::SharedData::instance()->readBool(&stats);
    sgct::SharedData::instance()->readBool(&wireframe);
    sgct::SharedData::instance()->readInt32(&texIndex);
    sgct::SharedData::instance()->readInt32(&incrIndex);
    sgct::SharedData::instance()->readBool(&takeScreenshot);
    sgct::SharedData::instance()->readBool(&renderDome);
    sgct::SharedData::instance()->readInt32(&domeCut);
    sgct::SharedData::instance()->readBool(&chromaKey);
    sgct::SharedData::instance()->readInt32(&chromaKeyColorIdx);
}

void myCleanUpFun()
{
    if (dome != NULL)
        delete dome;

    if (plane != NULL)
        delete plane;

    if (texId)
    {
        glDeleteTextures(1, &texId);
        texId = GL_FALSE;
    }
    
    for(std::size_t i=0; i < texIds.getSize(); i++)
    {
        GLuint tex = texIds.getValAt(i);
        if (tex)
        {
            glDeleteTextures(1, &tex);
            texIds.setValAt(i, GL_FALSE);
        }
    }
    texIds.clear();
    
    
    if(hiddenWindow)
        glfwDestroyWindow(hiddenWindow);
}

void myKeyCallback(int key, int action)
{
    if (gEngine->isMaster())
    {
        switch (key)
        {
        case SGCT_KEY_C:
            if (action == SGCT_PRESS)
                chromaKey.toggle();
            break;
        case SGCT_KEY_D:
            if (action == SGCT_PRESS)
                renderDome.setVal(true);
            break;
        case SGCT_KEY_S:
            if (action == SGCT_PRESS)
                stats.toggle();
            break;

        case SGCT_KEY_I:
            if (action == SGCT_PRESS)
                info.toggle();
            break;
                
        case SGCT_KEY_W:
            if (action == SGCT_PRESS)
                wireframe.toggle();
            break;

        case SGCT_KEY_F:
            if (action == SGCT_PRESS)
                wireframe.toggle();
            break;

        case SGCT_KEY_1:
            if (action == SGCT_PRESS)
                domeCut.setVal(1);
            break;

        case SGCT_KEY_2:
            if (action == SGCT_PRESS)
                domeCut.setVal(2);
            break;

            //plane mode
        case SGCT_KEY_P:
            if (action == SGCT_PRESS)
                renderDome.setVal(false);
            break;

        case SGCT_KEY_LEFT:
            if (action == SGCT_PRESS && numSyncedTex.getVal() > 0)
            {
                texIndex.getVal() > incrIndex.getVal() - 1 ? texIndex -= incrIndex.getVal() : texIndex.setVal(numSyncedTex.getVal() - 1);
                //fprintf(stderr, "Index set to: %d\n", texIndex.getVal());
            }
            break;

        case SGCT_KEY_RIGHT:
            if (action == SGCT_PRESS && numSyncedTex.getVal() > 0)
            {
                texIndex.setVal((texIndex.getVal() + incrIndex.getVal()) % numSyncedTex.getVal());
                //fprintf(stderr, "Index set to: %d\n", texIndex.getVal());
            }
            break;

        case SGCT_KEY_UP:
            if (action == SGCT_PRESS && chromaKeyColorIdx.getVal() < chromaKeyColors.size())
            {
                chromaKeyColorIdx.setVal(chromaKeyColorIdx.getVal() + 1);
            }
            break;

        case SGCT_KEY_DOWN:
            if (action == SGCT_PRESS && chromaKeyColorIdx.getVal() > 0)
            {
                chromaKeyColorIdx.setVal(chromaKeyColorIdx.getVal() - 1);
            }
            break;
        }
    }
}

void myContextCreationCallback(GLFWwindow * win)
{
    glfwWindowHint( GLFW_VISIBLE, GL_FALSE );
    
    sharedWindow = win;
    hiddenWindow = glfwCreateWindow( 1, 1, "Thread Window", NULL, sharedWindow );
     
    if( !hiddenWindow )
    {
        sgct::MessageHandler::instance()->print("Failed to create loader context!\n");
    }
    
    //restore to normal
    glfwMakeContextCurrent( sharedWindow );
}

void myDataTransferDecoder(void * receivedData, int receivedlength, int packageId, int clientIndex)
{
    sgct::MessageHandler::instance()->print("Decoding %d bytes in transfer id: %d on node %d\n", receivedlength, packageId, clientIndex);

    lastPackage.setVal(packageId);
    
    //read the image on slave
    readImage( reinterpret_cast<unsigned char*>(receivedData), receivedlength);
    uploadTexture();
}

void myDataTransferStatus(bool connected, int clientIndex)
{
    sgct::MessageHandler::instance()->print("Transfer node %d is %s.\n", clientIndex, connected ? "connected" : "disconnected");
}

void myDataTransferAcknowledge(int packageId, int clientIndex)
{
    sgct::MessageHandler::instance()->print("Transfer id: %d is completed on node %d.\n", packageId, clientIndex);
    
    static int counter = 0;
    if( packageId == lastPackage.getVal())
    {
        counter++;
        if( counter == (sgct_core::ClusterManager::instance()->getNumberOfNodes()-1) )
        {
            clientsUploadDone = true;
            counter = 0;
            
            sgct::MessageHandler::instance()->print("Time to distribute and upload textures on cluster: %f ms\n", (sgct::Engine::getTime() - sendTimer)*1000.0);
        }
    }
}

void threadWorker(void *arg)
{
    while (running.getVal())
    {
        //runs only on master
        if (transfer.getVal() && !serverUploadDone.getVal() && !clientsUploadDone.getVal())
        {
            startDataTransfer();
            transfer.setVal(false);
            
            //load textures on master
            uploadTexture();
            serverUploadDone = true;
            
            if(sgct_core::ClusterManager::instance()->getNumberOfNodes() == 1) //no cluster
            {
                clientsUploadDone = true;
            }
        }

        sgct::Engine::sleep(0.1); //ten iteration per second
    }
}

void startDataTransfer()
{
    //iterate
    int id = lastPackage.getVal();
    id++;

    //make sure to keep within bounds
    if(static_cast<int>(imagePaths.getSize()) > id)
    {
        sendTimer = sgct::Engine::getTime();

        int imageCounter = static_cast<int32_t>(imagePaths.getSize());
        lastPackage.setVal(imageCounter - 1);

        for (int i = id; i < imageCounter; i++)
        {
            //load from file
            std::pair<std::string, int> tmpPair = imagePaths.getValAt(static_cast<std::size_t>(i));

            std::ifstream file(tmpPair.first.c_str(), std::ios::binary);
            file.seekg(0, std::ios::end);
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<char> buffer(size + headerSize);
            char type = tmpPair.second;

            //write header (single unsigned char)
            buffer[0] = type;

            if (file.read(buffer.data() + headerSize, size))
            {
                //transfer
                gEngine->transferDataBetweenNodes(buffer.data(), static_cast<int>(buffer.size()), i);

                //read the image on master
                readImage(reinterpret_cast<unsigned char *>(buffer.data()), static_cast<int>(buffer.size()));
            }
        }
    }
}

void readImage(unsigned char * data, int len)
{
    mutex.lock();
    
    sgct_core::Image * img = new (std::nothrow) sgct_core::Image();
    
    char type = static_cast<char>(data[0]);
    
    bool result = false;
    switch( type )
    {
        case IM_JPEG:
            result = img->loadJPEG(reinterpret_cast<unsigned char*>(data + headerSize), len - headerSize);
            break;
            
        case IM_PNG:
            result = img->loadPNG(reinterpret_cast<unsigned char*>(data + headerSize), len - headerSize);
            break;
    }
    
    if (!result)
    {
        //clear if failed
        delete img;
    }
    else
        transImages.push_back(img);

    mutex.unlock();
}

void uploadTexture()
{
    mutex.lock();

    if (!transImages.empty())
    {
        glfwMakeContextCurrent(hiddenWindow);

        for (std::size_t i = 0; i < transImages.size(); i++)
        {
            if (transImages[i])
            {
                //create texture
                GLuint tex;
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                GLenum internalformat;
                GLenum type;
                unsigned int bpc = transImages[i]->getBytesPerChannel();

                switch (transImages[i]->getChannels())
                {
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

                glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, transImages[i]->getWidth(), transImages[i]->getHeight());
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, transImages[i]->getWidth(), transImages[i]->getHeight(), type, format, transImages[i]->getData());

                //---------------------
                // Disable mipmaps
                //---------------------
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                //unbind
                glBindTexture(GL_TEXTURE_2D, GL_FALSE);

                sgct::MessageHandler::instance()->print("Texture id %d loaded (%dx%dx%d).\n", tex, transImages[i]->getWidth(), transImages[i]->getHeight(), transImages[i]->getChannels());

                texIds.addVal(tex);

                delete transImages[i];
                transImages[i] = NULL;
            }
            else //if invalid load
            {
                texIds.addVal(GL_FALSE);
            }
        }//end for

        transImages.clear();
        glFinish();

        //restore
        glfwMakeContextCurrent(NULL);
    }//end if not empty

    mutex.unlock();
}

void myDropCallback(int count, const char** paths)
{
    if (gEngine->isMaster())
    {
        std::vector<std::string> pathStrings;
        for (int i = 0; i < count; i++)
        {
            //simpy pick the first path to transmit
            std::string tmpStr(paths[i]);

            //transform to lowercase
            std::transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);

            pathStrings.push_back(tmpStr);
        }

        //sort in alphabetical order
        std::sort(pathStrings.begin(), pathStrings.end());

        serverUploadCount.setVal(0);

        //iterate all drop paths
        for (int i = 0; i < pathStrings.size(); i++)
        {
            std::size_t found;

            std::string tmpStr = pathStrings[i];

            //find file type
            found = tmpStr.find(".jpg");
            if (found != std::string::npos)
            {
                imagePaths.addVal(std::pair<std::string, int>(pathStrings[i], IM_JPEG));
                transfer.setVal(true); //tell transfer thread to start processing data
                serverUploadCount++;
            }
            else
            {
                found = tmpStr.find(".jpeg");
                if (found != std::string::npos)
                {
                    imagePaths.addVal(std::pair<std::string, int>(pathStrings[i], IM_JPEG));
                    transfer.setVal(true); //tell transfer thread to start processing data
                    serverUploadCount++;
                }
                else
                {
                    found = tmpStr.find(".png");
                    if (found != std::string::npos)
                    {
                        imagePaths.addVal(std::pair<std::string, int>(pathStrings[i], IM_PNG));
                        transfer.setVal(true); //tell transfer thread to start processing data
                        serverUploadCount++;
                    }
                }
            }
        }
    }
}

void parseArguments(int& argc, char**& argv)
{
    int i = 0;
    while (i < argc)
    {
        if (strcmp(argv[i], "-host") == 0 && argc > (i + 1))
        {
            gCapture->setVideoHost(std::string(argv[i + 1]));
        }
        else if (strcmp(argv[i], "-video") == 0 && argc > (i + 1))
        {
            gCapture->setVideoDevice(std::string(argv[i + 1]));
        }
        else if (strcmp(argv[i], "-option") == 0 && argc > (i + 2))
        {
            gCapture->addOption(
                std::make_pair(std::string(argv[i + 1]), std::string(argv[i + 2])));
        }
        else if (strcmp(argv[i], "-flip") == 0)
        {
            flipFrame = true;
        }
        else if (strcmp(argv[i], "-plane") == 0 && argc > (i + 3))
        {
            planeAzimuth = static_cast<float>(atof(argv[i + 1]));
            planeElevation = static_cast<float>(atof(argv[i + 2]));
            planeRoll = static_cast<float>(atof(argv[i + 3]));
        }

        i++; //iterate
    }
}

void allocateTexture()
{
    int w = gCapture->getWidth();
    int h = gCapture->getHeight();

    if (w * h <= 0)
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Invalid texture size (%dx%d)!\n", w, h);
        return;
    }

    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, w, h);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void uploadData(uint8_t ** data, int width, int height)
{
    // At least two textures and GLSync objects
    // should be used to control that the uploaded texture is the same
    // for all viewports to prevent any tearing and maintain frame sync

    if (texId)
    {
        unsigned char * GPU_ptr = reinterpret_cast<unsigned char*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
        if (GPU_ptr)
        {
            int dataOffset = 0;
            int stride = width * 3;

            if (flipFrame)
            {
                for (int row = 0; row < height; row++)
                {
                    memcpy(GPU_ptr + dataOffset, data[0] + row * stride, stride);
                    dataOffset += stride;
                }
            }
            else
            {
                for (int row = height - 1; row > -1; row--)
                {
                    memcpy(GPU_ptr + dataOffset, data[0] + row * stride, stride);
                    dataOffset += stride;
                }
            }

            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, 0);
        }

        calculateStats();
    }
}

void captureLoop(void *arg)
{
    glfwMakeContextCurrent(hiddenWindow);

    int dataSize = gCapture->getWidth() * gCapture->getHeight() * 3;
    GLuint PBO;
    glGenBuffers(1, &PBO);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);

    while (captureRunning.getVal())
    {
        gCapture->poll();
        //sgct::Engine::sleep(0.02); //take a short break to offload the cpu

        if (gEngine->isMaster() && transfer.getVal() && !serverUploadDone.getVal() && !clientsUploadDone.getVal())
        {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

            startDataTransfer();
            transfer.setVal(false);

            //load textures on master
            uploadTexture();
            serverUploadDone = true;

            if (sgct_core::ClusterManager::instance()->getNumberOfNodes() == 1) //no cluster
            {
                clientsUploadDone = true;
            }

            sgct::Engine::sleep(0.1); //ten iteration per second

            //restore for capture
            glfwMakeContextCurrent(hiddenWindow);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
        }
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glDeleteBuffers(1, &PBO);

    glfwMakeContextCurrent(NULL); //detach context
}

void calculateStats()
{
    double timeStamp = sgct::Engine::getTime();
    static double previousTimeStamp = timeStamp;
    static double numberOfSamples = 0.0;
    static double duration = 0.0;

    timeStamp = sgct::Engine::getTime();
    duration += timeStamp - previousTimeStamp;
    previousTimeStamp = timeStamp;
    numberOfSamples++;

    if (duration >= 1.0)
    {
        captureRate.setVal(numberOfSamples / duration);
        duration = 0.0;
        numberOfSamples = 0.0;
    }
}