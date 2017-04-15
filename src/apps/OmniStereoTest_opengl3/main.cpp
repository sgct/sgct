#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPostSyncPreDrawFun();
void myPreSyncFun();
void myPostDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

void renderBoxes(glm::mat4 transform);
void renderGrid(glm::mat4 transform);
void initOmniStereo(float diameter, float tilt, bool mask);
void drawOmniStereo();

glm::mat4 levels[3];
sgct_utils::SGCTBox * myBox = NULL;
sgct_utils::SGCTDomeGrid * myGrid = NULL;
GLint Matrix_Loc = -1;
GLint Grid_Matrix_Loc = -1;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool takeScreenshot(true);

//omni var
struct omniData
{
    glm::mat4 mViewProjectionMatrix[3];
    bool enabled;
};
omniData ** omniProjections;
bool omniInited = false;

//Parameters to control omni rendering
bool maskOutSimilarities = false;
int tileSize = 2;
float domeDiameter = 14.8f;
float domeTilt = 30.0f;

std::string turnMapSrc;
std::string sepMapSrc;

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    for (int n = 0; n < argc; n++) {
        //printf("%s\n", argv[n]);

        if (strcmp(argv[n], "-turnmap") == 0 && argc > n + 1)
        {
            turnMapSrc.assign(std::string(argv[n + 1]));
            sgct::MessageHandler::instance()->print("Setting turn map path to '%s'\n", turnMapSrc.c_str());
        }
        if (strcmp(argv[n], "-sepmap") == 0 && argc > n + 1)
        {
            sepMapSrc.assign(std::string(argv[n + 1]));
            sgct::MessageHandler::instance()->print("Setting separation map path to '%s'\n", sepMapSrc.c_str());
        }
    }

    sgct::SGCTSettings::instance()->setSwapInterval(0);

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
    gEngine->setPostDrawFunction( myPostDrawFun );
    gEngine->setCleanUpFunction( myCleanUpFun );

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

    if (gEngine->getCurrentWindowIndex() == 1)
    {
        drawOmniStereo();
    }
    else
    {
        glm::mat4 VP = gEngine->getCurrentViewProjectionMatrix();
        glm::mat4 M = gEngine->getModelMatrix();

        sgct::ShaderManager::instance()->bindShaderProgram("grid");
        renderGrid(VP);
        //renderGrid(VP * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

        sgct::ShaderManager::instance()->bindShaderProgram("xform");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box"));
        renderBoxes(VP * M);
    }

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
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
        takeScreenshot.setVal(false);//take only one screenshot
    }
}

void myPostDrawFun()
{
    //render a single frame and exit
    gEngine->terminate();
}

void myInitOGLFun()
{
    sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::No_Compression);
    sgct::TextureManager::instance()->loadTexure("box", "../SharedResources/box.png", true);

    myBox = new sgct_utils::SGCTBox(0.5f, sgct_utils::SGCTBox::Regular);
    myGrid = new sgct_utils::SGCTDomeGrid(domeDiameter/2.0f, 180.0f, 64, 32, 256);

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise

    sgct::ShaderManager::instance()->addShaderProgram("grid",
        "grid.vert",
        "grid.frag");
    sgct::ShaderManager::instance()->bindShaderProgram("grid");
    Grid_Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram("grid").getUniformLocation("MVP");

    sgct::ShaderManager::instance()->addShaderProgram( "xform",
            "base.vert",
            "base.frag" );
    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );
    Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
    GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
    glUniform1i( Tex_Loc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();

    //create scene transform
    levels[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -3.0f));
    levels[1] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -2.75f));
    levels[2] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.5f, -1.25f));

    initOmniStereo(domeDiameter, domeTilt, maskOutSimilarities);
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
    if (myGrid != NULL)
        delete myGrid;
}

void renderBoxes(glm::mat4 transform)
{
    glm::mat4 boxTrans;
    for (unsigned int l = 0; l < 3; l++)
        for (float a = 0.0f; a < 360.0f; a += (15.0f * static_cast<float>(l + 1)))
        {
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(a), glm::vec3(0.0f, 1.0f, 0.0f));

            boxTrans = transform * rot * levels[l];
            glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &boxTrans[0][0]);

            //draw the box
            myBox->draw();
        }
}

void renderGrid(glm::mat4 transform)
{
    glUniformMatrix4fv(Grid_Matrix_Loc, 1, GL_FALSE, &transform[0][0]);
    myGrid->draw();
}

