/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTMutexManager.h>

//#define __SGCT_MUTEX_DEBUG__

namespace sgct {

SGCTMutexManager* SGCTMutexManager::mInstance = nullptr;

SGCTMutexManager* SGCTMutexManager::instance() {
    if (mInstance == nullptr) {
        mInstance = new SGCTMutexManager();
    }

    return mInstance;
}

/*! Destroy the SGCTSettings instance */
void SGCTMutexManager::destroy() {
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
    }
}

void SGCTMutexManager::lockMutex(MutexIndexes mi) {
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Locking mutex %d...\n", mi);
#endif
    mInternalMutexes[mi].lock();
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done [%d]\n", mi);
#endif
}

void SGCTMutexManager::unlockMutex(MutexIndexes mi) {
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Unlocking mutex %u...\n", mi);
#endif
    mInternalMutexes[mi].unlock();
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done [%d]\n", mi);
#endif
}

std::mutex& SGCTMutexManager::getMutexPtr(MutexIndexes mi) {
    return mInternalMutexes[mi];
}

} // namespace sgct
