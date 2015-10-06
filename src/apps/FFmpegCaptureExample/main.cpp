#include <stdlib.h>
#include <stdio.h>
#include <sgct.h>
#include "Capture.hpp"

sgct::Engine * gEngine;
Capture * gCapture = NULL;

//sgct callbacks
void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
void myKeyCallback(int key, int action);
void myContextCreationCallback(GLFWwindow * win);

//other callbacks
void uploadData(uint8_t ** data, int width, int height);

//functions
void parseArguments(int& argc, char**& argv);
void allocateTexture();
void captureLoop(void *arg);

sgct_utils::SGCTPlane * myPlane = NULL;

GLint Matrix_Loc = -1;
GLuint texId = GL_FALSE;

tthread::thread * workerThread;
GLFWwindow * hiddenWindow;
GLFWwindow * sharedWindow;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool info(false);
sgct::SharedBool stats(false);
sgct::SharedBool takeScreenshot(false);

//variables to share between threads
sgct::SharedBool workerRunning(true);

int main( int argc, char* argv[] )
{	
	gEngine = new sgct::Engine( argc, argv );
	gCapture = new Capture();

	// arguments:
	// -video <device name>
	// -option <key> <val>
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
	gEngine->setDrawFunction( myDrawFun );
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

void myDrawFun()
{
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix();


	sgct::ShaderManager::instance()->bindShaderProgram("xform");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texId);

	//draw the dome
	//glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);
	//myDome->draw();

	//transform and draw plane
	glm::mat4 planeTransform = glm::mat4(1.0f);
	planeTransform = glm::rotate(planeTransform, glm::radians(0.0f), glm::vec3(0.0f, -1.0f, 0.0f)); //azimuth
	planeTransform = glm::rotate(planeTransform, glm::radians(50.0f), glm::vec3(1.0f, 0.0f, 0.0f)); //elevation
	planeTransform = glm::rotate(planeTransform, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)); //roll
	planeTransform = glm::translate(planeTransform, glm::vec3(0.0f, 0.0f, -5.0f)); //distance

	planeTransform = MVP * planeTransform;
	glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &planeTransform[0][0]);
	myPlane->draw();

	sgct::ShaderManager::instance()->unBindShaderProgram();

	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
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
}

void myInitOGLFun()
{
	gCapture->init();

	//allocate texture
	allocateTexture();

	//start capture thread
	if (gEngine->isMaster())
		workerThread = new (std::nothrow) tthread::thread(captureLoop, NULL);

	std::function<void(uint8_t ** data, int width, int height)> callback = uploadData;
	gCapture->setVideoDecoderCallback(callback);
	
	float planeWidth = 8.0f;
	float planeHeight = planeWidth * (static_cast<float>(gCapture->getHeight()) / static_cast<float>(gCapture->getWidth()));
	myPlane = new sgct_utils::SGCTPlane(planeWidth, planeHeight);

	//Set up backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); //our polygon winding is counter clockwise

	sgct::ShaderManager::instance()->addShaderProgram( "xform",
			"xform.vert",
			"xform.frag" );

	sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

	Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
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
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readDouble(&curr_time);
	sgct::SharedData::instance()->readBool(&info);
	sgct::SharedData::instance()->readBool(&stats);
	sgct::SharedData::instance()->readBool(&takeScreenshot);
}

void myCleanUpFun()
{
	if (myPlane)
		delete myPlane;

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
		case 'S':
			if (action == SGCT_PRESS)
				stats.toggle();
			break;

		case 'I':
			if (action == SGCT_PRESS)
				info.toggle();
			break;

		case 'P':
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
		if (strcmp(argv[i], "-video") == 0 && argc >(i + 1))
		{
			gCapture->setVideoDevice(std::string(argv[i + 1]));
		}
		else if (strcmp(argv[i], "-option") == 0 && argc >(i + 2))
		{
			gCapture->addOption(
				std::make_pair(std::string(argv[i + 1]), std::string(argv[i + 2])));
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

			//flip and copy
			for (int row = height - 1; row > -1; row--)
			{
				memcpy(GPU_ptr + dataOffset, data[0] + row * stride, stride);
				dataOffset += stride;
			}

			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texId);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, 0);
		}
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
	
	while (workerRunning.getVal())
	{
		gCapture->poll();
		sgct::Engine::sleep(0.05); //pause the thread for a while
	}

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_FALSE);
	glDeleteBuffers(1, &PBO);

	glfwMakeContextCurrent(NULL); //detach context
}