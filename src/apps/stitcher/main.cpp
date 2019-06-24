#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myPreWinInitFun();
void keyCallback(int key, int action);

enum rotation { ROT_0_DEG = 0, ROT_90_DEG, ROT_180_DEG, ROT_270_DEG };
enum sides { RIGHT_SIDE_L = 0, BOTTOM_SIDE_L, TOP_SIDE_L, LEFT_SIDE_L,
    RIGHT_SIDE_R, BOTTOM_SIDE_R, TOP_SIDE_R, LEFT_SIDE_R};

sides getSideIndex(size_t index);
void face(rotation rot);

std::string texturePaths[8];
GLuint textureIndices[] = {0, 0, 0, 0, 0, 0, 0, 0};
rotation sideRotations[] = { ROT_0_DEG, ROT_0_DEG, ROT_0_DEG, ROT_0_DEG };
size_t activeTexture = 0;
size_t numberOfTextures = 0;

int startIndex;
int stopIndex;
int numberOfDigits = 0;
int iterator;
bool sequence = false;
bool cubic = true;

int counter = 0;
int startFrame = 0;
bool alpha = false;
bool stereo = false;
bool fxaa = false;
int numberOfMSAASamples = 1;
int resolution = 512;
int cubemapRes = 256;
float eyeSeparation = 0.065f;
float domeDiameter = 14.8f;

//sgct_utils::SGCTDome * dome = NULL;

