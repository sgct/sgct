/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/commandline.h>

#include <iostream>
#include <string_view>

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
            config.nodeId = std::stoi(arg[i + 1]);
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--notify" && arg.size() > (i + 1)) {
            const Log::Level level = [](std::string_view l) {
                if (l == "error")        { return Log::Level::Error; }
                else if (l == "warning") { return Log::Level::Warning; }
                else if (l == "info")    { return Log::Level::Info; }
                else if (l == "debug")   { return Log::Level::Debug; }
                else {
                    std::cerr << "Unknown logger level: " << std::string(l);
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
            config.nCaptureThreads = std::stoi(arg[i + 1]);
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--screenshot-path") {
            config.screenshotPath = arg[i + 1];
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "--screenshot-prefix") {
            config.screenshotPrefix = arg[i + 1];
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
        else if (arg[i] == "--wait-timeout") {
            config.waitTimeout = std::stof(arg[i + 1]);
            arg.erase(arg.begin() + 1, arg.begin() + i + 2);
        }
        else {
            // Ignore unknown commands
            i++;
        }
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
--capture-jpg
    Use jpg images for screen capture
--capture-tga
    Use tga images for screen capture
--export-correction-meshes
    Exports the correction warping meshes to OBJ files when loading them
--screenshot-path
    Sets the file path for the screenshots location
--screenshot-prefix
    Sets the prefix used for taking screenshots
--add-node-name-in-screenshot
    If set, screenshots will contain the name of the node in multi-node configurations
--omit-node-name-in-screenshot
    If set, screenshots will not contain the name of the window if multiple windows exist
--number-capture-threads <integer>
    Set the maximum amount of thread that should be used during framecapture
)";
}

} // namespace sgct
