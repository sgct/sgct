#include "sgct.h"

sgct::Engine * gEngine;

void drawFun();
void myPostSyncPreDrawFun();
void initOGLFun();
void keyCallback(int key, int action);
void cleanUpFun();

sgct_utils::SGCTBox * box = NULL;

bool info = false;
bool stats = false;
bool takeScreenshot = false;

int main( int argc, char* argv[] )
{

    // Allocate
    gEngine = new sgct::Engine( argc, argv );

    sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();
    if( thisNode != NULL )
        for(unsigned int i=0; i < thisNode->getNumberOfWindows(); i++)
            thisNode->getWindowPtr(i)->setAlpha(true);

    // Bind your functions
    gEngine->setInitOGLFunction( initOGLFun );
    gEngine->setDrawFunction( drawFun );
    gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );
    gEngine->setCleanUpFunction( cleanUpFun );

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
    glColor3f(1.0f,1.0f,1.0f);
    glEnable(GL_TEXTURE_2D);

    float r = 7.5f; //dome radius

    //right face
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("right") );
    glBegin(GL_QUADS);
        glTexCoord2d(0.0,0.0); glVertex3f(0.0f, -r, -r);
        glTexCoord2d(1.0,0.0); glVertex3f(r, -r, 0.0f);
        glTexCoord2d(1.0,1.0); glVertex3f(r, r, 0.0f);
        glTexCoord2d(0.0,1.0); glVertex3f(0.0f, r, -r);
    glEnd();

    //left face
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("left") );
    glBegin(GL_QUADS);
        glTexCoord2d(0.0,0.0); glVertex3f(-r, -r, 0.0f);
        glTexCoord2d(1.0,0.0); glVertex3f(0.0f, -r, -r);
        glTexCoord2d(1.0,1.0); glVertex3f(0.0f, r, -r);
        glTexCoord2d(0.0,1.0); glVertex3f(-r, r, 0.0f);
    glEnd();

    //top face
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("top") );
    glBegin(GL_QUADS);
        glTexCoord2d(0.0,0.0); glVertex3f(0.0f, r,  r);
        glTexCoord2d(1.0,0.0); glVertex3f(r,    r,  0.0f);
        glTexCoord2d(1.0,1.0); glVertex3f(0.0f, r,  -r);
        glTexCoord2d(0.0,1.0); glVertex3f(-r,    r,  0.0f);
    glEnd();

    //bottom face
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("bottom") );
    glBegin(GL_QUADS);
        glTexCoord2d(0.0,0.0); glVertex3f(0.0f, -r, r);
        glTexCoord2d(1.0,0.0); glVertex3f(-r,   -r, 0.0f);
        glTexCoord2d(1.0,1.0); glVertex3f(0.0f, -r, -r);
        glTexCoord2d(0.0,1.0); glVertex3f(r,   -r, 0.0f);
    glEnd();

    /*glPushMatrix();
    glTranslatef(0.0f, 0.0, -1.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("box") );
    
    glPushMatrix();
    glTranslatef(-1.0f, 0.0, 0.0f);
        box->draw();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0, 0.0f);
        box->draw();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0f, 0.0, 0.0f);
        box->draw();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.0f, -1.0, -1.0f);
        box->draw();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -1.0, -1.0f);
        box->draw();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0f, -1.0, -1.0f);
        box->draw();
    glPopMatrix();
    glPopMatrix();
    glDisable(GL_TEXTURE_1D);*/
    /*
    glColor3f(1.0f,1.0f,0.0f);
    glLineWidth(5.0);
    glBegin(GL_LINES);
        glVertex3f(-r/2.0f, r/2.0f, 0.0f);
        glVertex3f(0.0f, r/2.0f, -r/2.0f);

        glVertex3f(-r/2.0f, -r/2.0f, 0.0f);
        glVertex3f(0.0f, -r/2.0f, -r/2.0f);

        glVertex3f(r/2.0f, r/2.0f, 0.0f);
        glVertex3f(0.0f, r/2.0f, -r/2.0f);

        glVertex3f(r/2.0f, -r/2.0f, 0.0f);
        glVertex3f(0.0f, -r/2.0f, -r/2.0f);

        glVertex3f(0.0f, -r/2.0f, -r/2.0f);
        glVertex3f(0.0f, r/2.0f, -r/2.0f);
    glEnd();

    glColor3f(1.0f,0.5f,0.5f);
    glLineWidth(5.0);
    glBegin(GL_LINES);
        glVertex3f(-r/4.0f, r/4.0f, 0.0f);
        glVertex3f(0.0f, r/4.0f, -r/4.0f);

        glVertex3f(-r/4.0f, -r/4.0f, 0.0f);
        glVertex3f(0.0f, -r/4.0f, -r/4.0f);

        glVertex3f(r/4.0f, r/4.0f, 0.0f);
        glVertex3f(0.0f, r/4.0f, -r/4.0f);

        glVertex3f(r/4.0f, -r/4.0f, 0.0f);
        glVertex3f(0.0f, -r/4.0f, -r/4.0f);

        glVertex3f(0.0f, -r/4.0f, -r/4.0f);
        glVertex3f(0.0f, r/4.0f, -r/4.0f);
    glEnd();

    glColor3f(1.0f,0.0f,1.0f);
    glLineWidth(2.0);
    glBegin(GL_LINES);
        glVertex3f(-r*2.0f, r*2.0f, 0.0f);
        glVertex3f(0.0f, r*2.0f, -r*2.0f);

        glVertex3f(-r*2.0f, -r*2.0f, 0.0f);
        glVertex3f(0.0f, -r*2.0f, -r*2.0f);

        glVertex3f(r*2.0f, r*2.0f, 0.0f);
        glVertex3f(0.0f, r*2.0f, -r*2.0f);

        glVertex3f(r*2.0f, -r*2.0f, 0.0f);
        glVertex3f(0.0f, -r*2.0f, -r*2.0f);

        glVertex3f(0.0f, -r*2.0f, -r*2.0f);
        glVertex3f(0.0f, r*2.0f, -r*2.0f);
    glEnd();

    glColor3f(0.0f,1.0f,1.0f);
    glLineWidth(2.0);
    glBegin(GL_LINES);
        glVertex3f(-r, r, 0.0f);
        glVertex3f(0.0f, r, -r);

        glVertex3f(-r, -r, 0.0f);
        glVertex3f(0.0f, -r, -r);

        glVertex3f(r, r, 0.0f);
        glVertex3f(0.0f, r, -r);

        glVertex3f(r, -r, 0.0f);
        glVertex3f(0.0f, -r, -r);

        glVertex3f(0.0f, -r, -r);
        glVertex3f(0.0f, r, -r);
    glEnd();
    */
}

