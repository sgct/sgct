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
#include <cstdarg>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#endif // WIN32

namespace {
    const char* levelToString(sgct::Log::Level level) {
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

void Log::printv(Level lvl, const char* fmt, va_list ap) {
    // prevent writing to console simultaneously
    std::unique_lock lock(_mutex);

#ifdef WIN32
    const size_t size = _vscprintf(fmt, ap) + 1; // null-terminating char
#else // WIN32
    // Workaround for calling vscprintf() or vscwprintf() in a non-windows OS
    va_list argsCopy;
    va_copy(argsCopy, ap);
    const size_t size = static_cast<size_t>(vsnprintf(nullptr, 0, fmt, argsCopy));
    va_end(argsCopy);
#endif // WIN32

    if (size > _parseBuffer.size()) {
        _parseBuffer.resize(size);
        std::fill(_parseBuffer.begin(), _parseBuffer.end(), char(0));
    }

    _parseBuffer[0] = '\0';

    vsprintf(_parseBuffer.data(), fmt, ap);
    va_end(ap); // Results Are Stored In Text

    std::vector<char> buffer(size + 64);
    std::fill(buffer.begin(), buffer.end(), char(0));

    // print local
    if (_showTime) {
        constexpr int TimeBufferSize = 9;
        char TimeBuffer[TimeBufferSize];
        time_t now = time(nullptr);
        tm* timeInfoPtr;
        timeInfoPtr = localtime(&now);
        strftime(TimeBuffer, TimeBufferSize, "%X", timeInfoPtr);

        if (_showLevel) {
            sprintf(
                buffer.data(),
                "%s | (%s) %s\n",
                TimeBuffer,
                levelToString(lvl),
                _parseBuffer.data()
            );
        }
        else {
            sprintf(buffer.data(), "%s | %s\n", TimeBuffer, _parseBuffer.data());
        }
    }
    else {
        if (_showLevel) {
            sprintf(buffer.data(), "(%s) %s\n", levelToString(lvl), _parseBuffer.data());
        }
        else {
            sprintf(buffer.data(), "%s\n", _parseBuffer.data());
        }
    }

    if (_logToConsole) {
        std::cout << buffer.data();
#ifdef WIN32
        OutputDebugStringA(buffer.data());
#endif // WIN32
    }

    if (_messageCallback) {
        _messageCallback(lvl, _parseBuffer.data());
    }
}

void Log::Debug(const char* fmt, ...) {
    if (instance()._level <= Level::Debug) {
        va_list ap;
        va_start(ap, fmt);
        instance().printv(Level::Debug, fmt, ap);
        va_end(ap);
    }
}

void Log::Info(const char* fmt, ...) {
    if (instance()._level <= Level::Info) {
        va_list ap;
        va_start(ap, fmt);
        instance().printv(Level::Info, fmt, ap);
        va_end(ap);
    }
}

void Log::Warning(const char* fmt, ...) {
    if (instance()._level <= Level::Warning) {
        va_list ap;
        va_start(ap, fmt);
        instance().printv(Level::Warning, fmt, ap);
        va_end(ap);
    }
}

void Log::Error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    instance().printv(Level::Error, fmt, ap);
    va_end(ap);
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
