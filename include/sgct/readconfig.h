/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__READ_CONFIG__H__
#define __SGCT__READ_CONFIG__H__

#include <sgct/config.h>
#include <glm/glm.hpp>
#include <string>

namespace sgct::core::readconfig {

glm::quat parseMpcdiOrientationNode(float yaw, float pitch, float roll);

[[nodiscard]] sgct::config::Cluster readConfig(const std::string& filename);

} // namespace sgct_config

#endif // __SGCT__READ_CONFIG__H__
