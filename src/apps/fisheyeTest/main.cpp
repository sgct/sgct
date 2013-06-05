#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void keyCallback(int key, int action);

size_t textureIndexes[4];

bool info = false;
bool stats = false;
bool takeScreenshot = false;

int main( int argc, char* argv[] )
{

	// Allocate
	gEngine = new sgct::Engine( argc, argv );

	// Bind your functions
	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );

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
	glEnable(GL_TEXTURE_2D);

	float r = 7.5f; //dome radius

	//right face
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByHandle(textureIndexes[0]) );
	glBegin(GL_QUADS);
		glTexCoord2d(0.0,0.0); glVertex3f(0.0f, -r, -r);
		glTexCoord2d(1.0,0.0); glVertex3f(r, -r, 0.0f);
		glTexCoord2d(1.0,1.0); glVertex3f(r, r, 0.0f);
		glTexCoord2d(0.0,1.0); glVertex3f(0.0f, r, -r);
	glEnd();

	//left face
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByHandle(textureIndexes[1]) );
	glBegin(GL_QUADS);
		glTexCoord2d(0.0,0.0); glVertex3f(-r, -r, 0.0f);
		glTexCoord2d(1.0,0.0); glVertex3f(0.0f, -r, -r);
		glTexCoord2d(1.0,1.0); glVertex3f(0.0f, r, -r);
		glTexCoord2d(0.0,1.0); glVertex3f(-r, r, 0.0f);
	glEnd();

	//top face
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByHandle(textureIndexes[2]) );
	glBegin(GL_QUADS);
		glTexCoord2d(0.0,0.0); glVertex3f(0.0f, r,  r);
		glTexCoord2d(1.0,0.0); glVertex3f(r,    r,  0.0f);
		glTexCoord2d(1.0,1.0); glVertex3f(0.0f, r,  -r);
		glTexCoord2d(0.0,1.0); glVertex3f(-r,    r,  0.0f);
	glEnd();

	//bottom face
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByHandle(textureIndexes[3]) );
	glBegin(GL_QUADS);
		glTexCoord2d(0.0,0.0); glVertex3f(0.0f, -r, r);
		glTexCoord2d(1.0,0.0); glVertex3f(-r,   -r, 0.0f);
		glTexCoord2d(1.0,1.0); glVertex3f(0.0f, -r, -r);
		glTexCoord2d(0.0,1.0); glVertex3f(r,   -r, 0.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);
	/*
	glColor3f(1.0f,1.0f,0.0f);
	glLineWidth(5.0);
	glBegin(GL_LINES);
		glVertex3f(-r/2.0f, r/2.0f, 0.0f);
		glVertex3f(0.0f, r/2.0f, -r/2.0f);

		glVertex3f(-r/2.0f, -r/2.0f, 0.0f);
		glVertex3f(0.0f, -r/2.0f, -r/2.0f);

		glVertex3f(r/2.0f, r/2.0f, 0.0f);
		glVertex3f(0.0f, r/2.0f, -r/2.0f);

		glVertex3f(r/2.0f, -r/2.0f, 0.0f);
		glVertex3f(0.0f, -r/2.0f, -r/2.0f);

		glVertex3f(0.0f, -r/2.0f, -r/2.0f);
		glVertex3f(0.0f, r/2.0f, -r/2.0f);
	glEnd();

	glColor3f(1.0f,0.5f,0.5f);
	glLineWidth(5.0);
	glBegin(GL_LINES);
		glVertex3f(-r/4.0f, r/4.0f, 0.0f);
		glVertex3f(0.0f, r/4.0f, -r/4.0f);

		glVertex3f(-r/4.0f, -r/4.0f, 0.0f);
		glVertex3f(0.0f, -r/4.0f, -r/4.0f);

		glVertex3f(r/4.0f, r/4.0f, 0.0f);
		glVertex3f(0.0f, r/4.0f, -r/4.0f);

		glVertex3f(r/4.0f, -r/4.0f, 0.0f);
		glVertex3f(0.0f, -r/4.0f, -r/4.0f);

		glVertex3f(0.0f, -r/4.0f, -r/4.0f);
		glVertex3f(0.0f, r/4.0f, -r/4.0f);
	glEnd();

	glColor3f(1.0f,0.0f,1.0f);
	glLineWidth(2.0);
	glBegin(GL_LINES);
		glVertex3f(-r*2.0f, r*2.0f, 0.0f);
		glVertex3f(0.0f, r*2.0f, -r*2.0f);

		glVertex3f(-r*2.0f, -r*2.0f, 0.0f);
		glVertex3f(0.0f, -r*2.0f, -r*2.0f);

		glVertex3f(r*2.0f, r*2.0f, 0.0f);
		glVertex3f(0.0f, r*2.0f, -r*2.0f);

		glVertex3f(r*2.0f, -r*2.0f, 0.0f);
		glVertex3f(0.0f, -r*2.0f, -r*2.0f);

		glVertex3f(0.0f, -r*2.0f, -r*2.0f);
		glVertex3f(0.0f, r*2.0f, -r*2.0f);
	glEnd();

	glColor3f(0.0f,1.0f,1.0f);
	glLineWidth(2.0);
	glBegin(GL_LINES);
		glVertex3f(-r, r, 0.0f);
		glVertex3f(0.0f, r, -r);

		glVertex3f(-r, -r, 0.0f);
		glVertex3f(0.0f, -r, -r);

		glVertex3f(r, r, 0.0f);
		glVertex3f(0.0f, r, -r);

		glVertex3f(r, -r, 0.0f);
		glVertex3f(0.0f, -r, -r);

		glVertex3f(0.0f, -r, -r);
		glVertex3f(0.0f, r, -r);
	glEnd();
	*/
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

void myPostSyncPreDrawFun()
{
	gEngine->setDisplayInfoVisibility(info);
	gEngine->setStatsGraphVisibility(stats);

	if( takeScreenshot )
	{
		gEngine->takeScreenshot();
		takeScreenshot = false;
	}
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case 'S':
			if(action == SGCT_PRESS)
				stats = !stats;
			break;

		case 'I':
			if(action == SGCT_PRESS)
				info = !info;
			break;

		case 'P':
		case SGCT_KEY_F10:
			if(action == SGCT_PRESS)
				takeScreenshot = true;
			break;
		}
	}
}
