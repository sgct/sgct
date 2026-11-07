/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/commandline.h>

#include <format>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace {
    template <typename T>
    std::optional<T> parseNumber(std::string_view flag, const std::string& value) {
        try {
            if constexpr (std::is_same_v<T, int>) {
                return std::stoi(value);
            }
            else {
                return std::stof(value);
            }
        }
        catch (const std::logic_error&) {
            // std::invalid_argument and std::out_of_range
            std::cerr << "Invalid value for " << flag << ": " << value << '\n';
            return std::nullopt;
        }
    }

    // Screenshot numbers start at 0, so negative values are invalid
    std::optional<int> parseScreenshotNumber(std::string_view flag,
                                             const std::string& value)
    {
        const std::optional<int> n = parseNumber<int>(flag, value);
        if (n.has_value() && *n < 0) {
            std::cerr << std::format("Invalid value for {}: {}\n", flag, value);
            return std::nullopt;
        }
        return n;
    }
} // namespace

namespace sgct {

Configuration parseArguments(std::vector<std::string>& arg) {
    Configuration config;

    size_t i = 0;
    while (i < arg.size()) {
        if ((arg[i] == "--config" || arg[i] == "-c") && arg.size() > (i + 1)) {
            config.configFilename = arg[i + 1];
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--client") {
            config.isServer = false;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--debug") {
            config.logLevel = Log::Level::Debug;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--help" || arg[i] == "-h") {
            config.showHelpText = true;
            arg.erase(arg.begin() + i);
        }
        else if ((arg[i] == "--local" || arg[i] == "-l") && arg.size() > (i + 1)) {
            config.isServer = true;
            config.nodeId = parseNumber<int>(arg[i], arg[i + 1]);
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--notify" && arg.size() > (i + 1)) {
            const Log::Level level = [](std::string_view l) {
                if (l == "error")        { return Log::Level::Error; }
                else if (l == "warning") { return Log::Level::Warning; }
                else if (l == "info")    { return Log::Level::Info; }
                else if (l == "debug")   { return Log::Level::Debug; }
                else {
                    std::cerr << "Unknown logger level: " << std::string(l) << '\n';
                    return Log::Level::Info;
                }
            } (arg[i + 1]);
            config.logLevel = level;

            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--firm-sync") {
            config.firmSync = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--loose-sync") {
            config.firmSync = false;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--ignore-sync") {
            config.ignoreSync = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--number-capture-threads" && arg.size() > (i + 1)) {
            const std::optional<int> n = parseNumber<int>(arg[i], arg[i + 1]);
            if (n.has_value()) {
                config.nCaptureThreads = std::max(*n, 1);
            }
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--screenshot-path" && arg.size() > (i + 1)) {
            config.screenshotPath = arg[i + 1];
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--screenshot-prefix" && arg.size() > (i + 1)) {
            config.screenshotPrefix = arg[i + 1];
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--screenshot-range-begin" && arg.size() > (i + 1)) {
            config.screenshotRangeBegin = parseScreenshotNumber(arg[i], arg[i + 1]);
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--screenshot-range-end" && arg.size() > (i + 1)) {
            config.screenshotRangeEnd = parseScreenshotNumber(arg[i], arg[i + 1]);
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--screenshot-bit-depth" && arg.size() > (i + 1)) {
            if (arg[i + 1] == "8" || arg[i + 1] == "16") {
                config.screenshotBitDepth = std::stoi(arg[i + 1]);
            }
            else {
                std::cerr <<
                    std::format("Invalid value for {}: {}\n", arg[i], arg[i + 1]);
            }
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--add-node-name-in-screenshot") {
            config.addNodeNameInScreenshot = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--omit-node-name-in-screenshot") {
            config.omitWindowNameInScreenshot = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--print-wait-message") {
            config.printWaitMessage = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--wait-timeout" && arg.size() > (i + 1)) {
            config.waitTimeout = parseNumber<float>(arg[i], arg[i + 1]);
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else {
            // Ignore unknown commands
            i++;
        }
    }

    if (config.screenshotRangeEnd &&
        config.screenshotRangeBegin.value_or(0) >= *config.screenshotRangeEnd)
    {
        std::cerr << "The beginning of the screenshot range has to be before its end\n";
        config.screenshotRangeBegin = std::nullopt;
        config.screenshotRangeEnd = std::nullopt;
    }

    return config;
}

std::string helpMessage() {
    return R"(
Parameters:
--config <filename.json> or -c <filename.json>
    Set configuration file
--help or -h
    Display help message and exit
--local <integer> or -l <integer>
    Force node in configuration to localhost (index starts at 0)
--client
    Run the application as client (only available when running as local)
--debug
    Set the notify level of Log to debug
--firm-sync
    Enable firm frame sync
--loose-sync
    Disable firm frame sync
--ignore-sync
    Disable frame sync
--notify <"error", "warning", "info", or "debug">
    Set the notify level used in the Log
--screenshot-path
    Sets the file path for the screenshots location
--screenshot-prefix
    Sets the prefix used for taking screenshots
--screenshot-range-begin <integer>
    Sets the number of the first screenshot that is stored
--screenshot-range-end <integer>
    Sets the number of the first screenshot that is no longer stored
--screenshot-bit-depth <8 or 16>
    Sets the number of bits per color channel of the screenshots
--add-node-name-in-screenshot
    If set, screenshots will contain the name of the node in multi-node configurations
--omit-node-name-in-screenshot
    If set, screenshots will not contain the name of the window if multiple windows exist
--number-capture-threads <integer>
    Set the maximum amount of thread that should be used during framecapture
--print-wait-message
    Print a message while waiting for the synchronization with the other nodes
--wait-timeout <float>
    Set the number of seconds to wait for the other nodes before aborting
)";
}

} // namespace sgct
