/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__FMT__H__
#define __SGCT__FMT__H__

#include <filesystem>
#include <format>

template <>
struct std::formatter<std::filesystem::path> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const std::filesystem::path& path, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", path.string());
    }
};

#endif // __SGCT__FMT__H__
