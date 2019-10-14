/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_OBJ__H__
#define __SGCT__CORRECTION_OBJ__H__

#include <sgct/correction/buffer.h>
#include <string>

namespace sgct::core::correction {

Buffer generateOBJMesh(const std::string& path);

} // namespace sgct::core::correction

#endif // __SGCT__CORRECTION_OBJ__H__
