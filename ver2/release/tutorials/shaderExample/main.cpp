#include <stdlib.h>
#include <stdio.h>

#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void keyCallback(int key, int action);

int time_loc = -1;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool reloadShader(false);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );
	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	//set current shader program
	sgct::ShaderManager::instance()->bindShaderProgram( "SimpleColor" );
	glUniform1f( time_loc, static_cast<float>( curr_time.getVal() ) );

	float speed = 50.0f;
	glRotatef(static_cast<float>( curr_time.getVal() ) * speed, 0.0f, 1.0f, 0.0f);

	//render a single triangle
	glBegin(GL_TRIANGLES);
		glVertex3f(-0.5f, -0.5f, 0.0f);
		glVertex3f(0.0f, 0.5f, 0.0f);
		glVertex3f(0.5f, -0.5f, 0.0f);
	glEnd();

	//unset current shader program
	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void myInitOGLFun()
{
	sgct::ShaderManager::instance()->addShaderProgram( "SimpleColor", "simple.vert", "simple.frag" );
	sgct::ShaderManager::instance()->bindShaderProgram( "SimpleColor" );

	time_loc = sgct::ShaderManager::instance()->getShaderProgram( "SimpleColor").getUniformLocation( "curr_time" );

	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void myEncodeFun()
{
	sgct::SharedData::instance()->writeDouble( &curr_time );
	sgct::SharedData::instance()->writeBool( &reloadShader );
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readDouble( &curr_time );
	sgct::SharedData::instance()->readBool( &reloadShader );
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
	if( reloadShader.getVal() )
	{
		reloadShader.setVal(false); //reset

		sgct::ShaderProgram sp = sgct::ShaderManager::instance()->getShaderProgram( "SimpleColor" );
		sp.reload();

		//reset location variables
		sgct::ShaderManager::instance()->bindShaderProgram( "SimpleColor" );
		time_loc = sgct::ShaderManager::instance()->getShaderProgram( "SimpleColor").getUniformLocation( "curr_time" );
		sgct::ShaderManager::instance()->unBindShaderProgram();
	}
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case SGCT_KEY_R:
			if(action == SGCT_PRESS)
				reloadShader.setVal(true);
			break;
		}
	}
}
