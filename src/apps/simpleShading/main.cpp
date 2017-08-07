#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myInitOGLFun();

GLfloat lightPosition[] = { 5.0f, 5.0f, 10.0f, 1.0f };
GLfloat lightAmbient[]= { 0.1f, 0.1f, 0.1f, 1.0f };
GLfloat lightDiffuse[]= { 0.6f, 0.6f, 0.6f, 1.0f };
GLfloat lightSpecular[]= { 1.0f, 1.0f, 1.0f, 1.0f };

GLfloat materialAmbient[] = {1.0, 0.0, 0.0, 1.0};
GLfloat materialDiffuse[] = {1.0, 1.0, 0.0, 1.0};
GLfloat materialSpecular[] = {0.0, 1.0, 1.0, 1.0};
GLfloat materialShininess = 64;

size_t myTextureIndex;
sgct_utils::SGCTSphere * mySphere = NULL;

int main( int argc, char* argv[] )
{
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setDrawFunction( myDrawFun );

    if( !gEngine->init() )
    {
        delete gEngine;
        return EXIT_FAILURE;
    }

    // Main loop
    gEngine->render();

    // Clean up
    if(mySphere != NULL) delete mySphere;
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );
}

void myDrawFun()
{
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glTranslatef(0.0f, 0.0f, -3.0f);
    
    mySphere->draw();
}

void myInitOGLFun()
{
    mySphere = new sgct_utils::SGCTSphere(1.0f, 32);
    
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glEnable( GL_NORMALIZE );
    glShadeModel( GL_SMOOTH );
    glEnable( GL_LIGHTING );

    //Set up light 0
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    //Set up material
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    glMaterialf( GL_FRONT, GL_SHININESS, materialShininess);

    //Set up backface culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); //our polygon winding is counter clockwise
}
