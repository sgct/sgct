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

size_t myTextureHandle;
sgct_utils::SGCTBox * myBox = NULL;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);

//shader locs
int m_textureID = -1;
int m_worldMatrixID = -1;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setCleanUpFunction( myCleanUpFun );

	//force normal & position textures to be created & used in rendering loop
	sgct::SGCTSettings::instance()->setUseDepthTexture(true);
	sgct::SGCTSettings::instance()->setUseNormalTexture(true);
	sgct::SGCTSettings::instance()->setUsePositionTexture(true);

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	double speed = 25.0;
	
	glTranslatef(0.0f, 0.0f, -3.0f);
	glRotated(curr_time.getVal() * speed, 0.0, -1.0, 0.0);
	glRotated(curr_time.getVal() * (speed/2.0), 1.0, 0.0, 0.0);
	glColor3f(1.0f,1.0f,1.0f);

	float worldMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, worldMatrix);
	
	glActiveTexture(GL_TEXTURE0);
	//glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureByName("box") );
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureByHandle(myTextureHandle) );
	
	//set MRT shader program
	sgct::ShaderManager::instance()->bindShaderProgram("MRT");

	glUniform1i(m_textureID, 0);
	glUniformMatrix4fv(m_worldMatrixID, 1, false, worldMatrix);

	//draw the box
	myBox->draw();

	//unset current shader program
	sgct::ShaderManager::instance()->unBindShaderProgram();
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
	sgct::ShaderManager::instance()->addShaderProgram("MRT", "mrt.vert", "mrt.frag");
	sgct::ShaderManager::instance()->bindShaderProgram("MRT");

	m_textureID = sgct::ShaderManager::instance()->getShaderProgram("MRT").getUniformLocation("tDiffuse");
	m_worldMatrixID = sgct::ShaderManager::instance()->getShaderProgram("MRT").getUniformLocation("WorldMatrix");

	sgct::ShaderManager::instance()->unBindShaderProgram();
	
	sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
	sgct::TextureManager::instance()->loadTexure(myTextureHandle, "box", "box.png", true);

	myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
	//myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::CubeMap);
	//myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::SkyBox);
	
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_COLOR_MATERIAL );
	glDisable( GL_LIGHTING );
	glEnable( GL_CULL_FACE );
	glEnable( GL_TEXTURE_2D );

	//Set up backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); //our polygon winding is counter clockwise
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
}
