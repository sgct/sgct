/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/mutexmanager.h>

namespace sgct {

MutexManager* MutexManager::_instance = nullptr;

MutexManager* MutexManager::instance() {
    if (_instance == nullptr) {
        _instance = new MutexManager();
    }

    return _instance;
}

void MutexManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

} // namespace sgct
