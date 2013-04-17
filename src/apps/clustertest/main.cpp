#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"
//#include "sgct/PLYReader.h"
#define EXTENDED_SIZE 40000

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
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
bool takeScreenshot = false;
bool slowRendering = false;
float speed = 5.0f;
float extraData[EXTENDED_SIZE];
unsigned char flags = 0;

void drawGrid(float size, int steps);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );
	gEngine->setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setExternalControlCallback( externalControlCallback );
	gEngine->setKeyboardCallbackFunction( keyCallback );
	//gEngine->setExitKey( 'Y' );

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	//allocate extra data
	if( gEngine->isMaster() )
		for(int i=0;i<EXTENDED_SIZE;i++)
			extraData[i] = static_cast<float>(rand()%500)/500.0f;

	sgct::SharedData::Instance()->setCompression(true);
	sgct::SharedData::Instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::Instance()->setDecodeFunction(myDecodeFun);

	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
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
	if(slowRendering)
		gEngine->sleep(1.0/5.0);

	glPushMatrix();

	/*if( sgct_core::Frustum::Mono == gEngine->getActiveFrustum() )
		sgct::MessageHandler::Instance()->print("Mono!\n");
	else if( sgct_core::Frustum::StereoLeftEye == gEngine->getActiveFrustum() )
		sgct::MessageHandler::Instance()->print("Left eye!\n");
	else
		sgct::MessageHandler::Instance()->print("Right eye!\n");*/

	//sgct::MessageHandler::Instance()->print("Mouse wheel: %d\n", sgct::Engine::getMouseWheel());
	//sgct::MessageHandler::Instance()->print("Right mouse button: %d\n", sgct::Engine::getMouseButton(SGCT_MOUSE_BUTTON_RIGHT));
	//sgct::MessageHandler::Instance()->print("Y key: %d\n", sgct::Engine::getKey('Y'));
	//sgct::Engine::setMousePos( rand()%600, rand()%400);

	glRotatef(static_cast<float>(curr_time)*speed, 0.0f, 1.0f, 0.0f);
	glScalef(1.0f, 0.5f, 1.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glLineWidth(3.0);

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

	float xPos = static_cast<float>( gEngine->getWindowPtr()->getXFramebufferResolution() ) / 2.0f;

	glColor3f(1.0f,1.0f,0.0f);
	if( gEngine->getActiveFrustum() == sgct_core::Frustum::StereoLeftEye )
		sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 32 ), xPos, 200, "Left");
	else if( gEngine->getActiveFrustum() == sgct_core::Frustum::StereoRightEye )
		sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 32 ), xPos, 150, "Right");
	else if( gEngine->getActiveFrustum() == sgct_core::Frustum::Mono )
		sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 32 ), xPos, 200, "Mono");

	if( gEngine->getWindowPtr()->isUsingSwapGroups() )
	{
		sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 18 ), xPos - xPos/2.0f, 450, "Swap group: Active");

		sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 18 ), xPos - xPos/2.0f, 500, "Press B to toggle barrier and R to reset counter");
		
		if( gEngine->getWindowPtr()->isBarrierActive() )
			sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 18 ), xPos - xPos/2.0f, 400, "Swap barrier: Active");
		else
			sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 18 ), xPos - xPos/2.0f, 400, "Swap barrier: Inactive");

		if( gEngine->getWindowPtr()->isSwapGroupMaster() )
			sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 18 ), xPos - xPos/2.0f, 350, "Swap group master: True");
		else
			sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 18 ), xPos - xPos/2.0f, 350, "Swap group master: False");

		unsigned int fr_number;
		gEngine->getWindowPtr()->getSwapGroupFrameNumber(fr_number);
		sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 18 ), xPos - xPos/2.0f, 300, "Nvidia frame counter: %u", fr_number );
		sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 18 ), xPos - xPos/2.0f, 250, "Framerate: %.3lf", 1.0/gEngine->getDt() );
	}
	else
	{
		sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", 18 ), xPos - xPos/2.0f, 450, "Swap group: Inactive");
	}
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		dt = gEngine->getDt();
		curr_time = gEngine->getTime();
	}
}

