/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__FRUSTUM__H__
#define __SGCT__FRUSTUM__H__

#include <sgct/sgctexports.h>

#include <cstdint>

namespace sgct {

enum class SGCT_EXPORT Eye : uint8_t { MonoOrLeft, Right };
enum class SGCT_EXPORT FrustumMode : uint8_t { Mono, StereoLeft, StereoRight };

} // namespace sgct

#endif // __SGCT__FRUSTUM__H__
