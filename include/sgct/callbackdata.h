/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CALLBACKDATA__H__
#define __SGCT__CALLBACKDATA__H__

#include <sgct/sgctexports.h>
#include <sgct/frustum.h>
#include <sgct/math.h>
#include <utility>

namespace sgct {

class BaseViewport;
class Window;

struct SGCT_EXPORT RenderData {
    RenderData(const Window& window_, const BaseViewport& viewport_,
               Frustum::Mode frustumMode_, mat4 modelMatrix_, mat4 viewMatrix_,
               mat4 projectionMatrix_, mat4 modelViewProjectionMatrix_, ivec2 bufferSize_)
        : window(window_)
        , viewport(viewport_)
        , frustumMode(frustumMode_)
        , modelMatrix(std::move(modelMatrix_))
        , viewMatrix(std::move(viewMatrix_))
        , projectionMatrix(std::move(projectionMatrix_))
        , modelViewProjectionMatrix(std::move(modelViewProjectionMatrix_))
        , bufferSize(std::move(bufferSize_))
    {}
    const Window& window;
    const BaseViewport& viewport;
    const Frustum::Mode frustumMode;

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
