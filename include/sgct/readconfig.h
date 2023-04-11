/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__READCONFIG__H__
#define __SGCT__READCONFIG__H__

#include <sgct/sgctexports.h>
#include <sgct/config.h>
#include <string>

namespace sgct {

SGCT_EXPORT [[nodiscard]] config::Cluster readConfig(const std::string& filename);

SGCT_EXPORT [[nodiscard]] config::Cluster readJsonConfig(std::string_view configuration);

SGCT_EXPORT [[nodiscard]] std::string serializeConfig(const config::Cluster& cluster,
    std::optional<config::GeneratorVersion> genVersion = std::nullopt);

} // namespace sgct

#endif // __SGCT__READCONFIG__H__
