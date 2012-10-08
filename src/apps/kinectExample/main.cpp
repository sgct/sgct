#include "sgct.h"

sgct::Engine * gEngine;

void myInitOGLFun();
void myDrawFun();
void myPreSyncFun();
void myEncodeFun();
void myDecodeFun();

double curr_time = 0.0;
float size_factor = 0.5f;

//pointer to a left hand
core_sgct::SGCTTrackingDevice * leftHand = NULL;
//pointer to a right hand
core_sgct::SGCTTrackingDevice * rightHand = NULL;

bool error = false;

int main( int argc, char* argv[] )
{

	// Allocate
	gEngine = new sgct::Engine( argc, argv );

	// Bind your functions
	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	sgct::SharedData::Instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::Instance()->setDecodeFunction(myDecodeFun);

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

void myInitOGLFun()
{
	glEnable(GL_DEPTH_TEST);
 
	//get the tracking pointers
	leftHand	= sgct::Engine::getTrackingManager()->getTrackingPtr("Left Hand");
	rightHand	= sgct::Engine::getTrackingManager()->getTrackingPtr("Right Hand");
 
	if(leftHand == NULL || rightHand == NULL)
	{
		error = true;
		sgct::MessageHandler::Instance()->print("Failed to get pointers to hand trackers!\n");
	}
}

void myDrawFun()
{
	float speed = 50.0f;
	glRotatef(static_cast<float>( curr_time ) * speed, 0.0f, 1.0f, 0.0f);

	//render a single triangle
	glBegin(GL_TRIANGLES);
		glColor3f(1.0f, 0.0f, 0.0f); //Red
		glVertex3f(-0.5f * size_factor, -0.5f * size_factor, 0.0f);
 
		glColor3f(0.0f, 1.0f, 0.0f); //Green
		glVertex3f(0.0f, 0.5f * size_factor, 0.0f);
 
		glColor3f(0.0f, 0.0f, 1.0f); //Blue
		glVertex3f(0.5f * size_factor, -0.5f * size_factor, 0.0f);
	glEnd();
}

void myPreSyncFun()
{
	//set the time only on the master
	if( gEngine->isMaster() )
	{
		//get the time in seconds
		curr_time = sgct::Engine::getTime();

		if(!error)
		{
			glm::dvec3 leftPos = leftHand->getPosition();
			glm::dvec3 rightPos = rightHand->getPosition();
			float dist = static_cast<float>(glm::length(leftPos - rightPos));
			size_factor = (dist < 2.0f && dist > 0.2f) ? dist : 0.5f;
		}
	}
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble( curr_time );
	sgct::SharedData::Instance()->writeFloat( size_factor );
}

void myDecodeFun()
{
	curr_time = sgct::SharedData::Instance()->readDouble();
	size_factor = sgct::SharedData::Instance()->readFloat();
}