//variables to share across cluster
sgct::SharedBool takeScreenshot(false);

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    //parse arguments
    for( int i = 0; i < argc; i++ )
    {
        //fprintf(stderr, "Argument %d: %s (total %d)\n", i, argv[i], argc);

        if( strcmp(argv[i], "-tex") == 0 && argc > (i+1) )
        {
            std::string tmpStr(argv[i + 1]);
            //to lower case
            //std::transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);
            std::size_t found = tmpStr.find_last_of(".");
            if (found != std::string::npos && found > 0)
            {
                numberOfDigits = 0;
                for (std::size_t i = found - 1; i != 0; i--)
                {
                    //if not numerical
                    if (tmpStr[i] < '0' || tmpStr[i] > '9')
                    {
                        tmpStr = tmpStr.substr(0, i + 1);
                        break;
                    }
                    else
                        numberOfDigits++;

                }
            }

            texturePaths[getSideIndex(numberOfTextures)].assign(tmpStr);

            numberOfTextures++;
            sgct::MessageHandler::instance()->print("Adding texture: %s\n", argv[i+1]);
        }
        else if( strcmp(argv[i], "-seq") == 0 && argc > (i+2) )
        {
            sequence = true;
            startIndex        = atoi( argv[i+1] );
            stopIndex        = atoi( argv[i+2] );
            iterator = startIndex;
            sgct::MessageHandler::instance()->print("Loading sequence from %d to %d\n", startIndex, stopIndex);
        }
        else if( strcmp(argv[i], "-rot") == 0 && argc > (i+4) )
        {
            int tmpRots[] = { 0, 0, 0, 0 };
            tmpRots[0]        = atoi( argv[i+1] );
            tmpRots[1]        = atoi( argv[i+2] );
            tmpRots[2]        = atoi( argv[i+3] );
            tmpRots[3]        = atoi( argv[i+4] );
            sgct::MessageHandler::instance()->print("Setting image rotations to L: %d, R: %d, T: %d, B: %d\n",
                tmpRots[0], tmpRots[1], tmpRots[2], tmpRots[3]);

            for(size_t i=0; i<4; i++)
            {
                switch( tmpRots[i] )
                {
                case 0:
                default:
                    sideRotations[i] = ROT_0_DEG;
                    break;

                case 90:
                    sideRotations[i] = ROT_90_DEG;
                    break;

                case 180:
                    sideRotations[i] = ROT_180_DEG;
                    break;

                case 270:
                    sideRotations[i] = ROT_270_DEG;
                    break;
                }
            }
        }
        else if( strcmp(argv[i], "-start") == 0 && argc > (i+1) )
        {
            startFrame = atoi( argv[i+1] );
            sgct::MessageHandler::instance()->print("Start frame set to %d\n", startFrame);
        }
        else if (strcmp(argv[i], "-alpha") == 0 && argc > (i + 1))
        {
            alpha = (strcmp(argv[i + 1], "1") == 0);
            sgct::MessageHandler::instance()->print("Setting alpha to %s\n", alpha ? "true" : "false");
        }
        else if (strcmp(argv[i], "-stereo") == 0 && argc > (i + 1))
        {
            stereo = (strcmp(argv[i + 1], "1") == 0);
            sgct::MessageHandler::instance()->print("Setting stereo to %s\n", stereo ? "true" : "false");
        }
        else if (strcmp(argv[i], "-cubic") == 0 && argc > (i + 1))
        {
            cubic = (strcmp(argv[i + 1], "1") == 0);
            sgct::MessageHandler::instance()->print("Setting cubic interpolation to %s\n", cubic ? "true" : "false");
        }
        else if (strcmp(argv[i], "-fxaa") == 0 && argc > (i + 1))
        {
            fxaa = (strcmp(argv[i + 1], "1") == 0);
            sgct::MessageHandler::instance()->print("Setting fxaa to %s\n", alpha ? "true" : "false");
        }
        else if (strcmp(argv[i], "-eyeSep") == 0 && argc > (i + 1))
        {
            eyeSeparation = static_cast<float>( atof(argv[i + 1]) );
            sgct::MessageHandler::instance()->print("Setting eye separation to %f\n", eyeSeparation);
        }
        else if (strcmp(argv[i], "-diameter") == 0 && argc > (i + 1))
        {
            domeDiameter = static_cast<float>(atof(argv[i + 1]));
            sgct::MessageHandler::instance()->print("Setting dome diameter to %f\n", domeDiameter);
        }
        else if (strcmp(argv[i], "-msaa") == 0 && argc > (i + 1))
        {
            numberOfMSAASamples = atoi(argv[i + 1]);
            sgct::MessageHandler::instance()->print("Number of MSAA samples set to %d\n", numberOfMSAASamples);
        }
        else if (strcmp(argv[i], "-res") == 0 && argc > (i + 1))
        {
            resolution = atoi(argv[i + 1]);
            sgct::MessageHandler::instance()->print("Resolution set to %d\n", resolution);
        }
        else if (strcmp(argv[i], "-cubemap") == 0 && argc > (i + 1))
        {
            cubemapRes = atoi(argv[i + 1]);
            sgct::MessageHandler::instance()->print("Cubemap resolution set to %d\n", cubemapRes);
        }
        else if (strcmp(argv[i], "-format") == 0 && argc > (i + 1))
        {
            sgct::SGCTSettings::instance()->setCaptureFormat(argv[i + 1]);
            sgct::MessageHandler::instance()->print("Format set to %s\n", argv[i + 1]);
        }
        else if (strcmp(argv[i], "-leftPath") == 0 && argc > (i + 1))
        {
            sgct::SGCTSettings::instance()->setCapturePath( argv[i + 1], sgct::SGCTSettings::Mono );
            sgct::SGCTSettings::instance()->setCapturePath(argv[i + 1], sgct::SGCTSettings::LeftStereo);

            sgct::MessageHandler::instance()->print("Left path set to %s\n", argv[i + 1]);
        }
        else if (strcmp(argv[i], "-rightPath") == 0 && argc > (i + 1))
        {
            sgct::SGCTSettings::instance()->setCapturePath(argv[i + 1], sgct::SGCTSettings::RightStereo);

            sgct::MessageHandler::instance()->print("Right path set to %s\n", argv[i + 1]);
        }
        else if (strcmp(argv[i], "-compression") == 0 && argc > (i + 1))
        {
            int tmpi = atoi(argv[i + 1]);
            sgct::SGCTSettings::instance()->setPNGCompressionLevel(tmpi);

            sgct::MessageHandler::instance()->print("Compression set to %d\n", tmpi);
        }

    }

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );
    gEngine->setPreWindowFunction( myPreWinInitFun );

    gEngine->setClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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
    //if( dome != NULL )
    //    delete dome;
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void myDrawFun()
{
    size_t index = counter % numberOfTextures;

    if (textureIndices[index]) //if valid
    {
        //enter ortho mode
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glPushMatrix();
        glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);

        glColor3f(1.0f,1.0f,1.0f);
        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, textureIndices[index]);

        if( index == LEFT_SIDE_L || index == LEFT_SIDE_R )
            face( sideRotations[0] );
        else if( index == RIGHT_SIDE_L || index == RIGHT_SIDE_R )
            face( sideRotations[1] );
        else if( index == TOP_SIDE_L || index == TOP_SIDE_R )
            face( sideRotations[2] );
        else //bottom
            face( sideRotations[3] );

        glDisable(GL_TEXTURE_2D);

        glPopAttrib();

        //exit ortho mode
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }
    else //bad texture
    {
        //std::cout << "NoTex: Texture handle:" << textureHandles[index] << " ogl: " << sgct::TextureManager::instance()->getTextureByHandle(textureHandles[index]) <<
        //            " index: " << index << " counter: " << counter << std::endl;
    }

    counter++;
}

