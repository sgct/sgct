#include <stdlib.h>
#include <stdio.h>

#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();

unsigned int myTextureIndex;

//variables to share across cluster
double curr_time = 0.0;

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
	float speed = 50.0f;
	glRotatef(static_cast<float>( curr_time ) * speed, 0.0f, 1.0f, 0.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(myTextureIndex) );
	glBegin(GL_QUADS);

	glTexCoord2d(0.0,0.0);	glVertex3f( -0.5f, -0.5f, 0.0f);
	glTexCoord2d(0.0,1.0);	glVertex3f( -0.5f, 0.5f, 0.0f);
	glTexCoord2d(1.0,1.0);	glVertex3f( 0.5f, 0.5f, 0.0f);
	glTexCoord2d(1.0,0.0);	glVertex3f( 0.5f, -0.5f, 0.0f);

	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void myPreDrawFun()
{
	if( gEngine->isMaster() )
	{
		//@TODO JOEL __APPLE__ SOMEHOW glfwGetTime needs to be run in engine function, dosent work to use it directly here.
		curr_time = gEngine->getTime();
	//	printf("curr time %f\n", curr_time);
	}
}

void myInitOGLFun()
{
	sgct::TextureManager::Instance()->setAnisotropicFilterSize(4.0f);
	sgct::TextureManager::Instance()->loadTexure(myTextureIndex, "mug", "mug.png", true);
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble(curr_time);
}

void myDecodeFun()
{
	curr_time = sgct::SharedData::Instance()->readDouble();
}
