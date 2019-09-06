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

//variables to share across cluster
sgct::SharedDouble currentTime(0.0);

//Post FX shader locations
GLint PostFX_Matrix_Loc[] = { -1, -1, -1, -1};
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

    if( gEngine->getNumberOfWindows() > 1 )
        gEngine->getWindowPtr(1)->setUsePostFX( false );
}

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( initOGLFun );
    gEngine->setDrawFunction( drawFun );
    gEngine->setPreSyncFunction( preSyncFun );
    gEngine->setCleanUpFunction( cleanUpFun );

    if( !gEngine->init() )
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
    double speed = 25.0;
    
    glTranslatef(0.0f, 0.0f, -3.0f);
    glRotated(currentTime.getVal() * speed, 0.0, -1.0, 0.0);
    glRotated(currentTime.getVal() * (speed/2.0), 1.0, 0.0, 0.0);
    glColor3f(1.0f,1.0f,1.0f);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box"));
    
    //draw the box
    box->draw();
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
    //myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::CubeMap);
    //myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::SkyBox);
    
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_COLOR_MATERIAL );
    glDisable( GL_LIGHTING );
    glEnable( GL_CULL_FACE );
    glEnable( GL_TEXTURE_2D );

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise

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
    if(box != NULL)
        delete box;
}
