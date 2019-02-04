#include "sgct.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
//input callbacks
void keyCallback(int key, int action);
void mouseButtonCallback(int button, int action, int mods);

void drawXZGrid(int size, float yPos);
void drawPyramid(float width);

float rotationSpeed = 0.0017f;
float walkingSpeed = 2.5f;

GLuint myLandscapeDisplayList = 0;
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

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );
    gEngine->setMouseButtonCallbackFunction( mouseButtonCallback );
    gEngine->setClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    for(int i=0; i<4; i++)
        arrowButtons[i] = false;

    if( !gEngine->init() )
    {
        delete gEngine;
        return EXIT_FAILURE;
    }

    sgct::SharedData::instance()->setEncodeFunction( myEncodeFun );
    sgct::SharedData::instance()->setDecodeFunction( myDecodeFun );

    // Main loop
    gEngine->render();

    // Clean up
    glDeleteLists(myLandscapeDisplayList, 1);
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void myInitOGLFun()
{
    //create and compile display list
    myLandscapeDisplayList = glGenLists(1);
    glNewList(myLandscapeDisplayList, GL_COMPILE);

    drawXZGrid(landscapeSize, -1.5f);

    //pick a seed for the random function (must be same on all nodes)
    srand(9745);
    for(int i=0; i<numberOfPyramids; i++)
    {
        float xPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);
        float zPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);

        glPushMatrix();
        glTranslatef(xPos, -1.5f, zPos);
        drawPyramid(0.6f);
        glPopMatrix();
    }

    glEndList();
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
        result = glm::translate( glm::mat4(1.0f), sgct::Engine::getDefaultUserPtr()->getPos() );
        //3. apply view rotation
        result *= ViewRotateX;
        //2. apply navigation translation
        result *= glm::translate(glm::mat4(1.0f), pos);
        //1. transform user to coordinate system origin
        result *= glm::translate(glm::mat4(1.0f), -sgct::Engine::getDefaultUserPtr()->getPos());

        xform.setVal( result );
    }
}

void myDrawFun()
{
    //glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    
    glMultMatrixf(glm::value_ptr(xform.getVal()));
    glCallList(myLandscapeDisplayList);

    //glDisable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
}

void myEncodeFun()
{
    sgct::SharedData::instance()->writeObj( &xform );
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readObj( &xform );
}

void keyCallback(int key, int action)
{
    if( gEngine->isMaster() )
    {
        switch( key )
        {
        case SGCT_KEY_UP:
        case SGCT_KEY_W:
            arrowButtons[FORWARD] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
            break;

        case SGCT_KEY_DOWN:
        case SGCT_KEY_S:
            arrowButtons[BACKWARD] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
            break;

        case SGCT_KEY_LEFT:
        case SGCT_KEY_A:
            arrowButtons[LEFT] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
            break;

        case SGCT_KEY_RIGHT:
        case SGCT_KEY_D:
            arrowButtons[RIGHT] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
            break;
        }
    }
}

void mouseButtonCallback(int button, int action, int)
{
    if( gEngine->isMaster() )
    {
        switch( button )
        {
        case SGCT_MOUSE_BUTTON_LEFT:
            mouseLeftButton = (action == SGCT_PRESS ? true : false);
            double tmpYPos;
            //set refPos
            sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[1], &tmpYPos );
            break;
        }
    }
}

void drawXZGrid(int size, float yPos)
{
    glPolygonOffset(0.0f, 0.0f); //offset to avoid z-buffer fighting
    glPushMatrix();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glTranslatef(0.0f, yPos, 0.0f);

    glLineWidth(3.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);

    glBegin( GL_LINES );
    for(int x = -(size/2); x < (size/2); x++)
    {
        glVertex3i(x, 0, -(size/2));
        glVertex3i(x, 0, (size/2));
    }

    for(int z = -(size/2); z < (size/2); z++)
    {
        glVertex3i(-(size/2), 0, z);
        glVertex3i((size/2), 0, z);
    }
    glEnd();

    glDisable(GL_BLEND);
    glPopMatrix();
}

void drawPyramid(float width)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonOffset(1.0f, 0.1f); //offset to avoid z-buffer fighting
    //enhance the pyramids with lines in the edges
    glLineWidth(2.0f);
    glColor4f(1.0f, 0.0f, 0.5f, 0.8f);

    glBegin(GL_LINE_LOOP);
    glVertex3f(-width/2.0f, 0.0f, -width/2.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);
    glVertex3f(-width/2.0f, 0.0f, width/2.0f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-width/2.0f, 0.0f, width/2.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);
    glVertex3f(width/2.0f, 0.0f, width/2.0f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(width/2.0f, 0.0f, width/2.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);
    glVertex3f(width/2.0f, 0.0f, -width/2.0f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(width/2.0f, 0.0f, -width/2.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);
    glVertex3f(-width/2.0f, 0.0f, -width/2.0f);
    glEnd();

    glColor4f(1.0f, 0.0f, 0.5f, 0.3f);

    glPolygonOffset(0.0f, 0.0f); //offset to avoid z-buffer fighting
    glBegin(GL_TRIANGLE_FAN);
    //draw top
    glVertex3f(0.0f, 2.0f, 0.0f);

    //draw sides
    glVertex3f(-width / 2.0f, 0.0f, -width / 2.0f);
    glVertex3f(-width / 2.0f, 0.0f, width / 2.0f);
    glVertex3f(width / 2.0f, 0.0f, width / 2.0f);
    glVertex3f(width / 2.0f, 0.0f, -width / 2.0f);
    glVertex3f(-width / 2.0f, 0.0f, -width / 2.0f);

    glEnd();

    glDisable(GL_BLEND);
}
