#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
void keyCallback(int key, int action);

void myDataTransferDecoder(const char * receivedData, int receivedlength, int packageId, int clientIndex);
void myDataTransferStatus(bool connected, int clientIndex);
void myDataTransferAcknowledge(int packageId, int clientIndex);

void startDataTransfer();
void loadTexture(sgct_core::Image * imPtr);
void threadWorker(void *arg);

//sync variables
sgct::SharedBool info(false);
sgct::SharedBool stats(false);
//other mutex variables
sgct::SharedBool running(true);
sgct::SharedBool transfere(false);
sgct::SharedBool loadImage(false);

size_t myTextureHandle;
sgct_utils::SGCTBox * myBox = NULL;
GLint Matrix_Loc = -1;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);

int main( int argc, char* argv[] )
{
	//sgct::MessageHandler::instance()->setNotifyLevel(sgct::MessageHandler::NOTIFY_ALL);
	
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
	gEngine->setCleanUpFunction( myCleanUpFun );
	gEngine->setKeyboardCallbackFunction(keyCallback);

	gEngine->setDataTransferCallback(myDataTransferDecoder);
	gEngine->setDataTransferStatusCallback(myDataTransferStatus);
	gEngine->setDataAcknowledgeCallback(myDataTransferAcknowledge);

	if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

	tthread::thread * loadThread = new (std::nothrow) tthread::thread(threadWorker, NULL);

	// Main loop
	gEngine->render();

	running.setVal(false);

	if (loadThread)
	{
		loadThread->join();
		delete loadThread;
	}

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	double speed = 25.0;

	//create scene transform (animation)
	glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
	scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));
	scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * (speed/2.0) ), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 MVP = gEngine->getActiveModelViewProjectionMatrix() * scene_mat;

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureByName("box") );
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureByHandle(myTextureHandle) );

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

void myPostSyncPreDrawFun()
{
	gEngine->setDisplayInfoVisibility(info.getVal());
	gEngine->setStatsGraphVisibility(stats.getVal());
}

void myInitOGLFun()
{
	sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
	sgct::TextureManager::instance()->loadTexure(myTextureHandle, "box", "box.png", true);

	myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
	//myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::CubeMap);
	//myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::SkyBox);

	//Set up backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); //our polygon winding is counter clockwise

	sgct::ShaderManager::instance()->addShaderProgram( "xform",
			"xform.vert",
			"xform.frag" );

	sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

	Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
	GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
	glUniform1i( Tex_Loc, 0 );

	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void myEncodeFun()
{
	sgct::SharedData::instance()->writeDouble(&curr_time);
	sgct::SharedData::instance()->writeBool(&info);
	sgct::SharedData::instance()->writeBool(&stats);
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readDouble(&curr_time);
	sgct::SharedData::instance()->readBool(&info);
	sgct::SharedData::instance()->readBool(&stats);
}

void myCleanUpFun()
{
	if(myBox != NULL)
		delete myBox;
}

void keyCallback(int key, int action)
{
	if (gEngine->isMaster())
	{
		switch (key)
		{
		case SGCT_KEY_S:
			if (action == SGCT_PRESS)
				stats.toggle();
			break;

		case SGCT_KEY_I:
			if (action == SGCT_PRESS)
				info.toggle();
			break;
                
        case SGCT_KEY_SPACE:
            if (action == SGCT_PRESS)
                transfere.setVal(true);
            break;
		}
	}
}

void myDataTransferDecoder(const char * receivedData, int receivedlength, int packageId, int clientIndex)
{
	sgct::MessageHandler::instance()->print("Decoding %d bytes in transfer id: %d on node %d\n", receivedlength, packageId, clientIndex);

	if (receivedlength > 255)
		sgct::MessageHandler::instance()->print("Test %d\n", receivedData[255]);
    
    //signal load image
    loadImage.setVal(true);
}

void myDataTransferStatus(bool connected, int clientIndex)
{
	sgct::MessageHandler::instance()->print("Transfer node %d is %s.\n", clientIndex, connected ? "connected" : "disconnected");
}

void myDataTransferAcknowledge(int packageId, int clientIndex)
{
	sgct::MessageHandler::instance()->print("Transfer id: %d is completed on node %d.\n", packageId, clientIndex);
}

void threadWorker(void *arg)
{
	while (running.getVal())
	{
		//runs only on master
        if (transfere.getVal())
        {
			startDataTransfer();
            transfere.setVal(false);
            
            //load texture on master
            loadTexture();
        }
        
        if(loadImage.getVal())
        {
            //load texture on slave
            loadTexture();
            
            loadImage.setVal(false);
        }

		sgct::Engine::sleep(0.1); //ten iteration per second
	}
}

void startDataTransfer()
{
	static int packageId = 0;

	int size = 4096 * 4096 * 3; //4k image size
	char * data = new (std::nothrow) char[size];

	if (data)
	{
		data[255] = 55;
		gEngine->transferDataBetweenNodes(data, size, packageId);

		//clean up
		delete data;
	}

	packageId++;
}

void loadTexture(sgct_core::Image * imPtr)
{

}