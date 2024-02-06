/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/log.h>

#include <sgct/fmt.h>
#include <sgct/networkmanager.h>
#include <sgct/mutexes.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdarg>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VC_EXTRALEAN
#include <Windows.h>
#endif // WIN32

#include <cstdarg> // va_copy

namespace {
    std::string_view levelToString(sgct::Log::Level level) {
        switch (level) {
            case sgct::Log::Level::Debug: return "Debug";
            case sgct::Log::Level::Info: return "Info";
            case sgct::Log::Level::Warning: return "Warning";
            case sgct::Log::Level::Error: return "Error";
            default: throw std::logic_error("Missing case label");
        }
    }
} // namespace

namespace sgct {

Log* Log::_instance = nullptr;

Log& Log::instance() {
    if (!_instance) {
        _instance = new Log;
    }
    return *_instance;
}

void Log::destroy() {
    delete _instance;
    _instance = nullptr;
}

Log::Log() {
    _parseBuffer.resize(128);
}

void Log::printv(Level level, std::string message) {
    if (_showTime) {
        constexpr int TimeBufferSize = 9;
        char TimeBuffer[TimeBufferSize];
        time_t now = ::time(nullptr);
        tm* timeInfoPtr;
        timeInfoPtr = localtime(&now);
        strftime(TimeBuffer, TimeBufferSize, "%X", timeInfoPtr);

        message = fmt::format("{} | {}", TimeBuffer, message);
    }
    if (_showLevel) {
        message = fmt::format("({}) {}", levelToString(level), message);
    }

    if (_logToConsole) {
        // We need an endl here to make sure that any application listening to our log
        // messages (looking at you C-Troll) is actually getting the messages immediately.
        // If the `flush` doesn't happen here, all of the messages stack up in the buffer
        // and are only sent once the application is finished, which is no bueno
        std::cout << message << std::endl;
#ifdef WIN32
        OutputDebugStringA((message + '\n').c_str());
#endif // WIN32
    }

    if (_messageCallback) {
        _messageCallback(level, message);
    }
}

void Log::Debug(std::string_view message) {
    if (instance()._level <= Level::Debug) {
        instance().printv(Level::Debug, std::string(message));
    }
}

void Log::Info(std::string_view message) {
    if (instance()._level <= Level::Info) {
        instance().printv(Level::Info, std::string(message));
    }
}

void Log::Warning(std::string_view message) {
    if (instance()._level <= Level::Warning) {
        instance().printv(Level::Warning, std::string(message));
    }
}

void Log::Error(std::string_view message) {
    if (instance()._level <= Level::Error) {
        instance().printv(Level::Error, std::string(message));
    }
}

void Log::setNotifyLevel(Level nl) {
    _level = nl;
}

void Log::setShowTime(bool state) {
    _showTime = state;
}

void Log::setShowLogLevel(bool state) {
    _showLevel = state;
}

void Log::setLogToConsole(bool state) {
    _logToConsole = state;
}

void Log::setLogCallback(std::function<void(Level, std::string_view)> fn) {
    _messageCallback = std::move(fn);
}

} // namespace sgct
