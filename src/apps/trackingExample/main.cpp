#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void drawWireCube(float size);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );
	
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

void myDrawFun()
{
	size_t numberOfTrackedDevices = 
		sgct::Engine::getTrackingManager()->getNumberOfDevices();

	core_sgct::SGCTTrackingDevice * td;

	float textVerticalPos = static_cast<float>(sgct::Engine::getWindowPtr()->getVResolution()) - 100.0f;
	float lineSpace = 14.0f;
	int fontSize = 10;

	glColor3f(1.0f,0.0f,0.0f);
	Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 50.0f, textVerticalPos,
		"SGCT sampling freq: %lf Hz", 1.0/sgct::Engine::getTrackingManager()->getSamplingTime());
	textVerticalPos -= lineSpace;

	for(size_t i = 0; i < numberOfTrackedDevices; i++)
	{
		td = sgct::Engine::getTrackingManager()->getTrackingPtr( i );

		//has this device a positional tracker?
		if( td->hasTracker() )
		{
			if( static_cast<size_t>(sgct::Engine::getTrackingManager()->getHeadSensorIndex()) == i )
			{
				glColor3f(1.0f,0.0f,0.0f);
				glLineWidth(2.0);
				
				glPushMatrix();
				glLoadMatrixd( glm::value_ptr(td->getTransformMat()) );
				drawWireCube(0.1f);
				
				//draw ray
				glBegin(GL_LINES);
				glVertex3f(0.0f, 0.0f, 0.0f);
				glVertex3f(0.0f, 0.0f, -5.0f);
				glEnd();
				glPopMatrix();
			}

			glColor3f(1.0f,1.0f,0.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 50.0f, textVerticalPos,
				"Tracker %u (%s), freq=%lf Hz", i, td->getName().c_str(), 1.0/td->getTrackerTime());
			textVerticalPos -= lineSpace;
			
			glColor3f(1.0f,1.0f,1.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos,
				"Pos x=%.3lf y=%.3lf z=%.3lf",
				td->getPosition().x,
				td->getPosition().y,
				td->getPosition().z);
			textVerticalPos -= lineSpace;

			glColor3f(1.0f,1.0f,1.0f);
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos,
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
			Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 100.0f, textVerticalPos, "Analog axes");
			textVerticalPos -= lineSpace;

			glColor3f(1.0f,1.0f,1.0f);
			for(size_t j=0; j < td->getNumberOfAxes(); j++)
			{
				Freetype::print(sgct::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), 120.0f, textVerticalPos,
				"Axis %u: %lf", j, td->getAnalog(j));
				textVerticalPos -= lineSpace;
			}
		}
	}
}

void drawWireCube(float size)
{
	//draw a cube
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