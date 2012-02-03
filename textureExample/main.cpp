#include <stdlib.h>
#include <stdio.h>

#include "sgct.h"

sgct::RenderEngine * gRenderEngine;

void myDrawFun();
void myPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();

unsigned int myTextureIndex; 

//variables to share across cluster
double time = 0.0;

sgct::SharedData mySharedData(256);

int main( int argc, char* argv[] )
{	
	gRenderEngine = new sgct::RenderEngine( mySharedData, argc, argv );

	gRenderEngine->setInitOGLFunction( myInitOGLFun );
	gRenderEngine->setDrawFunction( myDrawFun );
	gRenderEngine->setPreDrawFunction( myPreDrawFun );

	if( !gRenderEngine->init() )
	{
		delete gRenderEngine;
		return EXIT_FAILURE;
	}

	mySharedData.setEncodeFunction(myEncodeFun);
	mySharedData.setDecodeFunction(myDecodeFun);


	// Main loop
	gRenderEngine->render();

	// Clean up
	sgct::TextureManager::Instance()->Destroy();
	delete gRenderEngine;
	
	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	glRotatef(static_cast<float>(time)*10.0f, 0.0f, 1.0f, 0.0f);
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
	if( gRenderEngine->isSyncServer() )
	{
		time = gRenderEngine->getTime();
	}
}

void myInitOGLFun()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);

	sgct::TextureManager::Instance()->setAnisotropicFilterSize(4.0f);
	sgct::TextureManager::Instance()->loadTexure(myTextureIndex, "mug", "mug.png", true);
	
}

void myEncodeFun()
{
	mySharedData.writeDouble(time);
}

void myDecodeFun()
{
	time = mySharedData.readDouble();
}