#pragma once

#if defined(__APPLE__)
// On MacOS, assume std::format is not supported, use fmt::format
#include <fmt/format.h>
namespace sgctcompat {
    using fmt::format;
    using fmt::format_to_n;
}
#elif defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
// Use std::format if available
#include <format>
namespace sgctcompat {
    using std::format;
    using std::format_to_n;
}
#else
#include <fmt/format.h>
namespace sgctcompat {
    using fmt::format;
    using fmt::format_to_n;
}
#endif
