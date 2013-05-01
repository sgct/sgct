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
sgct::SharedDouble dt(0.0);
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool showFPS(false);
sgct::SharedBool extraPackages(false);
sgct::SharedBool barrier(false);
sgct::SharedBool resetCounter(false);
sgct::SharedBool stats(false);
sgct::SharedBool takeScreenshot(false);
sgct::SharedBool slowRendering(false);
sgct::SharedBool frametest(false);
sgct::SharedFloat speed( 5.0f );
sgct::SharedFloat extraData[EXTENDED_SIZE];

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
			extraData[i].setVal(static_cast<float>(rand()%500)/500.0f);

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
	if(slowRendering.getVal())
		gEngine->sleep(1.0/5.0);

	//test quadbuffer
	if(frametest.getVal())
	{
		if( gEngine->getCurrentFrameNumber()%2 == 0 ) //even
		{
			if( gEngine->getActiveFrustum() == sgct_core::Frustum::StereoRightEye ) //left eye or mono since clear color is one step behind
				gEngine->setClearColor(0.0f, 0.0f, 1.0f, 1.0f); //is paired with red
			else //right
				gEngine->setClearColor(1.0f, 0.0f, 0.0f, 1.0f); //is paired with blue
		}
		else //odd
		{
			if( gEngine->getActiveFrustum() == sgct_core::Frustum::StereoRightEye ) //left eye or mono since clear color is one step behind
				gEngine->setClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			else //right
				gEngine->setClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		}

		//gEngine->takeScreenshot();
	}
	else
		gEngine->setClearColor(0.0f, 0.0f, 0.0f, 0.0f);

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

	glRotatef(static_cast<float>(curr_time.getVal())*speed.getVal(), 0.0f, 1.0f, 0.0f);
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
		dt.setVal( gEngine->getDt() );
		curr_time.setVal( gEngine->getTime() );
	}
}

void myPostSyncPreDrawFun()
{
	gEngine->setDisplayInfoVisibility( showFPS.getVal() );
	gEngine->getWindowPtr()->setBarrier( barrier.getVal() );
	gEngine->setStatsGraphVisibility( stats.getVal() );

	if( takeScreenshot.getVal() )
	{
		gEngine->takeScreenshot();
		takeScreenshot.setVal(false);
	}

	if(resetCounter.getVal())
	{
		gEngine->getWindowPtr()->resetSwapGroupFrameNumber();
	}
}

void myPostDrawFun()
{
	if( gEngine->isMaster() )
	{
		resetCounter.setVal(false);
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

			glm::mat4 prjMatMono  = thisNode->getViewport(i)->getViewProjectionMatrix( sgct_core::Frustum::Mono );
			glm::mat4 prjMatLeft  = thisNode->getViewport(i)->getViewProjectionMatrix( sgct_core::Frustum::StereoLeftEye );
			glm::mat4 prjMatRight = thisNode->getViewport(i)->getViewProjectionMatrix( sgct_core::Frustum::StereoRightEye );
		}

	sgct::MessageHandler::Instance()->print("Number of active viewports: %d\n", numberOfActiveViewports);
}

void myEncodeFun()
{
	unsigned char flags = 0;
	flags = showFPS.getVal()	? flags | 1 : flags & ~1; //bit 1
	flags = extraPackages.getVal() ? flags | 2 : flags & ~2; //bit 2
	flags = barrier.getVal() ? flags | 4 : flags & ~4; //bit 3
	flags = resetCounter.getVal() ? flags | 8 : flags & ~8; //bit 4
	flags = stats.getVal() ? flags | 16 : flags & ~16; //bit 5
	flags = takeScreenshot.getVal() ? flags | 32 : flags & ~32; //bit 6
	flags = slowRendering.getVal() ? flags | 64 : flags & ~64; //bit 7
	flags = frametest.getVal() ? flags | 128 : flags & ~128; //bit 8

	sgct::SharedUChar sf(flags);

	sgct::SharedData::Instance()->writeDouble( &dt);
	sgct::SharedData::Instance()->writeDouble( &curr_time);
	sgct::SharedData::Instance()->writeFloat( &speed );
	sgct::SharedData::Instance()->writeUChar( &sf );

	if(extraPackages.getVal())
		for(int i=0;i<EXTENDED_SIZE;i++)
			sgct::SharedData::Instance()->writeFloat( &extraData[i] );
}

void myDecodeFun()
{
	sgct::SharedUChar sf;
	sgct::SharedData::Instance()->readDouble( &dt );
	sgct::SharedData::Instance()->readDouble( &curr_time );
	sgct::SharedData::Instance()->readFloat( &speed );
	sgct::SharedData::Instance()->readUChar( &sf );

	unsigned char flags = sf.getVal();
	showFPS.setVal(flags & 0x0001);
	extraPackages.setVal((flags>>1) & 0x0001);
	barrier.setVal((flags>>2) & 0x0001);
	resetCounter.setVal((flags>>3) & 0x0001);
	stats.setVal((flags>>4) & 0x0001);
	takeScreenshot.setVal((flags>>5) & 0x0001);
	slowRendering.setVal((flags>>6) & 0x0001);
	frametest.setVal((flags>>7) & 0x0001);

	if(extraPackages.getVal())
		for(int i=0;i<EXTENDED_SIZE;i++)
			sgct::SharedData::Instance()->readFloat( &extraData[i] );
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		static bool mousePointer = true;

		switch( key )
		{
		case 'F':
			if(action == SGCT_PRESS)
				frametest.toggle();
			break;
		
		case 'I':
			if(action == SGCT_PRESS)
				showFPS.toggle();
			break;

		case 'E':
			if(action == SGCT_PRESS)
				extraPackages.toggle();
			break;

		case 'B':
			if(action == SGCT_PRESS)
				barrier.toggle();
			break;

		case 'R':
			if(action == SGCT_PRESS)
				resetCounter.toggle();
			break;

		case 'S':
			if(action == SGCT_PRESS)
				stats.toggle();
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
				slowRendering.toggle();
			break;

		case SGCT_KEY_F10:
			if(action == SGCT_PRESS)
				takeScreenshot.setVal(true);
			break;

		case SGCT_KEY_UP:
			speed.setVal( speed.getVal() * 1.1f );
			break;

		case SGCT_KEY_DOWN:
			speed.setVal( speed.getVal() / 1.1f );
			break;
		}
	}
}

void externalControlCallback(const char * receivedChars, int size, int clientId)
{
	if( gEngine->isMaster() )
	{
		if(strcmp(receivedChars, "info") == 0)
			showFPS.toggle();
		else if(strcmp(receivedChars, "size") == 0)
			gEngine->setExternalControlBufferSize(4096);
	}
}
