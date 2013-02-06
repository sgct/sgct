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

void drawScene();

//FBO-stuff
void createFBO();
void resizeFBO();
void createTextures();
int width, height;

unsigned int fbo = 0;
unsigned int renderBuffer = 0;
unsigned int depthBuffer = 0;
unsigned int numberOfTargets = 1;
unsigned int * textureTargets;

unsigned int myTextureIndex;
sgct_utils::SGCTBox * myBox = NULL;

//variables to share across cluster
double curr_time = 0.0;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::Instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::Instance()->setDecodeFunction(myDecodeFun);

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;
	if(myBox != NULL) delete myBox;

	//delete buffers
	glDeleteFramebuffers(1,	&fbo);
	glDeleteRenderbuffers(1, &renderBuffer);
	glGenRenderbuffers(1, &depthBuffer);

	//cleanup textures
	glDeleteTextures(numberOfTargets, &textureTargets[0]);
	delete [] textureTargets;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	//render a quad in ortho/2D mode with target texture

	//enter ortho mode (2D projection)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glPushMatrix();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, textureTargets[0]);

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
	glEnd();

	//restore
	glPopAttrib();

	//exit ortho mode
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void drawScene()
{
	glPushAttrib( GL_ENABLE_BIT );
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	double speed = 25.0;
	
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -3.0f);
	glRotated(curr_time * speed, 0.0, -1.0, 0.0);
	glRotated(curr_time * (speed/2.0), 1.0, 0.0, 0.0);
	glColor3f(1.0f, 1.0f, 1.0f);
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(myTextureIndex) );
	//draw the box
	myBox->draw();
	glPopMatrix();

	glPopAttrib();
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
	//bind fbo
	glBindFramebuffer(GL_FRAMEBUFFER, fbo );
	
	sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::Instance()->getThisNodePtr();
	sgct_core::Viewport * tmpVP;

	int vp_coords[4];

	for(unsigned int i=0; i < thisNode->getNumberOfViewports(); i++)
	{
		tmpVP = thisNode->getViewport(i);
		if( tmpVP->isEnabled() )
		{	
			//bind texture to frame buffer
			unsigned int id = gEngine->getTextureTargetIndex(i, 
				gEngine->isStereo() ? sgct_core::Frustu
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureTargets[id], 0);
			
			gEngine->getViewportCoords(i, vp_coords);
			glViewport(vp_coords[0], vp_coords[1], vp_coords[2], vp_coords[3]);

			glMatrixMode(GL_PROJECTION);

			glLoadMatrixf( glm::value_ptr(tmpVP->getProjectionMatrix( sgct_core::Frustum::Mono )) );

			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf( glm::value_ptr( gEngine->getSceneTransform() ) );

			//clear
			glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//draw scene to texture target
			drawScene();
		}
	}

	//unbind FBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void myInitOGLFun()
{
	sgct::TextureManager::Instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::Instance()->setCompression(sgct::TextureManager::S3TC_DXT);
	sgct::TextureManager::Instance()->loadTexure(myTextureIndex, "box", "box.png", true);

	myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::Regular);
	//myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::CubeMap);
	//myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::SkyBox);
	
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_COLOR_MATERIAL );
	glDisable( GL_LIGHTING );

	//Set up backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); //our polygon winding is counter clockwise

	numberOfTargets = gEngine->getNumberOfTextureTargets();
	sgct::MessageHandler::Instance()->print("Number of targets: %d\n", numberOfTargets);

	//create array of texture target indexes
	textureTargets = new unsigned int[ numberOfTargets ];
	//init to 0
	memset(textureTargets, 0, numberOfTargets * sizeof(unsigned int));

	createFBO();
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble(curr_time);
}

void myDecodeFun()
{
	curr_time = sgct::SharedData::Instance()->readDouble();
}

void createFBO()
{
	//get the dimensions
	gEngine->getFBODimensions(width, height);
	
	//create targets
	createTextures();
	
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &renderBuffer);
	glGenRenderbuffers(1, &depthBuffer);

	sgct::MessageHandler::Instance()->print("Creating a %dx%d fbo...\n", width, height);

	//setup color buffer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderBuffer);	

	//setup depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height );
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	//Does the GPU support current FBO configuration?
	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
		sgct::MessageHandler::Instance()->print("Something went wrong creating FBO!\n");

	//unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resizeFBO()
{
	//delete buffers
	glDeleteFramebuffers(1,	&fbo);
	glDeleteRenderbuffers(1, &renderBuffer);
	glGenRenderbuffers(1, &depthBuffer);

	fbo = 0;
	renderBuffer = 0;
	depthBuffer = 0;

	//delete textures
	glDeleteTextures(numberOfTargets,	&textureTargets[0]);
	memset(textureTargets, 0, numberOfTargets * sizeof(unsigned int));

	//create textures
	createTextures();

	//create FBO
	createFBO();
}

void createTextures()
{
	glEnable(GL_TEXTURE_2D);

	//allocate
	glGenTextures(numberOfTargets, &textureTargets[0]);

	for( unsigned int i=0; i<numberOfTargets; i++ )
	{
		glBindTexture(GL_TEXTURE_2D, textureTargets[i]);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); //must be linear if warping, blending or fix resolution is used
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	}

	gEngine->checkForOGLErrors();
	sgct::MessageHandler::Instance()->print("%d target textures created.\n", numberOfTargets);

	glDisable(GL_TEXTURE_2D);
}
