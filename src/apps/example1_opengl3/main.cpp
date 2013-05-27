#include "sgct.h"

sgct::Engine * gEngine;

void myInitFun();
void myDrawFun();
void myPreSyncFun();
void myEncodeFun();
void myDecodeFun();

sgct::SharedDouble curr_time(0.0);

//global vars
GLuint vertexbuffer;
GLuint vertexArray;
GLint Matrix_Loc = -1;

int main( int argc, char* argv[] )
{

	// Allocate
	gEngine = new sgct::Engine( argc, argv );

	// Bind your functions
	gEngine->setInitOGLFunction( myInitFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	sgct::SharedData::Instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::Instance()->setDecodeFunction(myDecodeFun);

	// Init the engine
	if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
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

void myInitFun()
{
	static const GLfloat vertex_data[] = { 
		-0.5f, -0.5f, 0.0f,
		 0.0f, 0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f
	};

	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind
	glBindVertexArray(0); //unbind

	sgct::ShaderManager::Instance()->addShader( "xform",
			"SimpleVertexShader.vertexshader",
			"SimpleFragmentShader.fragmentshader" );

	sgct::ShaderManager::Instance()->bindShader( "xform" );
 
	Matrix_Loc = sgct::ShaderManager::Instance()->getShader( "xform").getUniformLocation( "MVP" );
 
	sgct::ShaderManager::Instance()->unBindShader();
}

void myDrawFun()
{
	float speed = 50.0f;

	glm::mat4 scene_mat = glm::rotate( glm::mat4(1.0f), static_cast<float>( curr_time.getVal() ) * speed, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 MVP = gEngine->getActiveModelViewProjectionMatrix() * scene_mat;

	sgct::ShaderManager::Instance()->bindShader( "xform" );
		
	glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindVertexArray(vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices starting at 0 -> 1 triangle

	//unbind
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
	sgct::ShaderManager::Instance()->unBindShader();

	/*
	//render a single triangle
	glBegin(GL_TRIANGLES);
		glColor3f(1.0f, 0.0f, 0.0f); //Red
		glVertex3f(-0.5f, -0.5f, 0.0f);

		glColor3f(0.0f, 1.0f, 0.0f); //Green
		glVertex3f(0.0f, 0.5f, 0.0f);

		glColor3f(0.0f, 0.0f, 1.0f); //Blue
		glVertex3f(0.5f, -0.5f, 0.0f);
	glEnd();*/
}

void myPreSyncFun()
{
	//set the time only on the master
	if( gEngine->isMaster() )
	{
		//get the time in seconds
		curr_time.setVal(sgct::Engine::getTime());
	}
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble( &curr_time );
}

void myDecodeFun()
{
	sgct::SharedData::Instance()->readDouble( &curr_time );
}
