/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/logger.h>

#include <sgct/networkmanager.h>
#include <sgct/mutexes.h>
#include <sgct/helpers/portedfunctions.h>
#include <fstream>
#include <iostream>
#include <sstream>

namespace sgct {

Logger* Logger::_instance = nullptr;

Logger& Logger::instance() {
    if (!_instance) {
        _instance = new Logger;
    }
    return *_instance;
}

void Logger::destroy() {
    delete _instance;
    _instance = nullptr;
}

Logger::Logger() {
    _parseBuffer.resize(_maxMessageSize);
    _combinedBuffer.resize(_combinedMessageSize);
    setLogPath("");
}

void Logger::printv(const char* fmt, va_list ap) {
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

        if (_logToFile) {
            logToFile(_combinedBuffer);
        }
        if (_messageCallback) {
            _messageCallback(_combinedBuffer.data());
        }
    }
    else {
        if (_logToConsole) {
            std::cout << _parseBuffer.data() << '\n';
        }

        if (_logToFile) {
            logToFile(_parseBuffer);
        }
        if (_messageCallback) {
            _messageCallback(_parseBuffer.data());
        }
    }
}

void Logger::logToFile(const std::vector<char>& buffer) {
    if (_filename.empty()) {
        return;
    }
    
    std::ofstream file(_filename.c_str(), std::ios_base::app);
    if (!file.good()) {
        std::cerr << "Failed to open '" << _filename << "'!" << std::endl;
        return;
    }

    file << std::string(buffer.begin(), buffer.end()) << '\n';
}

void Logger::setLogPath(const std::string& path, int id) {
    time_t now = time(nullptr);

    std::stringstream ss;

    if (!path.empty()) {
        ss << path << "/";
    }

    char Buffer[64];
    tm* timeInfoPtr;
    timeInfoPtr = localtime(&now);

    strftime(Buffer, 64, "SGCT_log_%Y_%m_%d_T%H_%M_%S", timeInfoPtr);
    if (id > -1) {
        ss << Buffer << "_node" << id << ".txt";
    }
    else {
        ss << Buffer << ".txt";
    }

    _filename = ss.str();
}

void Logger::Debug(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    instance().printv(fmt, ap);
    va_end(ap);
}

void Logger::Warning(const char* fmt, ...) {
    if (instance()._level < Level::Warning || fmt == nullptr) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    instance().printv(fmt, ap);
    va_end(ap);
}

void Logger::Info(const char* fmt, ...) {
    if (instance()._level < Level::Info || fmt == nullptr) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    instance().printv(fmt, ap);
    va_end(ap);
}

void Logger::Error(const char* fmt, ...) {
    if (instance()._level < Level::Error || fmt == nullptr) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    instance().printv(fmt, ap);
    va_end(ap);
}

void Logger::setNotifyLevel(Level nl) {
    _level = nl;
}

void Logger::setShowTime(bool state) {
    _showTime = state;
}

void Logger::setLogToConsole(bool state) {
    _logToConsole = state;
}

void Logger::setLogToFile(bool state) {
    _logToFile = state;
}

void Logger::setLogCallback(std::function<void(const char*)> fn) {
    _messageCallback = std::move(fn);
}

} // namespace sgct
