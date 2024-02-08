/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_OBJ__H__
#define __SGCT__CORRECTION_OBJ__H__

#include <sgct/sgctexports.h>
#include <sgct/correction/buffer.h>
#include <filesystem>

namespace sgct::correction {

SGCT_EXPORT Buffer generateOBJMesh(const std::filesystem::path& path);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_OBJ__H__
