#include <GL/glew.h>
#ifdef __WIN32__
#include <GL/wglew.h>
#else //APPLE LINUX
#include <OpenGL/glext.h>
#include <GL/glxew.h>
#endif
#include <GL/glfw.h>
#include "../include/sgct/SGCTWindow.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include <stdio.h>

#ifdef __WIN32__
HDC hDC;
#else // APPLE || LINUX
GLXDrawable hDC;
Display * disp;
    #ifdef GLEW_MX
    GLXEWContext * glxewGetContext();
    #endif
#endif

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
//#ifdef __WITHSWAPBARRIERS__

#ifdef __WIN32__
		if( wglewIsSupported("WGL_NV_swap_group") )
		{
			//un-bind
			wglBindSwapBarrierNV(1,0);
			//un-join
			wglJoinSwapGroupNV(hDC,0);
		}
#else
    #ifndef __APPLE__
		if( glewIsSupported("GLX_NV_swap_group") )
		{
			//un-bind
			glXBindSwapBarrierNV(disp,1,0);
			//un-join
			glXJoinSwapGroupNV(disp,hDC,0);
		}
    #endif
#endif

//#endif
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
//#ifdef __WITHSWAPBARRIERS__

	if( mUseSwapGroups && state != mBarrier)
	{
#ifdef __WIN32__ //Windows uses wglew.h
		mBarrier = wglBindSwapBarrierNV(1, state ? 1 : 0) ? 1 : 0;
#else //Apple and Linux uses glext.h
    #ifndef __APPLE__
		mBarrier = glXBindSwapBarrierNV(disp, 1, state ? 1 : 0) ? 1 : 0;
    #endif
#endif
	}

//#endif
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

	return GL_TRUE == glfwOpenWindow( mWindowRes[0],
		mWindowRes[1],
		8,8,8,8,32,8,
		mWindowMode);
}

void core_sgct::SGCTWindow::initNvidiaSwapGroups()
{
//#ifdef __WITHSWAPBARRIERS__

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
#else //Apple and Linux uses glext.h
    #ifndef __APPLE__

    if (glewIsSupported("GLX_NV_swap_group") && mUseSwapGroups)
	{
		hDC = glXGetCurrentDrawable();
		disp = glXGetCurrentDisplay();
		sgct::MessageHandler::Instance()->print("WGL_NV_swap_group is supported\n");

		if( glXJoinSwapGroupNV(disp,hDC,1) )
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

    #endif
#endif

//#else
//        mUseSwapGroups = false;
//#endif
}

void GLFWCALL windowResizeCallback( int width, int height )
{
	core_sgct::ClusterManager::Instance()->getThisNodePtr()->getWindowPtr()->setWindowResolution(width, height > 0 ? height : 1);
}

void core_sgct::SGCTWindow::getSwapGroupFrameNumber(unsigned int &frameNumber)
{
	frameNumber = 0;

//#ifdef __WITHSWAPBARRIERS__

	if (mBarrier)
	{

    #ifdef __WIN32__ //Windows uses wglew.h
		if( wglewIsSupported("WGL_NV_swap_group") )
			wglQueryFrameCountNV(hDC, &frameNumber);
    #else //Apple and Linux uses glext.h
        #ifndef __APPLE__
		if( glewIsSupported("GLX_NV_swap_group") )
			glXQueryFrameCountNV(disp, hDC, &frameNumber);
        #endif
    #endif
	}
//#endif
}

void core_sgct::SGCTWindow::resetSwapGroupFrameNumber()
{

//#ifdef __WITHSWAPBARRIERS__

	if (mBarrier)
	{
#ifdef __WIN32__
		if( wglewIsSupported("WGL_NV_swap_group") && wglResetFrameCountNV(hDC) )
#else
    #ifdef __APPLE__
        if(false)
    #else //linux
		if( glewIsSupported("GLX_NV_swap_group") && glXResetFrameCountNV(disp,hDC) )
    #endif
#endif
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

//#endif
}
