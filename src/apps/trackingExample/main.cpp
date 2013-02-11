#include "sgct.h"

sgct::Engine * gEngine;

void myInitOGLFun();
void myPreSyncFun();
void myDrawFun();

void drawAxes(float size);
void drawWireCube(float size);

//store each device's transform 4x4 matrix
glm::dmat4 * trackedTransforms = NULL;

//pointer to a device
sgct::SGCTTrackingDevice * devicePtr = NULL;
//pointer to a tracker
sgct::SGCTTracker * trackerPtr = NULL;

int main( int argc, char* argv[] )
{
	// Allocate
	gEngine = new sgct::Engine( argc, argv );

	// Bind your functions
	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setDrawFunction( myDrawFun );

	// Init the engine
	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;
	delete [] trackedTransforms;
	trackedTransforms = NULL;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myInitOGLFun()
{
	glEnable(GL_DEPTH_TEST);

	unsigned int numberOfTrackedDevices =
		sgct::Engine::getTrackingManager()->getNumberOfDevices();

	//allocate the array
	trackedTransforms = new glm::dmat4[ numberOfTrackedDevices ];
}

/*
	This callback is called once per render loop iteration.
*/
void myPreSyncFun()
{
	/*
	Store all transforms in the array by looping through all trackers and all devices.
	*/
	unsigned int index = 0;
	for(size_t i = 0; i < sgct::Engine::getTrackingManager()->getNumberOfTrackers(); i++)
	{
		trackerPtr = sgct::Engine::getTrackingManager()->getTrackerPtr(i);
		
		for(size_t j = 0; j < trackerPtr->getNumberOfDevices(); j++)
		{
			devicePtr = trackerPtr->getDevicePtr(j);
			if( devicePtr->hasSensor() )
				trackedTransforms[ index ] = devicePtr->getTransformMat();

			index++;
		}
	}
}

/*
	This callback can be called several times per render loop iteration.
	Using a single viewport in stereo (3D) usually results in refresh rate of 120 Hz.
*/
void myDrawFun()
{
	//draw some cubes in space
	for( float i=-0.5f; i<=0.5f; i+=0.2f)
		for(float j=-0.5f; j<=0.5f; j+=0.2f)
		{
			glPushMatrix();
			glTranslatef(i, j, 0.0f);
			glColor3f(1.0f,1.0f,0.0f);
			drawWireCube(0.04f);
			glPopMatrix();
		}

	float textVerticalPos = static_cast<float>(sgct::Engine::getWindowPtr()->getYResolution()) - 100.0f;
	float lineSpace = 14.0f;
	int fontSize = 10;

	unsigned int index = 0;
	for(size_t i = 0; i < sgct::Engine::getTrackingManager()->getNumberOfTrackers(); i++)
	{
		trackerPtr = sgct::Engine::getTrackingManager()->getTrackerPtr(i);
		for(size_t j = 0; j < trackerPtr->getNumberOfDevices(); j++)
		{
			devicePtr = trackerPtr->getDevicePtr(j);

			glColor3f(1.0f,1.0f,0.0f);
			sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize + 2 ), 50.0f, textVerticalPos,
				"Device %u (%s)", i, devicePtr->getName().c_str());
			textVerticalPos -= lineSpace;

			//has this device a positional sensor?
			if( devicePtr->hasSensor() )
			{
				//Draw cube and pointer line for all devices except the head
				if( sgct::Engine::getTrackingManager()->getHeadDevicePtr() != devicePtr )
				{
					glLineWidth(2.0);

					glPushMatrix();

					//get transform from tracker
					glLoadMatrixd( glm::value_ptr(trackedTransforms[ index ]) );
					glColor3f(0.5f,0.5f,0.5f);
					drawWireCube(0.1f);

					drawAxes(0.1f);

					//draw pointer line
					glBegin(GL_LINES);
					glColor3f(1.0f,1.0f,0.0f);
					glVertex3f(0.0f, 0.0f, 0.0f);
					glVertex3f(0.0f, 0.0f, -5.0f);
					glEnd();

					glPopMatrix();
				}
				
				double trackerTime = devicePtr->getTrackerDeltaTime();
				glColor3f(0.0f,1.0f,1.0f);
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos,
					"Sensor id:%d, freq: %.1f Hz", devicePtr->getSensorId(), trackerTime <= 0.0 ? 0.0 : 1.0/trackerTime);
				textVerticalPos -= lineSpace;
				
				glColor3f(1.0f,1.0f,1.0f);
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
					"Pos x=%.3g y=%.3g z=%.3g",
					devicePtr->getPosition().x,
					devicePtr->getPosition().y,
					devicePtr->getPosition().z);
				textVerticalPos -= lineSpace;
				
				glColor3f(1.0f,1.0f,1.0f);
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
					"Rot rx=%.3g ry=%.3g rz=%.3g",
					devicePtr->getEulerAngles().x,
					devicePtr->getEulerAngles().y,
					devicePtr->getEulerAngles().z);
				textVerticalPos -= lineSpace;
			}
			if( devicePtr->hasButtons() )
			{
				glColor3f(0.0f,1.0f,1.0f);
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos, "Buttons");
				textVerticalPos -= lineSpace;

				glColor3f(1.0f,1.0f,1.0f);
				for(size_t k=0; k < devicePtr->getNumberOfButtons(); k++)
				{
					sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
					"Button %u: %s", k, devicePtr->getButton(k) ? "pressed" : "released");
					textVerticalPos -= lineSpace;
				}
			}
			if( devicePtr->hasAnalogs() )
			{
				glColor3f(0.0f,1.0f,1.0f);
				double analogTime = devicePtr->getAnalogDeltaTime();
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos,
					"Analog axes, freq: %.1f Hz", analogTime <= 0.0 ? 0.0 : 1.0/analogTime);
				textVerticalPos -= lineSpace;

				glColor3f(1.0f,1.0f,1.0f);
				for(size_t k=0; k < devicePtr->getNumberOfAxes(); k++)
				{
					sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
					"Axis %u: %.3g", j, devicePtr->getAnalog(k));
					textVerticalPos -= lineSpace;
				}
			}

			//add extra line after each device
			textVerticalPos -= lineSpace;
			
			//iterate the index
			index++;
		}
	}
}

void drawAxes(float size)
{
	glLineWidth(2.0);
	glBegin(GL_LINES);

	//x-axis
	glColor3f(1.0f,0.0f,0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(size, 0.0f, 0.0f);

	//y-axis
	glColor3f(0.0f,1.0f,0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, size, 0.0f);

	//z-axis
	glColor3f(0.0f,0.0f,1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, size);

	glEnd();
}

void drawWireCube(float size)
{
	//bottom
	glBegin(GL_LINE_STRIP);
	glVertex3f( -size, -size, -size);
	glVertex3f( size, -size, -size);
	glVertex3f( size, -size, size);
	glVertex3f( -size, -size, size);
	glVertex3f( -size, -size, -size);
	glEnd();

	//top
	glBegin(GL_LINE_STRIP);
	glVertex3f( -size, size, -size);
	glVertex3f( size, size, -size);
	glVertex3f( size, size, size);
	glVertex3f( -size, size, size);
	glVertex3f( -size, size, -size);
	glEnd();

	//sides
	glBegin(GL_LINES);
	glVertex3f( -size, -size, -size);
	glVertex3f( -size, size, -size);

	glVertex3f( size, -size, -size);
	glVertex3f( size, size, -size);

	glVertex3f( size, -size, size);
	glVertex3f( size, size, size);

	glVertex3f( -size, -size, size);
	glVertex3f( -size, size, size);
	glEnd();
}
