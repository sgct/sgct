#include "sgct.h"

sgct::Engine * gEngine;

void myInitOGLFun();
void myDrawFun();

void drawAxes(float size);
void drawWireCube(float size);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	gEngine->setDrawFunction( myDrawFun );

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myInitOGLFun()
{
	glEnable(GL_DEPTH_TEST);
}

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

	size_t numberOfTrackedDevices =
		sgct::Engine::getTrackingManager()->getNumberOfDevices();

	core_sgct::SGCTTrackingDevice * td;

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
			//Draw cube and ray for all devices except the head
			if( static_cast<size_t>(sgct::Engine::getTrackingManager()->getHeadSensorIndex()) != i )
			{
				glLineWidth(2.0);

				glPushMatrix();

				//get transform from tracker
				glLoadMatrixd( glm::value_ptr(td->getTransformMat()) );

				glColor3f(0.5f,0.5f,0.5f);
				drawWireCube(0.1f);

				drawAxes(0.1f);

				//draw ray
				glBegin(GL_LINES);
				glColor3f(1.0f,1.0f,0.0f);
				glVertex3f(0.0f, 0.0f, 0.0f);
				glVertex3f(0.0f, 0.0f, -5.0f);
				glEnd();

				glPopMatrix();
			}

			glColor3f(0.0f,1.0f,1.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos,
				"Tracker sensor:%d, freq: %.1f Hz", td->getSensor(), 1.0/td->getTrackerTime());
			textVerticalPos -= lineSpace;

			glColor3f(1.0f,1.0f,1.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
				"Pos x=%.3lf y=%.3lf z=%.3lf",
				td->getPosition().x,
				td->getPosition().y,
				td->getPosition().z);
			textVerticalPos -= lineSpace;

			glColor3f(1.0f,1.0f,1.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
				"Rot rx=%.3lf ry=%.3lf rz=%.3lf",
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
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos,
				"Analog axes, freq: %.1f Hz", 1.0/td->getAnalogTime());
			textVerticalPos -= lineSpace;

			glColor3f(1.0f,1.0f,1.0f);
			for(size_t j=0; j < td->getNumberOfAxes(); j++)
			{
				Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
				"Axis %u: %g", j, td->getAnalog(j));
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