void myPreSyncFun()
{
    if(sequence && iterator <= stopIndex)
    {
        for( size_t i=0; i < numberOfTextures; i++)
        {
            char tmpStr[256];

            if (numberOfDigits == 0)
            {
                #if (_MSC_VER >= 1400)
                sprintf_s(tmpStr, 256, "%s.png",
                    texturePaths[i].c_str() );
                #else
                sprintf(tmpStr, "%s.png",
                    texturePaths[i].c_str() );
                #endif
            }
            else
            {
                char digitStr[16];
                char zeros[16];
                zeros[0] = '\0';

                #if (_MSC_VER >= 1400)
                sprintf_s(digitStr, 16, "%d", iterator);
                #else
                sprintf(digitStr, "%d", iterator);
                #endif


                size_t currentSize = strlen(digitStr);

                for (size_t j = 0; j < (numberOfDigits - currentSize); j++)
                {
                    zeros[j] = '0';
                    zeros[j + 1] = '\0';
                }

                #if (_MSC_VER >= 1400)
                sprintf_s(tmpStr, 256, "%s%s%d.png",
                    texturePaths[i].c_str(), zeros, iterator);
                #else
                sprintf(tmpStr, "%s%s%d.png",
                    texturePaths[i].c_str(), zeros, iterator);
                #endif
            }

            //load the texture
            if( !sgct::TextureManager::instance()->loadTexture(texturePaths[i], std::string(tmpStr), true, 1) )
            {
                std::cout << "Error: texture: " << textureIndices[i] <<
                    " file: " << std::string(tmpStr) << " count: " << numberOfTextures << std::endl;
            }
            else
                textureIndices[i] = sgct::TextureManager::instance()->getTextureId(texturePaths[i]);

        }

        //for( size_t i=0; i < numberOfTextures; i++)
        //    fprintf(stderr, "Handle: %u, index: %u\n", textureHandles[i], sgct::TextureManager::instance()->getTextureByHandle(textureHandles[i]));

        takeScreenshot.setVal( true );
        iterator++;
    }
    else if(sequence && iterator <= stopIndex && numberOfDigits == 0 )
    {
        for (size_t i = 0; i < numberOfTextures; i++)
        {
            if( sgct::TextureManager::instance()->loadTexture(texturePaths[i], texturePaths[i], true, 1) )
                textureIndices[i] = sgct::TextureManager::instance()->getTextureId(texturePaths[i]);
        }

        takeScreenshot.setVal( true );
        iterator++;
    }

    counter = 0;
}

void myPostSyncPreDrawFun()
{
    if( takeScreenshot.getVal() )
    {
        gEngine->takeScreenshot();
        takeScreenshot.setVal( false );
    }
}

void myPreWinInitFun()
{
    gEngine->getDefaultUserPtr()->setEyeSeparation(eyeSeparation);
    for (std::size_t i = 0; i < gEngine->getNumberOfWindows(); i++)
    {
        gEngine->setScreenShotNumber(startFrame);
        gEngine->getWindowPtr(i)->setAlpha(alpha);

        for (std::size_t j = 0; j < gEngine->getWindowPtr(i)->getNumberOfViewports(); j++)
            if (gEngine->getWindowPtr(i)->getViewport(j)->hasSubViewports())
            {
                gEngine->getWindowPtr(i)->getViewport(j)->getNonLinearProjectionPtr()->setClearColor(glm::vec4(0.0, 0.0, 0.0, 1.0));
                gEngine->getWindowPtr(i)->getViewport(j)->getNonLinearProjectionPtr()->setCubemapResolution(cubemapRes);
                gEngine->getWindowPtr(i)->getViewport(j)->getNonLinearProjectionPtr()->setInterpolationMode(sgct_core::NonLinearProjection::Cubic);

                if (sgct_core::FisheyeProjection * fp = dynamic_cast<sgct_core::FisheyeProjection*>(gEngine->getWindowPtr(i)->getViewport(j)->getNonLinearProjectionPtr()))
                    fp->setDomeDiameter(domeDiameter);
            }
        
        gEngine->getWindowPtr(i)->setNumberOfAASamples(numberOfMSAASamples);
        gEngine->getWindowPtr(i)->setFramebufferResolution(resolution, resolution);
        gEngine->getWindowPtr(i)->setUseFXAA(fxaa);
        if (stereo)
            gEngine->getWindowPtr(i)->setStereoMode(sgct::SGCTWindow::Dummy_Stereo);
        else
            gEngine->getWindowPtr(i)->setStereoMode(sgct::SGCTWindow::No_Stereo);
    }
}

