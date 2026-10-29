/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PROFILING__H__
#define __SGCT__PROFILING__H__

#ifdef WIN32
#include <CodeAnalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif // WIN32

#ifdef TRACY_ENABLE

#include <vulkan/vulkan.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

// The Tracy GPU zones are wired to the Vulkan context. The Window creates one
// TracyVkCtx per in-flight command buffer (see Window::_tracyContexts) and marks
// the GPU zones with the TracyVkZone/TracyVkCollect macros from TracyVulkan.hpp.
// TracyVkCtx is the Tracy type for a Vulkan GPU context.

#ifdef SGCT_OVERRIDE_NEW_AND_DELETE
void* operator new(size_t count);
void operator delete(void* ptr) noexcept;
#endif // SGCT_OVERRIDE_NEW_AND_DELETE

#else // ^^^^ TRACY_ENABLE // !TRACY_ENABLE vvvv 

#define ZoneScoped
#define ZoneScopedN(x)
#define FrameMark
#define ZoneText(text, length)
#define TracyLockable(type, var) type var
#define TracyAlloc(ptr, bytes)
#define TracyAllocN(ptr, bytes, name)
/// The Tracy GPU context type is only defined when Tracy is enabled; provide a
/// placeholder so that code guarded by TRACY_ENABLE can still compile
using TracyVkCtx = void*;

#endif // TRACY_ENABLE

#ifdef WIN32
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // WIN32

#endif // __SGCT__PROFILING__H__
