/*
	sgct.h

	� 2012 Miroslav Andel, Alexander Fridlund
*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#define GLEW_STATIC //important when compileing with gcc

#ifndef _SGCT_H_
#define _SGCT_H_

#ifdef __WIN32__
    #include <windows.h> //must be declared before glfw
    #include <winsock2.h>
#endif
//@TODO FIX ME, which inlcudes needed?
//#ifdef __DARWIN__
//    #include <glew.h>
//    #include <GL/glew.h>
//
//#else
    #include <GL/glew.h>
#if __WIN32__
#include <GL/wglew.h>
#else
#include <OpenGL/glext.h>
#endif
    #include <GL/glfw.h>
//#endif
#include "sgct/Engine.h"
#include "sgct/SharedData.h"
#include "sgct/TextureManager.h"
#include "sgct/FontManager.h"
#include "sgct/MessageHandler.h"
#include "sgct/ShaderManager.h"

#endif
