/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_SCALABLE__H__
#define __SGCT__CORRECTION_SCALABLE__H__

#include <sgct/correction/buffer.h>
#include <string>
#include <glm/glm.hpp>

namespace sgct::core::correction {

Buffer generateScalableMesh(const std::string& path, const glm::ivec2& pos,
    const glm::ivec2& size);
} // namespace sgct::core::correction

#endif // __SGCT__CORRECTION_SCALABLE__H__
