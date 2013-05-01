/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_MUTEX_MANAGER
#define _SGCT_MUTEX_MANAGER

#define SGCT_NUMBER_OF_MUTEXES 4

#include <stddef.h>

namespace sgct
{

/*!
	Wrapper for GLFWmutex
*/
typedef void * SGCTmutex;

/*!
	This singleton class manages SGCTs mutexes
*/
class SGCTMutexManager
{
public:
	enum MutexIndexes { MainMutex = 0, SyncMutex, SharedDataMutex, TrackingMutex };

	/*! Get the SGCTSettings instance */
	static SGCTMutexManager * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new SGCTMutexManager();
		}

		return mInstance;
	}

	/*! Destroy the SGCTSettings instance */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	bool isValid();
	void lockMutex(MutexIndexes mi);
	void unlockMutex(MutexIndexes mi);
	SGCTmutex getMutex(MutexIndexes mi);

	static SGCTmutex createMutex();
	static void destroyMutex(SGCTmutex mutex);
	static void lockMutex(SGCTmutex mutex);
	static void unlockMutex(SGCTmutex mutex);

private:
	SGCTMutexManager();
	~SGCTMutexManager();

	// Don't implement these, should give compile warning if used
	SGCTMutexManager( const SGCTMutexManager & settings );
	const SGCTMutexManager & operator=(const SGCTMutexManager & settings );

private:
	static SGCTMutexManager * mInstance;
	
	SGCTmutex mInternalMutexes[SGCT_NUMBER_OF_MUTEXES];
	bool mValid;
};
}

#endif