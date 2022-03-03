/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SPOUTLIBRARYWRAPPER___H__
#define __SGCT__SPOUTLIBRARYWRAPPER___H__

#include "SpoutLibrary.h"
#include <string>

namespace sgct::spout {

class SpoutMain {
public:
    SpoutMain();
    virtual ~SpoutMain();

    virtual void Release();

    void SaveGLState();
    void RestoreGLState();

    void SaveGLTextureState();
    void RestoreGLTextureState();


protected:
    unsigned int _defaultTexture;
    unsigned int _defaultFBO;
    unsigned int _defaultReadFBO;
    unsigned int _defaultDrawFBO;
    unsigned int _defaultReadBuffer;
    unsigned int _defaultDrawBuffer[1] = { 0 };

    void* _spoutHandle = nullptr;

    std::string _currentSpoutName;
    unsigned int _spoutWidth = 0;
    unsigned int _spoutHeight = 0;
};

} // namespace sgct::spout

#endif // __SGCT__SPOUTLIBRARYWRAPPER___H__
