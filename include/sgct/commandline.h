/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__COMMANDLINE__H__
#define __SGCT__COMMANDLINE__H__

#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <optional>
#include <string>
#include <vector>

namespace sgct {

struct Configuration {
    std::optional<std::string> configFilename;
    std::optional<bool> isServer;
    std::optional<std::string> logPath;
    std::optional<MessageHandler::Level> logLevel;
    std::optional<bool> showHelpText;
    std::optional<int> nodeId;
    std::optional<bool> firmSync;
    std::optional<bool> ignoreSync;
    std::optional<bool> fxaa;
    std::optional<int> msaaSamples;
    std::optional<Settings::CaptureFormat> captureFormat;
    std::optional<int> nCaptureThreads;
    std::optional<bool> checkOpenGL;
};

/**
 * Command line parameters are used to load a configuration file and settings. Note that
 * parameter with one '\-' are followed by arguments but parameters with '\-\-' are just
 * options without arguments.
 */
Configuration parseArguments(std::vector<std::string> arg);

std::string_view getHelpMessage();

} // namespace sgct

#endif // __SGCT__COMMANDLINE__H__
