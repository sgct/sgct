/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CALLBACKDATA__H__
#define __SGCT__CALLBACKDATA__H__

#include <sgct/sgctexports.h>

#include <sgct/definitions.h>
#include <sgct/math.h>
#include <utility>

namespace sgct {

class BaseViewport;
class Window;

struct SGCT_EXPORT RenderData {
    const Window& window;
    const BaseViewport& viewport;
    const FrustumMode frustumMode;

    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    // @TODO (abock, 2019-12-03) Performance measurements needed to see whether this
    // caching is necessary
    mat4 modelViewProjectionMatrix;

    ivec2 bufferSize;
};

} // namespace sgct

#endif // __SGCT__CALLBACKDATA__H__
