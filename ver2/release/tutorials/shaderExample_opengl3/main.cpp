#include "sgct.h"

sgct::Engine * gEngine;

void myInitFun();
void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
void keyCallback(int key, int action);

sgct::SharedDouble curr_time(0.0);
sgct::SharedBool reloadShader(false);

//global vars
GLuint vertexArray = GL_FALSE;
GLuint vertexPositionBuffer = GL_FALSE;

GLint matrix_loc = -1;
GLint time_loc = -1;

int main( int argc, char* argv[] )
{
	// Allocate
	gEngine = new sgct::Engine( argc, argv );

	// Bind your functions
	gEngine->setInitOGLFunction( myInitFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setCleanUpFunction( myCleanUpFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );
	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

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

	sgct::ShaderManager::instance()->addShaderProgram( "xform",
			"simple.vert",
			"simple.frag" );

	sgct::ShaderManager::instance()->bindShaderProgram( "xform" );
 
	matrix_loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
	time_loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "curr_time" );
 
	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void myDrawFun()
{
	float speed = 50.0f;

	glm::mat4 scene_mat = glm::rotate( glm::mat4(1.0f), static_cast<float>( curr_time.getVal() ) * speed, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 MVP = gEngine->getActiveModelViewProjectionMatrix() * scene_mat;

	sgct::ShaderManager::instance()->bindShaderProgram( "xform" );
		
	glUniformMatrix4fv(matrix_loc, 1, GL_FALSE, &MVP[0][0]);
	glUniform1f( time_loc, static_cast<float>( curr_time.getVal() ) );

	glBindVertexArray(vertexArray);
	
	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 3);

	//unbind
	glBindVertexArray(0);
	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		curr_time.setVal( sgct::Engine::getTime() );
	}
}

void myPostSyncPreDrawFun()
{
	if( reloadShader.getVal() )
	{
		reloadShader.setVal(false); //reset

		sgct::ShaderProgram sp = sgct::ShaderManager::instance()->getShaderProgram( "xform" );
		sp.reload();

		//reset location variables
		sgct::ShaderManager::instance()->bindShaderProgram( "xform" );
		time_loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "curr_time" );
		matrix_loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
		sgct::ShaderManager::instance()->unBindShaderProgram();
	}
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case SGCT_KEY_R:
			if(action == SGCT_PRESS)
				reloadShader.setVal(true);
			break;
		}
	}
}

void myEncodeFun()
{
	sgct::SharedData::instance()->writeDouble( &curr_time );
	sgct::SharedData::instance()->writeBool( &reloadShader );
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readDouble( &curr_time );
	sgct::SharedData::instance()->readBool( &reloadShader );
}

void myCleanUpFun()
{
	if(vertexPositionBuffer)
		glDeleteBuffers(1, &vertexPositionBuffer);
	if(vertexArray)
		glDeleteVertexArrays(1, &vertexArray);
}
