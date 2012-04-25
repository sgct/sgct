#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
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
	gEngine->setPreSyncFunction( myPreSyncFun );

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
	sgct::TextureManager::Destroy();
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

	float boxSize = 1.5f;
	glBegin(GL_QUADS);

	//front
	glTexCoord2d(0.0,0.0);	glVertex3f( -boxSize/2.0f,	-boxSize/2.0f,	boxSize/2.0f);
	glTexCoord2d(0.0,1.0);	glVertex3f( -boxSize/2.0f,	boxSize/2.0f,	boxSize/2.0f);
	glTexCoord2d(1.0,1.0);	glVertex3f( boxSize/2.0f,	boxSize/2.0f,	boxSize/2.0f);
	glTexCoord2d(1.0,0.0);	glVertex3f( boxSize/2.0f,	-boxSize/2.0f,	boxSize/2.0f);

	//back
	glTexCoord2d(1.0,0.0);	glVertex3f( -boxSize/2.0f,	-boxSize/2.0f,	-boxSize/2.0f);
	glTexCoord2d(0.0,0.0);	glVertex3f( boxSize/2.0f,	-boxSize/2.0f,	-boxSize/2.0f);
	glTexCoord2d(0.0,1.0);	glVertex3f( boxSize/2.0f,	boxSize/2.0f,	-boxSize/2.0f);
	glTexCoord2d(1.0,1.0);	glVertex3f( -boxSize/2.0f,	boxSize/2.0f,	-boxSize/2.0f);

	//left
	glTexCoord2d(0.0,0.0);	glVertex3f( -boxSize/2.0f,	-boxSize/2.0f,	-boxSize/2.0f);
	glTexCoord2d(0.0,1.0);	glVertex3f( -boxSize/2.0f,	boxSize/2.0f,	-boxSize/2.0f);
	glTexCoord2d(1.0,1.0);	glVertex3f( -boxSize/2.0f,	boxSize/2.0f,	boxSize/2.0f);
	glTexCoord2d(1.0,0.0);	glVertex3f( -boxSize/2.0f,	-boxSize/2.0f,	boxSize/2.0f);

	//right
	glTexCoord2d(1.0,0.0);	glVertex3f( boxSize/2.0f,	-boxSize/2.0f,	-boxSize/2.0f);
	glTexCoord2d(0.0,0.0);	glVertex3f( boxSize/2.0f,	-boxSize/2.0f,	boxSize/2.0f);
	glTexCoord2d(0.0,1.0);	glVertex3f( boxSize/2.0f,	boxSize/2.0f,	boxSize/2.0f);
	glTexCoord2d(1.0,1.0);	glVertex3f( boxSize/2.0f,	boxSize/2.0f,	-boxSize/2.0f);

	//top
	glTexCoord2d(1.0,0.0);	glVertex3f( -boxSize/2.0f,	boxSize/2.0f,	-boxSize/2.0f);
	glTexCoord2d(0.0,0.0);	glVertex3f( boxSize/2.0f,	boxSize/2.0f,	-boxSize/2.0f);
	glTexCoord2d(0.0,1.0);	glVertex3f( boxSize/2.0f,	boxSize/2.0f,	boxSize/2.0f);
	glTexCoord2d(1.0,1.0);	glVertex3f( -boxSize/2.0f,	boxSize/2.0f,	boxSize/2.0f);

	//bottom
	glTexCoord2d(0.0,0.0);	glVertex3f( -boxSize/2.0f,	-boxSize/2.0f,	-boxSize/2.0f);
	glTexCoord2d(0.0,1.0);	glVertex3f( -boxSize/2.0f,	-boxSize/2.0f,	boxSize/2.0f);
	glTexCoord2d(1.0,1.0);	glVertex3f( boxSize/2.0f,	-boxSize/2.0f,	boxSize/2.0f);
	glTexCoord2d(1.0,0.0);	glVertex3f( boxSize/2.0f,	-boxSize/2.0f,	-boxSize/2.0f);

	glEnd();

	glDisable(GL_TEXTURE_2D);
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		curr_time = sgct::Engine::getTime();
	}
}

void myInitOGLFun()
{
	sgct::TextureManager::Instance()->setAnisotropicFilterSize(4.0f);
	sgct::TextureManager::Instance()->loadTexure(myTextureIndex, "box", "box.png", true);

	//Enable backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CW); //our polygon winding is counter clockwise
	glEnable(GL_CULL_FACE);
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble(curr_time);
}

void myDecodeFun()
{
	curr_time = sgct::SharedData::Instance()->readDouble();
}
