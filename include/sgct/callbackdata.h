/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CALLBACKDATA__H__
#define __SGCT__CALLBACKDATA__H__

#include <sgct/sgctexports.h>

#include <sgct/definitions.h>
#include <glm/glm.hpp>

namespace sgct {

class BaseViewport;
class Window;

struct SGCT_EXPORT RenderData {
    const Window& window;
    const BaseViewport& viewport;
    const FrustumMode frustumMode;

    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    // @TODO (abock, 2019-12-03) Performance measurements needed to see whether this
    // caching is necessary
    glm::mat4 modelViewProjectionMatrix;

    glm::ivec2 bufferSize;
};

} // namespace sgct

#endif // __SGCT__CALLBACKDATA__H__
