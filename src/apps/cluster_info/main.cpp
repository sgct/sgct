#include "sgct.h"

void myDrawFun();
void myInitOGLFun();
void keyCallback(int key, int action);

void initColors();

sgct::Engine * gEngine;
glm::vec3 mColors[6];
sgct_utils::SGCTDome * dome;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );

	initColors();

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{	
	size_t numberOfNodes = sgct_core::ClusterManager::Instance()->getNumberOfNodes();
	int counter = 0;
	glLineWidth(5.0);
	for(size_t i=0; i<numberOfNodes; i++)
	{
		sgct_core::SGCTNode * node = sgct_core::ClusterManager::Instance()->getNodePtr(i);
		size_t numberOfViewports = node->getNumberOfViewports();
		if( node != sgct_core::ClusterManager::Instance()->getThisNodePtr() )
			for(size_t j=0; j<numberOfViewports; j++)
			{
				if( node->getViewport(j)->isEnabled() )
				{
					glm::vec3 coorners[4];
					coorners[0] = node->getViewport(j)->getViewPlaneCoords( sgct_core::Viewport::LowerLeft );
					coorners[1] = node->getViewport(j)->getViewPlaneCoords( sgct_core::Viewport::UpperLeft );
					coorners[2] = node->getViewport(j)->getViewPlaneCoords( sgct_core::Viewport::UpperRight );
					coorners[3] = coorners[2] - (coorners[1] - coorners[0]);

					glColor4f( mColors[ counter%6 ].r, mColors[ counter%6 ].g, mColors[ counter%6 ].b, 1.0f);
					glBegin(GL_LINE_LOOP);
						glVertex3f( coorners[0].x, coorners[0].y, coorners[0].z);
						glVertex3f( coorners[1].x, coorners[1].y, coorners[1].z);
						glVertex3f( coorners[2].x, coorners[2].y, coorners[2].z);
						glVertex3f( coorners[3].x, coorners[3].y, coorners[3].z);
					glEnd();

					counter++;
				}
			}
	}

	glColor4f( 1.0f, 1.0f, 1.0f, 0.3f);
	glm::vec3 userPos = sgct_core::ClusterManager::Instance()->getUserPtr()->getPos();
	glPushMatrix();
	glTranslatef( userPos.x, userPos.y, userPos.z );
	dome->draw();
	glPopMatrix();

	/*counter = 0;
	for(size_t i=0; i<numberOfNodes; i++)
	{
		sgct_core::SGCTNode * node = sgct_core::ClusterManager::Instance()->getNodePtr(i);
		size_t numberOfViewports = node->getNumberOfViewports();
		if( node != sgct_core::ClusterManager::Instance()->getThisNodePtr() )
			for(size_t j=0; j<numberOfViewports; j++)
			{
				if( node->getViewport(j)->isEnabled() )
				{
					glm::vec3 coorners[4];
					coorners[0] = node->getViewport(j)->getViewPlaneCoords( sgct_core::Viewport::LowerLeft );
					coorners[1] = node->getViewport(j)->getViewPlaneCoords( sgct_core::Viewport::UpperLeft );
					coorners[2] = node->getViewport(j)->getViewPlaneCoords( sgct_core::Viewport::UpperRight );
					coorners[3] = coorners[2] - (coorners[1] - coorners[0]);

					glColor4f( mColors[ counter%6 ].r, mColors[ counter%6 ].g, mColors[ counter%6 ].b, 0.2f);
					glBegin(GL_QUADS);
						glVertex3f( coorners[0].x, coorners[0].y, coorners[0].z);
						glVertex3f( coorners[1].x, coorners[1].y, coorners[1].z);
						glVertex3f( coorners[2].x, coorners[2].y, coorners[2].z);
						glVertex3f( coorners[3].x, coorners[3].y, coorners[3].z);
					glEnd();

					counter++;
				}
			}
	}*/
}

void myInitOGLFun()
{
	//myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::Regular);
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_COLOR_MATERIAL );
	glDisable( GL_LIGHTING );
	glEnable( GL_TEXTURE_2D );

	dome = new sgct_utils::SGCTDome(10.5f, 180.0f, 72, 18, 1024); 
}

void initColors()
{
	//red
	mColors[0].r = 1.0f;
	mColors[0].g = 0.0f;
	mColors[0].b = 0.0f;

	//green
	mColors[1].r = 0.0f;
	mColors[1].g = 1.0f;
	mColors[1].b = 0.0f;

	//blue
	mColors[2].r = 0.0f;
	mColors[2].g = 0.0f;
	mColors[2].b = 1.0f;

	//cyan
	mColors[3].r = 0.0f;
	mColors[3].g = 1.0f;
	mColors[3].b = 1.0f;
	
	//magenta
	mColors[4].r = 1.0f;
	mColors[4].g = 0.0f;
	mColors[4].b = 1.0f;
	
	//yellow
	mColors[5].r = 1.0f;
	mColors[5].g = 1.0f;
	mColors[5].b = 0.0f;

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
				gEngine->takeScreenshot();
			break;
		}
	}
}