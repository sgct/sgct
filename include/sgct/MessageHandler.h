/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__MESSAGE_HANDLER__H__
#define __SGCT__MESSAGE_HANDLER__H__

#include <stddef.h> //get definition for NULL
#include <stdarg.h>
#include <vector>
#include <functional>
#include <string>

#include <atomic>

namespace sgct {

class MessageHandler {
public:
    /*!
        Different notify levels for messages
    */
    enum class Level {
        Error = 0,
        Important,
        VersionInfo,
        Info,
        Warning,
        Debug,
        NotifyAll
    };
    
    /*! Get the MessageHandler instance */
    static MessageHandler* instance();

    /*! Destroy the MessageHandler */
    static void destroy();

    void decode(const char* receivedData, int receivedlength, int clientIndex);
    void print(const char* fmt, ...);
    void print(Level nl, const char* fmt, ...);
    void printDebug(Level nl, const char* fmt, ...);
    void printIndent(Level nl, unsigned int indentation, const char* fmt, ...);
    void sendMessageToServer(const char* fmt);
    void setSendFeedbackToServer(bool state);
    void clearBuffer();
    void setNotifyLevel(Level nl);
    Level getNotifyLevel();
    void setShowTime(bool state);
    bool getShowTime();
    void setLogToConsole(bool state);
    void setLogToFile(bool state);
    void setLogPath(const char* path, int nodeId = -1);
    void setLogToCallback(bool state);
    void setLogCallback(void(*fnPtr)(const char *));
    void setLogCallback(std::function<void(const char *)> fn);
    const char* getTimeOfDayStr();
    std::size_t getDataSize();

    char* getMessage();

private:
    MessageHandler();
    ~MessageHandler();

    void printv(const char* fmt, va_list ap);
    void logToFile(const char* buffer);

private:
    using MessageCallbackFn = std::function<void(const char *)>;
    static const int TimeBufferSize = 9;

    static MessageHandler* mInstance;

    char* mParseBuffer;
    char* mCombinedBuffer;
    
    std::vector<char> mBuffer;
    std::vector<char> mRecBuffer;
    unsigned char* headerSpace;

    std::atomic<Level> mLevel;
    std::atomic<bool> mLocal = true;
    std::atomic<bool> mShowTime = true;
    std::atomic<bool> mLogToConsole = true;
    std::atomic<bool> mLogToFile = false;
    std::atomic<bool> mLogToCallback = false;

    MessageCallbackFn mMessageCallback = nullptr;
    char mTimeBuffer[TimeBufferSize];
    std::string mFilename;
    size_t mMaxMessageSize = 2048;
    size_t mCombinedMessageSize = mMaxMessageSize + 32;
};

} // namespace sgct

#endif // __SGCT__MESSAGE_HANDLER__H__
