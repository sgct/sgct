/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SGCTMutexManager.h"
#include "../include/sgct/ogl_headers.h"

sgct::SGCTMutexManager * sgct::SGCTMutexManager::mInstance = NULL;

sgct::SGCTMutexManager::SGCTMutexManager()
{
	mValid = false;

	int counter = 0;
	for(size_t i = 0; i<SGCT_NUMBER_OF_MUTEXES; i++)
	{
		mInternalMutexes[i] = NULL;
		mInternalMutexes[i] = createMutex();   

		if( mInternalMutexes[i] != NULL )
			counter++;
	}

    if( counter == SGCT_NUMBER_OF_MUTEXES )
		mValid = true;
}

sgct::SGCTMutexManager::~SGCTMutexManager()
{
	mValid = false;
	
	for(size_t i = 0; i<SGCT_NUMBER_OF_MUTEXES; i++)
	{
		if( mInternalMutexes[i] != NULL )
		{
			destroyMutex( mInternalMutexes[i] );
			mInternalMutexes[i] = NULL;
		}
	}
}

bool sgct::SGCTMutexManager::isValid()
{
	return mValid;
}

void sgct::SGCTMutexManager::lockMutex(sgct::SGCTMutexManager::MutexIndexes mi)
{
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Locking mutex %u...\n", mi);
#endif
    if(mInternalMutexes[mi] != NULL)
		glfwLockMutex(mInternalMutexes[mi]);
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done\n");
#endif
}

void sgct::SGCTMutexManager::unlockMutex(sgct::SGCTMutexManager::MutexIndexes mi)
{
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Unlocking mutex %u...\n", mi);
#endif
	if(mInternalMutexes[mi] != NULL)
		glfwUnlockMutex(mInternalMutexes[mi]);
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done\n");
#endif
}

sgct::SGCTmutex sgct::SGCTMutexManager::getMutex(sgct::SGCTMutexManager::MutexIndexes mi)
{
	return mInternalMutexes[mi];
}

sgct::SGCTmutex sgct::SGCTMutexManager::createMutex()
{
    return glfwCreateMutex();
}

void sgct::SGCTMutexManager::destroyMutex(sgct::SGCTmutex mutex)
{
    glfwDestroyMutex(mutex);
}

void sgct::SGCTMutexManager::lockMutex(sgct::SGCTmutex mutex)
{
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Locking mutex...\n");
#endif
    if(mutex != NULL)
		glfwLockMutex(mutex);
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done\n");
#endif
}

void sgct::SGCTMutexManager::unlockMutex(sgct::SGCTmutex mutex)
{
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Unlocking mutex...\n");
#endif
	if(mutex != NULL)
		glfwUnlockMutex(mutex);
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done\n");
#endif
}