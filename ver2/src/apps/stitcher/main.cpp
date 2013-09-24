#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void keyCallback(int key, int action);

std::string texturePaths[8];
size_t textureHandles[] = {0, 0, 0, 0, 0, 0, 0, 0};
size_t activeTexture = 0;
size_t numberOfTextures = 0;

int startIndex;
int stopIndex;
int numberOfDigits;
int iterator;
bool sequence = false;

int counter = 0;
int startFrame = 0;

//sgct_utils::SGCTDome * dome = NULL;

//variables to share across cluster
sgct::SharedBool takeScreenshot(false);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	//parse arguments
	for( int i = 0; i < argc; i++ )
	{
		//fprintf(stderr, "Argument %d: %s (total %d)\n", i, argv[i], argc);
		
		if( strcmp(argv[i], "-tex") == 0 && argc > (i+1) )
		{
			texturePaths[numberOfTextures].assign( argv[i+1] );
			numberOfTextures++;
			sgct::MessageHandler::instance()->print("Adding texture: %s\n", argv[i+1]);
		}
		else if( strcmp(argv[i], "-seq") == 0 && argc > (i+3) )
		{
			sequence = true;
			startIndex		= atoi( argv[i+1] );
			stopIndex		= atoi( argv[i+2] );
			numberOfDigits	= atoi( argv[i+3] );
			iterator = startIndex;
			sgct::MessageHandler::instance()->print("Loading sequence from %d to %d\n", startIndex, stopIndex);
		}
		else if( strcmp(argv[i], "-start") == 0 && argc > (i+1) )
		{
			startFrame = atoi( argv[i+1] );
			sgct::MessageHandler::instance()->print("Start frame set to %d\n", startFrame);
		}
	}

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	gEngine->setClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gEngine->setFisheyeClearColor(0.0f, 0.0f, 0.0f);

	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

	// Main loop
	gEngine->render();

	// Clean up
	//if( dome != NULL )
	//	delete dome;
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	size_t index = counter % numberOfTextures;

	unsigned int texId = sgct::TextureManager::instance()->getTextureByHandle(textureHandles[index]);
	
	if( texId ) //if valid
	{
		//enter ortho mode
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glPushMatrix();
		gluOrtho2D(0.0, 1.0, 0.0, 1.0);
		glMatrixMode(GL_MODELVIEW);

		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
	
		glColor3f(1.0f,1.0f,1.0f);
		glEnable(GL_TEXTURE_2D);

		glBindTexture( GL_TEXTURE_2D, texId );

		if( index == 2 || index == 6 ) //top is wierd
		{
			glBegin(GL_QUADS);
			glTexCoord2d(1.0, 0.0);
			glVertex2d(0.0, 0.0);

			glTexCoord2d(0.0, 0.0);
			glVertex2d(0.0, 1.0);

			glTexCoord2d(0.0, 1.0);
			glVertex2d(1.0, 1.0);

			glTexCoord2d(1.0, 1.0);
			glVertex2d(1.0, 0.0);
			glEnd();
		}
		else
		{
			glBegin(GL_QUADS);
			glTexCoord2d(0.0, 0.0);
			glVertex2d(0.0, 0.0);

			glTexCoord2d(0.0, 1.0);
			glVertex2d(0.0, 1.0);

			glTexCoord2d(1.0, 1.0);
			glVertex2d(1.0, 1.0);

			glTexCoord2d(1.0, 0.0);
			glVertex2d(1.0, 0.0);
			glEnd();
		}

		glDisable(GL_TEXTURE_2D);

		glPopAttrib();

		//exit ortho mode
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
	else //bad texture
	{
		//std::cout << "NoTex: Texture handle:" << textureHandles[index] << " ogl: " << sgct::TextureManager::instance()->getTextureByHandle(textureHandles[index]) << 
		//			" index: " << index << " counter: " << counter << std::endl;
	}

	counter++;
	
	/*glEnable( GL_TEXTURE_2D );
	glPushMatrix();
	glRotatef( 40.0f, 1.0f, 0.0f, 0.0f );
	glTranslatef(0.0f, 0.0f, -5.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureByHandle( activeTexture ) );
	
	float xSize = 8.0f;
	float ySize = xSize * (9.0f / 16.0f);

	//draw the quad
	glBegin(GL_QUADS);
	glTexCoord2d( 0.0, 0.0 );	glVertex3f( -xSize/2.0f, -ySize/2.0f, 0.0f );
	glTexCoord2d( 1.0, 0.0 );	glVertex3f(  xSize/2.0f, -ySize/2.0f, 0.0f );
	glTexCoord2d( 1.0, 1.0 );	glVertex3f(  xSize/2.0f,  ySize/2.0f, 0.0f );
	glTexCoord2d( 0.0, 1.0 );	glVertex3f( -xSize/2.0f,  ySize/2.0f, 0.0f );
	glEnd();
	glPopMatrix();*/

	/*glLineWidth(3.0f);
	glColor3f(0.3f,0.3f,0.3f);
	glDisable( GL_TEXTURE_2D );
	dome->draw();*/
}

