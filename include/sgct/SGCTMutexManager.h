/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__MUTEX_MANAGER__H__
#define __SGCT__MUTEX_MANAGER__H__

#include <mutex>

namespace sgct {

/**
 * This singleton class manages SGCTs mutexes
 */
struct SGCTMutexManager {
    /// Get the SGCTSettings instance
    static SGCTMutexManager* instance();

    /// Destroy the SGCTSettings instance
    static void destroy();

    std::mutex mDataSyncMutex;
    std::mutex mFrameSyncMutex;
    std::mutex mTrackingMutex;
    std::mutex mConsoleMutex;
    std::mutex mTransferMutex;

private:
    SGCTMutexManager() = default;
    ~SGCTMutexManager() = default;

    static SGCTMutexManager* mInstance;
};

} // namespace sgct

#endif // __SGCT__MUTEX_MANAGER__H__
