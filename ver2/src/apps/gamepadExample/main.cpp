#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();

const char * joyStick1Name = NULL;
int numberOfAxes = 0;
int numberOfButtons = 0;
const float * axesPos;
const unsigned char * buttons;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	joyStick1Name = sgct::Engine::getJoystickName( SGCT_JOYSTICK_1 );
	if( joyStick1Name != NULL )
	{
		sgct::MessageHandler::Instance()->print("Joystick 1 '%s' is present.\n", joyStick1Name);

		axesPos = sgct::Engine::getJoystickAxes( SGCT_JOYSTICK_1, &numberOfAxes );
		buttons = sgct::Engine::getJoystickButtons( SGCT_JOYSTICK_1, &numberOfButtons );

		sgct::MessageHandler::Instance()->print("Number of axes %d\nNumber of buttons %d\n", 
			numberOfAxes,
			numberOfButtons);
	}

	gEngine->setPreSyncFunction( myPreSyncFun );

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myPreSyncFun()
{
	if( joyStick1Name != NULL )
	{
		axesPos = sgct::Engine::getJoystickAxes( SGCT_JOYSTICK_1, &numberOfAxes );
		for(int i=0; i<numberOfAxes; i++)
			sgct::MessageHandler::Instance()->print("%.3f ", axesPos[i]);

		buttons = sgct::Engine::getJoystickButtons( SGCT_JOYSTICK_1, &numberOfButtons );
		for(int i=0; i<numberOfButtons; i++)
			sgct::MessageHandler::Instance()->print("%d ", buttons[i]);

		sgct::MessageHandler::Instance()->print("\r");
	}
}