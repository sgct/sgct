/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_PFM__H__
#define __SGCT__CORRECTION_PFM__H__

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <sgct/correction/buffer.h>
#include <filesystem>

namespace sgct::correction {

SGCT_EXPORT Buffer generatePerEyeMeshFromPFMImage(const std::filesystem::path& path,
    const vec2& pos, const vec2& size, bool textureRenderMode = false);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_PFM__H__
