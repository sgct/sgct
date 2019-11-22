/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_DOMEPROJECTION__H__
#define __SGCT__CORRECTION_DOMEPROJECTION__H__

#include <sgct/correction/buffer.h>
#include <glm/fwd.hpp>
#include <string>

namespace sgct::core::correction {

Buffer generateDomeProjectionMesh(const std::string& path, const glm::ivec2& pos,
    const glm::ivec2& size);

} // namespace sgct::core::correction

#endif // __SGCT__CORRECTION_DOMEPROJECTION__H__
