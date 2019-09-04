#include "sgct.h"

sgct::Engine * gEngine;

void drawFun();
void preSyncFun();
void encodeFun();
void decodeFun();

sgct::SharedDouble currentTime(0.0);

int main( int argc, char* argv[] )
{

    // Allocate
    gEngine = new sgct::Engine( argc, argv );

    // Bind your functions
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

void drawFun()
{
    float speed = 50.0f;
    glRotatef(static_cast<float>( currentTime.getVal() ) * speed, 0.0f, 1.0f, 0.0f);

    //render a single triangle
    glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f); //Red
        glVertex3f(-0.5f, -0.5f, 0.0f);

        glColor3f(0.0f, 1.0f, 0.0f); //Green
        glVertex3f(0.0f, 0.5f, 0.0f);

        glColor3f(0.0f, 0.0f, 1.0f); //Blue
        glVertex3f(0.5f, -0.5f, 0.0f);
    glEnd();
}

void preSyncFun()
{
    //set the time only on the master
    if( gEngine->isMaster() )
    {
        //get the time in seconds
        currentTime.setVal(sgct::Engine::getTime());
    }
}

void encodeFun()
{
    sgct::SharedData::instance()->writeDouble( &currentTime );
}

void decodeFun()
{
    sgct::SharedData::instance()->readDouble( &currentTime );
}
