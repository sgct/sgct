/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__MESSAGE_HANDLER__H__
#define __SGCT__MESSAGE_HANDLER__H__

#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace sgct {

class MessageHandler {
public:
    /// Different notify levels for messages
    enum class Level {
        Error = 0,
        Important,
        VersionInfo,
        Info,
        Warning,
        Debug,
        NotifyAll
    };
    
    /// Get the MessageHandler instance
    static MessageHandler* instance();

    /// Destroy the MessageHandler
    static void destroy();

    void decode(std::vector<char> receivedData, int clientIndex);

    /// Print messages to commandline and share to master
    void print(const char* fmt, ...);

    /**
     * Print messages to command line and share to master for easier debuging on a
     * cluster.
     *
     * \param nl is the notify level of this message
     */
    void print(Level nl, const char* fmt, ...);
    void clearBuffer();

    /// Set the notify level for displaying messages.
    void setNotifyLevel(Level nl);

    /// Get the notify level for displaying messages.
    Level getNotifyLevel();

    /// Set if time of day should be displayed with each print message
    void setShowTime(bool state);

    /// Set if log to console should be enabled. It is enabled on default
    void setLogToConsole(bool state);

    /// Set if log to file should be enabled
    void setLogToFile(bool state);
    
    /**
     * Set the log file path/directoy. The nodeId is optional and will be appended on the
     * filename if larger than -1.
     */
    void setLogPath(const char* path, int nodeId = -1);

    /// Set if a callback should be called for each incoming log message
    void setLogToCallback(bool state);

    /**
     * Set the std callback that gets invoked for each log if setLogToCallback is
     * <code>true</code>
     */
    void setLogCallback(std::function<void(const char *)> fn);

    std::size_t getDataSize();

    char* getMessage();

private:
    MessageHandler();

    void printv(const char* fmt, va_list ap);
    void logToFile(const std::vector<char>& buffer);

    static MessageHandler* mInstance;

    std::vector<char> mParseBuffer;
    std::vector<char> mCombinedBuffer;
    
    std::vector<char> mBuffer;
    std::vector<char> mRecBuffer;
    std::vector<char> headerSpace;

    std::atomic<Level> mLevel;
    std::atomic_bool mShowTime = true;
    std::atomic_bool mLogToConsole = true;
    std::atomic_bool mLogToFile = false;
    std::atomic_bool mLogToCallback = false;

    std::function<void(const char *)> mMessageCallback;
    std::string mFilename;
    size_t mMaxMessageSize = 2048;
    size_t mCombinedMessageSize = mMaxMessageSize + 32;
};

} // namespace sgct

#endif // __SGCT__MESSAGE_HANDLER__H__
