//tell Spout to use GLEW
#define USE_GLEW
#include "spout.h"

//avoid include conflicts between spout and sgct
#define SGCT_WINDOWS_INCLUDE
#define SGCT_WINSOCK_INCLUDE

#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"


sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

sgct_utils::SGCTBox * myBox = NULL;
GLint Matrix_Loc = -1;

SpoutReceiver * spoutReceiver = NULL;
char spoutSenderName[256];
unsigned int spoutWidth;
unsigned int spoutHeight;
bool spoutInited = false;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setCleanUpFunction( myCleanUpFun );

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
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	double speed = 25.0;

	//create scene transform (animation)
	glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
	scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));
	scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * (speed/2.0) ), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 MVP = gEngine->getActiveModelViewProjectionMatrix() * scene_mat;

	glActiveTexture(GL_TEXTURE0);

	//spout init
	if (!spoutInited && spoutReceiver->CreateReceiver(spoutSenderName, spoutWidth, spoutHeight, true))
	{	
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Spout: Initing %ux%u texture from '%s'.\n", spoutWidth, spoutHeight, spoutSenderName);
		spoutInited = true;
	}

	bool spooutError = true;
	if (spoutInited)
	{
		if (spoutReceiver->ReceiveTexture(spoutSenderName, spoutWidth, spoutHeight))
		{
			//sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Spout: Receiving %ux%u texture from '%s'.\n", SpoutWidth, SpoutHeight, SpoutSenderName);
			spoutReceiver->BindSharedTexture();
			spooutError = false;
		}
		else
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Spout disconnected.\n");
			
			spoutInited = false; //reset if disconnected
			spoutSenderName[0] = NULL;
			spoutReceiver->ReleaseReceiver();
		}
	}

	
	if (spooutError)
		glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box") );

	sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

	glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);

	//draw the box
	myBox->draw();

	sgct::ShaderManager::instance()->unBindShaderProgram();

	if (!spooutError)
		spoutReceiver->UnBindSharedTexture();

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

void myInitOGLFun()
{
	//setup spout
	spoutSenderName[0] = NULL;
	spoutReceiver = new SpoutReceiver;	// Create a new Spout receiver
	
	//set background
	sgct::Engine::instance()->setClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	
	sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
	sgct::TextureManager::instance()->loadTexure("box", "box.png", true);

	myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
	//myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::CubeMap);
	//myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::SkyBox);

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
}

void myEncodeFun()
{
	sgct::SharedData::instance()->writeDouble(&curr_time);
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readDouble(&curr_time);
}

void myCleanUpFun()
{
	if(myBox)
		delete myBox;

	if (spoutReceiver)
	{
		spoutReceiver->ReleaseReceiver();
		delete spoutReceiver;
	}
}
