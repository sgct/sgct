/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__MUTEX_MANAGER__H__
#define __SGCT__MUTEX_MANAGER__H__

#define SGCT_NUMBER_OF_MUTEXES 6

#include <mutex>
#include <stddef.h>

namespace sgct {

/*!
    This singleton class manages SGCTs mutexes
*/
class SGCTMutexManager {
public:
    enum MutexIndexes {
        DataSyncMutex = 0,
        FrameSyncMutex,
        TrackingMutex,
        ConsoleMutex,
        TransferMutex
    };

    /*! Get the SGCTSettings instance */
    static SGCTMutexManager* instance();

    /*! Destroy the SGCTSettings instance */
    static void destroy();

    void lockMutex(MutexIndexes mi);
    void unlockMutex(MutexIndexes mi);
    std::mutex& getMutexPtr(MutexIndexes mi);

private:
    SGCTMutexManager() = default;
    ~SGCTMutexManager() = default;

private:
    static SGCTMutexManager* mInstance;
    std::mutex mInternalMutexes[SGCT_NUMBER_OF_MUTEXES];
};

} // namespace sgct

#endif // __SGCT__MUTEX_MANAGER__H__