void myPreSyncFun()
{
	if(sequence && iterator <= stopIndex)
	{
		for( size_t i=0; i < numberOfTextures; i++)
		{
			char tmpStr[256];
			char digitStr[16];
			char zeros[16];
			zeros[0] = '\0';

			sprintf_s( digitStr, 16, "%d", iterator );
			size_t currentSize = strlen(digitStr);

			for( size_t j=0; j < (numberOfDigits - currentSize); j++ )
			{
				zeros[j] = '0';
				zeros[j+1] = '\0';
			}

			sprintf_s( tmpStr, 256, "%s%s%d.png", 
				texturePaths[i].c_str(), zeros, iterator);

			//fprintf(stderr, "Loading path: %s\n", tmpStr);
			
			//load the texture
			if( !sgct::TextureManager::instance()->loadTexure(textureHandles[i], texturePaths[i], std::string(tmpStr), true, 1) )
			{
				std::cout << "Error: Texture handle:" << textureHandles[i] << " ogl: " << sgct::TextureManager::instance()->getTextureByHandle(textureHandles[i]) << 
					" file: " << std::string(tmpStr) << " count: " << numberOfTextures << std::endl;
			}
		}

		//for( size_t i=0; i < numberOfTextures; i++)
		//	fprintf(stderr, "Handle: %u, index: %u\n", textureHandles[i], sgct::TextureManager::instance()->getTextureByHandle(textureHandles[i]));

		takeScreenshot.setVal( true );
		iterator++;
	}

	counter = 0;
}

void myPostSyncPreDrawFun()
{
	if( takeScreenshot.getVal() )
	{
		gEngine->takeScreenshot();
		takeScreenshot.setVal( false );
	}
}

void myInitOGLFun()
{
	sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::instance()->setCompression(sgct::TextureManager::No_Compression);
	sgct::TextureManager::instance()->setOverWriteMode(true);
	//sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);

	for (std::size_t i = 0; i < gEngine->getNumberOfWindows(); i++)
		gEngine->getWindowPtr(i)->setScreenShotNumber( startFrame );

	//load all textures
	if(!sequence)
	{
		for( size_t i=0; i<numberOfTextures; i++)
			sgct::TextureManager::instance()->loadTexure(textureHandles[i], texturePaths[i], texturePaths[i], true, 1);
	}

	//dome = new sgct_utils::SGCTDome(7.4f, 180.0f, 360, 90);
	
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_COLOR_MATERIAL );
	glDisable( GL_LIGHTING );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void myEncodeFun()
{
	sgct::SharedData::instance()->writeBool( &takeScreenshot );
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readBool( &takeScreenshot );
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case 'P':
		case SGCT_KEY_F10:
			if(action == SGCT_PRESS)
				takeScreenshot.setVal(true);
			break;
		}
	}
}
