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
double curr_time = 0.0;
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
	gEngine->setKeyboardCallbackFunction( keyCallback );

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

	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreDrawFunction( myPreDrawFun );
	gEngine->setPostDrawFunction( myPostDrawFun );

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
	
	/*if( core_sgct::Frustum::Mono == gEngine->getActiveFrustum() )
		sgct::MessageHandler::Instance()->print("Mono!\n");
	else if( core_sgct::Frustum::StereoLeftEye == gEngine->getActiveFrustum() )
		sgct::MessageHandler::Instance()->print("Left eye!\n");
	else
		sgct::MessageHandler::Instance()->print("Right eye!\n");*/

	//sgct::MessageHandler::Instance()->print("Mouse wheel: %d\n", sgct::Engine::getMouseWheel());
	//sgct::MessageHandler::Instance()->print("Right mouse button: %d\n", sgct::Engine::getMouseButton(GLFW_MOUSE_BUTTON_RIGHT));
	//sgct::MessageHandler::Instance()->print("Y key: %d\n", sgct::Engine::getKey('Y'));
	//sgct::Engine::setMousePos( rand()%600, rand()%400);

	glRotatef(static_cast<float>(curr_time)*10.0f, 0.0f, 1.0f, 0.0f);
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
		curr_time = gEngine->getTime();
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

	glEnable(GL_LINE_SMOOTH); 
	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
}

void myEncodeFun()
{
	flags = showFPS	? flags | 1 : flags & ~1;
	flags = extraPackages ? flags | 2 : flags & ~2;
	flags = barrier ? flags | 4 : flags & ~4;
	flags = resetCounter ? flags | 8 : flags & ~8;
	flags = stats ? flags | 16 : flags & ~16;

	sgct::SharedData::Instance()->writeDouble(dt);
	sgct::SharedData::Instance()->writeDouble(curr_time);
	sgct::SharedData::Instance()->writeUChar(flags);

	if(extraPackages)
		for(int i=0;i<EXTENDED_SIZE;i++)
			sgct::SharedData::Instance()->writeFloat( extraData[i] );
}

void myDecodeFun()
{
	dt = sgct::SharedData::Instance()->readDouble();
	curr_time = sgct::SharedData::Instance()->readDouble();
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
		static bool mousePointer = true;
		
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

		case 'M':
			if(action == GLFW_PRESS)
				mousePointer = !mousePointer; //toggle
				sgct::Engine::setMousePointerVisibility(mousePointer);
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
	