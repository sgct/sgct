#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"
#include <glm/gtc/matrix_inverse.hpp>

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

sgct_utils::SGCTSphere * mySphere = NULL;
GLint Matrix_Loc	= -1;
GLint NM_Loc		= -1;

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

	sgct::SharedData::Instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::Instance()->setDecodeFunction(myDecodeFun);

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

	//create scene transform (animation)
	glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
	
	glm::mat4 MVP		= gEngine->getActiveModelViewProjectionMatrix() * scene_mat;
	glm::mat4 MV		= gEngine->getActiveModelViewMatrix() * scene_mat;
	glm::mat3 NM		= glm::inverseTranspose( glm::mat3( MV ) );

	sgct::ShaderManager::Instance()->bindShader( "mrt" );

	glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix3fv(NM_Loc, 1, GL_FALSE, &NM[0][0]);

	//draw the box
	mySphere->draw();

	sgct::ShaderManager::Instance()->unBindShader();

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
	sgct::SGCTSettings::Instance()->setUseNormalTexture(true);
	//sgct::SGCTSettings::Instance()->setUseDepthTexture(true);
	gEngine->setNearAndFarClippingPlanes(0.1f, 10.0f);

	mySphere = new sgct_utils::SGCTSphere(1.0f, 128);

	//Set up backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); //our polygon winding is counter clockwise

	sgct::ShaderManager::Instance()->addShader( "mrt",
			"mrt.vert",
			"mrt.frag" );

	sgct::ShaderManager::Instance()->bindShader( "mrt" );

	Matrix_Loc = sgct::ShaderManager::Instance()->getShader( "mrt").getUniformLocation( "MVP" );
	NM_Loc = sgct::ShaderManager::Instance()->getShader( "mrt").getUniformLocation( "NM" );
	sgct::ShaderManager::Instance()->getShader( "mrt").bindFragDataLocation(0, "Color");
	sgct::ShaderManager::Instance()->getShader( "mrt").bindFragDataLocation(1, "Normal");

	sgct::ShaderManager::Instance()->unBindShader();
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble(&curr_time);
}

void myDecodeFun()
{
	sgct::SharedData::Instance()->readDouble(&curr_time);
}

void myCleanUpFun()
{
	if(mySphere != NULL)
		delete mySphere;
}
