/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_SIMCAD__H__
#define __SGCT__CORRECTION_SIMCAD__H__

#include <sgct/sgctexports.h>

#include <sgct/math.h>
#include <filesystem>

namespace sgct::correction {

struct Buffer;

SGCT_EXPORT Buffer generateSimCADMesh(const std::filesystem::path& path, const vec2& pos,
    const vec2& size);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_SIMCAD__H__
