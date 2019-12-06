/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/log.h>

#include <sgct/networkmanager.h>
#include <sgct/mutexes.h>
#include <sgct/helpers/portedfunctions.h>
#include <fstream>
#include <iostream>
#include <sstream>

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
    _parseBuffer.resize(_maxMessageSize);
    _combinedBuffer.resize(_combinedMessageSize);
}

void Log::printv(const char* fmt, va_list ap) {
    // prevent writing to console simultaneously
    std::unique_lock lock(_mutex);

    size_t size = static_cast<size_t>(1 + vscprintf(fmt, ap));
    if (size > _maxMessageSize) {
        _parseBuffer.resize(size);
        std::fill(_parseBuffer.begin(), _parseBuffer.end(), char(0));
        
        _maxMessageSize = size;
        _combinedMessageSize = _maxMessageSize + 32;

        _combinedBuffer.resize(_combinedMessageSize);
        std::fill(_combinedBuffer.begin(), _combinedBuffer.end(), char(0));
    }
        
    _parseBuffer[0] = '\0';
    
    vsprintf(_parseBuffer.data(), fmt, ap);
    va_end(ap); // Results Are Stored In Text

    // print local
    if (_showTime) {
        constexpr int TimeBufferSize = 9;
        char TimeBuffer[TimeBufferSize];
        time_t now = time(nullptr);
        tm* timeInfoPtr;
        timeInfoPtr = localtime(&now);
        strftime(TimeBuffer, TimeBufferSize, "%X", timeInfoPtr);
        sprintf(_combinedBuffer.data(), "%s| %s", TimeBuffer, _parseBuffer.data());

        if (_logToConsole) {
            std::cout << _combinedBuffer.data() << '\n';
        }

        if (_messageCallback) {
            _messageCallback(_combinedBuffer.data());
        }
    }
    else {
        if (_logToConsole) {
            std::cout << _parseBuffer.data() << '\n';
        }

        if (_messageCallback) {
            _messageCallback(_parseBuffer.data());
        }
    }
}

void Log::Debug(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    instance().printv(fmt, ap);
    va_end(ap);
}

void Log::Warning(const char* fmt, ...) {
    if (instance()._level < Level::Warning || fmt == nullptr) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    instance().printv(fmt, ap);
    va_end(ap);
}

void Log::Info(const char* fmt, ...) {
    if (instance()._level < Level::Info || fmt == nullptr) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    instance().printv(fmt, ap);
    va_end(ap);
}

void Log::Error(const char* fmt, ...) {
    if (instance()._level < Level::Error || fmt == nullptr) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    instance().printv(fmt, ap);
    va_end(ap);
}

void Log::setNotifyLevel(Level nl) {
    _level = nl;
}

void Log::setShowTime(bool state) {
    _showTime = state;
}

void Log::setLogToConsole(bool state) {
    _logToConsole = state;
}

void Log::setLogCallback(std::function<void(const char*)> fn) {
    _messageCallback = std::move(fn);
}

} // namespace sgct
