/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/messagehandler.h>

#include <sgct/networkmanager.h>
#include <sgct/mutexmanager.h>
#include <sgct/helpers/portedfunctions.h>
#include <fstream>
#include <iostream>
#include <sstream>

namespace sgct {

MessageHandler* MessageHandler::_instance = nullptr;

MessageHandler* MessageHandler::instance() {
    if (_instance == nullptr) {
        _instance = new MessageHandler();
    }

    return _instance;
}

void MessageHandler::destroy() {
    delete _instance;
    _instance = nullptr;
}

MessageHandler::MessageHandler() {
    _recBuffer.reserve(_maxMessageSize);
    _buffer.reserve(_maxMessageSize);

    _parseBuffer.resize(_maxMessageSize);
    _combinedBuffer.resize(_combinedMessageSize);
    _headerSpace.resize(core::Network::HeaderSize, core::Network::DefaultId);
    _headerSpace[0] = core::Network::DataId;
    _buffer.insert(_buffer.begin(), _headerSpace.begin(), _headerSpace.end());

#ifdef __SGCT_DEBUG__
    _level = Level::Debug;
#else
    _level = Level::Warning;
#endif

    setLogPath(nullptr);
}

void MessageHandler::decode(std::vector<char> receivedData, int clientIndex) {
    std::unique_lock lock(MutexManager::instance()->dataSyncMutex);
    _recBuffer = std::move(receivedData);
    _recBuffer.push_back('\0');
    print("[client %d]: %s [end]", clientIndex, &_recBuffer[0]);
}

void MessageHandler::printv(const char* fmt, va_list ap) {
    // prevent writing to console simultaneously
    std::unique_lock lock(MutexManager::instance()->consoleMutex);

    size_t size = static_cast<size_t>(1 + vscprintf(fmt, ap));
    if (size > _maxMessageSize) {
        _parseBuffer.resize(size);
        std::fill(_parseBuffer.begin(), _parseBuffer.end(), char(0));
        
        _maxMessageSize = size;
        _combinedMessageSize = _maxMessageSize + 32;

        _combinedBuffer.resize(_combinedMessageSize);
        std::fill(_combinedBuffer.begin(), _combinedBuffer.end(), char(0));
        _recBuffer.resize(_maxMessageSize);
        _buffer.resize(_maxMessageSize);
    }
        
    _parseBuffer[0] = '\0';
    
#if (_MSC_VER >= 1400)
    // And Converts Symbols To Actual Numbers
    vsprintf_s(_parseBuffer.data(), _maxMessageSize, fmt, ap);
#else
    vsprintf(_parseBuffer.data(), fmt, ap);
#endif
    va_end(ap); // Results Are Stored In Text

    // print local
    if (_showTime) {
        constexpr int TimeBufferSize = 9;
        char TimeBuffer[TimeBufferSize];
        time_t now = time(nullptr);
#if (_MSC_VER >= 1400) //visual studio 2005 or later
        tm timeInfo;
        errno_t err = localtime_s(&timeInfo, &now);
        if (err == 0) {
            strftime(TimeBuffer, TimeBufferSize, "%X", &timeInfo);
        }
#else
        tm* timeInfoPtr;
        timeInfoPtr = localtime(&now);
        strftime(TimeBuffer, TimeBufferSize, "%X", timeInfoPtr);
#endif

#if (_MSC_VER >= 1400)
        sprintf_s(
            _combinedBuffer.data(),
            _combinedMessageSize,
            "%s| %s",
            TimeBuffer,
            _parseBuffer.data()
        );
#else
        sprintf(_combinedBuffer.data(), "%s| %s", TimeBuffer, _parseBuffer.data());
#endif

        if (_logToConsole) {
            std::cout << _combinedBuffer.data() << '\n';
        }

        if (_logToFile) {
            logToFile(_combinedBuffer);
        }
        if (_logToCallback && _messageCallback) {
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
        if (_logToCallback && _messageCallback) {
            _messageCallback(_parseBuffer.data());
        }
    }
}

void MessageHandler::logToFile(const std::vector<char>& buffer) {
    if (_filename.empty()) {
        return;
    }
    
    std::ofstream file(_filename, 'a');
    if (!file.good()) {
        std::cerr << "Failed to open '" << _filename << "'!" << std::endl;
        return;
    }

    file << std::string(buffer.begin(), buffer.end()) << '\n';
}

void MessageHandler::setLogPath(const char* path, int nodeId) {
    time_t now = time(nullptr);

    std::stringstream ss;

    if (path) {
        ss << path << "/";
    }

    char Buffer[64];
#if (_MSC_VER >= 1400)
    tm timeInfo;
    errno_t err = localtime_s(&timeInfo, &now);
    if (err == 0) {
        strftime(Buffer, 64, "SGCT_log_%Y_%m_%d_T%H_%M_%S", &timeInfo);
        if (nodeId > -1) {
            ss << Buffer << "_node" << nodeId << ".txt";
        }
        else {
            ss << Buffer << ".txt";
        }
    }
#else
    tm* timeInfoPtr;
    timeInfoPtr = localtime(&now);

    strftime(Buffer, 64, "SGCT_log_%Y_%m_%d_T%H_%M_%S", timeInfoPtr);
    if (nodeId > -1) {
        ss << Buffer << "_node" << nodeId << ".txt";
    }
    else {
        ss << Buffer << ".txt";
    }
#endif

    _filename = ss.str();
}

void MessageHandler::print(const char* fmt, ...) {
    if (fmt == nullptr) {
        // If There's No Text
        instance()->_parseBuffer[0] = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    instance()->printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::print(Level nl, const char* fmt, ...) {
    if (nl > instance()->getNotifyLevel() || fmt == nullptr) {
        // If There's No Text
        instance()->_parseBuffer[0] = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    instance()->printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::printDebug(const char* fmt, ...) {
    if (instance()->getNotifyLevel() <= Level::Debug || fmt == nullptr) {
        // If There's No Text
        instance()->_parseBuffer[0] = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    instance()->printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::printWarning(const char* fmt, ...) {
    if (instance()->getNotifyLevel() <= Level::Warning || fmt == nullptr) {
        // If There's No Text
        instance()->_parseBuffer[0] = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    instance()->printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::printInfo(const char* fmt, ...) {
    if (instance()->getNotifyLevel() <= Level::Info || fmt == nullptr) {
        // If There's No Text
        instance()->_parseBuffer[0] = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    instance()->printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::printImportant(const char* fmt, ...) {
    if (instance()->getNotifyLevel() <= Level::Important || fmt == nullptr) {
        // If There's No Text
        instance()->_parseBuffer[0] = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    instance()->printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::printError(const char* fmt, ...) {
    if (instance()->getNotifyLevel() <= Level::Error || fmt == nullptr) {
        // If There's No Text
        instance()->_parseBuffer[0] = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    instance()->printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::clearBuffer() {
    std::unique_lock lock(MutexManager::instance()->dataSyncMutex);
    _buffer.clear();
}

void MessageHandler::setNotifyLevel(Level nl) {
    _level = nl;
}

MessageHandler::Level MessageHandler::getNotifyLevel() {
    return static_cast<Level>(_level.load());
}

void MessageHandler::setShowTime(bool state) {
    _showTime = state;
}

void MessageHandler::setLogToConsole(bool state) {
    _logToConsole = state;
}

void MessageHandler::setLogToFile(bool state) {
    _logToFile = state;
}

void MessageHandler::setLogToCallback(bool state) {
    _logToCallback = state;
}

void MessageHandler::setLogCallback(std::function<void(const char *)> fn) {
    _messageCallback = std::move(fn);
}

size_t MessageHandler::getDataSize() {
    return _buffer.size();
}

char* MessageHandler::getMessage() {
    return _buffer.data();
}

} // namespace sgct
