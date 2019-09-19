#include <stdlib.h>
#include <stdio.h>
#include <sgct.h>
#include "capture.h"

namespace {
    sgct::Engine* gEngine;
    Capture* gCapture = nullptr;
} // namespace

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
void myContextCreationCallback(GLFWwindow* win);

//other callbacks
void uploadData(uint8_t** data, int width, int height);

//functions
void parseArguments(int& argc, char**& argv);
void allocateTexture();
void captureLoop();
void calculateStats();

sgct_utils::SGCTPlane * myPlane = NULL;
sgct_utils::SGCTDome * myDome = NULL;

GLint Matrix_Loc = -1;
GLint ScaleUV_Loc = -1;
GLint OffsetUV_Loc = -1;
GLuint texId = GL_FALSE;

std::thread * workerThread;
GLFWwindow * hiddenWindow;
GLFWwindow * sharedWindow;
bool flipFrame = false;
bool fulldomeMode = false;
float planeAzimuth = 0.0f;
float planeElevation = 33.0f;
float planeRoll = 0.0f;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool info(false);
sgct::SharedBool stats(false);
sgct::SharedBool takeScreenshot(false);

//variables to share between threads
sgct::SharedBool workerRunning(true);
sgct::SharedBool renderDome(fulldomeMode);
sgct::SharedDouble captureRate(0.0);
sgct::SharedInt32 domeCut(2);

int main( int argc, char* argv[] )
{    
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
    gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
    gEngine->setCleanUpFunction( myCleanUpFun );
    gEngine->setKeyboardCallbackFunction( myKeyCallback );
    gEngine->setContextCreationCallback( myContextCreationCallback );

    if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
    {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
    sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

    // Main loop
    gEngine->render();

    //kill worker thread
    workerRunning.setVal(false);
    if (workerThread)
    {
        workerThread->join();
        delete workerThread;
    }

    // Clean up
    delete gCapture;
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void myDraw3DFun()
{
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix();


    sgct::ShaderManager::instance()->bindShaderProgram("xform");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    glm::vec2 texSize = glm::vec2(static_cast<float>(gCapture->getWidth()), static_cast<float>(gCapture->getHeight()));

    if (fulldomeMode)
    {
        // TextureCut 2 equals showing only the middle square of a capturing a widescreen input
        if (domeCut.getVal() == 2){
            glUniform2f(ScaleUV_Loc, texSize.y / texSize.x, 1.f);
            glUniform2f(OffsetUV_Loc, ((texSize.x - texSize.y)*0.5f) / texSize.x, 0.f);
        }
        else{
            glUniform2f(ScaleUV_Loc, 1.f, 1.f);
            glUniform2f(OffsetUV_Loc, 0.f, 0.f);
        }

        glCullFace(GL_FRONT); //camera on the inside of the dome

        glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);
        myDome->draw();
    }
    else //plane mode
    {
        glUniform2f(ScaleUV_Loc, 1.f, 1.f);
        glUniform2f(OffsetUV_Loc, 0.f, 0.f);

        glCullFace(GL_BACK);
        
        //transform and draw plane
        glm::mat4 planeTransform = glm::mat4(1.0f);
        planeTransform = glm::rotate(planeTransform, glm::radians(planeAzimuth), glm::vec3(0.0f, -1.0f, 0.0f)); //azimuth
        planeTransform = glm::rotate(planeTransform, glm::radians(planeElevation), glm::vec3(1.0f, 0.0f, 0.0f)); //elevation
        planeTransform = glm::rotate(planeTransform, glm::radians(planeRoll), glm::vec3(0.0f, 0.0f, 1.0f)); //roll
        planeTransform = glm::translate(planeTransform, glm::vec3(0.0f, 0.0f, -5.0f)); //distance

        planeTransform = MVP * planeTransform;
        glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &planeTransform[0][0]);
        myPlane->draw();
    }

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
}

