/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_PAULBOURKE__H__
#define __SGCT__CORRECTION_PAULBOURKE__H__

#include <sgct/correction/buffer.h>
#include <glm/fwd.hpp>
#include <string>

namespace sgct::correction {

Buffer generatePaulBourkeMesh(const std::string& path, const glm::ivec2& pos,
    const glm::ivec2& size, float aspectRatio);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_PAULBOURKE__H__
