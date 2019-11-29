/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__MESSAGEHANDLER__H__
#define __SGCT__MESSAGEHANDLER__H__

#include <atomic>
#include <functional>
#include <string>
#include <vector>

// @TODO (abock, 2019-11-28) Rename class to Logger

namespace sgct {

class MessageHandler {
public:
    /// Different notify levels for messages
    enum class Level {
        Error = 0,
        Important,
        Info,
        Warning,
        Debug
    };
    
    static MessageHandler& instance();
    static void destroy();

    /**
     * Print messages to command line and share to master for easier debuging on a
     * cluster.
     *
     * \param nl is the notify level of this message
     */
    static void print(Level nl, const char* fmt, ...);

    static void printDebug(const char* fmt, ...);
    static void printWarning(const char* fmt, ...);
    static void printInfo(const char* fmt, ...);
    static void printImportant(const char* fmt, ...);
    static void printError(const char* fmt, ...);

    void decode(std::vector<char> receivedData, int clientIndex);

    /// Set the notify level for displaying messages.
    void setNotifyLevel(Level nl);

    /// Get the notify level for displaying messages.
    Level getNotifyLevel() const;

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

private:
    MessageHandler();

    void print(const char* fmt, ...);
    void printv(const char* fmt, va_list ap);
    void logToFile(const std::vector<char>& buffer);

    static MessageHandler* _instance;

    std::vector<char> _parseBuffer;
    std::vector<char> _combinedBuffer;
    
    std::vector<char> _recBuffer;

    std::atomic<Level> _level = Level::Warning;
    std::atomic_bool _showTime = false;
    std::atomic_bool _logToConsole = true;
    std::atomic_bool _logToFile = false;
    std::atomic_bool _logToCallback = false;

    std::function<void(const char*)> _messageCallback;
    std::string _filename;
    size_t _maxMessageSize = 2048;
    size_t _combinedMessageSize = _maxMessageSize + 32;
};

} // namespace sgct

#endif // __SGCT__MESSAGEHANDLER__H__
