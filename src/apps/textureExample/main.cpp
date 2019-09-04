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

//variables to share across cluster
sgct::SharedDouble currentTime(0.0);

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
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box") );

    //draw the box
    myBox->draw();
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
