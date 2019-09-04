#include "sgct.h"

sgct::Engine * gEngine;

void initFun();
void drawFun();
void preSyncFun();
void postSyncPreDrawFun();
void encodeFun();
void decodeFun();
void cleanUpFun();
void keyCallback(int key, int action);

sgct::SharedDouble currentTime(0.0);
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
    gEngine->setInitOGLFunction( initFun );
    gEngine->setDrawFunction( drawFun );
    gEngine->setPreSyncFunction( preSyncFun );
    gEngine->setCleanUpFunction( cleanUpFun );
    gEngine->setPostSyncPreDrawFunction( postSyncPreDrawFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );
    sgct::SharedData::instance()->setEncodeFunction(encodeFun);
    sgct::SharedData::instance()->setDecodeFunction(decodeFun);

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

void initFun()
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

void drawFun()
{
    float speed = 0.87f;

    glm::mat4 scene_mat = glm::rotate( glm::mat4(1.0f), static_cast<float>( currentTime.getVal() ) * speed, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );
        
    glUniformMatrix4fv(matrix_loc, 1, GL_FALSE, &MVP[0][0]);
    glUniform1f( time_loc, static_cast<float>( currentTime.getVal() ) );

    glBindVertexArray(vertexArray);
    
    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, 3);

    //unbind
    glBindVertexArray(0);
    sgct::ShaderManager::instance()->unBindShaderProgram();
}

void preSyncFun()
{
    if( gEngine->isMaster() )
    {
        currentTime.setVal( sgct::Engine::getTime() );
    }
}

void postSyncPreDrawFun()
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

void encodeFun()
{
    sgct::SharedData::instance()->writeDouble( &currentTime );
    sgct::SharedData::instance()->writeBool( &reloadShader );
}

void decodeFun()
{
    sgct::SharedData::instance()->readDouble( &currentTime );
    sgct::SharedData::instance()->readBool( &reloadShader );
}

void cleanUpFun()
{
    if(vertexPositionBuffer)
        glDeleteBuffers(1, &vertexPositionBuffer);
    if(vertexArray)
        glDeleteVertexArrays(1, &vertexArray);
}
