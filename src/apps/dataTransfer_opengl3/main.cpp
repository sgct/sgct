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
void contextCreationCallback(GLFWwindow * win);

void myDataTransferDecoder(const char * receivedData, int receivedlength, int packageId, int clientIndex);
void myDataTransferStatus(bool connected, int clientIndex);
void myDataTransferAcknowledge(int packageId, int clientIndex);

void startDataTransfer();
void uploadTexture();
void threadWorker(void *arg);

tthread::thread * loadThread;
tthread::mutex mutex;
GLFWwindow * hiddenWindow;
GLFWwindow * sharedWindow;
sgct_core::Image * transImg = NULL;


//sync variables
sgct::SharedBool info(false);
sgct::SharedBool stats(false);
sgct::SharedInt texIndex(-1);

//other mutex variables
sgct::SharedInt currentPackage(-1);
sgct::SharedBool running(true);
sgct::SharedBool transfere(false);
sgct::SharedVector<std::string> imagePaths;
sgct::SharedVector<GLuint> texIds;
sgct::SharedInt localTexIndex(-1);
double sendTimer = 0.0;

size_t myTextureHandle;
sgct_utils::SGCTBox * myBox = NULL;
GLint Matrix_Loc = -1;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);

int main( int argc, char* argv[] )
{
	//sgct::MessageHandler::instance()->setNotifyLevel(sgct::MessageHandler::NOTIFY_ALL);
	
	gEngine = new sgct::Engine( argc, argv );
    
    imagePaths.addVal("test_00.jpg");
    imagePaths.addVal("test_01.jpg");
    imagePaths.addVal("test_02.jpg");

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
	gEngine->setCleanUpFunction( myCleanUpFun );
	gEngine->setKeyboardCallbackFunction(keyCallback);
    gEngine->setContextCreationCallback( contextCreationCallback );

	if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}
    
    gEngine->setDataTransferCallback(myDataTransferDecoder);
	gEngine->setDataTransferStatusCallback(myDataTransferStatus);
	gEngine->setDataAcknowledgeCallback(myDataTransferAcknowledge);
    //gEngine->setDataTransferCompression(true, 6);

    sgct::SharedData::instance()->setCompression(true);
	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

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
	
    if(texIndex.getVal() != -1)
        glBindTexture(GL_TEXTURE_2D, texIds.getValAt(texIndex.getVal()));
    else
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
        texIndex.setVal( localTexIndex.getVal() );
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
    sgct::SharedData::instance()->writeInt(&texIndex);
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readDouble(&curr_time);
	sgct::SharedData::instance()->readBool(&info);
	sgct::SharedData::instance()->readBool(&stats);
    sgct::SharedData::instance()->readInt(&texIndex);
}

void myCleanUpFun()
{
	if(myBox != NULL)
		delete myBox;
    
    for(std::size_t i=0; i < texIds.getSize(); i++)
    {
        GLuint tex = texIds.getValAt(i);
        glDeleteTextures(1, &tex);
        texIds.setValAt(i, GL_FALSE);
    }
    texIds.clear();
    
    
    if(hiddenWindow)
        glfwDestroyWindow(hiddenWindow);
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

void contextCreationCallback(GLFWwindow * win)
{
    glfwWindowHint( GLFW_VISIBLE, GL_FALSE );
    
    sharedWindow = win;
    hiddenWindow = glfwCreateWindow( 1, 1, "Thread Window", NULL, sharedWindow );
    
    if( !hiddenWindow )
    {
        sgct::MessageHandler::instance()->print("Failed to create loader context!\n");
    }
    
    //restore to normal
    glfwMakeContextCurrent( sharedWindow );
    
    if( gEngine->isMaster() )
        loadThread = new (std::nothrow) tthread::thread(threadWorker, NULL);
}

void myDataTransferDecoder(const char * receivedData, int receivedlength, int packageId, int clientIndex)
{
	sgct::MessageHandler::instance()->print("Decoding %d bytes in transfer id: %d on node %d\n", receivedlength, packageId, clientIndex);

	currentPackage.setVal(packageId);
    
    int offset = 0;
    int w, h, c;
    
    if( receivedlength > sizeof(int)*3 )
    {
        memcpy(&w, receivedData + offset, sizeof(int)); offset += sizeof(int);
        memcpy(&h, receivedData + offset, sizeof(int)); offset += sizeof(int);
        memcpy(&c, receivedData + offset, sizeof(int)); offset += sizeof(int);
    
        int totalSize = w * h * c + sizeof(int)*3;
        if( receivedlength >= totalSize )
        {
            unsigned char * data = new (std::nothrow) unsigned char[w*h*c];
            
            for( int i = 0; i<(h*c); i++ )
            {
                memcpy(data + i*w, receivedData + offset + i*w, w);
            }
            
            mutex.lock();
            if(!transImg)
            {
                transImg = new (std::nothrow) sgct_core::Image();
                transImg->setSize(w, h);
                transImg->setChannels(c);
                transImg->setDataPtr(data);
            }
            mutex.unlock();
            
            uploadTexture();
        }
    }
}

void myDataTransferStatus(bool connected, int clientIndex)
{
	sgct::MessageHandler::instance()->print("Transfer node %d is %s.\n", clientIndex, connected ? "connected" : "disconnected");
}

void myDataTransferAcknowledge(int packageId, int clientIndex)
{
	sgct::MessageHandler::instance()->print("Transfer id: %d is completed on node %d.\n", packageId, clientIndex);
    
    static int counter = 0;
    if( packageId == currentPackage.getVal())
    {
        counter++;
        if( counter == (sgct_core::ClusterManager::instance()->getNumberOfNodes()-1) )
        {
            int tmpIndex = localTexIndex.getVal();
            tmpIndex++;
            localTexIndex.setVal(tmpIndex);
            counter = 0;
            
            sgct::MessageHandler::instance()->print("Time to distribute and upload textures on cluster: %f ms\n", (sgct::Engine::getTime() - sendTimer)*1000.0);
        }
    }
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
            uploadTexture();
            
            if(sgct_core::ClusterManager::instance()->getNumberOfNodes() == 1) //no cluster
            {
                int tmpIndex = localTexIndex.getVal();
                tmpIndex++;
                localTexIndex.setVal(tmpIndex);
            }
        }

		sgct::Engine::sleep(0.1); //ten iteration per second
	}
}

