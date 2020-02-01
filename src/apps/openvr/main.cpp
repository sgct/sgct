/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct.h>
#include <sgct/openvr.h>
#include <sgct/user.h>
#include <sgct/window.h>
#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// This is basically the simpleNavgationExample
// Extended with only a few lines to support OpenVR

constexpr const char* gridVertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPosition;

  uniform mat4 mvp;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPosition, 1.0);
  })";

constexpr const char* gridFragmentShader = R"(
  #version 330 core

  uniform vec4 linecolor;
  out vec4 color;

  void main() { color = linecolor; }
)";

constexpr const char* pyramidVertexShader = R"(
  #version 330 core

  layout(location = 0) in vec3 vertPosition;

  uniform mat4 mvp;

  void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  mvp * vec4(vertPosition, 1.0);
  })";

constexpr const char* pyramidFragmentShader = R"(
  #version 330 core

  uniform float alpha;

  out vec4 color;

  void main() { color = vec4(1.0, 0.0, 0.5, alpha); }
)";

sgct::Engine* gEngine;
sgct::Window* FirstOpenVRWindow = NULL;

void myInitOGLFun();
void myPreSyncFun();
void myPreDrawFun();
void myDrawFun();
void myPostDrawFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

//input callbacks
void keyCallback(int key, int, int action, int);
void mouseButtonCallback(int button, int action, int mods);

void drawXZGrid(glm::mat4& MVP);
void drawPyramid(glm::mat4& MVP, int index);
void createXZGrid(int size, float yPos);
void createPyramid(float width);

float rotationSpeed = 0.0017f;
float walkingSpeed = 2.5f;

const int landscapeSize = 50;
const int numberOfPyramids = 150;

bool arrowButtons[4];
enum directions { FORWARD = 0, BACKWARD, LEFT, RIGHT };

//to check if left mouse button is pressed
bool mouseLeftButton = false;
/* Holds the difference in position between when the left mouse button
    is pressed and when the mouse button is held. */
double mouseDx = 0.0;
/* Stores the positions that will be compared to measure the difference. */
double mouseXPos[] = { 0.0, 0.0 };

