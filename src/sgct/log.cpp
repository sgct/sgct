/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/log.h>

#include <sgct/networkmanager.h>
#include <sgct/mutexes.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <fmt/format.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
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
        time_t now = time(nullptr);
        tm* timeInfoPtr;
        timeInfoPtr = localtime(&now);
        strftime(TimeBuffer, TimeBufferSize, "%X", timeInfoPtr);

        message = fmt::format("{} | {}", TimeBuffer, message);
    }
    if (_showLevel) {
        message = fmt::format("({}) {}", levelToString(level), message);
    }

    if (_logToConsole) {
        std::cout << message;
#ifdef WIN32
        OutputDebugStringA(message.c_str());
#endif // WIN32
    }

    if (_messageCallback) {
        _messageCallback(level, message.c_str());
    }
}

void Log::Debug(std::string message) {
    if (instance()._level <= Level::Debug) {
        instance().printv(Level::Debug, std::move(message));
    }
}

void Log::Info(std::string message) {
    if (instance()._level <= Level::Info) {
        instance().printv(Level::Info, std::move(message));
    }
}

void Log::Warning(std::string message) {
    if (instance()._level <= Level::Warning) {
        instance().printv(Level::Warning, std::move(message));
    }
}

void Log::Error(std::string message) {
    if (instance()._level <= Level::Error) {
        instance().printv(Level::Error, std::move(message));
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

void Log::setLogCallback(std::function<void(Level, const char*)> fn) {
    _messageCallback = std::move(fn);
}

} // namespace sgct
