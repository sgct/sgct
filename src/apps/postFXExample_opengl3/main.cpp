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

sgct_utils::SGCTBox * myBox = NULL;
GLint matrixLoc = -1;

//variables to share across cluster
sgct::SharedDouble currentTime(0.0);

//Post FX shader locations
GLint PostFX_Texture_Loc[] = { -1, -1, -1, -1};
GLint Tex2_Loc = -1;
GLint Size_Loc[] = { -1, -1 };

void updatePass1()
{
    glUniform1i( PostFX_Texture_Loc[0], 0 );
}

void updatePass2()
{
    glUniform1i( PostFX_Texture_Loc[1], 0 );
    glUniform1f( Size_Loc[0], static_cast<float>( gEngine->getCurrentXResolution() ) );
}

void updatePass3()
{
    glUniform1i( PostFX_Texture_Loc[2], 0 );
    glUniform1f( Size_Loc[1], static_cast<float>( gEngine->getCurrentYResolution() ) );
}

void updatePass4()
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gEngine->getCurrentDrawTexture() );
    glUniform1i( PostFX_Texture_Loc[3], 0 );
    glUniform1i( Tex2_Loc, 1 );
}

void setupPostFXs()
{
    sgct::PostFX fx[4];
    sgct::ShaderProgram * sp;

    fx[0].init("Threshold", "base.vert", "threshold.frag");
    fx[0].setUpdateUniformsFunction( updatePass1 );
    sp = fx[0].getShaderProgram();
    sp->bind();
        PostFX_Texture_Loc[0] = sp->getUniformLocation( "Tex" );
        Tex2_Loc = sp->getUniformLocation( "TexOrig" );
    sp->unbind();
    gEngine->addPostFX( fx[0] );

    fx[1].init("HBlur", "blur_h.vert", "blur.frag");
    fx[1].setUpdateUniformsFunction( updatePass2 );
    sp = fx[1].getShaderProgram();
    sp->bind();
        PostFX_Texture_Loc[1] = sp->getUniformLocation( "Tex" );
        Size_Loc[0] = sp->getUniformLocation( "Size" );
    sp->unbind();
    gEngine->addPostFX( fx[1] );

    fx[2].init("VBlur", "blur_v.vert", "blur.frag");
    fx[2].setUpdateUniformsFunction( updatePass3 );
    sp = fx[2].getShaderProgram();
    sp->bind();
        PostFX_Texture_Loc[2] = sp->getUniformLocation( "Tex" );
        Size_Loc[1] = sp->getUniformLocation( "Size" );
    sp->unbind();
    gEngine->addPostFX( fx[2] );

    fx[3].init("Glow", "base.vert", "glow.frag");
    fx[3].setUpdateUniformsFunction( updatePass4 );
    sp = fx[3].getShaderProgram();
    sp->bind();
        PostFX_Texture_Loc[3] = sp->getUniformLocation( "Tex" );
        Tex2_Loc = sp->getUniformLocation( "TexOrig" );
    sp->unbind();
    gEngine->addPostFX( fx[3] );

    //setup post FX only for first window
    if( gEngine->getNumberOfWindows() > 1 )
        gEngine->getWindowPtr(1)->setUsePostFX( false );
    gEngine->setNearAndFarClippingPlanes(0.1f, 50.0f);
}

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
    myBox->draw();

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

    myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
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

    setupPostFXs();
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
    if(myBox != NULL)
        delete myBox;
}
