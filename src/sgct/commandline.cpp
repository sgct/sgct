/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/commandline.h>

#include <iostream>

namespace sgct {

Configuration parseArguments(std::vector<std::string>& arg) {
    Configuration config;

    size_t i = 0;
    while (i < arg.size()) {
        if (arg[i] == "-config" && arg.size() > (i + 1)) {
            config.configFilename = arg[i + 1];
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "-client") {
            config.isServer = false;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-debug") {
            config.logLevel = Log::Level::Debug;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-help") {
            config.showHelpText = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-local" && arg.size() > (i + 1)) {
            config.isServer = true;
            config.nodeId = std::stoi(arg[i + 1]);
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "-notify" && arg.size() > (i + 1)) {
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
        else if (arg[i] == "-firm-sync") {
            config.firmSync = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-loose-sync") {
            config.firmSync = false;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-ignore-sync") {
            config.ignoreSync = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-capture-tga") {
            config.captureFormat = Settings::CaptureFormat::TGA;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-capture-png") {
            config.captureFormat = Settings::CaptureFormat::PNG;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-capture-jpg") {
            config.captureFormat = Settings::CaptureFormat::JPG;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-number-capture-threads" && arg.size() > (i + 1)) {
            config.nCaptureThreads = std::stoi(arg[i + 1]);
            arg.erase(arg.begin() + i, arg.begin() + i + 2);
        }
        else if (arg[i] == "-check-opengl") {
            config.checkOpenGL = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-check-fbos") {
            config.checkFBOs = true;
            arg.erase(arg.begin() + i);
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
-config <filename.xml>
    Set XML configuration file
-help
    Display help message and exit
-local <integer>
    Force node in configuration to localhost (index starts at 0)
-client
    Run the application as client (only available when running as local)
-debug
    Set the notify level of messagehandler to debug
-firm-sync
    Enable firm frame sync
-loose-sync
    Disable firm frame sync
-ignore-sync
    Disable frame sync
-notify <integer>
    Set the notify level used in the MessageHandler (0 = highest priority)
-capture-png
    Use png images for screen capture (default)
-capture-jpg
    Use jpg images for screen capture
-capture-tga
    Use tga images for screen capture
-number-capture-threads <integer>
    Set the maximum amount of thread that should be used during framecapture
-check-opengl
    Enables checking of OpenGL calls. This will reduce the overall performance
-check-fbos
    Enables the checking of framebuffer objects after window creation
)";
}

} // namespace sgct
