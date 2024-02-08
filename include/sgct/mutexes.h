/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__MUTEXES__H__
#define __SGCT__MUTEXES__H__

#include <mutex>

namespace sgct::mutex {

inline std::mutex DataSync;
inline std::mutex Tracking;

} // namespace sgct::mutex

#endif // __SGCT__MUTEXES__H__
