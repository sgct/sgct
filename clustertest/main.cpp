#include <stdlib.h>
#include <stdio.h>

#include "sgct.h"
#define EXTENDED_SIZE 2048

sgct::Engine * gEngine;

void myDrawFun();
void myPreDrawFun();
void myPostDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();

void keyCallback(int key, int action);
void externalControlCallback(const char * receivedChars, int size, int clientId);

//variables to share across cluster
double dt = 0.0;
double time = 0.0;
bool showFPS = false;
bool extraPackages = false;
bool barrier = false;
bool resetCounter = false;
bool stats = false;
float extraData[EXTENDED_SIZE];
unsigned char flags = 0;

void drawGrid(float size, int steps);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );
	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setExternalControlCallback( externalControlCallback );

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	//allocate extra data
	for(int i=0;i<EXTENDED_SIZE;i++)
		extraData[i] = static_cast<float>(i);

	sgct::SharedData::Instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::Instance()->setDecodeFunction(myDecodeFun);

	//init openGL
	
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreDrawFunction( myPreDrawFun );
	gEngine->setPostDrawFunction( myPostDrawFun );
	glfwSetKeyCallback( keyCallback );

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	glPushMatrix();

	/*if( gEngine->isUsingSwapGroups() )
	{
		GLuint frameNumber;
		gEngine->getSwapGroupFrameNumber(frameNumber);
		glTranslatef(0.0f, sinf(static_cast<float>(frameNumber)/100.0f), 0.0f);
	}
	else
		glTranslatef(0.0f, static_cast<float>(sin(gEngine->mSharedData->time)), 0.0f);
	glColor3f(1.0f,0.0f,0.0f); //red
	glBegin(GL_QUADS);

	glVertex3f( -3.0f, 0.01f, 0.0f);
	glVertex3f( 3.0f, 0.01f, 0.0f);
	glVertex3f( 3.0f, -0.01f, 0.0f);
	glVertex3f( -3.0f, -0.01f, 0.0f);

	glEnd();
	*/

	glRotatef(static_cast<float>(time)*10.0f, 0.0f, 1.0f, 0.0f);
	glScalef(1.0f, 0.5f, 1.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glLineWidth(2.0);

	//draw a cube
	//bottom
	glBegin(GL_LINE_STRIP);
	glVertex3f( -1.0f, -1.0f, -1.0f);
	glVertex3f( 1.0f, -1.0f, -1.0f);
	glVertex3f( 1.0f, -1.0f, 1.0f);
	glVertex3f( -1.0f, -1.0f, 1.0f);
	glVertex3f( -1.0f, -1.0f, -1.0f);
	glEnd();

	//top
	glBegin(GL_LINE_STRIP);
	glVertex3f( -1.0f, 1.0f, -1.0f);
	glVertex3f( 1.0f, 1.0f, -1.0f);
	glVertex3f( 1.0f, 1.0f, 1.0f);
	glVertex3f( -1.0f, 1.0f, 1.0f);
	glVertex3f( -1.0f, 1.0f, -1.0f);
	glEnd();

	//sides
	glBegin(GL_LINES);
	glVertex3f( -1.0f, -1.0f, -1.0f);
	glVertex3f( -1.0f, 1.0f, -1.0f);

	glVertex3f( 1.0f, -1.0f, -1.0f);
	glVertex3f( 1.0f, 1.0f, -1.0f);

	glVertex3f( 1.0f, -1.0f, 1.0f);
	glVertex3f( 1.0f, 1.0f, 1.0f);

	glVertex3f( -1.0f, -1.0f, 1.0f);
	glVertex3f( -1.0f, 1.0f, 1.0f);
	glEnd();

	glPopMatrix();

	//drawGrid(10.0, 100);
}

void myPreDrawFun()
{
	if( gEngine->isMaster() )
	{
		dt = gEngine->getDt();
		time = glfwGetTime();
	}

	gEngine->setDisplayInfoVisibility( showFPS );
	gEngine->getWindowPtr()->setBarrier( barrier );
	gEngine->setStatsGraphVisibility( stats );
	if(resetCounter)
	{
		gEngine->getWindowPtr()->resetSwapGroupFrameNumber();
	}
}

void myPostDrawFun()
{
	if( gEngine->isMaster() )
	{
		resetCounter = false;
	}
}

void myInitOGLFun()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
}

void myEncodeFun()
{
	flags = showFPS	? flags | 1 : flags & ~1;
	flags = extraPackages ? flags | 2 : flags & ~2;
	flags = barrier ? flags | 4 : flags & ~4;
	flags = resetCounter ? flags | 8 : flags & ~8;
	flags = stats ? flags | 16 : flags & ~16;

	sgct::SharedData::Instance()->writeDouble(dt);
	sgct::SharedData::Instance()->writeDouble(time);
	sgct::SharedData::Instance()->writeUChar(flags);

	if(extraPackages)
		for(int i=0;i<EXTENDED_SIZE;i++)
			sgct::SharedData::Instance()->writeFloat( extraData[i] );
}

void myDecodeFun()
{
	dt = sgct::SharedData::Instance()->readDouble();
	time = sgct::SharedData::Instance()->readDouble();
	flags = sgct::SharedData::Instance()->readUChar();

	showFPS	= flags & 0x0001;
	extraPackages = (flags>>1) & 0x0001;
	barrier = (flags>>2) & 0x0001;
	resetCounter = (flags>>3) & 0x0001;
	stats = (flags>>4) & 0x0001;

	if(extraPackages)
		for(int i=0;i<EXTENDED_SIZE;i++)
			extraData[i] = sgct::SharedData::Instance()->readFloat();
}

void drawGrid(float size, int steps)
{
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPushMatrix();
	glTranslatef(-size/2.0f, -size/2.0f, 0.0f);
	glColor4f(1.0f,1.0f,1.0f,0.5f);
	glLineWidth(1.0);

	glBegin(GL_LINES);
	//horizontal lines
	for( float f=0.0f; f<=size; f+= (size/static_cast<float>(steps)))
	{
		glVertex3f( 0.0f, f, 0.0f);
		glVertex3f( size, f, 0.0f);
	}
	//Vertical lines
	for( float f=0.0f; f<=size; f+= (size/static_cast<float>(steps)))
	{
		glVertex3f( f, 0.0f, 0.0f);
		glVertex3f( f, size, 0.0f);
	}
	glEnd();

	glVertex3f( -1.0f,  0.01f, 0.0f);
	glVertex3f(  1.0f,  0.01f, 0.0f);
	glVertex3f(  1.0f, -0.01f, 0.0f);
	glVertex3f( -1.0f, -0.01f, 0.0f);

	glPopMatrix();
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case 'I':
			if(action == GLFW_PRESS)
				showFPS = !showFPS;
			break;

		case 'E':
			if(action == GLFW_PRESS)
				extraPackages = !extraPackages;
			break;

		case 'B':
			if(action == GLFW_PRESS)
				barrier = !barrier;
			break;

		case 'R':
			if(action == GLFW_PRESS)
				resetCounter = true;
			break;

		case 'S':
			if(action == GLFW_PRESS)
				stats = !stats;
			break;
		}
	}
}

void externalControlCallback(const char * receivedChars, int size, int clientId)
{
	if( gEngine->isMaster() )
	{
		if(strcmp(receivedChars, "info") == 0)
			showFPS = !showFPS;
		else if(strcmp(receivedChars, "size") == 0)
			gEngine->setExternalControlBufferSize(4096);
	}
}
	