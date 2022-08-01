/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_SCISS__H__
#define __SGCT__CORRECTION_SCISS__H__

#include <sgct/correction/buffer.h>
#include <filesystem>

namespace sgct { class BaseViewport; }

namespace sgct::correction {

Buffer generateScissMesh(const std::filesystem::path& path, BaseViewport& parent);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_SCISS__H__
