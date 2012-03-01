#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>
#include "../include/sgct/SGCTWindow.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include <stdio.h>

HDC hDC;
void GLFWCALL windowResizeCallback( int width, int height );

core_sgct::SGCTWindow::SGCTWindow()
{
	mUseSwapGroups = false;
	mSwapGroupMaster = false;
	mUseQuadBuffer = false;
	mBarrier = false;

	mWindowRes[0] = 640;
	mWindowRes[1] = 480;
	mWindowPos[0] = 0;
	mWindowPos[1] = 0;
	mWindowMode = GLFW_WINDOW;
}

void core_sgct::SGCTWindow::close()
{
	if( mUseSwapGroups )
	{
		//un-bind
		wglBindSwapBarrierNV(1,0);
		//un-join
		wglJoinSwapGroupNV(hDC,0);
	}
}

void core_sgct::SGCTWindow::init(bool lockVerticalSync)
{
	glfwSwapInterval( lockVerticalSync ? 1 : 0 ); //0: vsync off, 1: vsync on
	
	if(mWindowMode == GLFW_WINDOW)
	{
		glfwSetWindowPos( mWindowPos[0], mWindowPos[1] );
		glfwSetWindowSizeCallback( windowResizeCallback );
	}

	initNvidiaSwapGroups();
}

/*!
	Sets the window title.
	@param	Title of the window.
*/
void core_sgct::SGCTWindow::setWindowTitle(const char * title)
{
	glfwSetWindowTitle( title );
}

void core_sgct::SGCTWindow::setWindowResolution(const int x, const int y)
{
	mWindowRes[0] = x;
	mWindowRes[1] = y;
}

void core_sgct::SGCTWindow::setWindowPosition(const int x, const int y)
{
	mWindowPos[0] = x;
	mWindowPos[1] = y;
}

void core_sgct::SGCTWindow::setWindowMode(const int mode)
{
	mWindowMode = mode;
}

void core_sgct::SGCTWindow::setBarrier(const bool state)
{
	if( mUseSwapGroups && state != mBarrier)
	{
		mBarrier = wglBindSwapBarrierNV(1, state ? 1 : 0) ? 1 : 0;
	}
}

void core_sgct::SGCTWindow::useSwapGroups(const bool state)
{
	mUseSwapGroups = state;
}

void core_sgct::SGCTWindow::useQuadbuffer(const bool state)
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

	//glfwOpenWindowHint( GLFW_OPENGL_VERSION_MAJOR, 4 );
	//glfwOpenWindowHint( GLFW_OPENGL_VERSION_MINOR, 1 );

	return GL_TRUE == glfwOpenWindow( mWindowRes[0],
		mWindowRes[1],
		0,0,0,0,0,0,
		mWindowMode);
}

void core_sgct::SGCTWindow::initNvidiaSwapGroups()
{
#ifdef __WIN32__ //Windows uses wglew.h
	if (wglewIsSupported("WGL_NV_swap_group") && mUseSwapGroups)
	{
		hDC = wglGetCurrentDC();
		sgct::MessageHandler::Instance()->print("WGL_NV_swap_group is supported\n");

		if( wglJoinSwapGroupNV(hDC,1) )
			sgct::MessageHandler::Instance()->print("Joining swapgroup 1 [ok].\n");
		else
		{
			sgct::MessageHandler::Instance()->print("Joining swapgroup 1 [failed].\n");
			mUseSwapGroups = false;
			return;
		}
	}
	else
		mUseSwapGroups = false;
#elif //Apple and Linux uses glext.h
    if (wglewIsSupported("WGL_NV_swap_group") && mUseSwapGroups)
	{
		hDC = wglGetCurrentDC();
		sgct::MessageHandler::Instance()->print("WGL_NV_swap_group is supported\n");
        
		if( wglJoinSwapGroupNV(hDC,1) )
			sgct::MessageHandler::Instance()->print("Joining swapgroup 1 [ok].\n");
		else
		{
			sgct::MessageHandler::Instance()->print("Joining swapgroup 1 [failed].\n");
			mUseSwapGroups = false;
			return;
		}
	}
	else
		mUseSwapGroups = false;
#else
    mUseSwapGroups = false;
#endif
}

void GLFWCALL windowResizeCallback( int width, int height )
{
	core_sgct::ClusterManager::Instance()->getThisNodePtr()->getWindowPtr()->setWindowResolution(width, height > 0 ? height : 1);
}

void core_sgct::SGCTWindow::getSwapGroupFrameNumber(unsigned int & frameNumber)
{
	frameNumber = 0;
	if (mBarrier)
	{
		wglQueryFrameCountNV(hDC, &frameNumber);
	}
}

void core_sgct::SGCTWindow::resetSwapGroupFrameNumber()
{
	if (mBarrier)
	{
		if( wglResetFrameCountNV(hDC) )
		{
			mSwapGroupMaster = true;
			sgct::MessageHandler::Instance()->print("Resetting frame counter. This computer is the master.\n");
		}
		else
		{
			mSwapGroupMaster = false;
			sgct::MessageHandler::Instance()->print("Resetting frame counter failed. This computer is the slave.\n");
		}
	}
}
