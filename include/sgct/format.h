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
// Prefer sgct::format if available, otherwise use the fmt library for Mac
#ifndef __clang__
    #include <format>
    // If you need a formatter for std::filesystem::path in sgct::format, you can add it here if not provided by your standard library.

    template <>
    struct std::formatter<std::filesystem::path> {
        constexpr auto parse(std::format_parse_context& ctx) {
            return ctx.begin();
        }

        auto format(const std::filesystem::path& path, std::format_context& ctx) const {
            return std::format_to(ctx.out(), "{}", path.string());
        }
    };
#else
    #include <fmt/format.h>

    template <>
    struct fmt::formatter<std::filesystem::path> {
        constexpr auto parse(fmt::format_parse_context& ctx) {
            return ctx.begin();
        }

        auto format(const std::filesystem::path& path, fmt::format_context& ctx) const {
            return fmt::format_to(ctx.out(), "{}", path.string());
        }
    };
#endif // ifndef __clang__

#endif // __SGCT__FMT__H__
