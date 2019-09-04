#include <stdlib.h>
#include <stdio.h>

#include "sgct.h"
#include <glm/gtc/matrix_inverse.hpp>

#define GRID_SIZE 256

sgct::Engine * gEngine;

void drawFun();
void preSyncFun();
void postSyncPreDrawFun();
void initOGLFun();
void encodeFun();
void decodeFun();
void cleanUpFun();

void keyCallback(int key, int action);
void generateTerrainGrid( float width, float height, unsigned int wRes, unsigned int dRes );

//shader data
sgct::ShaderProgram mSp;
GLint myTextureLocations[]    = { -1, -1 };
GLint currTimeLoc            = -1;
GLint MVP_Loc                = -1;
GLint MV_Loc                = -1;
GLint MVLight_Loc            = -1;
GLint NM_Loc                = -1;
GLint lightPos_Loc            = -1;
GLint lightAmb_Loc            = -1;
GLint lightDif_Loc            = -1;
GLint lightSpe_Loc            = -1;

//opengl objects
GLuint vertexArray = GL_FALSE;
GLuint vertexPositionBuffer = GL_FALSE;
GLuint texCoordBuffer = GL_FALSE;

//light data
glm::vec3 lightPosition( -2.0f, 5.0f, 5.0f );
glm::vec4 lightAmbient( 0.1f, 0.1f, 0.1f, 1.0f );
glm::vec4 lightDiffuse( 0.8f, 0.8f, 0.8f, 1.0f );
glm::vec4 lightSpecular( 1.0f, 1.0f, 1.0f, 1.0f );

bool mPause = false;

//variables to share across cluster
sgct::SharedDouble currentTime(0.0);
sgct::SharedBool wireframe(false);
sgct::SharedBool info(false);
sgct::SharedBool stats(false);
sgct::SharedBool takeScreenshot(false);
sgct::SharedBool useTracking(false);
sgct::SharedInt32 stereoMode(0);

