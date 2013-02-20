/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <GL/glew.h>
#ifdef __WIN32__
#include <GL/wglew.h>
#elif __LINUX__
#include <GL/glext.h>
#include <GL/glxew.h>
#else //APPLE
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

sgct_core::SGCTWindow::SGCTWindow()
{
	mUseFixResolution = false;
	mUseSwapGroups = false;
	mSwapGroupMaster = false;
	mUseQuadBuffer = false;
	mBarrier = false;

	mWindowRes[0] = 640;
	mWindowRes[1] = 480;
	mWindowResOld[0] = mWindowRes[0];
	mWindowResOld[1] = mWindowRes[1];
	mWindowPos[0] = 0;
	mWindowPos[1] = 0;
	mFramebufferResolution[0] = 512;
	mFramebufferResolution[1] = 256;
	mWindowMode = GLFW_WINDOW;
	mAspectRatio = 1.0f;
}

void sgct_core::SGCTWindow::close()
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

void sgct_core::SGCTWindow::init()
{
	if(mWindowMode == GLFW_WINDOW)
	{
		glfwSetWindowPos( mWindowPos[0], mWindowPos[1] );
		glfwSetWindowSizeCallback( windowResizeCallback );
	}

	//swap the buffers and update the window
	glfwSwapBuffers();

	//initNvidiaSwapGroups();
}

/*!
	Set the window title
	@param title The title of the window.
*/
void sgct_core::SGCTWindow::setWindowTitle(const char * title)
{
	glfwSetWindowTitle( title );
}

/*!
	Sets the window resolution.
	
	@param x The width of the window in pixels.
	@param y The height of the window in pixels.
*/
void sgct_core::SGCTWindow::setWindowResolution(const int x, const int y)
{
	mWindowRes[0] = x;
	mWindowRes[1] = y;
	mAspectRatio = static_cast<float>( x ) /
			static_cast<float>( y );

	if( !mUseFixResolution )
	{
		mFramebufferResolution[0] = x;
		mFramebufferResolution[1] = y;
	}
}

/*!
	Sets the framebuffer resolution. Theese parameters will only be used if a fixed resolution is used that is different from the window resolution.
	This might be useful in fullscreen mode on Apples retina displays to force 1080p resolution or similar.
	
	@param x The width of the frame buffer in pixels.
	@param y The height of the frame buffer in pixels.
*/
void sgct_core::SGCTWindow::setFramebufferResolution(const int x, const int y)
{
	mFramebufferResolution[0] = x;
	mFramebufferResolution[1] = y;
}

/*!
	Swap previus data and current data. This is done at the end of the render loop.
*/
void sgct_core::SGCTWindow::swap()
{
	mWindowResOld[0] = mWindowRes[0];
	mWindowResOld[1] = mWindowRes[1];
}

/*!
	Don't use this function if you want to set the window resolution. Use setWindowResolution(const int x, const int y) instead.
	This function is called within sgct when the window is created.
*/
void sgct_core::SGCTWindow::initWindowResolution(const int x, const int y)
{
	mWindowRes[0] = x;
	mWindowRes[1] = y;
	mWindowResOld[0] = mWindowRes[0];
	mWindowResOld[1] = mWindowRes[1];

	mAspectRatio = static_cast<float>( x ) /
			static_cast<float>( y );

	if( !mUseFixResolution )
	{
		mFramebufferResolution[0] = x;
		mFramebufferResolution[1] = y;
	}
}

bool sgct_core::SGCTWindow::isWindowResized()
{
	if( mWindowRes[0] != mWindowResOld[0] || mWindowRes[1] != mWindowResOld[1] )
		return true;
	else
		return false;
}

void sgct_core::SGCTWindow::setWindowPosition(const int x, const int y)
{
	mWindowPos[0] = x;
	mWindowPos[1] = y;
}

void sgct_core::SGCTWindow::setWindowMode(const int mode)
{
	mWindowMode = mode;
}

