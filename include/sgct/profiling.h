/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PROFILING__H__
#define __SGCT__PROFILING__H__

#include <Tracy.hpp>
#include <TracyOpenGL.hpp>

#ifdef TRACY_ENABLE

void* operator new(size_t count);
void operator delete(void* ptr) noexcept;

#endif // TRACY_ENABLE

#endif // __SGCT__PROFILING__H__
