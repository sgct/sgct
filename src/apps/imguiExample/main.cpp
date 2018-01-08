#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

sgct::Engine * gEngine;

void myDrawFun();
void myDraw2DFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

//input callbacks
void keyCallback(int key, int action);
void charCallback(unsigned int c);
void mouseButtonCallback(int button, int action);
void mouseScrollCallback(double xoffset, double yoffset);

sgct_utils::SGCTBox * myBox = NULL;
GLint Matrix_Loc = -1;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);
sgct::SharedFloat sharedSpeed(0.44f);
sgct::SharedBool sharedTextureOnOff(true);
sgct::SharedObject<glm::vec3> sharedClearColor(glm::vec3(60.0f));

//ImGUI variables
float speed = 0.44f;
bool use_texture = true;
ImVec4 clear_color = ImColor(60, 60, 60);
bool show_settings_window = true;
bool show_test_window = false;

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDraw2DFunction(myDraw2DFun);
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setCleanUpFunction( myCleanUpFun );
    
    gEngine->setKeyboardCallbackFunction( keyCallback );
    gEngine->setCharCallbackFunction( charCallback );
    gEngine->setMouseButtonCallbackFunction( mouseButtonCallback );
    gEngine->setMouseScrollCallbackFunction( mouseScrollCallback );

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
    if( gEngine->isMaster() )
        ImGui_ImplGlfwGL3_Shutdown();
    
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void myDrawFun()
{
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    gEngine->setClearColor(clear_color.x, clear_color.y, clear_color.z, 1.0f);

    //create scene transform (animation)
    glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
    scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));
    scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * (speed/2.0) ), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, (use_texture ? sgct::TextureManager::instance()->getTextureId("box") : NULL));

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);

    //draw the box
    myBox->draw();

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
}

void myDraw2DFun()
{
    if (gEngine->isMaster())
    {
        ImGui_ImplGlfwGL3_NewFrame(gEngine->getCurrentWindowPtr()->getXFramebufferResolution(), gEngine->getCurrentWindowPtr()->getYFramebufferResolution());

        // Show a settings window custom made for this application
        // Toggle this windows with the 'W' key.
        if (show_settings_window)
        {
            ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Settings");
            ImGui::SliderFloat("Rotation Speed", &(speed), 0.0f, 1.0f);
            ImGui::Checkbox("Texture On/Off", &use_texture);
            ImGui::ColorEdit3("Clear Color", (float*)&clear_color);
            if (ImGui::Button("Toggle Test Window")) show_test_window ^= 1;
            ImGui::End();
        }

        // Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(100, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        ImGui::Render();
    }
}

void myPreSyncFun()
{
    if( gEngine->isMaster() )
    {
        curr_time.setVal( sgct::Engine::getTime() );
    }
}

void myInitOGLFun()
{
    sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
    sgct::TextureManager::instance()->loadTexture("box", "../SharedResources/box.png", true);

    myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise

    sgct::ShaderManager::instance()->addShaderProgram( "xform",
            "SimpleVertexShader.vertexshader",
            "SimpleFragmentShader.fragmentshader" );

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
    GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
    glUniform1i( Tex_Loc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();
    
    // Setup ImGui binding
    if( gEngine->isMaster() )
        ImGui_ImplGlfwGL3_Init(gEngine->getCurrentWindowPtr()->getWindowHandle());
}

void myEncodeFun()
{
    sgct::SharedData::instance()->writeDouble(&curr_time);
    sharedSpeed.setVal(speed);
    sgct::SharedData::instance()->writeFloat(&sharedSpeed);
    sharedTextureOnOff.setVal(use_texture);
    sgct::SharedData::instance()->writeBool(&sharedTextureOnOff);
    sharedClearColor.setVal(glm::vec3(clear_color.x, clear_color.y, clear_color.z));
    sgct::SharedData::instance()->writeObj(&sharedClearColor);
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readDouble(&curr_time);
    sgct::SharedData::instance()->readFloat(&sharedSpeed);
    speed = sharedSpeed.getVal();
    sgct::SharedData::instance()->readBool(&sharedTextureOnOff);
    use_texture = sharedTextureOnOff.getVal();
    sgct::SharedData::instance()->readObj(&sharedClearColor);
    clear_color.x = sharedClearColor.getVal().x;
    clear_color.y = sharedClearColor.getVal().y;
    clear_color.z = sharedClearColor.getVal().z;
}

void keyCallback(int key, int action)
{
    if( gEngine->isMaster() )
    {
        switch( key )
        {
            case SGCT_KEY_W:
                if(action == SGCT_PRESS)
                    show_settings_window = !show_settings_window;
                break;
        }
        
        ImGui_ImplGlfwGL3_KeyCallback(gEngine->getCurrentWindowPtr()->getWindowHandle(), key, 0, action, 0);
    }
}

void charCallback(unsigned int c)
{
    if( gEngine->isMaster() )
    {
        ImGui_ImplGlfwGL3_CharCallback(gEngine->getCurrentWindowPtr()->getWindowHandle(), c);
    }
}

void mouseButtonCallback(int button, int action)
{
    if( gEngine->isMaster() )
    {
        ImGui_ImplGlfwGL3_MouseButtonCallback(gEngine->getCurrentWindowPtr()->getWindowHandle(), button, action, 0);
    }
}

void mouseScrollCallback(double xoffset, double yoffset)
{
    if( gEngine->isMaster() )
    {
        ImGui_ImplGlfwGL3_ScrollCallback(gEngine->getCurrentWindowPtr()->getWindowHandle(), xoffset, yoffset);
    }
}

void myCleanUpFun()
{
    if(myBox != NULL)
        delete myBox;
}