void startDataTransfer()
{
    //iterate
    int id = currentPackage.getVal();
    id++;
    currentPackage.setVal(id);
    
    mutex.lock();
    
    //make sure to keep within bounds
    if(static_cast<int>(imagePaths.getSize()) > id)
    {
        sendTimer = sgct::Engine::getTime();
        
        transImg = new (std::nothrow) sgct_core::Image();
        transImg->load(imagePaths.getValAt(static_cast<std::size_t>(id)).c_str());
        
        if (transImg->getData())
        {
            int w, h, c, offset;
            
            w = transImg->getWidth();
            h = transImg->getHeight();
            c = transImg->getChannels();
            offset = 0;
            
            int imSize = w * h * c;
            int totalSize = imSize + sizeof(int)*3;
            
            //create datablock
            char * data = new (std::nothrow) char[totalSize];
            if(data)
            {
                memcpy(data + offset, &w, sizeof(int)); offset += sizeof(int);
                memcpy(data + offset, &h, sizeof(int)); offset += sizeof(int);
                memcpy(data + offset, &c, sizeof(int)); offset += sizeof(int);
                for( int i = 0; i<(h*c); i++ )
                {
                    memcpy(data + offset + i*w, transImg->getData() + i*w, w);
                }
            
                gEngine->transferDataBetweenNodes(data, totalSize, id);
                delete data;
                data = NULL;
            }
        }
    }
    
    mutex.unlock();
}

void uploadTexture()
{
    mutex.lock();
    
    if( transImg )
    {
        glfwMakeContextCurrent( hiddenWindow );
        
        //create texture
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        GLenum internalformat;
        GLenum type;
        switch( transImg->getChannels() )
        {
            case 1:
                internalformat = GL_R8;
                type = GL_RED;
                break;
                
            case 2:
                internalformat = GL_RG8;
                type = GL_RG;
                break;
                
            case 3:
            default:
                internalformat = GL_RGB8;
                type = GL_BGR;
                break;
                
            case 4:
                internalformat = GL_RGBA8;
                type = GL_BGRA;
                break;
        }
        
        int mipMapLevels = 8;
        glTexStorage2D(GL_TEXTURE_2D, mipMapLevels, internalformat, transImg->getWidth(), transImg->getHeight());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, transImg->getWidth(), transImg->getHeight(), type, GL_UNSIGNED_BYTE, transImg->getData());
        
        //glTexImage2D(GL_TEXTURE_2D, 0, internalformat, transImg->getWidth(), transImg->getHeight(), 0, type, GL_UNSIGNED_BYTE, transImg->getData());
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapLevels-1);
		
		glGenerateMipmap( GL_TEXTURE_2D ); //allocate the mipmaps
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        //unbind
        glBindTexture(GL_TEXTURE_2D, GL_FALSE);
        
        sgct::MessageHandler::instance()->print("Texture id %d loaded (%dx%dx%d).\n", tex, transImg->getWidth(), transImg->getHeight(), transImg->getChannels());
        
        texIds.addVal(tex);
        
        delete transImg;
        transImg = NULL;
        
        glFinish();
        
        //restore
        glfwMakeContextCurrent( sharedWindow );
    }
    
    mutex.unlock();
}