#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"
#include "objloader.hpp"
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

//regular functions
void loadModel( std::string filename );
bool hasUVs = false; //init to false and set to true when loading UVs
bool hasNormals = false; //init to false and set to true when loading normals

enum VBO_INDEXES { VBO_POSITIONS = 0, VBO_UVS, VBO_NORMALS };
GLuint vertexBuffers[3];
GLsizei numberOfVertices = 0;

//variables to share across cluster
sgct::SharedDouble currentTime(0.0);

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setCleanUpFunction( myCleanUpFun );
    gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );

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
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glEnable( GL_TEXTURE_2D );

    double speed = 0.44;

    //create scene transform (animation)
    glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
    scene_mat = glm::rotate( scene_mat, static_cast<float>( currentTime.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));

    glMultMatrixf( glm::value_ptr(scene_mat) ); //multiply the modelview matrix with the scene transform

    // ------ draw model --------------- //
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT); //push attributes to the stack in order to unset them correctly
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ VBO_POSITIONS ] );
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<void*>(0));

    if(hasUVs)
    {
        glClientActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box"));
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ VBO_UVS ] );

        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, reinterpret_cast<void*>(0));
    }
    
    if(hasNormals)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ VBO_NORMALS ] );

        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, reinterpret_cast<void*>(0));
    }
    
    glDrawArrays(GL_TRIANGLES, 0, numberOfVertices);

    glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE); //unbind
    glPopClientAttrib();
    // ----------------------------------//

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
}

void myPreSyncFun()
{
    if( gEngine->isMaster() )
    {
        currentTime.setVal( sgct::Engine::getTime() );
    }
}

void myPostSyncPreDrawFun()
{
    ;
}

void myInitOGLFun()
{
    sgct::TextureManager::instance()->setWarpingMode(GL_REPEAT, GL_REPEAT);
    sgct::TextureManager::instance()->setAnisotropicFilterSize(4.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
    sgct::TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    loadModel( "../SharedResources/box.obj" );
    
    glEnable( GL_TEXTURE_2D );
    glDisable(GL_LIGHTING); //no lights at the moment

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise
}

void myEncodeFun()
{
    sgct::SharedData::instance()->writeDouble(&currentTime);
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readDouble(&currentTime);
}

/*!
    De-allocate data from GPU
    Textures are deleted automatically when using texture manager
    Shaders are deleted automatically when using shader manager
*/
void myCleanUpFun()
{
    if( vertexBuffers[0] ) //if first is created, all has been created.
    {
        glDeleteBuffers(3, &vertexBuffers[0]);
        for(unsigned int i=0; i<3; i++)
            vertexBuffers[i] = GL_FALSE;
    }
}

/*
    Loads obj model and uploads to the GPU 
*/
void loadModel( std::string filename )
{
    // Read our .obj file
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    
    //if successful
    if( loadOBJ( filename.c_str(), positions, uvs, normals) )
    {
        //store the number of triangles
        numberOfVertices = static_cast<GLsizei>( positions.size() );
        
        //init VBOs
        for(unsigned int i=0; i<3; i++)
            vertexBuffers[i] = GL_FALSE;
        glGenBuffers(3, &vertexBuffers[0]);
        
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ VBO_POSITIONS ] );
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), &positions[0], GL_STATIC_DRAW);
    
        if( uvs.size() > 0 )
        {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ VBO_UVS ] );
            glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
            hasUVs = true;
        }
        else
            sgct::MessageHandler::instance()->print("Warning: Model is missing UV data.\n");

        if( normals.size() > 0 )
        {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ VBO_NORMALS ] );
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
            hasNormals = true;
        }
        else
            sgct::MessageHandler::instance()->print("Warning: Model is missing normal data.\n");

        glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE); //unbind VBO

        //clear vertex data that is uploaded on GPU
        positions.clear();
        uvs.clear();
        normals.clear();

        //print some usefull info
        sgct::MessageHandler::instance()->print("Model '%s' loaded successfully (%u vertices, VBOs: %u %u %u).\n",
            filename.c_str(),
            numberOfVertices,
            vertexBuffers[VBO_POSITIONS],
            vertexBuffers[VBO_UVS],
            vertexBuffers[VBO_NORMALS] );
    }
    else
        sgct::MessageHandler::instance()->print("Failed to load model '%s'!\n", filename.c_str() );

}

void keyCallback(int key, int action)
{
    if( gEngine->isMaster() )
    {
        /*switch( key )
        {
        case SGCT_KEY_R:
            if(action == SGCT_PRESS)
                ;//do something
            break;
        }*/
    }
}
