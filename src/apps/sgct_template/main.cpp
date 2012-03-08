#include "sgct.h"

sgct::Engine * gEngine;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

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