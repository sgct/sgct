#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myInitOGLFun();

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );

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
	//enter ortho mode
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glPushMatrix();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	
	glColor3f(1.0f,1.0f,1.0f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("grid"));

	glBegin(GL_QUADS);
	glTexCoord2d(0.0, 0.0);
	glVertex2d(0.0, 0.0);

	glTexCoord2d(0.0, 1.0);
	glVertex2d(0.0, 1.0);

	glTexCoord2d(1.0, 1.0);
	glVertex2d(1.0, 1.0);

	glTexCoord2d(1.0, 0.0);
	glVertex2d(1.0, 0.0);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	glPopAttrib();

	//exit ortho mode
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void myInitOGLFun()
{
	sgct::TextureManager::instance()->loadTexure("grid", "grid.png", true, 0);
}