/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PROFILING__H__
#define __SGCT__PROFILING__H__

#include <sgct/opengl.h>

#ifdef WIN32
#include <CodeAnalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif // WIN32

#ifdef TRACY_ENABLE

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#ifdef SGCT_OVERRIDE_NEW_AND_DELETE
void* operator new(size_t count);
void operator delete(void* ptr) noexcept;
#endif // SGCT_OVERRIDE_NEW_AND_DELETE

#else

#define ZoneScoped
#define ZoneScopedN(x)
#define TracyGpuZone(x)
#define TracyGpuContext
#define TracyGpuCollect
#define FrameMark


#endif // TRACY_ENABLE

#ifdef WIN32
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // WIN32

#endif // __SGCT__PROFILING__H__
