/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__READCONFIG__H__
#define __SGCT__READCONFIG__H__

#include <sgct/config.h>
#include <string>

namespace sgct {

[[nodiscard]] config::Cluster readConfig(const std::string& filename);

[[nodiscard]] config::Cluster readJsonConfig(const std::string& configuration);

[[nodiscard]] std::string serializeConfig(const config::Cluster& cluster);

[[nodiscard]] std::string serializeConfig(const config::Cluster& cluster,
    const config::GeneratorVersion& genVersion);

} // namespace sgct

#endif // __SGCT__READCONFIG__H__
