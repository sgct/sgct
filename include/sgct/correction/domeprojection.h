/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_DOMEPROJECTION__H__
#define __SGCT__CORRECTION_DOMEPROJECTION__H__

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <sgct/correction/buffer.h>
#include <filesystem>

namespace sgct::correction {

SGCT_EXPORT Buffer generateDomeProjectionMesh(const std::filesystem::path& path,
    const vec2& pos, const vec2& size);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_DOMEPROJECTION__H__
