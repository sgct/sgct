/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vioso.h>

#include <sgct/error.h>
#include <fmt/format.h>

#define Error(code, msg) Error(Error::Component::MPCDI, code, msg)

namespace {

} // namespace

namespace sgct {

void loadViosoWarpFile(std::filesystem::path path) {
    if (!std::filesystem::is_regular_file(path)) {
        throw Error(10000, fmt::format("Could not find warping file '()'", path));
    }
}

} // namespace sgct
