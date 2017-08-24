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
void renderToTextureSetup();
void calculateStats();

sgct_utils::SGCTPlane * myPlane = NULL;
sgct_utils::SGCTPlane * mySquare = NULL;
sgct_utils::SGCTDome * myDome = NULL;

GLint Matrix_Loc = -1;
GLint Matrix_Loc_RT = -1;
GLint ScaleUV_Loc = -1;
GLint OffsetUV_Loc = -1;
GLuint texId = GL_FALSE;

//RRender to texture struct
struct RT
{
	unsigned int texture;
	unsigned int fbo;
	unsigned int renderBuffer;
	unsigned long width;
	unsigned long height;
};
RT captureGangingRT;

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
sgct::SharedBool captureRunning(false);
sgct::SharedBool renderDome(fulldomeMode);
sgct::SharedDouble captureRate(0.0);
sgct::SharedInt32 domeCut(1);

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

    // Clean up
	sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();
	if (thisNode->getAddress() == gCapture->getCaptureHost()) {
		gCapture->deinitializeGL();
		gCapture->deinitialize();
		delete gCapture;
	}
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

	if (captureRunning.getVal() && gCapture->prepareForRendering()) {
		if (fulldomeMode)
		{
			//Rendering to square texture when assumig ganing (i.e. from 2x1 to 1x2) as 1x2 does not seem to function properly
			if(gCapture->getGanging()) {
				sgct::ShaderManager::instance()->bindShaderProgram("sbs2tb");

				sgct_core::OffScreenBuffer * fbo = gEngine->getCurrentFBO();

				//get viewport data and set the viewport
				glViewport(0, 0, captureGangingRT.width, captureGangingRT.height);

				//bind fbo
				glBindFramebuffer(GL_FRAMEBUFFER, captureGangingRT.fbo);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, captureGangingRT.texture, 0);

				//transform
				glm::mat4 planeTransform = glm::mat4(1.0f);
				glUniformMatrix4fv(Matrix_Loc_RT, 1, GL_FALSE, &planeTransform[0][0]);

				glCullFace(GL_BACK);

				//draw square
				mySquare->draw();

				sgct::ShaderManager::instance()->unBindShaderProgram();

				//restore
				if (fbo)
					fbo->bind();
				sgct::ShaderManager::instance()->bindShaderProgram("xform");
				const int * coords = gEngine->getCurrentViewportPixelCoords();
				glViewport(coords[0], coords[1], coords[2], coords[3]);

				//bind our RT such that we can use it for rendering
				glBindTexture(GL_TEXTURE_2D, captureGangingRT.texture);
			}

			/* Test to see what we would get back
			glUniform2f(ScaleUV_Loc, 1.f, 1.f);
			glUniform2f(OffsetUV_Loc, 0.f, 0.f);

			glm::mat4 planeTransform = glm::mat4(1.0f);
			glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &planeTransform[0][0]);

			glCullFace(GL_BACK);

			//draw square
			mySquare->draw();*/

			// TextureCut 2 equals showing only the middle square of a capturing a widescreen input
			if (!gCapture->getGanging() && domeCut.getVal() == 2) {
				glm::vec2 texSize = glm::vec2(static_cast<float>(gCapture->getWidth()), static_cast<float>(gCapture->getHeight()));
				glUniform2f(ScaleUV_Loc, texSize.y / texSize.x, 1.f);
				glUniform2f(OffsetUV_Loc, ((texSize.x - texSize.y)*0.5f) / texSize.x, 0.f);
			}
			else {
				glUniform2f(ScaleUV_Loc, 1.f, 1.f);
				glUniform2f(OffsetUV_Loc, 0.f, 0.f);
			}

			glFrontFace(GL_CW);
			//glCullFace(GL_BACK); //camera on the inside of the dome

			glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);
			myDome->draw();
		}
		else //plane mode
		{
			glCullFace(GL_BACK);

			//Rendering to square texture when assumig ganing (i.e. from 2x1 to 1x2) as 1x2 does not seem to function properly
			if (gCapture->getGanging() && domeCut.getVal() == 2) {
				sgct::ShaderManager::instance()->bindShaderProgram("sbs2tb");

				glm::mat4 planeTransform = glm::mat4(1.0f);
				glUniformMatrix4fv(Matrix_Loc_RT, 1, GL_FALSE, &planeTransform[0][0]);

				//draw square
				mySquare->draw();
			}
			else
			{
				glUniform2f(ScaleUV_Loc, 1.f, 1.f);
				glUniform2f(OffsetUV_Loc, 0.f, 0.f);

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
		}

		gCapture->renderingCompleted();
	}

    sgct::ShaderManager::instance()->unBindShaderProgram();
	glFrontFace(GL_CCW);

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
		captureRunning.setVal(true);

		// start capture thread
		if (gCapture->initialize()) {
			// initalize capture OpenGL (running on this thread)
			gCapture->initializeGL();

			// Start capture
			gCapture->runCapture();
		}

		// check if we are ganing inputs
		// thus we assume 2x1 (sbs) which we want to change to 1x2 (tb)
		if (gCapture->getGanging()) {
			renderToTextureSetup();
		}
	}
    
    //create plane
    float planeWidth = 8.0f;
	float planeHeight = planeWidth * (static_cast<float>(gCapture->getHeight()) / static_cast<float>(gCapture->getWidth()));
    myPlane = new sgct_utils::SGCTPlane(planeWidth, planeHeight);

	//create square
	mySquare = new sgct_utils::SGCTPlane(2.0f, 2.0f);
    
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

	sgct::ShaderManager::instance()->addShaderProgram("sbs2tb",
		"xform.vert",
		"sbs2tb.frag");

	sgct::ShaderManager::instance()->bindShaderProgram("sbs2tb");

	Matrix_Loc_RT = sgct::ShaderManager::instance()->getShaderProgram("sbs2tb").getUniformLocation("MVP");
	GLint Tex_Loc_RT = sgct::ShaderManager::instance()->getShaderProgram("sbs2tb").getUniformLocation("Tex");
	glUniform1i(Tex_Loc_RT, 0);

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

	if (mySquare)
		delete mySquare;

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
		else if (strcmp(argv[i], "-fulldome") == 0)
		{
			renderDome.setVal(true);
		}

        i++; //iterate
    }
}

void renderToTextureSetup()
{
	captureGangingRT.width = gCapture->getWidth() / 2;
	captureGangingRT.height = gCapture->getHeight() * 2;
	captureGangingRT.fbo = GL_FALSE;
	captureGangingRT.renderBuffer = GL_FALSE;
	captureGangingRT.texture = GL_FALSE;

	//create targets
	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &(captureGangingRT.texture));
	glBindTexture(GL_TEXTURE_2D, captureGangingRT.texture);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, captureGangingRT.width, captureGangingRT.height);

	//---------------------
	// Disable mipmaps
	//---------------------
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, captureGangingRT.width, captureGangingRT.height, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);

	gEngine->checkForOGLErrors();

	glBindTexture(GL_TEXTURE_2D, GL_FALSE);

	glGenFramebuffers(1, &(captureGangingRT.fbo));
	glGenRenderbuffers(1, &(captureGangingRT.renderBuffer));

	//setup color buffer
	glBindFramebuffer(GL_FRAMEBUFFER, captureGangingRT.fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, captureGangingRT.renderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, captureGangingRT.width, captureGangingRT.height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, captureGangingRT.renderBuffer);

	//Does the GPU support current FBO configuration?
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		sgct::MessageHandler::instance()->print("Something went wrong creating FBO!\n");

	gEngine->checkForOGLErrors();

	//unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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