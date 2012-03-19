#include "sgct.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

sgct::Engine * gEngine;

void myDrawFun();
void myPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
//input callbacks
void keyCallback(int key, int action);
void mouseButtonCallback(int button, int action);

void drawXZGrid(int size, float yPos);
void drawPyramid(float width);

float rotationSpeed = 0.002f;
float walkingSpeed = 2.5f;

GLuint myLandscapeDisplayList = 0;
const int landscapeSize = 50;
bool arrowButtons[4];
enum directions { FORWARD = 0, BACKWARD, LEFT, RIGHT };

bool mouseLeftButton = false;
int mouseDx = 0;
int mouseXPos[] = { 0, 0 };

glm::vec3 view(0.0f, 0.0f, 1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
glm::vec3 pos(0.0f, 0.0f, 0.0f);
glm::mat4 xform(1.0f);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreDrawFunction( myPreDrawFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );
	gEngine->setMouseButtonCallbackFunction( mouseButtonCallback );

	for(int i=0; i<4; i++)
		arrowButtons[i] = false;

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::Instance()->setEncodeFunction( myEncodeFun );
	sgct::SharedData::Instance()->setDecodeFunction( myDecodeFun );

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;
	glDeleteLists(myLandscapeDisplayList, 1);

	// Exit program
	exit( EXIT_SUCCESS );
}

void myInitOGLFun()
{
	glEnable(GL_LINE_SMOOTH); 
	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
	
	//create and compile display list
	myLandscapeDisplayList = glGenLists(1);
	glNewList(myLandscapeDisplayList, GL_COMPILE);
	
	drawXZGrid(landscapeSize, -1.5f);

	//pick a seed for the random function (must be same on all nodes)
	srand(9745);
	for(int i=0; i<150; i++)
	{
		float xPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);
		float zPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);

		glPushMatrix();
		glTranslatef(xPos, -1.5f, zPos);
		drawPyramid(0.6f);
		glPopMatrix();
	}
	

	glEndList();
}

void myPreDrawFun()
{	
	if( gEngine->isMaster() )
	{
		
		glm::mat4 ViewRotateX(1.0f);
		if( mouseLeftButton )
		{
			int tmpYPos;
			sgct::Engine::getMousePos( &mouseXPos[0], &tmpYPos );
			mouseDx = mouseXPos[0] - mouseXPos[1];
		}
		else
		{
			mouseDx = 0;
		}

		static float panRot = 0.0f;
		panRot += static_cast<float>(mouseDx) * rotationSpeed;

		ViewRotateX = glm::rotate(
			glm::mat4(1.0f),
			panRot,
			glm::vec3(0.0f, 1.0f, 0.0f));

		view = glm::inverse(glm::mat3(ViewRotateX)) * glm::vec3(0.0f, 0.0f, 1.0f);
		
		glm::vec3 right = glm::cross(view, up);

		if( arrowButtons[FORWARD] )
			pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * view);
		if( arrowButtons[BACKWARD] )
			pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * view);
		if( arrowButtons[LEFT] )
			pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);
		if( arrowButtons[RIGHT] )
			pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);

		/*
			To get a first person camera, the world needs
			to be transformed around the users head.

			This is done by:
			1, Transform the user to coordinate system origin
			2, Apply transformation
			3, Transform the user back to original position

			However, mathwise this process need to be reversed
			due to the matrix multiplication order.
		*/

		//3. transform user back to original position
		xform = glm::translate( glm::mat4(1.0f), sgct::Engine::getUserPtr()->getPos() );
		//2. apply transformation
		xform *= (ViewRotateX * glm::translate( glm::mat4(1.0f), pos ));
		//1. transform user to coordinate system origin
		xform *= glm::translate( glm::mat4(1.0f), -sgct::Engine::getUserPtr()->getPos() );
	}
}

void myDrawFun()
{	
	glLoadMatrixf(glm::value_ptr(xform));
	glCallList(myLandscapeDisplayList);
}

