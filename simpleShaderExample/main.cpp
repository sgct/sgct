#include <stdlib.h>
#include <stdio.h>

#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();

int timeLoc = -1;

//variables to share across cluster
double time = 0.0;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreDrawFunction( myPreDrawFun );

	if( !gEngine->init() )
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
    //set current shader program
	sgct::ShaderManager::Instance()->bindShader( "SimpleColor" );
	glUniform1f( timeLoc, static_cast<float>( time ) );

	float speed = 50.0f;
	glRotatef(static_cast<float>( time ) * speed, 0.0f, 1.0f, 0.0f);

	//render a single triangle
	glBegin(GL_TRIANGLES);
		glVertex3f(-0.5f, -0.5f, 0.0f);
		glVertex3f(0.0f, 0.5f, 0.0f);
		glVertex3f(0.5f, -0.5f, 0.0f);
	glEnd();

	//unset current shader program
	sgct::ShaderManager::Instance()->unBindShader();
}

void myInitOGLFun()
{
	sgct::ShaderManager::Instance()->addShader( "SimpleColor", "simple.vert", "simple.frag" );
	sgct::ShaderManager::Instance()->bindShader( "SimpleColor" );

	timeLoc = sgct::ShaderManager::Instance()->getShader( "SimpleColor").getUniformLocation( "time" );

	sgct::ShaderManager::Instance()->unBindShader();
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble( time );
}

void myDecodeFun()
{
	time = sgct::SharedData::Instance()->readDouble();
}

void myPreDrawFun()
{
	if( gEngine->isMaster() )
	{
		time = glfwGetTime();
	}
}
