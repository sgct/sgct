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

void keyCallback(int key, int action);
void drawTerrainGrid( float width, float height, unsigned int wRes, unsigned int dRes );

unsigned int myTextureIds[2];
int myTextureLocations[2];
int curr_timeLoc;
GLuint myTerrainDisplayList = 0;

//variables to share across cluster
double curr_time = 0.0;
bool wireframe = false;
bool info = false;
bool stats = false;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

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

	sgct::SharedData::Instance()->setEncodeFunction( myEncodeFun );
	sgct::SharedData::Instance()->setDecodeFunction( myDecodeFun );

	// Main loop
	gEngine->render();

	// Clean up
	glDeleteLists(myTerrainDisplayList, 1);
	sgct::ShaderManager::Destroy();
	sgct::TextureManager::Destroy();
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	glTranslatef( 0.0f, -0.15f, 2.5f );
	glRotatef( static_cast<float>( curr_time ) * 8.0f, 0.0f, 1.0f, 0.0f );

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex( myTextureIds[0] ));
	glEnable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex( myTextureIds[1] ));
	glEnable(GL_TEXTURE_2D);

	//set current shader program
	sgct::ShaderManager::Instance()->bindShader( "Heightmap" );
	glUniform1f( curr_timeLoc, static_cast<float>( curr_time ) );

	glCallList(myTerrainDisplayList);

	//unset current shader program
	sgct::ShaderManager::Instance()->unBindShader();

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		curr_time = sgct::Engine::getTime();
	}
}

void myPostSyncPreDrawFun()
{
	gEngine->setWireframe(wireframe);
	gEngine->setDisplayInfoVisibility(info);
	gEngine->setStatsGraphVisibility(stats);
}

void myInitOGLFun()
{
	glEnable( GL_DEPTH_TEST );
	//glDepthMask( GL_TRUE );
	//glDisable( GL_CULL_FACE );
	glEnable( GL_NORMALIZE );
	glEnable( GL_COLOR_MATERIAL );
	glShadeModel( GL_SMOOTH );
	glEnable( GL_LIGHTING );

	//Set up light 0
	glEnable(GL_LIGHT0);
	GLfloat lightPosition[] = { -2.0f, 3.0f, 10.0f, 1.0f };
	GLfloat lightAmbient[]= { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat lightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat lightSpecular[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

	//create and compile display list
	myTerrainDisplayList = glGenLists(1);
	glNewList(myTerrainDisplayList, GL_COMPILE);
	//draw the terrain once to add it to the display list
	drawTerrainGrid( 1.0f, 1.0f, 256, 256 );
	glEndList();

	//sgct::TextureManager::Instance()->setAnisotropicFilterSize(4.0f);
	sgct::TextureManager::Instance()->loadTexure(myTextureIds[0], "heightmap", "heightmap.png", true, 0);
	sgct::TextureManager::Instance()->loadTexure(myTextureIds[1], "normalmap", "normalmap.png", true, 0);

	sgct::ShaderManager::Instance()->addShader( "Heightmap", "heightmap.vert", "heightmap.frag" );

	sgct::ShaderManager::Instance()->bindShader( "Heightmap" );
	myTextureLocations[0] = -1;
	myTextureLocations[1] = -1;
	curr_timeLoc = -1;
	myTextureLocations[0] = sgct::ShaderManager::Instance()->getShader( "Heightmap").getUniformLocation( "hTex" );
	myTextureLocations[1] = sgct::ShaderManager::Instance()->getShader( "Heightmap").getUniformLocation( "nTex" );
	curr_timeLoc = sgct::ShaderManager::Instance()->getShader( "Heightmap").getUniformLocation( "curr_time" );

	glUniform1i( myTextureLocations[0], 0 );
	glUniform1i( myTextureLocations[1], 1 );
	sgct::ShaderManager::Instance()->unBindShader();
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble( curr_time );
	sgct::SharedData::Instance()->writeBool( wireframe );
	sgct::SharedData::Instance()->writeBool( info );
	sgct::SharedData::Instance()->writeBool( stats );
}

void myDecodeFun()
{
	curr_time = sgct::SharedData::Instance()->readDouble();
	wireframe = sgct::SharedData::Instance()->readBool();
	info = sgct::SharedData::Instance()->readBool();
	stats = sgct::SharedData::Instance()->readBool();
}

/*!
Will draw a flat surface that can be used for the heightmapped terrain.
@param	width	Width of the surface
@param	depth	Depth of the surface
@param	wRes	Width resolution of the surface
@param	dRes	Depth resolution of the surface
*/
void drawTerrainGrid( float width, float depth, unsigned int wRes, unsigned int dRes )
{
	float wStart = -width * 0.5f;
	float dStart = -depth * 0.5f;

	float dW = width / static_cast<float>( wRes );
	float dD = depth / static_cast<float>( dRes );

	for( unsigned int depthIndex = 0; depthIndex < dRes; ++depthIndex )
    {
		float dPosLow = dStart + dD * static_cast<float>( depthIndex );
		float dPosHigh = dStart + dD * static_cast<float>( depthIndex + 1 );
		float dTexCoordLow = depthIndex / static_cast<float>( dRes );
		float dTexCoordHigh = (depthIndex+1) / static_cast<float>( dRes );

		glBegin( GL_TRIANGLE_STRIP );

		glNormal3f(0.0f,1.0f,0.0);
        for( unsigned widthIndex = 0; widthIndex < wRes; ++widthIndex )
        {
			float wPos = wStart + dW * static_cast<float>( widthIndex );
			float wTexCoord = widthIndex / static_cast<float>( wRes );

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, wTexCoord, dTexCoordLow);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, wTexCoord, dTexCoordLow);
			glVertex3f( wPos, 0.0f, dPosLow );

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, wTexCoord, dTexCoordHigh);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, wTexCoord, dTexCoordHigh);
			glVertex3f( wPos, 0.0f, dPosHigh );
        }

		glEnd();
    }
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case 'S':
			if(action == GLFW_PRESS)
				stats = !stats;
			break;

		case 'I':
			if(action == GLFW_PRESS)
				info = !info;
			break;

		case 'W':
			if(action == GLFW_PRESS)
				wireframe = !wireframe;
			break;

		case 'Q':
			if(action == GLFW_PRESS)
				gEngine->terminate();
			break;
		}
	}
}
