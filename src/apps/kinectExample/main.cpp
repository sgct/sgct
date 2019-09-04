#include "sgct.h"

sgct::Engine * gEngine;

void initOGLFun();
void drawFun();
void preSyncFun();
void encodeFun();
void decodeFun();

sgct::SharedDouble currentTime(0.0);
sgct::SharedFloat size_factor(0.5f);

//pointer to a left hand
sgct::SGCTTrackingDevice * leftHand = NULL;
//pointer to a right hand
sgct::SGCTTrackingDevice * rightHand = NULL;

bool error = false;

int main( int argc, char* argv[] )
{

    // Allocate
    gEngine = new sgct::Engine( argc, argv );

    // Bind your functions
    gEngine->setInitOGLFunction( initOGLFun );
    gEngine->setDrawFunction( drawFun );
    gEngine->setPreSyncFunction( preSyncFun );
    sgct::SharedData::instance()->setEncodeFunction(encodeFun);
    sgct::SharedData::instance()->setDecodeFunction(decodeFun);

    // Init the engine
    if( !gEngine->init() )
    {
        delete gEngine;
        return EXIT_FAILURE;
    }

    // Main loop
    gEngine->render();

    // Clean up (de-allocate)
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void initOGLFun()
{
    glEnable(GL_DEPTH_TEST);
 
    //connect only to VRPN on the master
    if( gEngine->isMaster() )
    {
        //get the tracking pointers
        sgct::SGCTTracker * tracker = sgct::Engine::getTrackingManager()->getTrackerPtr("Kinect0");
        if(tracker != NULL)
        {
            leftHand    = tracker->getDevicePtr("Left Hand");
            rightHand    = tracker->getDevicePtr("Right Hand");
        }

        if(leftHand == NULL || rightHand == NULL)
        {
            error = true;
            sgct::MessageHandler::instance()->print("Failed to get pointers to hand trackers!\n");
        }
    }
}

void drawFun()
{
    float speed = 50.0f;
    glRotatef(static_cast<float>( currentTime.getVal() ) * speed, 0.0f, 1.0f, 0.0f);

    float size = size_factor.getVal();

    //render a single triangle
    glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f); //Red
        glVertex3f(-0.5f * size, -0.5f * size, 0.0f);
 
        glColor3f(0.0f, 1.0f, 0.0f); //Green
        glVertex3f(0.0f, 0.5f * size, 0.0f);
 
        glColor3f(0.0f, 0.0f, 1.0f); //Blue
        glVertex3f(0.5f * size, -0.5f * size, 0.0f);
    glEnd();
}

void preSyncFun()
{
    //set the time only on the master
    if( gEngine->isMaster() )
    {
        //get the time in seconds
        currentTime.setVal(sgct::Engine::getTime());

        if(!error)
        {
            glm::vec3 leftPos = leftHand->getPosition();
            glm::vec3 rightPos = rightHand->getPosition();
            float dist = glm::length(leftPos - rightPos);
            size_factor.setVal( (dist < 2.0f && dist > 0.2f) ? dist : 0.5f );
        }
    }
}

void encodeFun()
{
    sgct::SharedData::instance()->writeDouble( &currentTime );
    sgct::SharedData::instance()->writeFloat( &size_factor );
}

void decodeFun()
{
    sgct::SharedData::instance()->readDouble( &currentTime );
    sgct::SharedData::instance()->readFloat( &size_factor );
}