void myInitOGLFun()
{
    sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::No_Compression);
    sgct::TextureManager::instance()->setOverWriteMode(true);
    //sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);

    //load all textures
    if(!sequence)
    {
        for( size_t i=0; i<numberOfTextures; i++)
        if (sgct::TextureManager::instance()->loadTexture(texturePaths[i], texturePaths[i], true, 1))
            textureIndices[i] = sgct::TextureManager::instance()->getTextureId(texturePaths[i]);
    }

    //dome = new sgct_utils::SGCTDome(7.4f, 180.0f, 360, 90);

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_COLOR_MATERIAL );
    glDisable( GL_LIGHTING );
    glEnable( GL_BLEND );
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void myEncodeFun()
{
    sgct::SharedData::instance()->writeBool( &takeScreenshot );
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readBool( &takeScreenshot );
}

void keyCallback(int key, int action)
{
    if( gEngine->isMaster() )
    {
        switch( key )
        {
        case 'P':
        case SGCT_KEY_F10:
            if(action == SGCT_PRESS)
                takeScreenshot.setVal(true);
            break;
        }
    }
}

sides getSideIndex(size_t index)
{
    sides tmpSide;

    switch(index)
    {
    case 0:
    default:
        tmpSide = LEFT_SIDE_L;
        break;

    case 1:
        tmpSide = RIGHT_SIDE_L;
        break;

    case 2:
        tmpSide = TOP_SIDE_L;
        break;

    case 3:
        tmpSide = BOTTOM_SIDE_L;
        break;

    case 4:
        tmpSide = LEFT_SIDE_R;
        break;

    case 5:
        tmpSide = RIGHT_SIDE_R;
        break;

    case 6:
        tmpSide = TOP_SIDE_R;
        break;

    case 7:
        tmpSide = BOTTOM_SIDE_R;
        break;
    }

    return tmpSide;
}

void face(rotation rot)
{
    switch(rot)
    {
    case ROT_0_DEG:
    default:
        {
            glBegin(GL_QUADS);
            glTexCoord2d(0.0, 0.0);
            glVertex2d(0.0, 0.0);

            glTexCoord2d(0.0, 1.0);
            glVertex2d(0.0, 1.0);

            glTexCoord2d(1.0, 1.0);
            glVertex2d(1.0, 1.0);

            glTexCoord2d(1.0, 0.0);
            glVertex2d(1.0, 0.0);
            glEnd();
        }
        break;

    case ROT_90_DEG:
        {
            glBegin(GL_QUADS);
            glTexCoord2d(1.0, 0.0);
            glVertex2d(0.0, 0.0);

            glTexCoord2d(0.0, 0.0);
            glVertex2d(0.0, 1.0);

            glTexCoord2d(0.0, 1.0);
            glVertex2d(1.0, 1.0);

            glTexCoord2d(1.0, 1.0);
            glVertex2d(1.0, 0.0);
            glEnd();
        }
        break;

    case ROT_180_DEG:
        {
            glBegin(GL_QUADS);
            glTexCoord2d(1.0, 1.0);
            glVertex2d(0.0, 0.0);

            glTexCoord2d(1.0, 0.0);
            glVertex2d(0.0, 1.0);

            glTexCoord2d(0.0, 0.0);
            glVertex2d(1.0, 1.0);

            glTexCoord2d(0.0, 1.0);
            glVertex2d(1.0, 0.0);
            glEnd();
        }
        break;

    case ROT_270_DEG:
        {
            glBegin(GL_QUADS);
            glTexCoord2d(0.0, 1.0);
            glVertex2d(0.0, 0.0);

            glTexCoord2d(1.0, 1.0);
            glVertex2d(0.0, 1.0);

            glTexCoord2d(1.0, 0.0);
            glVertex2d(1.0, 1.0);

            glTexCoord2d(0.0, 0.0);
            glVertex2d(1.0, 0.0);
            glEnd();
        }
        break;
    }
}
