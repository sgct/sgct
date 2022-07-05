/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PROFILING__H__
#define __SGCT__PROFILING__H__

#include <sgct/opengl.h>

#ifdef WIN32
#include <CodeAnalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif // WIN32

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

#include <Tracy.hpp>
#include <TracyOpenGL.hpp>

#ifdef TRACY_ENABLE

void* operator new(size_t count);
void operator delete(void* ptr) noexcept;

#endif // TRACY_ENABLE

#ifdef WIN32
#pragma warning(pop)
#endif // WIN32

#pragma GCC diagnostic pop

#endif // __SGCT__PROFILING__H__