void initOmniStereo(float diameter, float tilt, bool mask)
{
    double t0 = gEngine->getTime();

    if (gEngine->getNumberOfWindows() < 2)
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to allocate omni stereo in secondary window!\n");
        return;
    }

    sgct_core::Image turnMap;
    if (!(!turnMapSrc.empty() && turnMap.load(turnMapSrc)))
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Failed to load turn map!\n");
    }

    sgct_core::Image sepMap;
    if (!(!sepMapSrc.empty() && sepMap.load(sepMapSrc)))
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Failed to load separation map!\n");
    }

    sgct::SGCTWindow * winPtr = gEngine->getWindowPtr(1);

    int xRes = winPtr->getXFramebufferResolution() / tileSize;
    int yRes = winPtr->getYFramebufferResolution() / tileSize;

    sgct::MessageHandler::instance()->print("Allocating: %d MB data\n", (sizeof(omniData)*xRes*yRes) / (1024 * 1024));
    omniProjections = new omniData*[xRes];
    for (int i = 0; i < xRes; i++)
        omniProjections[i] = new omniData[yRes];

    sgct_core::Frustum::FrustumMode fm;

    float xResf = static_cast<float>(xRes);
    float yResf = static_cast<float>(yRes);

    float fovInDegrees = 180.0f;
    float halfFov = glm::radians<float>(fovInDegrees / 2.0f);
    float radius = diameter/2.0f;

    sgct_core::SGCTProjection proj;
    sgct_core::SGCTProjectionPlane projPlane;

    glm::vec3 p[3];
    glm::vec3 convergencePos;
    float s;
    float t;

    float near_clipping_plane = gEngine->getNearClippingPlane();
    float far_clipping_plane = gEngine->getFarClippingPlane();

    glm::vec2 tileCorners[3];
    tileCorners[0] = glm::vec2( 0.0f, 0.0f ); //lower left
    tileCorners[1] = glm::vec2( 0.0f, 1.0f ); //upper left
    tileCorners[2] = glm::vec2( 1.0f, 1.0f ); //upper right*/
    //if any precition problems
    /*
    tileCorners[0] = glm::vec2(-0.00001f, -0.00001f); //lower left
    tileCorners[1] = glm::vec2(-0.00001f, 1.00001f); //upper left
    tileCorners[2] = glm::vec2(1.00001f, 1.00001f); //upper right*/

    glm::mat3 rotMat = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
    glm::mat3 tiltEyeMat = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(tilt), glm::vec3(1.0f, 0.0f, 0.0f)));
    glm::mat3 rotEyeMat;

    glm::vec3 eyePos[3]; //mono, left stereo and right stereo
    glm::vec3 newEyePos;
    eyePos[sgct_core::Frustum::MonoEye] = glm::vec3(0.0f, 0.0f, 0.0f);
    eyePos[sgct_core::Frustum::StereoLeftEye] = glm::vec3(-gEngine->getDefaultUserPtr()->getHalfEyeSeparation(), 0.0f, 0.0f);
    eyePos[sgct_core::Frustum::StereoRightEye] = glm::vec3(gEngine->getDefaultUserPtr()->getHalfEyeSeparation(), 0.0f, 0.0f);
    glm::vec3 rotatedEyePos;
    glm::vec3 tiltedEyePos;
    glm::vec3 norm_pos;

    float eyeRot;
    float r2, phi, theta;

    glm::vec2 turnMapPos;
    glm::vec2 sepMapPos;

    int VPCounter = 0;

    for (int eye = 0; eye <= 2; eye++)
    {
        switch (eye)
        {
        case 0:
        default:
            fm = sgct_core::Frustum::MonoEye;
            break;

        case 1:
            fm = sgct_core::Frustum::StereoLeftEye;
            break;

        case 2:
            fm = sgct_core::Frustum::StereoRightEye;
            break;
        }

        for (int y = 0; y<yRes; y++)
            for (int x = 0; x<xRes; x++)
            {
                //scale to [-1, 1)
                //Center of each pixel
                s = ((static_cast<float>(x) + 0.5f) / xResf - 0.5f) * 2.0f;
                t = ((static_cast<float>(y) + 0.5f) / yResf - 0.5f) * 2.0f;
                r2 = s*s + t*t;

                phi = sqrtf(r2) * halfFov;
                theta = atan2f(s, -t); //get angle as azimuth

                norm_pos.x = sinf(phi) * sinf(theta);
                norm_pos.y = -sinf(phi) * cosf(theta);
                norm_pos.z = cosf(phi);

                float tmpY = norm_pos.y*cosf(glm::radians<float>(tilt)) - norm_pos.z*sinf(glm::radians<float>(tilt));
                eyeRot = atan2f(norm_pos.x, -tmpY);

                //get corresponding map positions
                bool omni_needed = true;
                if (turnMap.getChannels() > 0)
                {
                    turnMapPos.x = (static_cast<float>(x) / xResf)
                        * static_cast<float>(turnMap.getWidth() - 1);
                    turnMapPos.y = (static_cast<float>(y) / yResf)
                        * static_cast<float>(turnMap.getHeight() - 1);

                    //float head_turn_multiplier = (turnMap.getInterpolatedSampleAt( turnMapPos.x, turnMapPos.y, sgct_core::Image::Blue) / 255.0f);

                    //inverse gamma
                    float head_turn_multiplier = powf(turnMap.getInterpolatedSampleAt(turnMapPos.x, turnMapPos.y, sgct_core::Image::Blue) / 255.0f, 2.2f);

                    if (head_turn_multiplier == 0.0f)
                        omni_needed = false;

                    eyeRot *= head_turn_multiplier;
                }

                if (sepMap.getChannels() > 0)
                {
                    sepMapPos.x = (static_cast<float>(x) / xResf)
                        * static_cast<float>(sepMap.getWidth() - 1);
                    sepMapPos.y = (static_cast<float>(y) / yResf)
                        * static_cast<float>(sepMap.getHeight() - 1);

                    //gamma correction 2.2
                    //float separation_multiplier = powf(sepMap.getInterpolatedSampleAt( sepMapPos.x, sepMapPos.y, sgct_core::Image::Blue) /255.0f, 1.0f/2.2f);

                    //inverse gamma 2.2
                    float separation_multiplier = powf(sepMap.getInterpolatedSampleAt(sepMapPos.x, sepMapPos.y, sgct_core::Image::Blue) / 255.0f, 2.2f);

                    if (separation_multiplier == 0.0f)
                        omni_needed = false;

                    //get values at positions
                    newEyePos = eyePos[fm] * separation_multiplier;
                }
                else
                    newEyePos = eyePos[fm];

                /*-----------------------------------------------------------
                // IF VALID
                ----------------------------------------------------------------*/
                if (r2 <= 1.1f && (omni_needed || !mask))
                {
                    //loop tru corner points
                    for (int i = 0; i<3; i++)
                    {
                        //scale to [-1, 1)
                        s = ((static_cast<float>(x) + tileCorners[i].x) / xResf - 0.5f) * 2.0f;
                        t = ((static_cast<float>(y) + tileCorners[i].y) / yResf - 0.5f) * 2.0f;

                        r2 = s*s + t*t;
                        phi = sqrtf(r2) * halfFov; //zenith - elevation (0 degrees in zenith, 90 degrees at the rim/springline)
                        theta = atan2f(s, t); //azimuth (0 degrees at back of dome and 180 degrees at front)

                        p[i].x = radius * sinf(phi) * sinf(theta);
                        p[i].y = radius * -sinf(phi) * cosf(theta);
                        p[i].z = radius * cosf(phi);

                        convergencePos = rotMat * p[i];

                        projPlane.setCoordinate(i, convergencePos);
                    }

                    rotEyeMat = glm::mat3(glm::rotate(glm::mat4(1.0f), eyeRot, glm::vec3(0.0f, -1.0f, 0.0f)));
                    rotatedEyePos = rotEyeMat * newEyePos;

                    //tilt
                    tiltedEyePos = tiltEyeMat * rotatedEyePos;
                    
                    //calc projection
                    proj.calculateProjection(tiltedEyePos, &projPlane, near_clipping_plane, far_clipping_plane);

                    omniProjections[x][y].enabled = true;
                    omniProjections[x][y].mViewProjectionMatrix[fm] = proj.getViewProjectionMatrix();
                    VPCounter++;
                }
                else
                    omniProjections[x][y].enabled = false;
            }
    }

    int percentage = (100 * VPCounter) / (xRes * yRes * 3);
    sgct::MessageHandler::instance()->print("Time to init viewports: %f s\n%d %% will be rendered.\n", gEngine->getTime() - t0, percentage);
    omniInited = true;
}

