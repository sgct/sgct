#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"
#include "objloader.hpp"
#include <glm/gtc/matrix_inverse.hpp>

sgct::Engine * gEngine;

//callbacks
void drawFun();
void preSyncFun();
void postSyncPreDrawFun();
void initOGLFun();
void encodeFun();
void decodeFun();
void cleanUpFun();
void keyCallback(int key, int action);

//regular functions
void loadModel( std::string filename );

enum VBO_INDEXES { VBO_POSITIONS = 0, VBO_UVS, VBO_NORMALS };
GLuint vertexBuffers[3];
GLuint VertexArrayID = GL_FALSE;
GLsizei numberOfVertices = 0;

//shader locations
GLint MVP_Loc = -1;
GLint NM_Loc = -1;

//variables to share across cluster
sgct::SharedDouble currentTime(0.0);
sgct::SharedBool reloadShader(false);

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( initOGLFun );
    gEngine->setDrawFunction( drawFun );
    gEngine->setPreSyncFunction( preSyncFun );
    gEngine->setCleanUpFunction( cleanUpFun );
    gEngine->setPostSyncPreDrawFunction( postSyncPreDrawFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );

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

void drawFun()
{
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );

    double speed = 0.44;

    //create scene transform (animation)
    glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
    scene_mat = glm::rotate( scene_mat, static_cast<float>( currentTime.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;
    glm::mat3 NM = glm::inverseTranspose(glm::mat3( gEngine->getCurrentModelViewMatrix() * scene_mat ));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box"));

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    glUniformMatrix4fv(MVP_Loc, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix3fv(NM_Loc, 1, GL_FALSE, &MVP[0][0]);

    // ------ draw model --------------- //
    glBindVertexArray(VertexArrayID);
    glDrawArrays(GL_TRIANGLES, 0, numberOfVertices );
    glBindVertexArray(GL_FALSE); //unbind
    // ----------------------------------//

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
}

void preSyncFun()
{
    if( gEngine->isMaster() )
    {
        currentTime.setVal( sgct::Engine::getTime() );
    }
}

void postSyncPreDrawFun()
{
    if( reloadShader.getVal() )
    {
        sgct::ShaderProgram sp = sgct::ShaderManager::instance()->getShaderProgram( "xform" );
        sp.reload();

        //reset locations
        sp.bind();

        MVP_Loc = sp.getUniformLocation( "MVP" );
        NM_Loc = sp.getUniformLocation( "NM" );
        GLint Tex_Loc = sp.getUniformLocation( "Tex" );
        glUniform1i( Tex_Loc, 0 );

        sp.unbind();
        reloadShader.setVal(false);
    }
}

void initOGLFun()
{
    sgct::TextureManager::instance()->setWarpingMode(GL_REPEAT, GL_REPEAT);
    sgct::TextureManager::instance()->setAnisotropicFilterSize(4.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
    sgct::TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    loadModel( "../SharedResources/box.obj" );
    
    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise

    sgct::ShaderManager::instance()->addShaderProgram( "xform",
            "simple.vert",
            "simple.frag" );

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    MVP_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
    NM_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "NM" );
    GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
    glUniform1i( Tex_Loc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();
}

void encodeFun()
{
    sgct::SharedData::instance()->writeDouble( &currentTime );
    sgct::SharedData::instance()->writeBool( &reloadShader );
}

void decodeFun()
{
    sgct::SharedData::instance()->readDouble( &currentTime );
    sgct::SharedData::instance()->readBool( &reloadShader );
}

/*!
    De-allocate data from GPU
    Textures are deleted automatically when using texture manager
    Shaders are deleted automatically when using shader manager
*/
void cleanUpFun()
{
    if( VertexArrayID )
    {
        glDeleteVertexArrays(1, &VertexArrayID);
        VertexArrayID = GL_FALSE;
    }

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
        
        //create VAO
        glGenVertexArrays(1, &VertexArrayID);
        glBindVertexArray(VertexArrayID);

        //init VBOs
        for(unsigned int i=0; i<3; i++)
            vertexBuffers[i] = GL_FALSE;
        glGenBuffers(3, &vertexBuffers[0]);
        
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ VBO_POSITIONS ] );
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), &positions[0], GL_STATIC_DRAW);
        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,                  // attribute
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            reinterpret_cast<void*>(0) // array buffer offset
        );

        if( uvs.size() > 0 )
        {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ VBO_UVS ] );
            glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
            // 2nd attribute buffer : UVs
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(
                1,                                // attribute
                2,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                reinterpret_cast<void*>(0) // array buffer offset
            );
        }
        else
            sgct::MessageHandler::instance()->print("Warning: Model is missing UV data.\n");

        if( normals.size() > 0 )
        {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ VBO_NORMALS ] );
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
            // 3nd attribute buffer : Normals
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(
                2,                                // attribute
                3,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                reinterpret_cast<void*>(0) // array buffer offset
            );
        }
        else
            sgct::MessageHandler::instance()->print("Warning: Model is missing normal data.\n");

        glBindVertexArray(GL_FALSE); //unbind VAO

        //clear vertex data that is uploaded on GPU
        positions.clear();
        uvs.clear();
        normals.clear();

        //print some usefull info
        sgct::MessageHandler::instance()->print("Model '%s' loaded successfully (%u vertices, VAO: %u, VBOs: %u %u %u).\n",
            filename.c_str(),
            numberOfVertices,
            VertexArrayID,
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
        switch( key )
        {
        case SGCT_KEY_R:
            if(action == SGCT_PRESS)
                reloadShader.setVal(true);
            break;
        }
    }
}
