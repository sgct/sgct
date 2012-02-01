#include <stdlib.h>
#include <stdio.h>

#include "RenderEngine.h"

sgct::RenderEngine * gRenderEngine;

void myDrawFun();
void myPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();

void keyCallback(int key, int action);

//variables to share across cluster
double dt = 0.0;
double time = 0.0;
bool showFPS = false;
unsigned char flags = 0;

void drawGrid(float size, int steps);

sgct::SharedData mySharedData(1024);

int main( int argc, char* argv[] )
{	
	gRenderEngine = new sgct::RenderEngine( mySharedData, argc, argv );

	mySharedData.setEncodeFunction(myEncodeFun);
	mySharedData.setDecodeFunction(myDecodeFun);

	//init openGL
	gRenderEngine->setInitOGLFunction( myInitOGLFun );
	gRenderEngine->setDrawFunction( myDrawFun );
	gRenderEngine->setPreDrawFunction( myPreDrawFun );
	glfwSetKeyCallback( keyCallback );

	// Main loop
	gRenderEngine->render();

	// Clean up
	delete gRenderEngine;
	
	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	glPushMatrix();
	
	/*if( gRenderEngine->isUsingSwapGroups() )
	{
		GLuint frameNumber;
		gRenderEngine->getSwapGroupFrameNumber(frameNumber);
		glTranslatef(0.0f, sinf(static_cast<float>(frameNumber)/100.0f), 0.0f);
	}
	else
		glTranslatef(0.0f, static_cast<float>(sin(gRenderEngine->mSharedData->time)), 0.0f);
	glColor3f(1.0f,0.0f,0.0f); //red
	glBegin(GL_QUADS);
	
	glVertex3f( -3.0f, 0.01f, 0.0f);
	glVertex3f( 3.0f, 0.01f, 0.0f);
	glVertex3f( 3.0f, -0.01f, 0.0f);
	glVertex3f( -3.0f, -0.01f, 0.0f);

	glEnd();
	*/

	glRotatef(static_cast<float>(time)*10.0f, 0.0f, 1.0f, 0.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glLineWidth(2.0);
	
	//draw a cube
	//bottom
	glBegin(GL_LINE_STRIP);
	glVertex3f( -0.5f, -0.5f, -0.5f);
	glVertex3f( 0.5f, -0.5f, -0.5f);
	glVertex3f( 0.5f, -0.5f, 0.5f);
	glVertex3f( -0.5f, -0.5f, 0.5f);
	glVertex3f( -0.5f, -0.5f, -0.5f);
	glEnd();

	//top
	glBegin(GL_LINE_STRIP);
	glVertex3f( -0.5f, 0.5f, -0.5f);
	glVertex3f( 0.5f, 0.5f, -0.5f);
	glVertex3f( 0.5f, 0.5f, 0.5f);
	glVertex3f( -0.5f, 0.5f, 0.5f);
	glVertex3f( -0.5f, 0.5f, -0.5f);
	glEnd();

	//sides
	glBegin(GL_LINES);
	glVertex3f( -0.5f, -0.5f, -0.5f);
	glVertex3f( -0.5f, 0.5f, -0.5f);

	glVertex3f( 0.5f, -0.5f, -0.5f);
	glVertex3f( 0.5f, 0.5f, -0.5f);

	glVertex3f( 0.5f, -0.5f, 0.5f);
	glVertex3f( 0.5f, 0.5f, 0.5f);

	glVertex3f( -0.5f, -0.5f, 0.5f);
	glVertex3f( -0.5f, 0.5f, 0.5f);
	glEnd();

	glPopMatrix();

	//drawGrid(10.0, 100);
}

void myPreDrawFun()
{
	//fprintf(stderr, "preframe\n");
	if( gRenderEngine->isSyncServer() )
	{
		dt = gRenderEngine->getDt();
		time = gRenderEngine->getTime();
	}

	gRenderEngine->setDisplayInfoVisibility( showFPS );
}

void myInitOGLFun()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
}

void myEncodeFun()
{
	flags = showFPS	? flags | 1 : flags & ~1;

	mySharedData.writeDouble(dt);
	mySharedData.writeDouble(time);
	mySharedData.writeUChar(flags);
}

void myDecodeFun()
{
	dt = mySharedData.readDouble();
	time = mySharedData.readDouble();
	flags = mySharedData.readUChar();

	showFPS	= flags & 0x0001;
}

void drawGrid(float size, int steps)
{
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPushMatrix();
	glTranslatef(-size/2.0f, -size/2.0f, 0.0f);
	glColor4f(1.0f,1.0f,1.0f,0.5f);
	glLineWidth(1.0);
	
	glBegin(GL_LINES);
	//horizontal lines
	for( float f=0.0f; f<=size; f+= (size/static_cast<float>(steps)))
	{
		glVertex3f( 0.0f, f, 0.0f);
		glVertex3f( size, f, 0.0f);
	}
	//Vertical lines
	for( float f=0.0f; f<=size; f+= (size/static_cast<float>(steps)))
	{
		glVertex3f( f, 0.0f, 0.0f);
		glVertex3f( f, size, 0.0f);
	}
	glEnd();
	
	glVertex3f( -1.0f, 0.01f, 0.0f);
	glVertex3f( 1.0f, 0.01f, 0.0f);
	glVertex3f( 1.0f, -0.01f, 0.0f);
	glVertex3f( -1.0f, -0.01f, 0.0f);

	glPopMatrix();
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}

void keyCallback(int key, int action)
{
	switch( key )
	{
	case 'I':
		if(action == GLFW_PRESS)
			showFPS = !showFPS;
		break;
	}
}