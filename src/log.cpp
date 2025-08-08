/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/log.h>

#include <sgct/format.h>
#include <sgct/networkmanager.h>
#include <sgct/mutexes.h>
#include <cstdarg>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

namespace {
    std::string_view levelToString(sgct::Log::Level level) {
        switch (level) {
            case sgct::Log::Level::Debug:   return "Debug";
            case sgct::Log::Level::Info:    return "Info";
            case sgct::Log::Level::Warning: return "Warning";
            case sgct::Log::Level::Error:   return "Error";
            default:                        throw std::logic_error("Missing case label");
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
        std::array<char, TimeBufferSize> timeBuffer;
        const time_t now = ::time(nullptr);
        tm* timeInfoPtr = nullptr;
        timeInfoPtr = localtime(&now);
        strftime(timeBuffer.data(), TimeBufferSize, "%X", timeInfoPtr);

        message = std::format("{} | {}", timeBuffer.data(), message);
    }
    if (_showLevel) {
        message = std::format("({}) {}", levelToString(level), message);
    }

    if (_logToConsole) {
        // We need an endl here to make sure that any application listening to our log
        // messages (looking at you C-Troll) is actually getting the messages immediately.
        // If the `flush` doesn't happen here, all of the messages stack up in the buffer
        // and are only sent once the application is finished, which is no bueno
        std::cout << message << '\n';
#ifdef WIN32
        OutputDebugStringA((message + '\n').c_str());
#endif // WIN32
    }

    if (_messageCallback) {
        _messageCallback(level, message);
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

} // namespace sgct
