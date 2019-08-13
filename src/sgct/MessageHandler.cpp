/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/MessageHandler.h>

#include <sgct/NetworkManager.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/helpers/SGCTPortedFunctions.h>
#include <iostream>
#include <sstream>

namespace sgct {

MessageHandler* MessageHandler::mInstance = nullptr;

/*! Get the MessageHandler instance */
MessageHandler* MessageHandler::instance() {
    if (mInstance == nullptr) {
        mInstance = new MessageHandler();
    }

    return mInstance;
}

/*! Destroy the MessageHandler */
void MessageHandler::destroy() {
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
    }
}

MessageHandler::MessageHandler() {
    //nothrow makes sure that a null pointer is returned upon failure
    mParseBuffer    = new (std::nothrow) char[mMaxMessageSize];
    mCombinedBuffer = new (std::nothrow) char[mCombinedMessageSize];
    headerSpace     = new (std::nothrow) unsigned char[sgct_core::SGCTNetwork::mHeaderSize];

    if (!headerSpace || !mCombinedBuffer || !headerSpace) {
        fprintf(stderr, "Fatal error while allocating memory for MessageHandler!\n");
        return;
    }

#ifdef __SGCT_DEBUG__
    mLevel = Level::Debug;
#else
    mLevel = Level::Warning;
#endif

    mRecBuffer.reserve(mMaxMessageSize);
    mBuffer.reserve(mMaxMessageSize);

    headerSpace[0] = sgct_core::SGCTNetwork::DataId;
    
    //fill rest of header with SGCTNetwork::DefaultId
    memset(
        headerSpace + 1,
        sgct_core::SGCTNetwork::DefaultId,
        sgct_core::SGCTNetwork::mHeaderSize - 1
    );
    
    mBuffer.insert(
        mBuffer.begin(),
        headerSpace,
        headerSpace + sgct_core::SGCTNetwork::mHeaderSize
    );

    setLogPath(nullptr);
}

MessageHandler::~MessageHandler() {
    mMessageCallback = nullptr;

    if (mParseBuffer) {
        delete[] mParseBuffer;
    }
    mParseBuffer = nullptr;

    if (mCombinedBuffer) {
        delete[] mCombinedBuffer;
    }
    mCombinedBuffer = nullptr;

    if (headerSpace) {
        delete[] headerSpace;
    }
    headerSpace = nullptr;

    mBuffer.clear();
    mRecBuffer.clear();
}

void MessageHandler::decode(const char* receivedData, int receivedlength,
                                  int clientIndex)
{
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::DataSyncMutex);
    mRecBuffer.clear();
    mRecBuffer.insert(mRecBuffer.end(), receivedData, receivedData + receivedlength);
    mRecBuffer.push_back('\0');
    print("\n[client %d]: %s [end]\n", clientIndex, &mRecBuffer[0]);
    SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::DataSyncMutex );
}

void MessageHandler::printv(const char*fmt, va_list ap) {
    //prevent writing to console simultaneously
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::ConsoleMutex);

    size_t size = static_cast<size_t>(1 + vscprintf(fmt, ap));
    if (size > mMaxMessageSize) {
        delete[] mParseBuffer;
        mParseBuffer = new (std::nothrow) char[size];
        memset(mParseBuffer, 0, size);
        
        mMaxMessageSize = size;
        mCombinedMessageSize = mMaxMessageSize + 32;

        delete[] mCombinedBuffer;
        mCombinedBuffer = new (std::nothrow) char[mCombinedMessageSize];
        if (mCombinedBuffer == nullptr) {
            return;
        }

        memset(mCombinedBuffer, 0, mCombinedMessageSize);
        
        mRecBuffer.resize(mMaxMessageSize);
        mBuffer.resize(mMaxMessageSize);
    }
        
    mParseBuffer[0] = '\0';
    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    vsprintf_s(mParseBuffer, mMaxMessageSize, fmt, ap);    // And Converts Symbols To Actual Numbers
#else
    vsprintf(mParseBuffer, fmt, ap);