void drawOmniStereo()
{
    if (omniInited)
    {
        double t0 = gEngine->getTime();

        sgct::SGCTWindow * winPtr = gEngine->getWindowPtr(1);
        int xRes = winPtr->getXFramebufferResolution() / tileSize;
        int yRes = winPtr->getYFramebufferResolution() / tileSize;

        sgct::ShaderManager::instance()->bindShaderProgram("xform");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box"));


        for (int x = 0; x < xRes; x++)
            for (int y = 0; y < yRes; y++)
            {
                if (omniProjections[x][y].enabled)
                {
                    glViewport(x * tileSize, y * tileSize, tileSize, tileSize);
                    glm::mat4 VP = omniProjections[x][y].mViewProjectionMatrix[gEngine->getCurrentFrustumMode()];
                    
                    renderBoxes(VP * gEngine->getModelMatrix());
                }
            }

        sgct::ShaderManager::instance()->bindShaderProgram("grid");
        for (int x = 0; x < xRes; x++)
            for (int y = 0; y < yRes; y++)
            {
                if (omniProjections[x][y].enabled)
                {
                    glViewport(x * tileSize, y * tileSize, tileSize, tileSize);
                    glm::mat4 VP = omniProjections[x][y].mViewProjectionMatrix[gEngine->getCurrentFrustumMode()];

                    renderGrid(VP);
                }
            }

        sgct::MessageHandler::instance()->print("Time to draw frame: %f s\n", gEngine->getTime() - t0);
    }
}