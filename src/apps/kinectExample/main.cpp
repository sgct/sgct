#include "sgct.h"

sgct::Engine * gEngine;

void myInitOGLFun();
void myDrawFun();
void myPreSyncFun();
void myEncodeFun();
void myDecodeFun();

sgct::SharedDouble curr_time(0.0);
sgct::SharedFloat size_factor(0.5f);

//pointer to a left hand
sgct::SGCTTrackingDevice * leftHand = NULL;
//pointer to a right hand
sgct::SGCTTrackingDevice * rightHand = NULL;

bool error = false;

int main( int argc, char* argv[] )
{

	// Allocate
	gEngine = new sgct::Engine( argc, argv );

	// Bind your functions
	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

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
 
	//connect only to VRPN on the master
	if( gEngine->isMaster() )
	{
		//get the tracking pointers
		sgct::SGCTTracker * tracker = sgct::Engine::getTrackingManager()->getTrackerPtr("Kinect0");
		if(tracker != NULL)
		{
			leftHand	= tracker->getDevicePtr("Left Hand");
			rightHand	= tracker->getDevicePtr("Right Hand");
		}

		if(leftHand == NULL || rightHand == NULL)
		{
			error = true;
			sgct::MessageHandler::instance()->print("Failed to get pointers to hand trackers!\n");
		}
	}
}

void myDrawFun()
{
	float speed = 50.0f;
	glRotatef(static_cast<float>( curr_time.getVal() ) * speed, 0.0f, 1.0f, 0.0f);

	float size = size_factor.getVal();

	//render a single triangle
	glBegin(GL_TRIANGLES);
		glColor3f(1.0f, 0.0f, 0.0f); //Red
		glVertex3f(-0.5f * size, -0.5f * size, 0.0f);
 
		glColor3f(0.0f, 1.0f, 0.0f); //Green
		glVertex3f(0.0f, 0.5f * size, 0.0f);
 
		glColor3f(0.0f, 0.0f, 1.0f); //Blue
		glVertex3f(0.5f * size, -0.5f * size, 0.0f);
	glEnd();
}

void myPreSyncFun()
{
	//set the time only on the master
	if( gEngine->isMaster() )
	{
		//get the time in seconds
		curr_time.setVal(sgct::Engine::getTime());

		if(!error)
		{
			glm::dvec3 leftPos = leftHand->getPosition();
			glm::dvec3 rightPos = rightHand->getPosition();
			float dist = static_cast<float>(glm::length(leftPos - rightPos));
			size_factor.setVal( (dist < 2.0f && dist > 0.2f) ? dist : 0.5f );
		}
	}
}

void myEncodeFun()
{
	sgct::SharedData::instance()->writeDouble( &curr_time );
	sgct::SharedData::instance()->writeFloat( &size_factor );
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readDouble( &curr_time );
	sgct::SharedData::instance()->readFloat( &size_factor );
}
