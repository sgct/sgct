#include "sgct.h"

sgct::Engine * gEngine;

void myDraw2DFun();

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
		sgct::MessageHandler::instance()->print("Joystick 1 '%s' is present.\n", joyStick1Name);

		axesPos = sgct::Engine::getJoystickAxes( SGCT_JOYSTICK_1, &numberOfAxes );
		buttons = sgct::Engine::getJoystickButtons( SGCT_JOYSTICK_1, &numberOfButtons );

		sgct::MessageHandler::instance()->print("Number of axes %d\nNumber of buttons %d\n", 
			numberOfAxes,
			numberOfButtons);
	}

	gEngine->setDraw2DFunction( myDraw2DFun );

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDraw2DFun()
{
	if( joyStick1Name != NULL )
	{
		char buffer[32];
		std::string joystickInfoStr;

		axesPos = sgct::Engine::getJoystickAxes( SGCT_JOYSTICK_1, &numberOfAxes );
		joystickInfoStr.append( "Axes: " );
		for(int i=0; i<numberOfAxes; i++)
		{
#if (_MSC_VER >= 1400) //visual studio 2005 or later
			sprintf_s(buffer, 32, "%.3f ", axesPos[i]);
#else
			sprintf(buffer, "%.3f ", axesPos[i]);
#endif
			joystickInfoStr.append( buffer );
		}

		buttons = sgct::Engine::getJoystickButtons( SGCT_JOYSTICK_1, &numberOfButtons );
		joystickInfoStr.append( "\nButtons: " );
		for(int i=0; i<numberOfButtons; i++)
		{
#if (_MSC_VER >= 1400) //visual studio 2005 or later
			sprintf_s(buffer, 32, "%d ", buttons[i]);
#else
			sprintf(buffer, "%d ", buttons[i]);
#endif
			joystickInfoStr.append( buffer );
		}

		const sgct_text::Font * font = sgct_text::FontManager::instance()->getFont( "SGCTFont", 10 );
		sgct_text::print(font, 18, 32, glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), "%s", joystickInfoStr.c_str());
	}
}