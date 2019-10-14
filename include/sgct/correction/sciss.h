/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_SCISS__H__
#define __SGCT__CORRECTION_SCISS__H__

#include <sgct/correction/buffer.h>
#include <string>

namespace sgct::core { class Viewport; }

namespace sgct::core::correction {

Buffer generateScissMesh(const std::string& path, sgct::core::Viewport& parent);

} // namespace sgct::core::correction

#endif // __SGCT__CORRECTION_SCISS__H__
