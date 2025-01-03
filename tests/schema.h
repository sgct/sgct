/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT_TEST__SCHEMA__H__
#define __SGCT_TEST__SCHEMA__H__

#include <nlohmann/json-schema.hpp>
#include <sgct/config.h>

struct ParsingError : std::runtime_error {
    explicit ParsingError(std::string msg) : std::runtime_error(std::move(msg)) {}
};

inline void validate(std::string_view cfgString) {
    const std::string schema = std::string(BASE_PATH) + "/sgct.schema.json";
    std::string err = sgct::validateConfigAgainstSchema(cfgString, schema);
    if (!err.empty()) {
        throw ParsingError(err);
    }
}

#endif // __SGCT_TEST__SCHEMA__H__
