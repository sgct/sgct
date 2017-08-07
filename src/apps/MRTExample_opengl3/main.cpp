#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"
#include <glm/gtc/matrix_inverse.hpp>

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
int m_MVPMatrixID = -1;
int m_worldMatrixTransposeID = -1;
int m_normalMatrix = -1;

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

    if (!gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ))
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
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    double speed = 0.44;

    //create scene transform (animation)
    glm::mat4 scene_mat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
    scene_mat = glm::rotate(scene_mat, static_cast<float>(curr_time.getVal() * speed), glm::vec3(0.0f, -1.0f, 0.0f));
    scene_mat = glm::rotate(scene_mat, static_cast<float>(curr_time.getVal() * (speed / 2.0)), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;
    glm::mat4 MV = gEngine->getCurrentModelViewMatrix() * scene_mat;
    glm::mat3 NormalMatrix = glm::inverseTranspose(glm::mat3(MV));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box") );

    sgct::ShaderManager::instance()->bindShaderProgram("MRT");

    glUniformMatrix4fv(m_MVPMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(m_worldMatrixTransposeID, 1, GL_TRUE, &MV[0][0]); //transpose in transfere
    glUniformMatrix3fv(m_normalMatrix, 1, GL_FALSE, &NormalMatrix[0][0]);
    glUniform1i(m_textureID, 0);

    //draw the box
    myBox->draw();

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
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
    m_MVPMatrixID = sgct::ShaderManager::instance()->getShaderProgram("MRT").getUniformLocation("MVPMatrix");
    m_normalMatrix = sgct::ShaderManager::instance()->getShaderProgram("MRT").getUniformLocation("NormalMatrix");

    //bind the multiple rendering target (MRT) variables to the shader 
    /*sgct::ShaderManager::instance()->getShaderProgram("MRT").bindFragDataLocation(0, "diffuse");
    sgct::ShaderManager::instance()->getShaderProgram("MRT").bindFragDataLocation(1, "normal");
    sgct::ShaderManager::instance()->getShaderProgram("MRT").bindFragDataLocation(2, "position");*/

    sgct::ShaderManager::instance()->unBindShaderProgram();
    
    sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
    sgct::TextureManager::instance()->loadTexure("box", "../SharedResources/box.png", true);

    //test
    int size_x, size_y, size_c;
    std::string path = sgct::TextureManager::instance()->getTexturePath("box");
    sgct::TextureManager::instance()->getDimensions("box", size_x, size_y, size_c);
    sgct::MessageHandler::instance()->print("Texture info, x=%d, y=%d, c=%d, path=%s\n", size_x, size_y, size_c, path.c_str());

    myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
    //myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::CubeMap);
    //myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::SkyBox);

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