//geometry
std::vector<float> mVertPos;
std::vector<float> mTexCoord;
GLsizei mNumberOfVerts = 0;

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( initOGLFun );
    gEngine->setDrawFunction( drawFun );
    gEngine->setPreSyncFunction( preSyncFun );
    gEngine->setPostSyncPreDrawFunction( postSyncPreDrawFun );
    gEngine->setCleanUpFunction( cleanUpFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );

    if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
    {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction( encodeFun );
    sgct::SharedData::instance()->setDecodeFunction( decodeFun );

    // Main loop
    gEngine->render();

    // Clean up
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void drawFun()
{    
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glLineWidth(1.0); //for wireframe

    double speed = 0.14;

    //create scene transform (animation)
    glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, -0.15f, 2.5f ) );
    scene_mat = glm::rotate( scene_mat, static_cast<float>( currentTime.getVal() * speed ), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 MVP        = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;
    glm::mat4 MV        = gEngine->getCurrentModelViewMatrix() * scene_mat;
    glm::mat4 MV_light    = gEngine->getCurrentModelViewMatrix();
    glm::mat3 NM        = glm::inverseTranspose( glm::mat3( MV ) );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId( "heightmap" ));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId( "normalmap" ));

    mSp.bind();

    glUniformMatrix4fv(MVP_Loc,        1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(MV_Loc,        1, GL_FALSE, &MV[0][0]);
    glUniformMatrix4fv(MVLight_Loc, 1, GL_FALSE, &MV_light[0][0]);
    glUniformMatrix3fv(NM_Loc,        1, GL_FALSE, &NM[0][0]);
    glUniform1f( currTimeLoc, static_cast<float>( currentTime.getVal() ) );

    glBindVertexArray(vertexArray);

    // Draw the triangles !
    for (unsigned int i = 0; i < GRID_SIZE; i++)
        glDrawArrays(GL_TRIANGLE_STRIP, i * GRID_SIZE * 2, GRID_SIZE * 2);

    //unbind
    glBindVertexArray(0);

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

void preSyncFun()
{
    if( gEngine->isMaster() && !mPause)
    {
        currentTime.setVal( currentTime.getVal() + gEngine->getAvgDt());
    }
}

void postSyncPreDrawFun()
{
    gEngine->setWireframe(wireframe.getVal());
    gEngine->setDisplayInfoVisibility(info.getVal());
    gEngine->setStatsGraphVisibility(stats.getVal());
    sgct_core::ClusterManager::instance()->getTrackingManagerPtr()->setEnabled( useTracking.getVal() );

    if( takeScreenshot.getVal() )
    {
        gEngine->takeScreenshot();
        takeScreenshot.setVal(false);
    }
}

void initOGLFun()
{
    stereoMode.setVal( gEngine->getWindowPtr(0)->getStereoMode() );

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise
    glDepthFunc(GL_LESS);

    //setup textures
    sgct::TextureManager::instance()->loadTexture("heightmap", "../SharedResources/heightmap.png", true, 0);
    sgct::TextureManager::instance()->loadTexture("normalmap", "../SharedResources/normalmap.png", true, 0);

    //setup shader
    sgct::ShaderManager::instance()->addShaderProgram( mSp, "Heightmap", "heightmap.vert", "heightmap.frag" );

    mSp.bind();
    myTextureLocations[0]    = mSp.getUniformLocation( "hTex" );
    myTextureLocations[1]    = mSp.getUniformLocation( "nTex" );
    currTimeLoc            = mSp.getUniformLocation( "curr_time" );
    MVP_Loc                    = mSp.getUniformLocation( "MVP" );
    MV_Loc                    = mSp.getUniformLocation( "MV" );
    MVLight_Loc                = mSp.getUniformLocation( "MV_light" );
    NM_Loc                    = mSp.getUniformLocation( "normalMatrix" );
    lightPos_Loc            = mSp.getUniformLocation( "lightPos" );
    lightAmb_Loc            = mSp.getUniformLocation( "light_ambient" );
    lightDif_Loc            = mSp.getUniformLocation( "light_diffuse" );
    lightSpe_Loc            = mSp.getUniformLocation( "light_specular" );
    glUniform1i( myTextureLocations[0], 0 );
    glUniform1i( myTextureLocations[1], 1 );
    glUniform4f( lightPos_Loc, lightPosition.x, lightPosition.y, lightPosition.z, 1.0f );
    glUniform4f( lightAmb_Loc, lightAmbient.r, lightAmbient.g, lightAmbient.b, lightAmbient.a );
    glUniform4f( lightDif_Loc, lightDiffuse.r, lightDiffuse.g, lightDiffuse.b, lightDiffuse.a );
    glUniform4f( lightSpe_Loc, lightSpecular.r, lightSpecular.g, lightSpecular.b, lightSpecular.a );
    sgct::ShaderManager::instance()->unBindShaderProgram();

    //generate mesh
    generateTerrainGrid(1.0f, 1.0f, GRID_SIZE, GRID_SIZE);

    //generate vertex array
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    //generate vertex position buffer
    glGenBuffers(1, &vertexPositionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mVertPos.size(), &mVertPos[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        reinterpret_cast<void*>(0) // array buffer offset
    );

    //generate texture coord buffer
    glGenBuffers(1, &texCoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mTexCoord.size(), &mTexCoord[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        reinterpret_cast<void*>(0) // array buffer offset
    );

    glBindVertexArray(0); //unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //cleanup
    mVertPos.clear();
    mTexCoord.clear();
}

void encodeFun()
{
    sgct::SharedData::instance()->writeDouble( &currentTime );
    sgct::SharedData::instance()->writeBool( &wireframe );
    sgct::SharedData::instance()->writeBool( &info );
    sgct::SharedData::instance()->writeBool( &stats );
    sgct::SharedData::instance()->writeBool( &takeScreenshot );
    sgct::SharedData::instance()->writeBool( &useTracking );
    sgct::SharedData::instance()->writeInt32( &stereoMode );
}

void decodeFun()
{
    sgct::SharedData::instance()->readDouble( &currentTime );
    sgct::SharedData::instance()->readBool( &wireframe );
    sgct::SharedData::instance()->readBool( &info );
    sgct::SharedData::instance()->readBool( &stats );
    sgct::SharedData::instance()->readBool( &takeScreenshot );
    sgct::SharedData::instance()->readBool( &useTracking );
    sgct::SharedData::instance()->readInt32( &stereoMode );
}

/*!
Will draw a flat surface that can be used for the heightmapped terrain.
@param    width    Width of the surface
@param    depth    Depth of the surface
@param    wRes    Width resolution of the surface
@param    dRes    Depth resolution of the surface
*/
void generateTerrainGrid( float width, float depth, unsigned int wRes, unsigned int dRes )
{
    float wStart = -width * 0.5f;
    float dStart = -depth * 0.5f;

    float dW = width / static_cast<float>( wRes );
    float dD = depth / static_cast<float>( dRes );

    //cleanup
    mVertPos.clear();
    mTexCoord.clear();

    for( unsigned int depthIndex = 0; depthIndex < dRes; ++depthIndex )
    {
        float dPosLow = dStart + dD * static_cast<float>( depthIndex );
        float dPosHigh = dStart + dD * static_cast<float>( depthIndex + 1 );
        float dTexCoordLow = depthIndex / static_cast<float>( dRes );
        float dTexCoordHigh = (depthIndex+1) / static_cast<float>( dRes );

        for( unsigned widthIndex = 0; widthIndex < wRes; ++widthIndex )
        {
            float wPos = wStart + dW * static_cast<float>( widthIndex );
            float wTexCoord = widthIndex / static_cast<float>( wRes );

            //p0
            mVertPos.push_back( wPos ); //x
            mVertPos.push_back( 0.0f ); //y
            mVertPos.push_back( dPosLow ); //z

            //p1
            mVertPos.push_back( wPos ); //x
            mVertPos.push_back( 0.0f ); //y
            mVertPos.push_back( dPosHigh ); //z

            //tex0
            mTexCoord.push_back( wTexCoord ); //s
            mTexCoord.push_back( dTexCoordLow ); //t

            //tex1
            mTexCoord.push_back( wTexCoord ); //s
            mTexCoord.push_back( dTexCoordHigh ); //t
        }
    }

    mNumberOfVerts = static_cast<GLsizei>(mVertPos.size() / 3); //each vertex has three componets (x, y & z)
}

void keyCallback(int key, int action)
{
    if( gEngine->isMaster() )
    {
        switch( key )
        {
        case 'S':
            if(action == SGCT_PRESS)
                stats.toggle();
            break;

        case 'I':
            if(action == SGCT_PRESS)
                info.toggle();
            break;

        case 'W':
            if(action == SGCT_PRESS)
                wireframe.toggle();
            break;

        case 'Q':
            if(action == SGCT_PRESS)
                gEngine->terminate();
            break;

        case 'T':
            if(action == SGCT_PRESS)
                useTracking.toggle();
            break;

        case 'E':
            if(action == SGCT_PRESS)
            {
                glm::dmat4 xform = glm::translate( glm::dmat4(1.0), glm::dvec3(0.0f, 0.0f, 4.0f) );
                sgct_core::ClusterManager::instance()->getDefaultUserPtr()->setTransform(xform);
            }
            break;

        case SGCT_KEY_SPACE:
            if(action == SGCT_PRESS)
                mPause = !mPause;
            break;

        case 'F':
            if(action == SGCT_PRESS)
                for(std::size_t i=0; i<gEngine->getNumberOfWindows(); i++)
                {
                    gEngine->getWindowPtr(i)->setUseFXAA( !gEngine->getWindowPtr(i)->useFXAA() );
                }
            break;

        case 'P':
        case SGCT_KEY_F10:
            if(action == SGCT_PRESS)
                takeScreenshot.setVal( true );
            break;

        case SGCT_KEY_LEFT:
            if(action == SGCT_PRESS)
            {
                if( stereoMode.getVal() > 0 )
                    stereoMode.setVal( (stereoMode.getVal() - 1) % sgct::SGCTWindow::Number_Of_Stereo_Items );
                else
                    stereoMode.setVal( sgct::SGCTWindow::Number_Of_Stereo_Items - 1 );
            }
            break;

        case SGCT_KEY_RIGHT:
            if(action == SGCT_PRESS)
                stereoMode.setVal( (stereoMode.getVal() + 1) % sgct::SGCTWindow::Number_Of_Stereo_Items );
            break;
        }
    }
}

void cleanUpFun()
{
    if(vertexPositionBuffer)
        glDeleteBuffers(1, &vertexPositionBuffer);
    if(texCoordBuffer)
        glDeleteBuffers(1, &texCoordBuffer);
    if(vertexArray)
        glDeleteVertexArrays(1, &vertexArray);
}
