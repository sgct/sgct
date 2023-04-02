/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__READCONFIG__H__
#define __SGCT__READCONFIG__H__

#include <sgct/config.h>
#include <filesystem>
#include <string>

namespace sgct {

[[nodiscard]] config::Cluster readConfig(const std::string& filename,
    const std::string additionalErrorDescription = "");

[[nodiscard]] config::Cluster readJsonConfig(std::string_view configuration);

[[nodiscard]] config::GeneratorVersion readJsonGeneratorVersion(
    const std::string& configuration);

[[nodiscard]] config::GeneratorVersion readConfigGenerator(const std::string& filename);

[[nodiscard]] std::string serializeConfig(const config::Cluster& cluster,
    std::optional<config::GeneratorVersion> genVersion = std::nullopt);

[[nodiscard]] std::string stringifyJsonFile(const std::string& filename);

bool loadFileAndSchemaThenValidate(const std::string& config, const std::string& schema,
    const std::string& validationTypeExplanation);

bool validateConfigAgainstSchema(const std::string& stringifiedConfig,
    const std::string& stringifiedSchema, std::filesystem::path& schemaDir);

void convertToSgctExceptionAndThrow(const std::string& schema,
    const std::string& validationTypeExplanation, const std::string& exceptionMessage);

} // namespace sgct

#endif // __SGCT__READCONFIG__H__
