/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/commandline.h>

#include <string_view>

namespace sgct {

Configuration parseArguments(std::vector<std::string> arg) {
    Configuration config;

    size_t i = 0;
    while (i < arg.size()) {
        if (arg[i] == "-config" && arg.size() > (i + 1)) {
            config.configFilename = arg[i + 1];
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-client" || arg[i] == "-slave") {
            config.isServer = false;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-debug") {
            config.logLevel = MessageHandler::Level::Debug;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-help") {
            config.showHelpText = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-local" && arg.size() > (i + 1)) {
            config.isServer = true;
            config.nodeId = std::stoi(arg[i + 1]);
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-logPath") {
            // Remove unwanted chars
            std::string tmpStr = arg[i + 1];
            tmpStr.erase(remove(tmpStr.begin(), tmpStr.end(), '\"'), tmpStr.end());
            size_t lastPos = tmpStr.length() - 1;

            // Remove a trailing /
            const char last = tmpStr.at(lastPos);
            if (last == '\\' || last == '/') {
                tmpStr.erase(lastPos);
            }
            config.logPath = tmpStr;

            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-notify" && arg.size() > (i + 1)) {
            const int level = std::stoi(arg[i + 1]);
            config.logLevel = static_cast<MessageHandler::Level>(level);

            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-Firm-Sync") {
            config.firmSync = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-Loose-Sync") {
            config.firmSync = false;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-Ignore-Sync" || arg[i] == "-No-Sync") {
            config.ignoreSync = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-FXAA") {
            config.fxaa = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-MSAA" && arg.size() > (i + 1)) {
            config.msaaSamples = std::stoi(arg[i + 1]);
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-Capture-TGA") {
            config.captureFormat = Settings::CaptureFormat::TGA;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-Capture-PNG") {
            config.captureFormat = Settings::CaptureFormat::PNG;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-Capture-JPG") {
            config.captureFormat = Settings::CaptureFormat::JPG;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-numberOfCaptureThreads" && arg.size() > (i + 1)) {
            config.nCaptureThreads = std::stoi(arg[i + 1]);
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else {
            // Ignore unknown commands
            i++;
        }
    }

    return config;
}

std::string_view getHelpMessage() {
    return R"(
Parameters:
-config <filename.xml>
    Set XML confiuration file
-logPath <filepath>
    Set log file path
-help
    Display help message and exit
-local <integer>
    Force node in configuration to localhost (index starts at 0)
-client
    Run the application as client\n\t(only available when running as local)
-slave
    Run the application as client\n\t(only available when running as local)
-debug
    Set the notify level of messagehandler to debug
-Firm-Sync
    Enable firm frame sync
-Loose-Sync
    Disable firm frame sync
-Ignore-Sync
    Disable frame sync
-MSAA <integer>
    Enable MSAA as default (argument must be a power of two)
-FXAA
    Enable FXAA as default
-notify <integer>
    Set the notify level used in the MessageHandler\n\t(0 = highest priority)
-Capture-PNG
    Use png images for screen capture (default)
-Capture-JPG
    Use jpg images for screen capture
-Capture-TGA
    Use tga images for screen capture
-numberOfCaptureThreads <integer>
    Set the maximum amount of thread that should be used during framecapture
)";
}

} // namespace sgct
