/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PROFILING__H__
#define __SGCT__PROFILING__H__

#include <sgct/opengl.h>

#if __has_include(<Tracy.hpp>)
#include <Tracy.hpp>
#include <TracyOpenGL.hpp>
#else
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#endif

#ifdef TRACY_ENABLE

void* operator new(size_t count);
void operator delete(void* ptr) noexcept;

#endif // TRACY_ENABLE

#endif // __SGCT__PROFILING__H__
