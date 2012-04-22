#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();

int joyStick1Present = GL_FALSE;
int numberOfAxes = 0;
int numberOfButtons = 0;
float * axesPos;
unsigned char * buttons;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	joyStick1Present = sgct::Engine::getJoystickParam( GLFW_JOYSTICK_1, GLFW_PRESENT );
	if( joyStick1Present == GL_TRUE )
	{
		sgct::MessageHandler::Instance()->print("Joystick 1 is present.\n");

		numberOfAxes = sgct::Engine::getJoystickParam( GLFW_JOYSTICK_1, GLFW_AXES );
		numberOfButtons = sgct::Engine::getJoystickParam( GLFW_JOYSTICK_1, GLFW_BUTTONS );

		sgct::MessageHandler::Instance()->print("Number of axes %d\nNumber of buttons %d\n", 
			numberOfAxes,
			numberOfButtons);

		if( numberOfAxes > 0 )
			axesPos = new float[numberOfAxes];

		if( numberOfButtons > 0 )
			buttons = new unsigned char[numberOfButtons];
	}

	gEngine->setPreSyncFunction( myPreSyncFun );

	// Main loop
	gEngine->render();

	if( numberOfAxes > 0 )
		delete [] axesPos;
	if( numberOfButtons > 0 )
		delete [] buttons;

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myPreSyncFun()
{
	if( joyStick1Present == GL_TRUE )
	{
		sgct::Engine::getJoystickAxes( GLFW_JOYSTICK_1, axesPos, numberOfAxes );
		for(int i=0; i<numberOfAxes; i++)
			sgct::MessageHandler::Instance()->print("%.3f ", axesPos[i]);

		sgct::Engine::getJoystickButtons( GLFW_JOYSTICK_1, buttons, numberOfButtons );
		for(int i=0; i<numberOfButtons; i++)
			sgct::MessageHandler::Instance()->print("%d ", buttons[i]);

		sgct::MessageHandler::Instance()->print("\r");
	}
}