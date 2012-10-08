#include "sgct.h"

sgct::Engine * gEngine;

void myInitOGLFun();
void myPreSyncFun();
void myDrawFun();

void drawAxes(float size);
void drawWireCube(float size);

unsigned int numberOfTrackedDevices;

//store each device's transform 4x4 matrix
glm::dmat4 * trackedTransforms = NULL;

//pointer to a device
core_sgct::SGCTTrackingDevice * td = NULL;

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

	numberOfTrackedDevices =
		sgct::Engine::getTrackingManager()->getNumberOfDevices();

	//allocate the array
	trackedTransforms = new glm::dmat4[ numberOfTrackedDevices ];
}

/*
	This callback is called once per render loop iteration.
*/
void myPreSyncFun()
{
	//store the transforms
	for(size_t i = 0; i < numberOfTrackedDevices; i++)
	{
		td = sgct::Engine::getTrackingManager()->getTrackingPtr( i );
		if( td->hasTracker() )
			trackedTransforms[ i ] = td->getTransformMat();
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

	float textVerticalPos = static_cast<float>(sgct::Engine::getWindowPtr()->getVResolution()) - 100.0f;
	float lineSpace = 14.0f;
	int fontSize = 10;

	for(size_t i = 0; i < numberOfTrackedDevices; i++)
	{
		td = sgct::Engine::getTrackingManager()->getTrackingPtr( i );

		glColor3f(1.0f,1.0f,0.0f);
		Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize + 2 ), 50.0f, textVerticalPos,
			"Device %u (%s)", i, td->getName().c_str());
		textVerticalPos -= lineSpace;

		//has this device a positional tracker?
		if( td->hasTracker() )
		{
			//Draw cube and pointer line for all devices except the head
			if( static_cast<size_t>(sgct::Engine::getTrackingManager()->getHeadSensorIndex()) != i )
			{
				glLineWidth(2.0);

				glPushMatrix();

				//get transform from tracker
				glLoadMatrixd( glm::value_ptr(trackedTransforms[ i ]) );

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

            double trackerTime = td->getTrackerTime();
			glColor3f(0.0f,1.0f,1.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos,
				"Tracker sensor:%d, freq: %.1f Hz", td->getSensor(), trackerTime <= 0.0 ? 0.0 : 1.0/trackerTime);
			textVerticalPos -= lineSpace;

			glColor3f(1.0f,1.0f,1.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
				"Pos x=%.3g y=%.3g z=%.3g",
				td->getPosition().x,
				td->getPosition().y,
				td->getPosition().z);
			textVerticalPos -= lineSpace;

			glColor3f(1.0f,1.0f,1.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
				"Rot rx=%.3g ry=%.3g rz=%.3g",
				td->getEulerAngles().x,
				td->getEulerAngles().y,
				td->getEulerAngles().z);
			textVerticalPos -= lineSpace;
		}
		if( td->hasButtons() )
		{
			glColor3f(0.0f,1.0f,1.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos, "Buttons");
			textVerticalPos -= lineSpace;

			glColor3f(1.0f,1.0f,1.0f);
			for(size_t j=0; j < td->getNumberOfButtons(); j++)
			{
				Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
				"Button %u: %s", j, td->getButton(j) ? "pressed" : "released");
				textVerticalPos -= lineSpace;
			}
		}
		if( td->hasAnalogs() )
		{
			glColor3f(0.0f,1.0f,1.0f);
			double analogTime = td->getAnalogTime();
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos,
				"Analog axes, freq: %.1f Hz", analogTime <= 0.0 ? 0.0 : 1.0/analogTime);
			textVerticalPos -= lineSpace;

			glColor3f(1.0f,1.0f,1.0f);
			for(size_t j=0; j < td->getNumberOfAxes(); j++)
			{
				Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
				"Axis %u: %.3g", j, td->getAnalog(j));
				textVerticalPos -= lineSpace;
			}
		}

		//add extra line after each device
		textVerticalPos -= lineSpace;
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
