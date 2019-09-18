/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/mutexmanager.h>

namespace sgct {

MutexManager* MutexManager::mInstance = nullptr;

MutexManager* MutexManager::instance() {
    if (mInstance == nullptr) {
        mInstance = new MutexManager();
    }

    return mInstance;
}

void MutexManager::destroy() {
    delete mInstance;
    mInstance = nullptr;
}

} // namespace sgct
