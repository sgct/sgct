/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/commandline.h>

namespace sgct {

Configuration parseArguments(std::vector<std::string>& arg) {
    Configuration config;

    size_t i = 0;
    while (i < arg.size()) {
        if (arg[i] == "-config" && arg.size() > (i + 1)) {
            config.configFilename = arg[i + 1];
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-client") {
            config.isServer = false;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-debug") {
            config.logLevel = Logger::Level::Debug;
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
        else if (arg[i] == "-log") {
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
            config.logLevel = static_cast<Logger::Level>(level);

            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-firm-sync") {
            config.firmSync = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-loose-sync") {
            config.firmSync = false;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-ignore-sync" || arg[i] == "-no-sync") {
            config.ignoreSync = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-fxaa") {
            config.fxaa = true;
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-msaa" && arg.size() > (i + 1)) {
            config.msaaSamples = std::stoi(arg[i + 1]);
            arg.erase(arg.begin() + i);
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
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
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

std::string getHelpMessage() {
    return R"(
Parameters:
-config <filename.xml>
    Set XML configuration file
-log <filepath>
    Set log file path
-help
    Display help message and exit
-local <integer>
    Force node in configuration to localhost (index starts at 0)
-client
    Run the application as client\n\t(only available when running as local)
-debug
    Set the notify level of messagehandler to debug
-firm-sync
    Enable firm frame sync
-loose-sync
    Disable firm frame sync
-ignore-sync
    Disable frame sync
-msaa <integer>
    Enable MSAA as default (argument must be a power of two)
-fxaa
    Enable FXAA as default
-notify <integer>
    Set the notify level used in the MessageHandler\n\t(0 = highest priority)
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