glm::vec3 view(0.0f, 0.0f, 1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
glm::vec3 pos(0.0f, 0.0f, 0.0f);

sgct::SharedObject<glm::mat4> xform;
glm::mat4 pyramidTransforms[numberOfPyramids];

enum geometryType { PYRAMID = 0, GRID };
GLuint VAOs[2] = { 0, 0 };
GLuint VBOs[2] = { 0, 0 };
//shader locations
GLint Matrix_Locs[2] = { -1, -1 };
GLint linecolor_loc = -1;
GLint alpha_Loc = -1;

int numberOfVerts[2] = { 0, 0 };

using namespace sgct;

class Vertex
{
public:
    Vertex() { mX = mY = mZ = 0.0f; }
    Vertex(float z, float y, float x) { mX = x; mY = y; mZ = z; }
    float mX, mY, mZ;
};

int main( int argc, char* argv[] )
{
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setPostSyncPreDrawFunction( myPreDrawFun );
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPostDrawFunction( myPostDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );
    gEngine->setMouseButtonCallbackFunction( mouseButtonCallback );
    gEngine->setClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
    gEngine->setCleanUpFunction( myCleanUpFun );

    for(int i=0; i<4; i++)
        arrowButtons[i] = false;

    if (!gEngine->init(Engine::RunMode::Default_Mode, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction( myEncodeFun );
    sgct::SharedData::instance()->setDecodeFunction( myDecodeFun );

    // Main loop
    gEngine->render();

    // Clean up OpenVR
    sgct::openvr::shutdown();

    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void myInitOGLFun()
{
    //Find if we have at least one OpenVR window
    //Save reference to first OpenVR window, which is the one we will copy to the HMD.
    for (size_t i = 0; i < gEngine->getNumberOfWindows(); i++) {
        if (gEngine->getWindow(i).hasTag("OpenVR")) {
            FirstOpenVRWindow = &gEngine->getWindow(i);
            break;
        }
    }
    //If we have an OpenVRWindow, initialize OpenVR.
    if (FirstOpenVRWindow) {
        sgct::openvr::initialize(gEngine->getNearClippingPlane(), gEngine->getFarClippingPlane());
    }

    //generate the VAOs
    glGenVertexArrays(2, &VAOs[0]);
    //generate VBOs for vertex positions
    glGenBuffers(2, &VBOs[0]);

    createXZGrid(landscapeSize, -1.5f);
    createPyramid(0.6f);

    //pick a seed for the random function (must be same on all nodes)
    srand(9745);
    for(int i=0; i<numberOfPyramids; i++)
    {
        float xPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);
        float zPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);

        pyramidTransforms[i] = glm::translate(glm::mat4(1.0f), glm::vec3(xPos, -1.5f, zPos));
    }

    sgct::ShaderManager::instance()->addShaderProgram(
        "gridShader",
        gridVertexShader,
        gridFragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    sgct::ShaderManager::instance()->bindShaderProgram("gridShader");
    Matrix_Locs[GRID] = sgct::ShaderManager::instance()->getShaderProgram("gridShader").getUniformLocation("mvp");
    linecolor_loc = sgct::ShaderManager::instance()->getShaderProgram("gridShader").getUniformLocation("linecolor");
    sgct::ShaderManager::instance()->unBindShaderProgram();

    sgct::ShaderManager::instance()->addShaderProgram(
        "pyramidShader",
        pyramidVertexShader,
        pyramidFragmentShader,
        ShaderProgram::ShaderSourceType::String
    );
    sgct::ShaderManager::instance()->bindShaderProgram("pyramidShader");
    Matrix_Locs[PYRAMID] = sgct::ShaderManager::instance()->getShaderProgram("pyramidShader").getUniformLocation("mvp");
    alpha_Loc = sgct::ShaderManager::instance()->getShaderProgram("pyramidShader").getUniformLocation("alpha");
    sgct::ShaderManager::instance()->unBindShaderProgram();
}

void myPreSyncFun()
{
    if( gEngine->isMaster() )
    {
        if( mouseLeftButton )
        {
            double tmpYPos;
            //get the mouse pos from first window
            sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[0], &tmpYPos );
            mouseDx = mouseXPos[0] - mouseXPos[1];
        }
        else
        {
            mouseDx = 0.0;
        }

        static float panRot = 0.0f;
        panRot += (static_cast<float>(mouseDx) * rotationSpeed * static_cast<float>(gEngine->getDt()));

        glm::mat4 ViewRotateX = glm::rotate(
            glm::mat4(1.0f),
            panRot,
            glm::vec3(0.0f, 1.0f, 0.0f)); //rotation around the y-axis

        view = glm::inverse(glm::mat3(ViewRotateX)) * glm::vec3(0.0f, 0.0f, 1.0f);

        glm::vec3 right = glm::cross(view, up);

        if( arrowButtons[FORWARD] )
            pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * view);
        if( arrowButtons[BACKWARD] )
            pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * view);
        if( arrowButtons[LEFT] )
            pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);
        if( arrowButtons[RIGHT] )
            pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);

        /*
            To get a first person camera, the world needs
            to be transformed around the users head.

            This is done by:
            1, Transform the user to coordinate system origin
            2, Apply navigation
            3, Apply rotation
            4, Transform the user back to original position

            However, mathwise this process need to be reversed
            due to the matrix multiplication order.
        */

        glm::mat4 result;
        //4. transform user back to original position
        result = glm::translate(glm::mat4(1.0f), Engine::getDefaultUser().getPosMono());
        //3. apply view rotation
        result *= ViewRotateX;
        //2. apply navigation translation
        result *= glm::translate(glm::mat4(1.0f), pos);
        //1. transform user to coordinate system origin
        result *= glm::translate(glm::mat4(1.0f), -Engine::getDefaultUser().getPosMono());

        xform.setVal( result );
    }
}

void myPreDrawFun()
{
    if (FirstOpenVRWindow) {
        //Update pose matrices for all tracked OpenVR devices once per frame
        sgct::openvr::updatePoses();
    }
}

void myDrawFun()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 MVP;
    if (sgct::openvr::isHMDActive() &&
        (FirstOpenVRWindow == &gEngine->getCurrentWindow() || gEngine->getCurrentWindow().hasTag("OpenVR"))) {
        MVP = sgct::openvr::getHMDCurrentViewProjectionMatrix(gEngine->getCurrentFrustumMode());

        if (gEngine->getCurrentFrustumMode() == sgct::core::Frustum::Mode::MonoEye) {
            //Reversing rotation around z axis (so desktop view is more pleasent to look at).
            glm::quat inverserotation = sgct::openvr::getInverseRotation(sgct::openvr::getHMDPoseMatrix());
            inverserotation.x = inverserotation.y = 0.f;
            MVP *= glm::mat4_cast(inverserotation);
        }
    }
    else {
        MVP = gEngine->getCurrentModelViewProjectionMatrix();
    }
    MVP *= xform.getVal();

    drawXZGrid(MVP);

    for (int i = 0; i < numberOfPyramids; i++)
        drawPyramid(MVP, i);

    glDisable(GL_BLEND);
}

void myPostDrawFun()
{
    if (FirstOpenVRWindow) {
        //Copy the first OpenVR window to the HMD
        sgct::openvr::copyWindowToHMD(FirstOpenVRWindow);
    }
}

void myEncodeFun()
{
    SharedData::instance()->writeObj(xform);
}

void myDecodeFun()
{
    SharedData::instance()->readObj(xform);
}

