//tell Spout to use GLEW
#define USE_GLEW
#include "spoutGLextensions.h" // include before spout.h to avoid gl before glew issue
#include "spout.h"

//avoid include conflicts between spout and sgct
#define SGCT_WINDOWS_INCLUDE
#define SGCT_WINSOCK_INCLUDE

#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include "sgct.h"


sgct::Engine * gEngine;

void myPreWindowInitFun();
void myDrawFun();
void myPostDrawFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

sgct_utils::SGCTBox * myBox = NULL;
//sgct_utils::SGCTPlane * myPlane = NULL;
GLint Matrix_Loc = -1;

struct SpoutData
{
    SpoutSender * spoutSender;
    char spoutSenderName[256];
    bool spoutInited;
};

SpoutData * spoutSendersData = NULL;
std::size_t spoutSendersCount = 0;
std::vector<std::pair<std::size_t, bool>> windowData; //index and if lefteye
std::vector<std::string> senderNames;


//variables to share across cluster
sgct::SharedDouble curr_time(0.0);

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPostDrawFunction( myPostDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setCleanUpFunction( myCleanUpFun );
    gEngine->setPreWindowFunction( myPreWindowInitFun );

    if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
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
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );

    double speed = 0.44;

    //create scene transform (animation)
    glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
    scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));
    scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * (speed/2.0) ), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;

    glActiveTexture(GL_TEXTURE0);

    sgct::ShaderManager::instance()->bindShaderProgram("xform");

    glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box"));

    glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);

    //draw the box
    myBox->draw();
    //myPlane->draw();

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
}

void myPostDrawFun()
{
    glActiveTexture(GL_TEXTURE0);
    
    for (std::size_t i = 0; i < spoutSendersCount; i++)
    {
        if (spoutSendersData[i].spoutInited)
        {
            std::size_t winIndex = windowData[i].first;

            GLuint texId;
            if (windowData[i].second)
                texId = gEngine->getWindowPtr(winIndex)->getFrameBufferTexture(sgct::Engine::LeftEye);
            else
                texId = gEngine->getWindowPtr(winIndex)->getFrameBufferTexture(sgct::Engine::RightEye);
            
            glBindTexture(GL_TEXTURE_2D, texId);
            
            //sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Spout disconnected.\n");
            spoutSendersData[i].spoutSender->SendTexture(
                texId,
                GL_TEXTURE_2D,
                gEngine->getWindowPtr(winIndex)->getXFramebufferResolution(),
                gEngine->getWindowPtr(winIndex)->getYFramebufferResolution());
        }
    }

    glBindTexture(GL_TEXTURE_2D, GL_FALSE);
}

void myPreSyncFun()
{
    if( gEngine->isMaster() )
    {
        curr_time.setVal( sgct::Engine::getTime() );
    }
}

void myPreWindowInitFun()
{
    std::string baseName = "SGCT_Window";
    std::stringstream ss;

    //get number of framebuffer textures
    sgct::SGCTWindow::StereoMode sm;
    for (std::size_t i = 0; i < gEngine->getNumberOfWindows(); i++)
    {
        sm = gEngine->getWindowPtr(i)->getStereoMode();

//#if SGCT_VERSION_MAJOR >= 2 && SGCT_VERSION_MINOR >= 6
// enable rendering while window is closed / hidden
// however other methods to exit application needs to be implemented
// external tcp gui may be used to control this behaviour and exit the application in a clean way 
//        gEngine->getWindowPtr(i)->setRenderWhileHidden(true); //render even if closed
//#endif
        gEngine->getWindowPtr(i)->setFixResolution(true); //do not resize buffers while minimized
        //gEngine->getWindowPtr(i)->setVisibility(false); //hide window

        //check if stereo
        if (sm != sgct::SGCTWindow::No_Stereo && sm < sgct::SGCTWindow::Side_By_Side_Stereo)
        {
            ss.str(std::string()); //clear string stream
            ss << baseName << i << "_Left";
            senderNames.push_back(ss.str());
            windowData.push_back(std::pair<std::size_t, bool>(i, true));

            ss.str(std::string()); //clear string stream
            ss << baseName << i << "_Right";
            senderNames.push_back(ss.str());
            windowData.push_back(std::pair<std::size_t, bool>(i, false));
        }
        else
        {
            ss.str(std::string()); //clear string stream
            ss << baseName << i;
            senderNames.push_back(ss.str());
            windowData.push_back(std::pair<std::size_t, bool>(i, true));
        }
    }

    spoutSendersCount = senderNames.size();
}

void myInitOGLFun()
{
    //setup spout
    spoutSendersData = new SpoutData[spoutSendersCount];    // Create a new SpoutData for every SGCT window
    for (std::size_t i = 0; i < spoutSendersCount; i++)
    {
        spoutSendersData[i].spoutSender = new SpoutSender();
        spoutSendersData[i].spoutInited = false;

        strcpy_s(spoutSendersData[i].spoutSenderName, senderNames[i].c_str());
        std::size_t winIndex = windowData[i].first;
        
        if( spoutSendersData[i].spoutSender->CreateSender(
            spoutSendersData[i].spoutSenderName,
            gEngine->getWindowPtr(winIndex)->getXFramebufferResolution(),
            gEngine->getWindowPtr(winIndex)->getYFramebufferResolution()
            ) )
            spoutSendersData[i].spoutInited = true;
    }
    
    //set background
    sgct::Engine::instance()->setClearColor(0.3f, 0.3f, 0.3f, 0.0f);
    
    sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
    sgct::TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
    //myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::CubeMap);
    //myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::SkyBox);

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
    glUniform1i( Tex_Loc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();

    sgct::Engine::checkForOGLErrors();
}

void myEncodeFun()
{
    sgct::SharedData::instance()->writeDouble(&curr_time);
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readDouble(&curr_time);
}

void myCleanUpFun()
{
    if(myBox)
        delete myBox;

    //if (myPlane)
    //    delete myPlane;

    if (spoutSendersData)
    {
        for (std::size_t i = 0; i < spoutSendersCount; i++)
        {
            spoutSendersData[i].spoutSender->ReleaseSender();
            delete spoutSendersData[i].spoutSender;
        }

        delete[] spoutSendersData;
        spoutSendersData = NULL;
    }
}
