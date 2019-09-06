#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

sgct::Engine * gEngine;

void drawFun();
void preSyncFun();
void initOGLFun();
void encodeFun();
void decodeFun();
void cleanUpFun();

sgct_utils::SGCTBox * box = NULL;
GLint matrixLoc = -1;

//variables to share across cluster
sgct::SharedDouble currentTime(0.0);

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( initOGLFun );
    gEngine->setDrawFunction( drawFun );
    gEngine->setPreSyncFunction( preSyncFun );
    gEngine->setCleanUpFunction( cleanUpFun );

    if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
    {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction(encodeFun);
    sgct::SharedData::instance()->setDecodeFunction(decodeFun);

    // Main loop
    gEngine->render();

    // Clean up
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void drawFun()
{
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );

    double speed = 0.44;

    //create scene transform (animation)
    glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
    scene_mat = glm::rotate( scene_mat, static_cast<float>( currentTime.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));
    scene_mat = glm::rotate( scene_mat, static_cast<float>( currentTime.getVal() * (speed/2.0) ), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box") );

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, &MVP[0][0]);

    //draw the box
    box->draw();

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
}

void preSyncFun()
{
    if( gEngine->isMaster() )
    {
        currentTime.setVal( sgct::Engine::getTime() );
    }
}

void initOGLFun()
{
    sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
    sgct::TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    box = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
    //myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::CubeMap);
    //myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::SkyBox);

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise

    sgct::ShaderManager::instance()->addShaderProgram( "xform",
            "SimpleVertexShader.vertexshader",
            "SimpleFragmentShader.fragmentshader" );

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    matrixLoc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
    GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
    glUniform1i( Tex_Loc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();
}

void encodeFun()
{
    sgct::SharedData::instance()->writeDouble(&currentTime);
}

void decodeFun()
{
    sgct::SharedData::instance()->readDouble(&currentTime);
}

void cleanUpFun()
{
    if(box != NULL)
        delete box;
}
