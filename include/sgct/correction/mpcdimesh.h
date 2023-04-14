/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_MPCDIMESH__H__
#define __SGCT__CORRECTION_MPCDIMESH__H__

#include <sgct/sgctexports.h>
#include <sgct/correction/buffer.h>

namespace sgct::correction {

SGCT_EXPORT Buffer generateMpcdiMesh(const std::vector<char>& mpcdiMesh);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_MPCDIMESH__H__

