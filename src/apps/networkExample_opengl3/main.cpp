#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
void keyCallback(int key, int action);

void parseArguments(int& argc, char**& argv);
void connect();
void disconnect();
void sendData(const void * data, int length, int packageId);
void sendTestMessage();
void networkLoop(void * arg);

tthread::thread * connectionThread = NULL;
tthread::atomic<bool> connected;
tthread::atomic<bool> running;

//network callbacks
void networkConnectionUpdated(sgct_core::SGCTNetwork * conn);
void networkAck(int packageId, int clientId);
void networkDecode(void * receivedData, int receivedlength, int packageId, int clientId);

sgct_utils::SGCTBox * myBox = NULL;
GLint Matrix_Loc = -1;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);

std::string port;
std::string address;
bool server = false;
sgct_core::SGCTNetwork * networkPtr = NULL;

std::pair<double, int> timerData;

int main( int argc, char* argv[] )
{
    connected = false;
    running = true;
    
    gEngine = new sgct::Engine( argc, argv );

    parseArguments(argc, argv);

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setCleanUpFunction( myCleanUpFun );
    gEngine->setKeyboardCallbackFunction(keyCallback);

    if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
    {
        delete gEngine;
        return EXIT_FAILURE;
    }

    connectionThread = new tthread::thread(networkLoop, NULL);

    sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
    sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

    // Main loop
    gEngine->render();

    // Clean up
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void parseArguments(int& argc, char**& argv)
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-port") == 0 && argc >(i + 1))
        {
            port.assign(argv[i + 1]);
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Setting port to: %s\n", port.c_str());
        }
        else if (strcmp(argv[i], "-address") == 0 && argc >(i + 1))
        {
            address.assign(argv[i + 1]);
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Setting address to: %s\n", address.c_str());
        }
        else if (strcmp(argv[i], "--server") == 0)
        {
            server = true;
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "This computer will host the connection.\n");
        }
    }
}

void myDrawFun()
{
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );

    double speed = 0.44;

    //create scene transform (animation)
    glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
    scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));
    scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * (speed/2.0) ), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 MVP = gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box") );

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);

    //draw the box
    myBox->draw();

    sgct::ShaderManager::instance()->unBindShaderProgram();

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
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
    //myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::CubeMap);
    //myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::SkyBox);

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise

    sgct::ShaderManager::instance()->addShaderProgram( "xform",
            "box.vert",
            "box.frag" );

    sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

    Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
    GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
    glUniform1i( Tex_Loc, 0 );

    sgct::ShaderManager::instance()->unBindShaderProgram();

    for (std::size_t i = 0; i < gEngine->getNumberOfWindows(); i++)
        gEngine->getWindowPtr(i)->setWindowTitle(server ? "SERVER" : "CLIENT");
}

void myEncodeFun()
{
    sgct::SharedData::instance()->writeDouble(&curr_time);
}

void myDecodeFun()
{
    sgct::SharedData::instance()->readDouble(&curr_time);
}

void myCleanUpFun()
{
    if(myBox != NULL)
        delete myBox;

    running = false;

    if (connectionThread != NULL)
    {
        if (networkPtr)
            networkPtr->initShutdown();
        connectionThread->join();

        delete connectionThread;
        connectionThread = NULL;
    }

    disconnect();
}

void networkConnectionUpdated(sgct_core::SGCTNetwork * conn)
{
    if (conn->isServer())
    {
        //wake up the connection handler thread on server
        //if node disconnects to enable reconnection
        conn->mStartConnectionCond.notify_all();
    }

    connected = conn->isConnected();

    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Network is %s.\n", conn->isConnected() ? "connected" : "disconneced");
}

void networkAck(int packageId, int clientId)
{
    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Network package %d is received.\n", packageId);

    if (timerData.second == packageId)
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Loop time: %lf ms\n", (sgct::Engine::getTime() - timerData.first)*1000.0);
}

void networkDecode(void * receivedData, int receivedlength, int packageId, int clientId)
{
    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Network decoding package %d...\n", packageId);

    std::string test;
    test.insert(0, reinterpret_cast<char *>(receivedData), receivedlength);

    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Message: \"%s\"\n", test.c_str());
}

void networkLoop(void * arg)
{
    connect();

    //if client try to connect to server even after disconnection
    if (!server)
    {
        while (running.load())
        {
            if (connected.load() == false)
            {
                connect();
            }
            else
            {
                //just check if connected once per second
                tthread::this_thread::sleep_for(tthread::chrono::seconds(1));
            }
        }
    }
}

void connect()
{
    if (!gEngine->isMaster())
        return;
    
    if (port.empty())
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
            "Network error: No port set!\n");
        return;
    }

    //no need to specify the address on the host/server
    if (!server && address.empty())
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
            "Network error: No address set!\n");
        return;
    }

    //reset if set
    if (networkPtr)
    {
        delete networkPtr;
        networkPtr = NULL;
    }
    
    //allocate
    networkPtr = new sgct_core::SGCTNetwork();

    //init
    try
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Initiating network connection at port %s.\n", port.c_str());

        sgct_cppxeleven::function< void(sgct_core::SGCTNetwork *) > updateCallback;
        updateCallback = networkConnectionUpdated;
        networkPtr->setUpdateFunction(updateCallback);

        sgct_cppxeleven::function< void(void*, int, int, int) > decodeCallback;
        decodeCallback = networkDecode;
        networkPtr->setPackageDecodeFunction(decodeCallback);

        sgct_cppxeleven::function< void(int, int) > ackCallback;
        ackCallback = networkAck;
        networkPtr->setAcknowledgeFunction(ackCallback);

        //must be inited after binding
        networkPtr->init(port, address, server, sgct_core::SGCTNetwork::DataTransfer);
    }
    catch (const char * err)
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Network error: %s\n", err);
        networkPtr->initShutdown();
        tthread::this_thread::sleep_for(tthread::chrono::seconds(1));
        networkPtr->closeNetwork(true);
        return;
    }

    connected = true;
}

void disconnect()
{
    if (networkPtr)
    {
        networkPtr->initShutdown();

        //wait for all nodes callbacks to run
        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(250));

        //wait for threads to die
        networkPtr->closeNetwork(false);
        delete networkPtr;
        networkPtr = NULL;
    }
}

void sendData(const void * data, int length, int packageId)
{
    if (networkPtr)
    {
        sgct_core::NetworkManager::instance()->transferData(data, length, packageId, networkPtr);
        timerData.first = sgct::Engine::getTime();
        timerData.second = packageId;
    }
}

void sendTestMessage()
{
    std::string test("What's up?");
    static int counter = 0;
    sendData(test.data(), static_cast<int>(test.size()), counter);
    counter++;
}

void keyCallback(int key, int action)
{
    if (gEngine->isMaster())
    {
        switch (key)
        {
        case SGCT_KEY_SPACE:
            if (action == SGCT_PRESS)
                sendTestMessage();
            break;
        }
    }
}
