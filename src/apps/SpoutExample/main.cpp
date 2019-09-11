//tell Spout to use GLEW
#define USE_GLEW
#include "spoutGLextensions.h" // include before spout.h to avoid gl before glew issue
#include "spout.h"

//avoid include conflicts between spout and sgct
#define SGCT_WINDOWS_INCLUDE
#define SGCT_WINSOCK_INCLUDE

#include <stdlib.h>
#include <stdio.h>
#include <sgct.h>

sgct::Engine * gEngine;

void drawFun();
void preSyncFun();
void initOGLFun();
void encodeFun();
void decodeFun();
void cleanUpFun();

bool bindSpout();

sgct_utils::SGCTBox * box = NULL;
//sgct_utils::SGCTPlane * myPlane = NULL;
GLint Matrix_Loc = -1;
GLint Flip_Loc = -1;

SpoutReceiver * spoutReceiver = NULL;
char spoutSenderName[256];
unsigned int spoutWidth;
unsigned int spoutHeight;
bool spoutInited = false;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);

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

bool bindSpout()
{
    if (!spoutInited && spoutReceiver->CreateReceiver(spoutSenderName, spoutWidth, spoutHeight))
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Spout: Initing %ux%u texture from '%s'.\n", spoutWidth, spoutHeight, spoutSenderName);
        spoutInited = true;
    }

    if (spoutInited)
    {
        if (spoutReceiver->ReceiveTexture(spoutSenderName, spoutWidth, spoutHeight))
        {
            //sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Spout: Receiving %ux%u texture from '%s'.\n", SpoutWidth, SpoutHeight, SpoutSenderName);
            return spoutReceiver->BindSharedTexture();
        }
        else
        {
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Spout disconnected.\n");

            spoutInited = false; //reset if disconnected
            spoutSenderName[0] = NULL;
            spoutReceiver->ReleaseReceiver();
        }
    }

    return false;
}

void drawFun()
{
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );

    double speed = 0.44;

    //create scene transform (animation)
    glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
    scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));
    scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * (speed/2.0) ), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;

    glActiveTexture(GL_TEXTURE0);

    //spout init
    bool spoutStatus = false;
    //check if spout supported (DX11 interop)
    if( glfwExtensionSupported("WGL_NV_DX_interop2"))
        spoutStatus = bindSpout();

    sgct::ShaderManager::instance()->bindShaderProgram("xform");

    //DirectX textures are flipped around the Y axis compared to OpenGL
    if (!spoutStatus)
    {
        glUniform1i(Flip_Loc, 0);
        glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box"));
    }
    else
        glUniform1i(Flip_Loc, 1);    

    glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);

    //draw the box
    box->draw();
    //myPlane->draw();

    sgct::ShaderManager::instance()->unBindShaderProgram();

    if (spoutStatus)
        spoutReceiver->UnBindSharedTexture();

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
}

void preSyncFun()
{
    if( gEngine->isMaster() )
    {
        curr_time.setVal( sgct::Engine::getTime() );
    }
}

void initOGLFun()
{
    //setup spout
    spoutSenderName[0] = NULL;
    spoutReceiver = new SpoutReceiver;    // Create a new Spout receiver
    
    //set background
    sgct::Engine::instance()->setClearColor(0.3f, 0.3f, 0.3f, 0.0f);
    
    sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
    sgct::TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    box = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
    //box = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::CubeMap);
    //box = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::SkyBox);

    //myPlane = new sgct_utils::SGCTPlane(2.0f, 2.0f);

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise

    sgct::ShaderManager::instance()->addShaderProgram( "xform",
            "xform.vert",
            "xform.frag" );

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
    GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
    Flip_Loc = sgct::ShaderManager::instance()->getShaderProgram("xform").getUniformLocation("flip");
    glUniform1i( Tex_Loc, 0 );
    glUniform1i( Flip_Loc, 0);

    sgct::ShaderManager::instance()->unBindShaderProgram();

    sgct::Engine::checkForOGLErrors();
}

void encodeFun()
{
    sgct::SharedData::instance()->writeDouble(&curr_time);
}

void decodeFun()
{
    sgct::SharedData::instance()->readDouble(&curr_time);
}

void cleanUpFun()
{
    if(box)
        delete box;

    //if (myPlane)
    //    delete myPlane;

    if (spoutReceiver)
    {
        spoutReceiver->ReleaseReceiver();
        delete spoutReceiver;
    }
}
