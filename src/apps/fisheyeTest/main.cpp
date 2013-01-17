#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myInitOGLFun();

unsigned int textureIndexes[4];

int main( int argc, char* argv[] )
{

	// Allocate
	gEngine = new sgct::Engine( argc, argv );

	// Bind your functions
	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );

	// Init the engine
	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	// Main loop
	gEngine->render();

	// Clean up (de-allocate)
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	glColor3f(1.0f,1.0f,1.0f);

	float dd = 15.0f; //dome diameter

	//right face
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(textureIndexes[0]) );
	glBegin(GL_QUADS);
		glTexCoord2d(0.0,0.0); glVertex3f(0.0f, -dd, -dd);
		glTexCoord2d(1.0,0.0); glVertex3f(dd, -dd, 0.0f);
		glTexCoord2d(1.0,1.0); glVertex3f(dd, dd, 0.0f);
		glTexCoord2d(0.0,1.0); glVertex3f(0.0f, dd, -dd);
	glEnd();

	//left face
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(textureIndexes[1]) );
	glBegin(GL_QUADS);
		glTexCoord2d(0.0,0.0); glVertex3f(-dd, -dd, 0.0f);
		glTexCoord2d(1.0,0.0); glVertex3f(0.0f, -dd, -dd);
		glTexCoord2d(1.0,1.0); glVertex3f(0.0f, dd, -dd);
		glTexCoord2d(0.0,1.0); glVertex3f(-dd, dd, 0.0f);
	glEnd();

	//top face
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(textureIndexes[2]) );
	glBegin(GL_QUADS);
		glTexCoord2d(0.0,0.0); glVertex3f(0.0f, dd, -dd);
		glTexCoord2d(1.0,0.0); glVertex3f(-dd, dd, 0.0f);
		glTexCoord2d(1.0,1.0); glVertex3f(0.0f, dd, dd);
		glTexCoord2d(0.0,1.0); glVertex3f(dd, dd, 0.0f);
	glEnd();

	//bottom face
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(textureIndexes[3]) );
	glBegin(GL_QUADS);
		glTexCoord2d(0.0,0.0); glVertex3f(0.0f, -dd, -dd);
		glTexCoord2d(1.0,0.0); glVertex3f(dd, -dd, 0.0f);
		glTexCoord2d(1.0,1.0); glVertex3f(0.0f, -dd, dd);
		glTexCoord2d(0.0,1.0); glVertex3f(-dd, -dd, 0.0f);
	glEnd();
}

void myInitOGLFun()
{
	for(unsigned int i=0; i<4; i++)
		textureIndexes[i] = 0;
	
	sgct::TextureManager::Instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::Instance()->setCompression(sgct::TextureManager::No_Compression);
	sgct::TextureManager::Instance()->loadTexure(textureIndexes[0], "right", "grid_right.png", true, 0);
	sgct::TextureManager::Instance()->loadTexure(textureIndexes[1], "left", "grid_left.png", true, 0);
	sgct::TextureManager::Instance()->loadTexure(textureIndexes[2], "top", "grid_top.png", true, 0);
	sgct::TextureManager::Instance()->loadTexure(textureIndexes[3], "bottom", "grid_bottom.png", true, 0);

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_COLOR_MATERIAL );
	glDisable( GL_LIGHTING );
	glEnable(GL_TEXTURE_2D);
}