void myDraw2DFun()
{
    if (info.getVal())
    {
        unsigned int font_size = static_cast<unsigned int>(9.0f*gEngine->getCurrentWindowPtr()->getXScale());
        sgct_text::Font * font = sgct_text::FontManager::instance()->getFont("SGCTFont", font_size);
        float padding = 10.0f;

        sgct_text::print(font, sgct_text::TOP_LEFT,
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
    }
}

void myPostSyncPreDrawFun()
{
    gEngine->setDisplayInfoVisibility(info.getVal());
    gEngine->setStatsGraphVisibility(stats.getVal());
    
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

    //start capture thread
    sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();
    if (thisNode->getAddress() == gCapture->getVideoHost())
        workerThread = new (std::nothrow) std::thread(captureLoop);

    std::function<void(uint8_t ** data, int width, int height)> callback = uploadData;
    gCapture->setVideoDecoderCallback(callback);
    
    //create plane
    float planeWidth = 8.0f;
    float planeHeight = planeWidth * (static_cast<float>(gCapture->getHeight()) / static_cast<float>(gCapture->getWidth()));
    myPlane = new sgct_utils::SGCTPlane(planeWidth, planeHeight);
    
    //create dome
    myDome = new sgct_utils::SGCTDome(7.4f, 180.0f, 256, 128);

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise

    sgct::ShaderManager::instance()->addShaderProgram( "xform",
            "xform.vert",
            "xform.frag" );

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "mvp" );
    ScaleUV_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation("scaleUV");
    OffsetUV_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation("offsetUV");
    GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "tex" );
    glUniform1i( Tex_Loc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();

    sgct::Engine::checkForOGLErrors();
}

void myEncodeFun()
{
    sgct::SharedData::instance()->writeDouble(&curr_time);
    sgct::SharedData::instance()->writeBool(&info);
    sgct::SharedData::instance()->writeBool(&stats);
    sgct::SharedData::instance()->writeBool(&takeScreenshot);
    sgct::SharedData::instance()->writeBool(&renderDome);
    sgct::SharedData::instance()->writeInt32(&domeCut);
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readDouble(&curr_time);
    sgct::SharedData::instance()->readBool(&info);
    sgct::SharedData::instance()->readBool(&stats);
    sgct::SharedData::instance()->readBool(&takeScreenshot);
    sgct::SharedData::instance()->readBool(&renderDome);
    sgct::SharedData::instance()->readInt32(&domeCut);
}

void myCleanUpFun()
{
    if (myPlane)
        delete myPlane;

    if (myDome)
        delete myDome;

    if (texId)
    {
        glDeleteTextures(1, &texId);
        texId = GL_FALSE;
    }
}

void myKeyCallback(int key, int action)
{
    if (gEngine->isMaster())
    {
        switch (key)
        {
        //dome mode
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
            
        case SGCT_KEY_PRINT_SCREEN:
        case SGCT_KEY_F10:
            if (action == SGCT_PRESS)
                takeScreenshot.setVal(true);
            break;
        }
    }
}

void myContextCreationCallback(GLFWwindow * win)
{
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

    sharedWindow = win;
    hiddenWindow = glfwCreateWindow(1, 1, "Thread Window", NULL, sharedWindow);

    if (!hiddenWindow)
    {
        sgct::MessageHandler::instance()->print("Failed to create loader context!\n");
    }

    //restore to normal
    glfwMakeContextCurrent(sharedWindow);
}

void parseArguments(int& argc, char**& argv)
{
    int i = 0;
    while (i<argc)
    {
        if (strcmp(argv[i], "-host") == 0 && argc > (i + 1))
        {
            gCapture->setVideoHost(std::string(argv[i + 1]));
        }
        else if (strcmp(argv[i], "-video") == 0 && argc >(i + 1))
        {
            gCapture->setVideoDevice(std::string(argv[i + 1]));
        }
        else if (strcmp(argv[i], "-option") == 0 && argc >(i + 2))
        {
            gCapture->addOption(
                std::make_pair(std::string(argv[i + 1]), std::string(argv[i + 2])));
        }
        else if (strcmp(argv[i], "-flip") == 0)
        {
            flipFrame = true;
        }
        else if (strcmp(argv[i], "-plane") == 0 && argc >(i + 3))
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

void captureLoop()
{
    glfwMakeContextCurrent(hiddenWindow);

    int dataSize = gCapture->getWidth() * gCapture->getHeight() * 3;
    GLuint PBO;
    glGenBuffers(1, &PBO);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
    
    while (workerRunning.getVal())
    {
        gCapture->poll();
        //sgct::Engine::sleep(0.02); //take a short break to offload the cpu
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_FALSE);
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