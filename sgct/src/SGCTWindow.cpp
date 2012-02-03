#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>
#include "sgct/SGCTWindow.h"
#include <stdio.h>

HDC hDC;
void GLFWCALL windowResizeCallback( int width, int height );

//using namespace sgct;

core_sgct::SGCTWindow * instancePtr;

core_sgct::SGCTWindow::SGCTWindow()
{
	instancePtr = this;
	mUseSwapGroups = false;
	mSwapGroupMaster = false;
	mUseQuadBuffer = false;

	mWindowRes[0] = 640;
	mWindowRes[1] = 480;
	mWindowPos[0] = 0;
	mWindowPos[1] = 0;
	mWindowMode = GLFW_WINDOW;
}

core_sgct::SGCTWindow::~SGCTWindow()
{
	if( mUseSwapGroups )
	{
		//un-bind
		wglBindSwapBarrierNV(1,0);
		//un-join
		wglJoinSwapGroupNV(hDC,0);
	}
}

void core_sgct::SGCTWindow::init(const char * windowTitle)
{
	glfwSwapInterval( 1 ); //0: vsync off, 1: vsync on
	glfwSetWindowPos( mWindowPos[0], mWindowPos[1] );
	glfwSetWindowSizeCallback( windowResizeCallback );
	glfwSetWindowTitle( windowTitle );
	if( mUseSwapGroups )
		initNvidiaSwapGroups();
}

void core_sgct::SGCTWindow::setWindowResolution(int x, int y)
{
	mWindowRes[0] = x;
	mWindowRes[1] = y;
}

void core_sgct::SGCTWindow::setWindowPosition(int x, int y)
{
	mWindowPos[0] = x;
	mWindowPos[1] = y;
}

void core_sgct::SGCTWindow::setWindowMode(int mode)
{
	mWindowMode = mode;
}

void core_sgct::SGCTWindow::useSwapGroups(bool state)
{
	mUseSwapGroups = state;
}

void core_sgct::SGCTWindow::useQuadbuffer(bool state)
{
	mUseQuadBuffer = state;
	if( mUseQuadBuffer )
		glfwOpenWindowHint(GLFW_STEREO, GL_TRUE);
}

bool core_sgct::SGCTWindow::openWindow()
{
	/* Open an OpenGL window
	param: 
	int width
	int height
	int redbits
	int greenbits
	int bluebits
	int alphabits
	int depthbits
	int stencilbits
	int mode
	*/

	return GL_TRUE == glfwOpenWindow( mWindowRes[0],
		mWindowRes[1],
		0,0,0,0,0,0,
		mWindowMode);
}

void core_sgct::SGCTWindow::initNvidiaSwapGroups()
{
	if (wglewIsSupported("WGL_NV_swap_group"))
	{
		mUseSwapGroups = true;
		
		hDC = wglGetCurrentDC();
		fprintf(stdout, "WGL_NV_swap_group is supported\n");

		if( wglJoinSwapGroupNV(hDC,1) )
			fprintf(stdout, "Joining swapgroup 1 [ok].\n");
		else
		{
			fprintf(stdout, "Joining swapgroup 1 [failed].\n");
			mUseSwapGroups = false;
		}

		if( wglBindSwapBarrierNV(1,1) )
			fprintf(stdout, "Setting up swap barrier [ok].\n");
		else
		{
			fprintf(stdout, "Setting up swap barrier [failed].\n");
			mUseSwapGroups = false;
		}
		
		if( wglResetFrameCountNV(hDC) )
		{
			mUseSwapGroups = true;
			fprintf(stdout, "Resetting frame counter. This computer is the master.\n");
		}
		else
			fprintf(stdout, "Resetting frame counter failed. This computer is the slave.\n");
	}
	else
		mUseSwapGroups = false;
}

void GLFWCALL windowResizeCallback( int width, int height )
{ 
	instancePtr->setWindowResolution(width, height > 0 ? height : 1);
}

void core_sgct::SGCTWindow::getSwapGroupFrameNumber(unsigned int & frameNumber)
{
	frameNumber = 0;
	if (mUseSwapGroups)
	{
		wglQueryFrameCountNV(hDC, &frameNumber);
	}		
}

void core_sgct::SGCTWindow::resetSwapGroupFrameNumber()
{
	if (mUseSwapGroups)
	{
		wglResetFrameCountNV(hDC);
	}
}