void keyCallback(int key, int, int action, int)
{
    if( gEngine->isMaster() )
    {
        switch (key) {
        case key::Up:
        case key::W:
            arrowButtons[FORWARD] = (action == action::Repeat || action == action::Press);
            break;
        case key::Down:
        case key::S:
            arrowButtons[BACKWARD] = (action == action::Repeat || action == action::Press);
            break;
        case key::Left:
        case key::A:
            arrowButtons[LEFT] = (action == action::Repeat || action == action::Press);
            break;
        case key::Right:
        case key::D:
            arrowButtons[RIGHT] = (action == action::Repeat || action == action::Press);
            break;
        }
    }
}

void mouseButtonCallback(int button, int action, int mods)
{
    if( gEngine->isMaster() )
    {
        switch( button )
        {
        case mouse::ButtonLeft:
            mouseLeftButton = (action == action::Press ? true : false);
            double tmpYPos;
            //set refPos
            sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[1], &tmpYPos );
            break;
        }
    }
}

void drawXZGrid(glm::mat4& MVP)
{
    sgct::ShaderManager::instance()->bindShaderProgram("gridShader");

    glUniformMatrix4fv(Matrix_Locs[GRID], 1, GL_FALSE, &MVP[0][0]);

    glBindVertexArray(VAOs[GRID]);

    glUniform4f(linecolor_loc, 1.f, 1.f, 1.f, 0.8f);

    glLineWidth(3.0f);
    glPolygonOffset(0.0f, 0.0f); //offset to avoid z-buffer fighting
    glDrawArrays(GL_LINES, 0, numberOfVerts[GRID]);

    glBindVertexArray(0);
    sgct::ShaderManager::instance()->unBindShaderProgram();
}

void drawPyramid(glm::mat4& MVP, int index)
{
    glm::mat4 MVP_pyramid = MVP * pyramidTransforms[index];

    sgct::ShaderManager::instance()->bindShaderProgram("pyramidShader");

    glUniformMatrix4fv(Matrix_Locs[PYRAMID], 1, GL_FALSE, &MVP_pyramid[0][0]);

    glBindVertexArray(VAOs[PYRAMID]);

    //draw lines
    glLineWidth(2.0f);
    glPolygonOffset(1.0f, 0.1f); //offset to avoid z-buffer fighting
    glUniform1f(alpha_Loc, 0.8f);
    glDrawArrays(GL_LINES, 0, 16);
    //draw triangles
    glPolygonOffset(0.0f, 0.0f); //offset to avoid z-buffer fighting
    glUniform1f(alpha_Loc, 0.3f);
    glDrawArrays(GL_TRIANGLES, 16, 12);

    glBindVertexArray(0);
    sgct::ShaderManager::instance()->unBindShaderProgram();
}

void createXZGrid(int size, float yPos)
{
    numberOfVerts[GRID] = size * 4;
    Vertex * vertData = new (std::nothrow) Vertex[numberOfVerts[GRID]];

    int i = 0;
    for (int x = -(size / 2); x < (size / 2); x++)
    {
        vertData[i].mX = static_cast<float>(x);
        vertData[i].mY = yPos;
        vertData[i].mZ = static_cast<float>(-(size / 2));

        vertData[i + 1].mX = static_cast<float>(x);
        vertData[i + 1].mY = yPos;
        vertData[i + 1].mZ = static_cast<float>(size / 2);

        i += 2;
    }

    for (int z = -(size / 2); z < (size / 2); z++)
    {
        vertData[i].mX = static_cast<float>(-(size / 2));
        vertData[i].mY = yPos;
        vertData[i].mZ = static_cast<float>(z);

        vertData[i + 1].mX = static_cast<float>(size / 2);
        vertData[i + 1].mY = yPos;
        vertData[i + 1].mZ = static_cast<float>(z);

        i += 2;
    }

    glBindVertexArray(VAOs[GRID]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[GRID]);

    //upload data to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*numberOfVerts[GRID], vertData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        reinterpret_cast<void*>(0) // array buffer offset
        );

    //unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //clean up
    delete[] vertData;
    vertData = NULL;
}

void createPyramid(float width)
{
    std::vector<Vertex> vertData;

    //enhance the pyramids with lines in the edges
    //-x
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
    vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
    //+x
    vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
    vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
    vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
    vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
    //-z
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
    vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
    vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
    //+z
    vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
    vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
    vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));

    //triangles
    //-x
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
    vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
    //+x
    vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
    vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
    vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
    //-z
    vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
    vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
    //+z
    vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
    vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
    vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));

    numberOfVerts[PYRAMID] = static_cast<int>( vertData.size() );

    glBindVertexArray(VAOs[PYRAMID]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[PYRAMID]);

    //upload data to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*numberOfVerts[PYRAMID], &vertData[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        reinterpret_cast<void*>(0) // array buffer offset
        );

    //unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //clean up
    vertData.clear();
}

void myCleanUpFun()
{
    if (VBOs[0])
        glDeleteBuffers(2, &VBOs[0]);
    if (VAOs[0])
        glDeleteVertexArrays(2, &VAOs[0]);
}