#endif
    va_end(ap);        // Results Are Stored In Text

    //print local
    if (getShowTime()) {
#if (_MSC_VER >= 1400) //visual studio 2005 or later
        sprintf_s(
            mCombinedBuffer,
            mCombinedMessageSize,
            "%s| %s",
            getTimeOfDayStr(),
            mParseBuffer
        );
#else
        sprintf(mCombinedBuffer, "%s| %s", getTimeOfDayStr(), mParseBuffer);
#endif
        if (mLogToConsole) {
            std::cerr << mCombinedBuffer;
        }

        if (mLogToFile) {
            logToFile(mCombinedBuffer);
        }
        if (mLogToCallback && mMessageCallback != nullptr) {
            mMessageCallback(mCombinedBuffer);
        }
    }
    else {
        if (mLogToConsole) {
            std::cerr << mParseBuffer;
        }

        if (mLogToFile) {
            logToFile(mParseBuffer);
        }
        if (mLogToCallback && mMessageCallback != nullptr) {
            mMessageCallback(mParseBuffer);
        }
    }

    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::ConsoleMutex);

    //if client send to server
    sendMessageToServer(mParseBuffer);
}

void MessageHandler::logToFile(const char* buffer) {
    if (mFilename.empty()) {
        return;
    }
    
    FILE* pFile = nullptr;
    bool error = false;

#if (_MSC_VER >= 1400) //visual studio 2005 or later
    errno_t err = fopen_s(&pFile, mFilename.c_str(), "a");
    if (err != 0 || !pFile) {
        error = true;
    }
#else
    pFile = fopen(mFilename.c_str(), "a");
    if (pFile == nullptr) {
        error = true;
    }
#endif

    if (error) {
        std::cerr << "Failed to open '" << mFilename << "'!" << std::endl;
        return;
    }

    fprintf(pFile, "%s", buffer);
    fclose(pFile);
}

/*!
 Set the log file path/directoy. The nodeId is optional and will be appended on the
 filename if larger than -1.
 */
void MessageHandler::setLogPath(const char* path, int nodeId) {
    time_t now = time(nullptr);

    std::stringstream ss;

    if (path != nullptr) {
        ss << path << "/";
    }

    char tmpBuff[64];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    tm timeInfo;
    errno_t err = localtime_s(&timeInfo, &now);
    if (err == 0) {
        strftime(tmpBuff, 64, "SGCT_log_%Y_%m_%d_T%H_%M_%S", &timeInfo);
        if (nodeId > -1) {
            ss << tmpBuff << "_node" << nodeId << ".txt";
        }
        else {
            ss << tmpBuff << ".txt";
        }
    }
#else
    tm* timeInfoPtr;
    timeInfoPtr = localtime(&now);

    strftime(tmpBuff, 64, "SGCT_log_%Y_%m_%d_T%H_%M_%S", timeInfoPtr);
    if (nodeId > -1) {
        ss << tmpBuff << "_node" << nodeId << ".txt";
    }
    else {
        ss << tmpBuff << ".txt";
    }
#endif

    mFilename = ss.str();
}

/*!
    Print messages to command line and share to master for easier debuging on a cluster.
*/
void MessageHandler::print(const char* fmt, ...) {
    if (fmt == nullptr) {
        // If There's No Text
        *mParseBuffer = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    printv(fmt, ap);
    va_end(ap);
}

/*!
    Print messages to command line and share to master for easier debuging on a cluster.

    \param nl is the notify level of this message
*/
void MessageHandler::print(Level nl, const char* fmt, ...) {
    if (nl > getNotifyLevel() || fmt == nullptr) {
        // If There's No Text
        *mParseBuffer = 0;    // Do Nothing
        return;
    }

    va_list ap; // Pointer To List Of Arguments
    va_start(ap, fmt); // Parses The String For Variables
    printv(fmt, ap);
    va_end(ap);
}

void MessageHandler::clearBuffer() {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::DataSyncMutex);
    mBuffer.clear();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::DataSyncMutex);
}