void myPostSyncPreDrawFun()
{
	gEngine->setDisplayInfoVisibility( showFPS );
	gEngine->getWindowPtr()->setBarrier( barrier );
	gEngine->setStatsGraphVisibility( stats );

	if( takeScreenshot )
	{
		gEngine->takeScreenshot();
		takeScreenshot = false;
	}

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

	unsigned int numberOfActiveViewports = 0;
	sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::Instance()->getThisNodePtr();
	for(unsigned int i=0; i < thisNode->getNumberOfViewports(); i++)
		if( thisNode->getViewport(i)->isEnabled() )
		{
			numberOfActiveViewports++;

			glm::mat4 prjMatMono  = thisNode->getViewport(i)->getProjectionMatrix( sgct_core::Frustum::Mono );
			glm::mat4 prjMatLeft  = thisNode->getViewport(i)->getProjectionMatrix( sgct_core::Frustum::StereoLeftEye );
			glm::mat4 prjMatRight = thisNode->getViewport(i)->getProjectionMatrix( sgct_core::Frustum::StereoRightEye );
		}

	sgct::MessageHandler::Instance()->print("Number of active viewports: %d\n", numberOfActiveViewports);
}

void myEncodeFun()
{
	flags = showFPS	? flags | 1 : flags & ~1; //bit 1
	flags = extraPackages ? flags | 2 : flags & ~2; //bit 2
	flags = barrier ? flags | 4 : flags & ~4; //bit 3
	flags = resetCounter ? flags | 8 : flags & ~8; //bit 4
	flags = stats ? flags | 16 : flags & ~16; //bit 5
	flags = takeScreenshot ? flags | 32 : flags & ~32; //bit 6
	flags = slowRendering ? flags | 64 : flags & ~64; //bit 7

	sgct::SharedData::Instance()->writeDouble(dt);
	sgct::SharedData::Instance()->writeDouble(curr_time);
	sgct::SharedData::Instance()->writeFloat( speed );
	sgct::SharedData::Instance()->writeUChar(flags);

	if(extraPackages)
		for(int i=0;i<EXTENDED_SIZE;i++)
			sgct::SharedData::Instance()->writeFloat( extraData[i] );
}

void myDecodeFun()
{
	dt = sgct::SharedData::Instance()->readDouble();
	curr_time = sgct::SharedData::Instance()->readDouble();
	speed = sgct::SharedData::Instance()->readFloat();
	flags = sgct::SharedData::Instance()->readUChar();

	showFPS	= flags & 0x0001;
	extraPackages = (flags>>1) & 0x0001;
	barrier = (flags>>2) & 0x0001;
	resetCounter = (flags>>3) & 0x0001;
	stats = (flags>>4) & 0x0001;
	takeScreenshot = (flags>>5) & 0x0001;
	slowRendering = (flags>>6) & 0x0001;

	if(extraPackages)
		for(int i=0;i<EXTENDED_SIZE;i++)
			extraData[i] = sgct::SharedData::Instance()->readFloat();
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		static bool mousePointer = true;

		switch( key )
		{
		case 'I':
			if(action == SGCT_PRESS)
				showFPS = !showFPS;
			break;

		case 'E':
			if(action == SGCT_PRESS)
				extraPackages = !extraPackages;
			break;

		case 'B':
			if(action == SGCT_PRESS)
				barrier = !barrier;
			break;

		case 'R':
			if(action == SGCT_PRESS)
				resetCounter = true;
			break;

		case 'S':
			if(action == SGCT_PRESS)
				stats = !stats;
			break;

		case 'G':
			if(action == SGCT_PRESS)
				gEngine->sendMessageToExternalControl("Testar!!\r\n");
			break;

		case 'M':
			if(action == SGCT_PRESS)
			{
				mousePointer = !mousePointer; //toggle
				sgct::Engine::setMousePointerVisibility(mousePointer);
			}
			break;

		case SGCT_KEY_F9:
			if(action == SGCT_PRESS)
				slowRendering = !slowRendering;
			break;

		case SGCT_KEY_F10:
			if(action == SGCT_PRESS)
				takeScreenshot = true;
			break;

		case SGCT_KEY_UP:
			speed *= 1.1f;
			break;

		case SGCT_KEY_DOWN:
			speed /= 1.1f;
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
