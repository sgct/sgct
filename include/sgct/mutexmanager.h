/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__MUTEX_MANAGER__H__
#define __SGCT__MUTEX_MANAGER__H__

#include <mutex>

namespace sgct::mutex {

inline std::mutex ConsoleMutex;
inline std::mutex DataSyncMutex;
inline std::mutex FrameSyncMutex;
inline std::mutex TrackingMutex;
inline std::mutex TransferMutex;

} // namespace sgct::mutex

#endif // __SGCT__MUTEX_MANAGER__H__
