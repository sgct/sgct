/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_MUTEX_MANAGER
#define _SGCT_MUTEX_MANAGER

#define SGCT_NUMBER_OF_MUTEXES 5

#include <stddef.h>
#include "external/tinythread.h"

namespace sgct
{

/*!
	This singleton class manages SGCTs mutexes
*/
class SGCTMutexManager
{
public:
	enum MutexIndexes { DataSyncMutex = 0, FrameSyncMutex, SharedVariableMutex, TrackingMutex, ConsoleMutex };

	/*! Get the SGCTSettings instance */
	static SGCTMutexManager * instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new SGCTMutexManager();
		}

		return mInstance;
	}

	/*! Destroy the SGCTSettings instance */
	static void destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	void lockMutex(MutexIndexes mi);
	void unlockMutex(MutexIndexes mi);
	tthread::mutex * getMutexPtr(MutexIndexes mi);

private:
	SGCTMutexManager();
	~SGCTMutexManager();

	// Don't implement these, should give compile warning if used
	SGCTMutexManager( const SGCTMutexManager & settings );
	const SGCTMutexManager & operator=(const SGCTMutexManager & settings );

private:
	static SGCTMutexManager * mInstance;
	tthread::mutex mInternalMutexes[SGCT_NUMBER_OF_MUTEXES];
};
}

#endif