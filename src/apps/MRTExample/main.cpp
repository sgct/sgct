#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

sgct::Engine * gEngine;

//callbacks
void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
void keyCallback(int key, int action);

sgct_utils::SGCTBox * myBox = NULL;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool takeScreenshot(false);

//shader locs
int m_textureID = -1;
int m_worldMatrixTransposeID = -1;

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
    gEngine->setCleanUpFunction( myCleanUpFun );
    gEngine->setKeyboardCallbackFunction(keyCallback);

    //force normal & position textures to be created & used in rendering loop
    //sgct::SGCTSettings::instance()->setBufferFloatPrecision(sgct::SGCTSettings::Float_32Bit); //default is 16-bit
    //sgct::SGCTSettings::instance()->setUseDepthTexture(true);
    sgct::SGCTSettings::instance()->setUseNormalTexture(true);
    sgct::SGCTSettings::instance()->setUsePositionTexture(true);

    if( !gEngine->init() )
    {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
    sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

    // Main loop
    gEngine->render();

    // Clean up
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void myDrawFun()
{
    double speed = 25.0;
    
    glTranslatef(0.0f, 0.0f, -3.0f);
    glRotated(curr_time.getVal() * speed, 0.0, -1.0, 0.0);
    glRotated(curr_time.getVal() * (speed/2.0), 1.0, 0.0, 0.0);
    glColor3f(1.0f,1.0f,1.0f);

    float worldMatrix[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, worldMatrix);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box") );
    
    //set MRT shader program
    sgct::ShaderManager::instance()->bindShaderProgram("MRT");

    glUniform1i(m_textureID, 0);
    glUniformMatrix4fv(m_worldMatrixTransposeID, 1, GL_TRUE, worldMatrix);  //transpose in transfere

    //draw the box
    myBox->draw();

    //unset current shader program
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
    if (takeScreenshot.getVal())
    {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void myInitOGLFun()
{
    sgct::ShaderManager::instance()->addShaderProgram("MRT", "mrt.vert", "mrt.frag");
    sgct::ShaderManager::instance()->bindShaderProgram("MRT");

    m_textureID = sgct::ShaderManager::instance()->getShaderProgram("MRT").getUniformLocation("tDiffuse");
    m_worldMatrixTransposeID = sgct::ShaderManager::instance()->getShaderProgram("MRT").getUniformLocation("WorldMatrixTranspose");

    sgct::ShaderManager::instance()->unBindShaderProgram();
    
    sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
    sgct::TextureManager::instance()->loadTexure("box", "../SharedResources/box.png", true);

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

void myEncodeFun()
{
    sgct::SharedData::instance()->writeDouble(&curr_time);
    sgct::SharedData::instance()->writeBool(&takeScreenshot);
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readDouble(&curr_time);
    sgct::SharedData::instance()->readBool(&takeScreenshot);
}

void myCleanUpFun()
{
    if(myBox != NULL)
        delete myBox;
}

void keyCallback(int key, int action)
{
    if (gEngine->isMaster())
    {
        switch (key)
        {
        case SGCT_KEY_P:
        case SGCT_KEY_F10:
            if (action == SGCT_PRESS)
                takeScreenshot.setVal(true);
            break;
        }
    }
}
