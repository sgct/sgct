#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreDrawFun();
void myEncodeFun();
void myDecodeFun();

double time = 0.0;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );
	
	//Bind your draw function to the render loop
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreDrawFunction( myPreDrawFun );
	sgct::SharedData::Instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::Instance()->setDecodeFunction(myDecodeFun);
	

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

void myDrawFun()
{
	float speed = 50.0f;
	glRotatef(static_cast<float>( time ) * speed, 0.0f, 1.0f, 0.0f);
	
	//render a single triangle
	glBegin(GL_TRIANGLES);
		glColor3f(1.0f, 0.0f, 0.0f); //Red
		glVertex3f(-0.5f, -0.5f, 0.0f);

		glColor3f(0.0f, 1.0f, 0.0f); //Green
		glVertex3f(0.0f, 0.5f, 0.0f);

		glColor3f(0.0f, 0.0f, 1.0f); //Blue
		glVertex3f(0.5f, -0.5f, 0.0f);
	glEnd();
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble( time );
}

void myDecodeFun()
{
	time = sgct::SharedData::Instance()->readDouble();
}

void myPreDrawFun()
{
	if( gEngine->isMaster() )
	{
		time = glfwGetTime();
	}
}