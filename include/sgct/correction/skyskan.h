/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_SKYSKAN__H__
#define __SGCT__CORRECTION_SKYSKAN__H__

#include <sgct/correction/buffer.h>
#include <string>

namespace sgct::core { class BaseViewport; }

namespace sgct::core::correction {

Buffer generateSkySkanMesh(const std::string& meshPath, BaseViewport& parent);

} // namespace sgct::core::correction

#endif // __SGCT__CORRECTION_SKYSKAN__H__
