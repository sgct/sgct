/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTMutexManager.h>

namespace sgct {

SGCTMutexManager* SGCTMutexManager::mInstance = nullptr;

SGCTMutexManager* SGCTMutexManager::instance() {
    if (mInstance == nullptr) {
        mInstance = new SGCTMutexManager();
    }

    return mInstance;
}

void SGCTMutexManager::destroy() {
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
    }
}

} // namespace sgct
