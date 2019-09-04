#include <stdlib.h>
#include <stdio.h>

#include "sgct.h"

sgct::Engine * gEngine;

void drawFun();
void preSyncFun();
void postSyncPreDrawFun();
void initOGLFun();
void encodeFun();
void decodeFun();
void keyCallback(int key, int action);

int time_loc = -1;

//variables to share across cluster
sgct::SharedDouble currentTime(0.0);
sgct::SharedBool reloadShader(false);

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( initOGLFun );
    gEngine->setDrawFunction( drawFun );
    gEngine->setPreSyncFunction( preSyncFun );
    gEngine->setPostSyncPreDrawFunction( postSyncPreDrawFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );
    sgct::SharedData::instance()->setEncodeFunction(encodeFun);
    sgct::SharedData::instance()->setDecodeFunction(decodeFun);

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

void drawFun()
{
    //set current shader program
    sgct::ShaderManager::instance()->bindShaderProgram( "SimpleColor" );
    glUniform1f( time_loc, static_cast<float>( currentTime.getVal() ) );

    float speed = 50.0f;
    glRotatef(static_cast<float>( currentTime.getVal() ) * speed, 0.0f, 1.0f, 0.0f);

    //render a single triangle
    glBegin(GL_TRIANGLES);
        glVertex3f(-0.5f, -0.5f, 0.0f);
        glVertex3f(0.0f, 0.5f, 0.0f);
        glVertex3f(0.5f, -0.5f, 0.0f);
    glEnd();

    //unset current shader program
    sgct::ShaderManager::instance()->unBindShaderProgram();
}

void initOGLFun()
{
    sgct::ShaderManager::instance()->addShaderProgram( "SimpleColor", "simple.vert", "simple.frag" );
    sgct::ShaderManager::instance()->bindShaderProgram( "SimpleColor" );

    time_loc = sgct::ShaderManager::instance()->getShaderProgram( "SimpleColor").getUniformLocation( "curr_time" );

    sgct::ShaderManager::instance()->unBindShaderProgram();
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

        sgct::ShaderProgram sp = sgct::ShaderManager::instance()->getShaderProgram( "SimpleColor" );
        sp.reload();

        //reset location variables
        sgct::ShaderManager::instance()->bindShaderProgram( "SimpleColor" );
        time_loc = sgct::ShaderManager::instance()->getShaderProgram( "SimpleColor").getUniformLocation( "curr_time" );
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
