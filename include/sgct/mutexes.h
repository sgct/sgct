/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__MUTEXES__H__
#define __SGCT__MUTEXES__H__

#include <mutex>

namespace sgct::core::mutex {

inline std::mutex DataSync;
inline std::mutex FrameSync;
inline std::mutex Tracking;
inline std::mutex Transfer;

} // namespace sgct::core::mutex

#endif // __SGCT__MUTEXES__H__
