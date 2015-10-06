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
void keyCallback(int key, int action);

//other callbacks
void uploadData(uint8_t ** data, int width, int height);

//functions
void parseArguments(int& argc, char**& argv);
void allocateTexture();

sgct_utils::SGCTBox * myBox = NULL;
//sgct_utils::SGCTPlane * myPlane = NULL;
GLint Matrix_Loc = -1;
GLint Flip_Loc = -1;
GLuint texId = GL_FALSE;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool info(false);
sgct::SharedBool stats(false);
sgct::SharedBool takeScreenshot(false);

int main( int argc, char* argv[] )
{	
	gEngine = new sgct::Engine( argc, argv );
	gCapture = new Capture();

	parseArguments(argc, argv);

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
	gEngine->setCleanUpFunction( myCleanUpFun );
	gEngine->setKeyboardCallbackFunction(keyCallback);

	if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

	// Main loop
	gEngine->render();

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

	double speed = 0.44;

	//create scene transform (animation)
	glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
	scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));
	scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * (speed/2.0) ), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;

	glActiveTexture(GL_TEXTURE0);

	sgct::ShaderManager::instance()->bindShaderProgram("xform");

	glUniform1i(Flip_Loc, 0);
	glBindTexture(GL_TEXTURE_2D, texId);

	glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);

	//draw the box
	myBox->draw();
	//myPlane->draw();

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
	gCapture->poll();

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

	std::function<void(uint8_t ** data, int width, int height)> callback = uploadData;
	gCapture->setVideoDecoderCallback(callback);
	
	//set background
	sgct::Engine::instance()->setClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	
	myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
	//myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::CubeMap);
	//myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::SkyBox);

	//myPlane = new sgct_utils::SGCTPlane(2.0f, 2.0f);

	//Set up backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); //our polygon winding is counter clockwise

	sgct::ShaderManager::instance()->addShaderProgram( "xform",
			"xform.vert",
			"xform.frag" );

	sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

	Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
	GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
	Flip_Loc = sgct::ShaderManager::instance()->getShaderProgram("xform").getUniformLocation("flip");
	glUniform1i( Tex_Loc, 0 );
	glUniform1i( Flip_Loc, 0);

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
	if(myBox)
		delete myBox;

	//if (myPlane)
	//	delete myPlane;

	if (texId)
	{
		glDeleteTextures(1, &texId);
		texId = GL_FALSE;
	}
}

void keyCallback(int key, int action)
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
	/*if (texId)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texId);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, data[0]);
	}*/
}