/*!
Set the notify level for displaying messages\n
This function is mutex protected/thread safe
*/
void MessageHandler::setNotifyLevel(Level nl) {
    mLevel = nl;
}

/*!
Get the notify level for displaying messages\n
This function is mutex protected/thread safe
*/
MessageHandler::Level MessageHandler::getNotifyLevel() {
    return static_cast<Level>(mLevel.load());
}

/*!
Set if time of day should be displayed with each print message.\n
This function is mutex protected/thread safe
*/
void MessageHandler::setShowTime(bool state) {
    mShowTime = state;
}

/*!
Get if time of day should be displayed with each print message.\n
This function is mutex protected/thread safe
*/
bool MessageHandler::getShowTime() {
    return mShowTime.load();
}

/*!
Set if log to console should be enabled. It is enabled on default
*/
void MessageHandler::setLogToConsole(bool state) {
    mLogToConsole = state;
}

/*!
Set if log to file should be enabled
*/
void MessageHandler::setLogToFile(bool state) {
    mLogToFile = state;
}

/*!
Set if a callback should be called for each incoming log message
*/
void MessageHandler::setLogToCallback(bool state) {
    mLogToCallback = state;
}

/*!
Set the callback that gets invoked for each log if setLogToCallback is <code>true</code>
*/
void MessageHandler::setLogCallback(void(*fnPtr)(const char *)) {
    mMessageCallback = fnPtr;
}

/*!
Set the std callback that gets invoked for each log if setLogToCallback is <code>true</code>
*/
void MessageHandler::setLogCallback(std::function<void(const char *)> fn) {
    mMessageCallback = fn;
}

/*!
Get the time of day string
*/
const char* MessageHandler::getTimeOfDayStr() {
    time_t now = time(nullptr);
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    tm timeInfo;
    errno_t err = localtime_s(&timeInfo, &now);
    if (err == 0) {
        strftime(mTimeBuffer, TimeBufferSize, "%X", &timeInfo);
    }
#else
    tm* timeInfoPtr;
    timeInfoPtr = localtime(&now);
    strftime(mTimeBuffer, TIME_BUFFER_SIZE, "%X", timeInfoPtr);
#endif

    return mTimeBuffer;
}

size_t MessageHandler::getDataSize() {
    return mBuffer.size();
}


char* MessageHandler::getMessage() {
    return &mBuffer[0];
}

void MessageHandler::printDebug(Level nl, const char* fmt, ...) {
#ifdef __SGCT_DEBUG__
    if (nl > getNotifyLevel() || fmt == nullptr) {
        *mParseBuffer = 0;
        return;
    }

    va_list ap;
    va_start(ap, fmt);    // Parses The String For Variables
    printv(fmt, ap);
    va_end(ap);
#endif
}

void MessageHandler::printIndent(Level nl, unsigned int indentation,
                                 const char* fmt, ...)
{
    if (nl > getNotifyLevel() || fmt == nullptr) {
        *mParseBuffer = 0;
        return;
    }

    va_list ap;

    if (indentation > 0) {
        const std::string padding(indentation, ' ');
        const std::string fmtString = std::string(fmt);
        const std::string fmtComplete = padding + fmtString;

        const char* fmtIndented = fmtComplete.c_str();
        va_start(ap, fmt);    // Parses The String For Variables
        printv(fmtIndented, ap);
        va_end(ap);
    }
    else {
        va_start(ap, fmt);    // Parses The String For Variables
        printv(fmt, ap);
        va_end(ap);
    }
}

void MessageHandler::sendMessageToServer(const char* str) {
    if (str == nullptr) {
        return;
    }

    // if client send to server
    if (!mLocal) {
        SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::DataSyncMutex);
        if (mBuffer.empty()) {
            mBuffer.insert(
                mBuffer.begin(),
                headerSpace,
                headerSpace + sgct_core::SGCTNetwork::mHeaderSize
            );
        }
        mBuffer.insert(mBuffer.end(), str, str + strlen(str));
        SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::DataSyncMutex);
    }
}

void MessageHandler::setSendFeedbackToServer(bool state) {
    mLocal = !state;
}

} // namespace sgct
