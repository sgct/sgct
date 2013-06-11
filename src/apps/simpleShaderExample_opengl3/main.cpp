#include "sgct.h"

sgct::Engine * gEngine;

void myInitFun();
void myDrawFun();
void myPreSyncFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

sgct::SharedDouble curr_time(0.0);

//global vars
GLuint vertexArray = GL_FALSE;
GLuint vertexPositionBuffer = GL_FALSE;

GLint Matrix_Loc = -1;
GLint Time_Loc = -1;

int main( int argc, char* argv[] )
{
	// Allocate
	gEngine = new sgct::Engine( argc, argv );

	// Bind your functions
	gEngine->setInitOGLFunction( myInitFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setCleanUpFunction( myCleanUpFun );
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
	const GLfloat vertex_position_data[] = { 
		-0.5f, -0.5f, 0.0f,
		 0.0f, 0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f
	};

	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	//vertex positions
	glGenBuffers(1, &vertexPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_position_data), vertex_position_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		reinterpret_cast<void*>(0) // array buffer offset
	);

	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind
	glBindVertexArray(0); //unbind

	sgct::ShaderManager::Instance()->addShader( "xform",
			"simple.vert",
			"simple.frag" );

	sgct::ShaderManager::Instance()->bindShader( "xform" );
 
	Matrix_Loc = sgct::ShaderManager::Instance()->getShader( "xform").getUniformLocation( "MVP" );
	Time_Loc = sgct::ShaderManager::Instance()->getShader( "xform").getUniformLocation( "curr_time" );
 
	sgct::ShaderManager::Instance()->unBindShader();
}

void myDrawFun()
{
	float speed = 50.0f;

	glm::mat4 scene_mat = glm::rotate( glm::mat4(1.0f), static_cast<float>( curr_time.getVal() ) * speed, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 MVP = gEngine->getActiveModelViewProjectionMatrix() * scene_mat;

	sgct::ShaderManager::Instance()->bindShader( "xform" );
		
	glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);
	glUniform1f( Time_Loc, static_cast<float>( curr_time.getVal() ) );

	glBindVertexArray(vertexArray);
	
	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 3);

	//unbind
	glBindVertexArray(0);
	sgct::ShaderManager::Instance()->unBindShader();
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

void myCleanUpFun()
{
	if(vertexPositionBuffer)
		glDeleteBuffers(1, &vertexPositionBuffer);
	if(vertexArray)
		glDeleteVertexArrays(1, &vertexArray);
}
