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

#ifdef WIN32
#include <CodeAnalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif // WIN32

#if __has_include(<Tracy.hpp>)
#include <Tracy.hpp>
#include <TracyOpenGL.hpp>
#else
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#endif

#ifdef WIN32
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // WIN32

#endif // __SGCT__PROFILING__H__