void sgct_core::SGCTWindow::setBarrier(const bool state)
{
//#ifdef __WITHSWAPBARRIERS__

	if( mUseSwapGroups && state != mBarrier)
	{
		sgct::MessageHandler::Instance()->print("SGCTWindow: Enabling Nvidia swap barrier.\n");

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

/*!
	Force the frame buffer to have a fixed size which may be different from the window size.
*/
void sgct_core::SGCTWindow::setFixResolution(const bool state)
{
	mUseFixResolution = state;
}

/*!
	Use nvidia swap groups. This freature is only supported on quadro cards together with a compatible sync card.
	This function can only be used before the window is created.
*/
void sgct_core::SGCTWindow::useSwapGroups(const bool state)
{
	mUseSwapGroups = state;
}

/*!
	Use quad buffer (hardware stereoscopic rendering).
	This function can only be used before the window is created. 
	The quad buffer feature is only supported on professional CAD graphics cards such as
	Nvidia Quadro or AMD/ATI FireGL.
*/
void sgct_core::SGCTWindow::useQuadbuffer(const bool state)
{
	mUseQuadBuffer = state;
	if( mUseQuadBuffer )
		glfwOpenWindowHint(GLFW_STEREO, GL_TRUE);
}

/*!
	This function is used internally within sgct to open the window.

	/returns True if window was created successfully.
*/
bool sgct_core::SGCTWindow::openWindow()
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

void sgct_core::SGCTWindow::initNvidiaSwapGroups()
{
//#ifdef __WITHSWAPBARRIERS__

#ifdef __WIN32__ //Windows uses wglew.h
	if (wglewIsSupported("WGL_NV_swap_group") && mUseSwapGroups)
	{
		sgct::MessageHandler::Instance()->print("SGCTWindow: Joining Nvidia swap group.\n");
		
		hDC = wglGetCurrentDC();

		unsigned int maxBarrier = 0;
		unsigned int maxGroup = 0;
		wglQueryMaxSwapGroupsNV( hDC, &maxGroup, &maxBarrier );
		sgct::MessageHandler::Instance()->print("WGL_NV_swap_group extension is supported.\n\tMax number of groups: %u\n\tMax number of barriers: %u\n",
			maxGroup, maxBarrier);

		/*
		wglJoinSwapGroupNV adds <hDC> to the swap group specified by <group>.
		If <hDC> is already a member of a different group, it is 
		implicitly removed from that group first. A swap group is specified as 
		an integer value between 0 and the value returned in <maxGroups> by 
		wglQueryMaxSwapGroupsNV. If <group> is zero, the hDC is unbound from its 
		current group, if any. If <group> is larger than <maxGroups>, 
		wglJoinSwapGroupNV fails.

		*/
		if( wglJoinSwapGroupNV(hDC, 1) )
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
		sgct::MessageHandler::Instance()->print("SGCTWindow: Joining Nvidia swap group.\n");
		
		hDC = glXGetCurrentDrawable();
		disp = glXGetCurrentDisplay();

		unsigned int maxBarrier = 0;
		unsigned int maxGroup = 0;
		glXQueryMaxSwapGroupsNV( disp, hDC, &maxGroup, &maxBarrier );
		sgct::MessageHandler::Instance()->print("WGL_NV_swap_group extension is supported.\n\tMax number of groups: %u\n\tMax number of barriers: %u\n",
			maxGroup, maxBarrier);

		if( glXJoinSwapGroupNV(disp, hDC, 1) )
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
	sgct_core::ClusterManager::Instance()->getThisNodePtr()->getWindowPtr()->setWindowResolution(width, height > 0 ? height : 1);
}

void sgct_core::SGCTWindow::getSwapGroupFrameNumber(unsigned int &frameNumber)
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

void sgct_core::SGCTWindow::resetSwapGroupFrameNumber()
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
