/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__READCONFIG__H__
#define __SGCT__READCONFIG__H__

#include <sgct/sgctexports.h>
#include <sgct/config.h>
#include <filesystem>
#include <string>

namespace sgct {

SGCT_EXPORT [[nodiscard]] config::Cluster readConfig(const std::string& filename,
    const std::string additionalErrorDescription = "");

SGCT_EXPORT [[nodiscard]] config::Cluster readJsonConfig(std::string_view configuration);

SGCT_EXPORT [[nodiscard]] config::GeneratorVersion readJsonGeneratorVersion(
    const std::string& configuration);

SGCT_EXPORT [[nodiscard]] config::GeneratorVersion readConfigGenerator(
    const std::string& filename);

SGCT_EXPORT [[nodiscard]] std::string serializeConfig(const config::Cluster& cluster,
    std::optional<config::GeneratorVersion> genVersion = std::nullopt);

SGCT_EXPORT [[nodiscard]] std::string stringifyJsonFile(const std::string& filename);

SGCT_EXPORT bool loadFileAndSchemaThenValidate(const std::string& config,
    const std::string& schema, const std::string& validationTypeExplanation);

SGCT_EXPORT bool validateConfigAgainstSchema(const std::string& stringifiedConfig,
    const std::string& stringifiedSchema, std::filesystem::path& schemaDir);

SGCT_EXPORT [[noreturn]] void convertToSgctExceptionAndThrow(const std::string& schema,
    const std::string& validationTypeExplanation, const std::string& exceptionMessage);

/**
 * Reads and returns meta information in the configuration file. Since meta is
 * non-critical, and is composed of only strings (blank by default), a Meta object is
 * always returned even if the source file does not contain any meta information.
 */
SGCT_EXPORT [[nodiscard]] sgct::config::Meta readMeta(const std::string& filename);

} // namespace sgct

#endif // __SGCT__READCONFIG__H__
