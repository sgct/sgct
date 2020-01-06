/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CALLBACKDATA__H__
#define __SGCT__CALLBACKDATA__H__

#include <sgct/frustum.h>
#include <glm/glm.hpp>

namespace sgct {

class BaseViewport;
class Window;

struct RenderData {
    RenderData(const Window& window_, const BaseViewport& viewport_,
               Frustum::Mode frustumMode_, glm::mat4 modelMatrix_, glm::mat4 viewMatrix_,
               glm::mat4 projectionMatrix_, glm::mat4 modelViewProjectionMatrix_)
        : window(window_)
        , viewport(viewport_)
        , frustumMode(frustumMode_)
        , modelMatrix(std::move(modelMatrix_))
        , viewMatrix(std::move(viewMatrix_))
        , projectionMatrix(std::move(projectionMatrix_))
        , modelViewProjectionMatrix(std::move(modelViewProjectionMatrix_))
    {}
    const Window& window;
    const BaseViewport& viewport;
    const Frustum::Mode frustumMode;

    const glm::mat4 modelMatrix;
    const glm::mat4 viewMatrix;
    const glm::mat4 projectionMatrix;
    // @TODO (abock, 2019-12-03) Performance measurements needed to see whether this
    // caching is necessary
    const glm::mat4 modelViewProjectionMatrix;
};

} // namespace sgct

#endif // __SGCT__CALLBACKDATA__H__
