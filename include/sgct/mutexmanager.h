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

namespace sgct {

// @TODO (abock, 2019-10-07) move these out of the struct

/**
 * This singleton class manages SGCTs mutexes
 */
struct MutexManager {
    /// Get the Settings instance
    static MutexManager* instance();

    /// Destroy the Settings instance
    static void destroy();

    std::mutex dataSyncMutex;
    std::mutex frameSyncMutex;
    std::mutex trackingMutex;
    std::mutex consoleMutex;
    std::mutex transferMutex;

private:
    MutexManager() = default;
    ~MutexManager() = default;

    static MutexManager* _instance;
};

} // namespace sgct

#endif // __SGCT__MUTEX_MANAGER__H__
