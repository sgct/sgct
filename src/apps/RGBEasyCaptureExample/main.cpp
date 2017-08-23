#include <stdlib.h>
#include <stdio.h>
#include <sgct.h>
#include "RGBEasyCapture.hpp"

sgct::Engine * gEngine;
RGBEasyCapture * gCapture = NULL;

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

//functions
void parseArguments(int& argc, char**& argv);
void captureLoop(void *arg);
void calculateStats();

sgct_utils::SGCTPlane * myPlane = NULL;
sgct_utils::SGCTDome * myDome = NULL;

GLint Matrix_Loc = -1;
GLint ScaleUV_Loc = -1;
GLint OffsetUV_Loc = -1;
GLuint texId = GL_FALSE;

tthread::thread * workerThread;
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
sgct::SharedBool workerRunning(false);
sgct::SharedBool renderDome(fulldomeMode);
sgct::SharedDouble captureRate(0.0);
sgct::SharedInt32 domeCut(2);

int main( int argc, char* argv[] )
{    
    gEngine = new sgct::Engine( argc, argv );
    gCapture = new RGBEasyCapture();

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

    if( !gEngine->init( sgct::Engine::OpenGL_4_3_Core_Profile ) )
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

	if (workerRunning.getVal() && gCapture->prepareForRendering()) {
		glm::vec2 texSize = glm::vec2(static_cast<float>(gCapture->getWidth()), static_cast<float>(gCapture->getHeight()));

		if (fulldomeMode)
		{
			// TextureCut 2 equals showing only the middle square of a capturing a widescreen input
			if (domeCut.getVal() == 2) {
				glUniform2f(ScaleUV_Loc, texSize.y / texSize.x, 1.f);
				glUniform2f(OffsetUV_Loc, ((texSize.x - texSize.y)*0.5f) / texSize.x, 0.f);
			}
			else {
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
			/*planeTransform = glm::rotate(planeTransform, glm::radians(planeAzimuth), glm::vec3(0.0f, -1.0f, 0.0f)); //azimuth
			planeTransform = glm::rotate(planeTransform, glm::radians(planeElevation), glm::vec3(1.0f, 0.0f, 0.0f)); //elevation
			planeTransform = glm::rotate(planeTransform, glm::radians(planeRoll), glm::vec3(0.0f, 0.0f, 1.0f)); //roll*/
			planeTransform = glm::translate(planeTransform, glm::vec3(0.0f, 0.0f, -5.0f)); //distance

			planeTransform = MVP * planeTransform;
			glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &planeTransform[0][0]);
			myPlane->draw();
		}

		gCapture->renderingCompleted();
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
            "Resolution: %d x %d\nRate: %.2lf Hz",
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
    //start capture thread (which will intialize RGBEasy things)
    sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();
	if (thisNode->getAddress() == gCapture->getCaptureHost()) {
		workerRunning.setVal(true);

		// start capture thread
		//workerThread = new (std::nothrow) tthread::thread(captureLoop, NULL);
		if (gCapture->initialize()) {
			// initalize capture OpenGL (running on this thread)
			gCapture->initializeGL();

			// Start capture
			gCapture->runCapture();
		}
	}
    
    //create plane
    float planeWidth = 8.0f;
	float planeHeight = 8.0f;//planeWidth * (static_cast<float>(gCapture->getHeight()) / static_cast<float>(gCapture->getWidth()));
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

    Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
    ScaleUV_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation("scaleUV");
    OffsetUV_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation("offsetUV");
    GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
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
            gCapture->setCaptureHost(std::string(argv[i + 1]));
        }
        else if (strcmp(argv[i], "-input") == 0 && argc >(i + 1))
        {
            gCapture->setCaptureInput(static_cast<int>(atoi(argv[i + 1])));
        }
		else if (strcmp(argv[i], "-ganging") == 0)
		{
			gCapture->setCaptureGanging(true);
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

void captureLoop(void *arg)
{
    //glfwMakeContextCurrent(hiddenWindow);

	if (gCapture->initialize()) {
		gCapture->runCapture();


		while (workerRunning.getVal())
		{
			//gCapture->poll();
			//sgct::Engine::sleep(0.02); //take a short break to offload the cpu
		}

		gCapture->deinitialize();
	}

    //glfwMakeContextCurrent(NULL); //detach context
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