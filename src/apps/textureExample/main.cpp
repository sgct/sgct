#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

//test
sgct_utils::SGCTDome * gDome;

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
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	
	float speed = 50.0f;
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -2.0f);
	glRotatef(static_cast<float>( curr_time ) * speed, 0.0f, 1.0f, 0.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(myTextureIndex) );

	/*glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
			//glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
	glMatrixMode(GL_MODELVIEW);*/

	float boxSize = 2.0f;
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
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);

	glPushMatrix();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
	glLineWidth(3.0f);
	glTranslated(0.0, 0.0, 4.0);
	gDome->draw();
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
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

	gDome = new sgct_utils::SGCTDome(15.0f, 180.0f, 36, 9);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);
	//gEngine->setWireframe(true);

	//Enable backface culling
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CW); //our polygon winding is counter clockwise
	//glEnable(GL_CULL_FACE);
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble(curr_time);
}

void myDecodeFun()
{
	curr_time = sgct::SharedData::Instance()->readDouble();
}
