/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <stdio.h>
#include <sgct/SGCTMutexManager.h>

//#define __SGCT_MUTEX_DEBUG__

sgct::SGCTMutexManager * sgct::SGCTMutexManager::mInstance = NULL;

sgct::SGCTMutexManager::SGCTMutexManager()
{

}

sgct::SGCTMutexManager::~SGCTMutexManager()
{
    
}

void sgct::SGCTMutexManager::lockMutex(sgct::SGCTMutexManager::MutexIndexes mi)
{
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Locking mutex %d...\n", mi);
#endif
    mInternalMutexes[mi].lock();
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done [%d]\n", mi);
#endif
}

void sgct::SGCTMutexManager::unlockMutex(sgct::SGCTMutexManager::MutexIndexes mi)
{
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Unlocking mutex %u...\n", mi);
#endif
    mInternalMutexes[mi].unlock();
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done [%d]\n", mi);
#endif
}

std::mutex * sgct::SGCTMutexManager::getMutexPtr(sgct::SGCTMutexManager::MutexIndexes mi)
{
    return &mInternalMutexes[mi];
}