/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__TINYXML__H__
#define __SGCT__TINYXML__H__

#ifdef WIN32
#include <CodeAnalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#elif defined(__clang__)
// nothing to do here, but __GNUC__ somehow also fires on clang
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif // WIN32

#include <tinyxml2.h>

#ifdef WIN32
#pragma warning(pop)
#elif defined(__clang__)
// nothing to do here, but __GNUC__ somehow also fires on clang
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // WIN32



#endif // __SGCT__TINYXML__H__
