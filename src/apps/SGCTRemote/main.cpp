#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myEncodeFun();
void myDecodeFun();

sgct::SharedDouble curr_time(0.0);

void myPostSyncPreDrawFun();
void externalControlMessageCallback(const char * receivedChars, int size);
void externalControlStatusCallback(bool connected);

sgct::SharedBool showStats(false);
sgct::SharedBool showGraph(false);
sgct::SharedBool showWireframe(false);
sgct::SharedFloat size_factor(0.5f);

int main( int argc, char* argv[] )
{

    // Allocate
    gEngine = new sgct::Engine( argc, argv );

    // Bind your functions
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
    gEngine->setExternalControlCallback( externalControlMessageCallback );
    gEngine->setExternalControlStatusCallback( externalControlStatusCallback );

    sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
    sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

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

void myDrawFun()
{
    float speed = 50.0f;
    glRotatef(static_cast<float>( curr_time.getVal() ) * speed, 0.0f, 1.0f, 0.0f);

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

void myPreSyncFun()
{
    //set the time only on the master
    if( gEngine->isMaster() )
    {
        //get the time in seconds
        curr_time.setVal( sgct::Engine::getTime() );
    }
}

void myPostSyncPreDrawFun()
{
    gEngine->setDisplayInfoVisibility( showStats.getVal() );
    gEngine->setStatsGraphVisibility( showGraph.getVal() );
    gEngine->setWireframe( showWireframe.getVal() );
}

void myEncodeFun()
{
    sgct::SharedData::instance()->writeDouble( &curr_time );
    sgct::SharedData::instance()->writeFloat( &size_factor );
    sgct::SharedData::instance()->writeBool( &showStats );
    sgct::SharedData::instance()->writeBool( &showGraph );
    sgct::SharedData::instance()->writeBool( &showWireframe );
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readDouble( &curr_time );
    sgct::SharedData::instance()->readFloat( &size_factor );
    sgct::SharedData::instance()->readBool( &showStats );
    sgct::SharedData::instance()->readBool( &showGraph );
    sgct::SharedData::instance()->readBool( &showWireframe );
}

void externalControlMessageCallback(const char * receivedChars, int size)
{
    if( gEngine->isMaster() )
    {
        if(size == 7 && strncmp(receivedChars, "stats", 5) == 0)
        {
            showStats.setVal(strncmp(receivedChars + 6, "1", 1) == 0);
        }
        else if(size == 7 && strncmp(receivedChars, "graph", 5) == 0)
        {
            showGraph.setVal(strncmp(receivedChars + 6, "1", 1) == 0);
        }
        else if(size == 6 && strncmp(receivedChars, "wire", 4) == 0)
        {
            showWireframe.setVal(strncmp(receivedChars + 5, "1", 1) == 0);
        }
        else if(size >= 6 && strncmp(receivedChars, "size", 4) == 0)
        {
            //parse string to int
            int tmpVal = atoi(receivedChars + 5);
            //recalc percent to float
            size_factor.setVal(static_cast<float>(tmpVal)/100.0f);
        }

        sgct::MessageHandler::instance()->print("Message: '%s', size: %d\n", receivedChars, size);
    }
}

void externalControlStatusCallback(bool connected)
{
    if(connected)
        sgct::MessageHandler::instance()->print("External control connected.\n");
    else
        sgct::MessageHandler::instance()->print("External control disconnected.\n");
}