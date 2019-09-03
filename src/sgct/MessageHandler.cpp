/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/MessageHandler.h>

#include <sgct/NetworkManager.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/helpers/SGCTPortedFunctions.h>
#include <fstream>
#include <iostream>
#include <sstream>

namespace sgct {

MessageHandler* MessageHandler::mInstance = nullptr;

MessageHandler* MessageHandler::instance() {
    if (mInstance == nullptr) {
        mInstance = new MessageHandler();
    }

    return mInstance;
}

void MessageHandler::destroy() {
    delete mInstance;
    mInstance = nullptr;
}

MessageHandler::MessageHandler() {
    mRecBuffer.reserve(mMaxMessageSize);
    mBuffer.reserve(mMaxMessageSize);

    mParseBuffer.resize(mMaxMessageSize);
    mCombinedBuffer.resize(mCombinedMessageSize);
    headerSpace.resize(
        sgct_core::SGCTNetwork::HeaderSize,
        sgct_core::SGCTNetwork::DefaultId
    );
    headerSpace[0] = sgct_core::SGCTNetwork::DataId;
    mBuffer.insert(mBuffer.begin(), headerSpace.begin(), headerSpace.end());

#ifdef __SGCT_DEBUG__
    mLevel = Level::Debug;
#else
    mLevel = Level::Warning;
#endif

    setLogPath(nullptr);
}

void MessageHandler::decode(std::vector<char> receivedData, int clientIndex) {
    std::unique_lock lock(SGCTMutexManager::instance()->mDataSyncMutex);
    mRecBuffer = std::move(receivedData);
    mRecBuffer.push_back('\0');
    print("\n[client %d]: %s [end]\n", clientIndex, &mRecBuffer[0]);
}

void MessageHandler::printv(const char* fmt, va_list ap) {
    // prevent writing to console simultaneously
    std::unique_lock lock(SGCTMutexManager::instance()->mConsoleMutex);

    size_t size = static_cast<size_t>(1 + vscprintf(fmt, ap));
    if (size > mMaxMessageSize) {
        mParseBuffer.resize(size);
        std::fill(mParseBuffer.begin(), mParseBuffer.end(), 0);
        
        mMaxMessageSize = size;
        mCombinedMessageSize = mMaxMessageSize + 32;

        mCombinedBuffer.resize(mCombinedMessageSize);
        std::fill(mCombinedBuffer.begin(), mCombinedBuffer.end(), 0);
        mRecBuffer.resize(mMaxMessageSize);
        mBuffer.resize(mMaxMessageSize);
    }
        
    mParseBuffer[0] = '\0';
    
#if (_MSC_VER >= 1400)
    // And Converts Symbols To Actual Numbers
    vsprintf_s(mParseBuffer.data(), mMaxMessageSize, fmt, ap);
#else
    vsprintf(mParseBuffer.data(), fmt, ap);
#endif
    va_end(ap); // Results Are Stored In Text

    // print local
    if (mShowTime) {
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
        strftime(TimeBuffer, TIME_BUFFER_SIZE, "%X", timeInfoPtr);
#endif

#if (_MSC_VER >= 1400)
        sprintf_s(
            mCombinedBuffer.data(),
            mCombinedMessageSize,
            "%s| %s",
            TimeBuffer,
            mParseBuffer.data()
        );
#else
        sprintf(mCombinedBuffer.data(), "%s| %s", TimeBuffer, mParseBuffer.data());
#endif
        if (mLogToConsole) {
            std::cerr << mCombinedBuffer.data();
        }

        if (mLogToFile) {
            logToFile(mCombinedBuffer);
        }
        if (mLogToCallback && mMessageCallback) {
            mMessageCallback(mCombinedBuffer.data());
        }
    }
    else {
        if (mLogToConsole) {
            std::cerr << mParseBuffer.data();
        }

        if (mLogToFile) {
            logToFile(mParseBuffer);
        }
        if (mLogToCallback && mMessageCallback) {
            mMessageCallback(mParseBuffer.data());
        }
    }
}

void MessageHandler::logToFile(const std::vector<char>& buffer) {
    if (mFilename.empty()) {
        return;
    }
    
    std::ofstream file(mFilename, 'a');
    if (!file.good()) {
        std::cerr << "Failed to open '" << mFilename << "'!" << std::endl;
        return;
    }

    file << std::string(buffer.begin(), buffer.end());
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

    mFilename = ss.str();
}

void MessageHandler::print(const char* fmt, ...) {
    if (fmt == nullptr) {
        // If There's No Text
        mParseBuffer[0] = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::print(Level nl, const char* fmt, ...) {
    if (nl > getNotifyLevel() || fmt == nullptr) {
        // If There's No Text
        mParseBuffer[0] = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::clearBuffer() {
    std::unique_lock lock(SGCTMutexManager::instance()->mDataSyncMutex);
    mBuffer.clear();
}

void MessageHandler::setNotifyLevel(Level nl) {
    mLevel = nl;
}

MessageHandler::Level MessageHandler::getNotifyLevel() {
    return static_cast<Level>(mLevel.load());
}

void MessageHandler::setShowTime(bool state) {
    mShowTime = state;
}

void MessageHandler::setLogToConsole(bool state) {
    mLogToConsole = state;
}

void MessageHandler::setLogToFile(bool state) {
    mLogToFile = state;
}

void MessageHandler::setLogToCallback(bool state) {
    mLogToCallback = state;
}

void MessageHandler::setLogCallback(std::function<void(const char *)> fn) {
    mMessageCallback = std::move(fn);
}

size_t MessageHandler::getDataSize() {
    return mBuffer.size();
}

char* MessageHandler::getMessage() {
    return mBuffer.data();
}

} // namespace sgct
