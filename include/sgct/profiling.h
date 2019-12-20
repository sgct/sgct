/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__PROFILING__H__
#define __SGCT__PROFILING__H__

#ifdef SGCT_HAS_TRACY

#include <sgct/ogl_headers.h>

#include <Tracy.hpp>
#include <TracyOpenGL.hpp>

#endif // SGCT_HAS_TRACY


#ifdef TRACY_ENABLE

// (abock, 2019-12-20) Not sure if we want to keep this or not
inline void* operator new(size_t count) {
    void* ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}

inline void operator delete(void* ptr) noexcept {
    TracyFree(ptr);
    free(ptr);
}

#endif // TRACY_ENABLE

namespace sgct {

} // namespace sgct

#endif // __SGCT__PROFILING__H__