void initOGLFun()
{
    //sgct::TextureManager::instance()->setAnisotropicFilterSize(2.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::No_Compression);
    sgct::TextureManager::instance()->loadTexure("right", "grid_right.png", true, 4);
    sgct::TextureManager::instance()->loadTexure("left", "grid_left.png", true, 4);
    sgct::TextureManager::instance()->loadTexure("top", "grid_top.png", true, 4);
    sgct::TextureManager::instance()->loadTexure("bottom", "grid_bottom.png", true, 4);

    sgct::TextureManager::instance()->loadTexure("box", "box.png", true, 4);
    box = new sgct_utils::SGCTBox(0.5f, sgct_utils::SGCTBox::Regular);

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_COLOR_MATERIAL );
    glDisable( GL_LIGHTING );
    glEnable(GL_TEXTURE_2D);
}

void myPostSyncPreDrawFun()
{
    gEngine->setDisplayInfoVisibility(info);
    gEngine->setStatsGraphVisibility(stats);

    if( takeScreenshot )
    {
        gEngine->takeScreenshot();
        takeScreenshot = false;
    }
}

void keyCallback(int key, int action)
{
    if( gEngine->isMaster() )
    {
        switch( key )
        {
        case 'S':
            if(action == SGCT_PRESS)
                stats = !stats;
            break;

        case 'I':
            if(action == SGCT_PRESS)
                info = !info;
            break;

        case 'P':
        case SGCT_KEY_F10:
            if(action == SGCT_PRESS)
                takeScreenshot = true;
            break;
        }
    }
}

void cleanUpFun()
{
    if(box != NULL)
        delete box;
}
