/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_OBJ__H__
#define __SGCT__CORRECTION_OBJ__H__

#include <sgct/correction/buffer.h>
#include <string>

namespace sgct::correction {

Buffer generateOBJMesh(const std::string& path);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_OBJ__H__
