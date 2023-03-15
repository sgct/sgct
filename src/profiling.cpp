/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/profiling.h>

#ifdef TRACY_ENABLE
#ifdef SGCT_OVERRIDE_NEW_AND_DELETE

#ifdef WIN32
#include <CodeAnalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif // WIN32

void* operator new(size_t count) {
    void* ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    TracyFree(ptr);
    free(ptr);
}

#ifdef WIN32
#pragma warning(pop)
#endif // WIN32

#endif // SGCT_OVERRIDE_NEW_AND_DELETE
#endif // TRACY_ENABLE