void myEncodeFun()
{
	for(int i=0; i<16; i++)
		sgct::SharedData::Instance()->writeFloat( glm::value_ptr(xform)[i] );
}

void myDecodeFun()
{
	for(int i=0; i<16; i++)
		glm::value_ptr(xform)[i] = sgct::SharedData::Instance()->readFloat();
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case GLFW_KEY_UP:
		case 'W':
			arrowButtons[FORWARD] = (action == GLFW_PRESS ? true : false);
			break;

		case GLFW_KEY_DOWN:
		case 'S':
			arrowButtons[BACKWARD] = (action == GLFW_PRESS ? true : false);
			break;

		case GLFW_KEY_LEFT:
		case 'A':
			arrowButtons[LEFT] = (action == GLFW_PRESS ? true : false);
			break;

		case GLFW_KEY_RIGHT:
		case 'D':
			arrowButtons[RIGHT] = (action == GLFW_PRESS ? true : false);
			break;
		}
	}
}

void mouseButtonCallback(int button, int action)
{
	if( gEngine->isMaster() )
	{	
		switch( button )
		{
		case GLFW_MOUSE_BUTTON_LEFT:
			mouseLeftButton = (action == GLFW_PRESS ? true : false);
			int tmpYPos;
			//set refPos
			sgct::Engine::getMousePos( &mouseXPos[1], &tmpYPos );
			break;
		}
	}
}

void drawXZGrid(int size, float yPos)
{
	glPushMatrix();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glTranslatef(0.0f, yPos, 0.0f);
	
	glLineWidth(3.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	
	glBegin( GL_LINES );
	for(int x = -(size/2); x < (size/2); x++)
	{
		glVertex3i(x, 0, -(size/2));
		glVertex3i(x, 0, (size/2));
	}

	for(int z = -(size/2); z < (size/2); z++)
	{
		glVertex3i(-(size/2), 0, z);
		glVertex3i((size/2), 0, z);
	}
	glEnd();

	glDisable(GL_BLEND);
	glPopMatrix();
}

void drawPyramid(float width)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	//disable depth sorting to avoid flickering
	glDisable(GL_DEPTH_TEST);
	
	glColor4f(1.0f, 0.0f, 0.5f, 0.3f);

	glBegin(GL_TRIANGLE_FAN);
	//draw top
	glVertex3f(0.0f, 2.0f, 0.0f);

	//draw sides
	glVertex3f(-width/2.0f, 0.0f, -width/2.0f);
	glVertex3f(-width/2.0f, 0.0f, width/2.0f);
	glVertex3f(width/2.0f, 0.0f, width/2.0f);
	glVertex3f(width/2.0f, 0.0f, -width/2.0f);
	glVertex3f(-width/2.0f, 0.0f, -width/2.0f);

	glEnd();
	
	glLineWidth(2.0f);
	glColor4f(1.0f, 0.0f, 0.5f, 0.5f);

	glBegin(GL_LINE_LOOP);
	glVertex3f(-width/2.0f, 0.0f, -width/2.0f);
	glVertex3f(0.0f, 2.0f, 0.0f);
	glVertex3f(-width/2.0f, 0.0f, width/2.0f);
	glEnd();
	
	glBegin(GL_LINE_LOOP);
	glVertex3f(-width/2.0f, 0.0f, width/2.0f);
	glVertex3f(0.0f, 2.0f, 0.0f);
	glVertex3f(width/2.0f, 0.0f, width/2.0f);
	glEnd();
	
	glBegin(GL_LINE_LOOP);
	glVertex3f(width/2.0f, 0.0f, width/2.0f);
	glVertex3f(0.0f, 2.0f, 0.0f);
	glVertex3f(width/2.0f, 0.0f, -width/2.0f);
	glEnd();
	
	glBegin(GL_LINE_LOOP);
	glVertex3f(width/2.0f, 0.0f, -width/2.0f);
	glVertex3f(0.0f, 2.0f, 0.0f);
	glVertex3f(-width/2.0f, 0.0f, -width/2.0f);
	glEnd();

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}