/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__READCONFIG__H__
#define __SGCT__READCONFIG__H__

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <sgct/config.h>
#include <string>

namespace sgct {

[[nodiscard]] config::Cluster readConfig(const std::string& filename);

[[nodiscard]] config::Cluster readJsonConfig(std::string_view configuration);

[[nodiscard]] std::string serializeConfig(const config::Cluster& cluster,
    std::optional<config::GeneratorVersion> genVersion = std::nullopt);

class custom_error_handler : public nlohmann::json_schema::basic_error_handler
{
public:
    void error(const nlohmann::json::json_pointer &ptr, const json &instance,
               const std::string &message) override;
    bool validationSucceeded();
    std::string& message();
private:
    std::string mErrMessage;
};

} // namespace sgct

#endif // __SGCT__READCONFIG__H__
