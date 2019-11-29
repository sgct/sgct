/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__MESSAGEHANDLER__H__
#define __SGCT__MESSAGEHANDLER__H__

#include <functional>
#include <mutex>
#include <string>
#include <vector>

// @TODO (abock, 2019-11-28) Rename class to Logger

namespace sgct {

class MessageHandler {
public:
    /// Different notify levels for messages
    enum class Level {
        Error = 0,
        Info,
        Warning,
        Debug
    };
    
    static MessageHandler& instance();
    static void destroy();

    static void printDebug(const char* fmt, ...);
    static void printWarning(const char* fmt, ...);
    static void printInfo(const char* fmt, ...);
    static void printError(const char* fmt, ...);

    /// Set the notify level for displaying messages
    void setNotifyLevel(Level nl);

    /// Set if time of day should be displayed with each print message
    void setShowTime(bool state);

    /// Set if log to console should be enabled. It is enabled on default
    void setLogToConsole(bool state);

    /// Set if log to file should be enabled
    void setLogToFile(bool state);
    
    /// Set the log file path. The id will be appended on the filename if larger than -1
    void setLogPath(const char* path, int id = -1);

    /// Set the callback that gets invoked for each log
    void setLogCallback(std::function<void(const char *)> fn);

private:
    MessageHandler();

    void printv(const char* fmt, va_list ap);
    void logToFile(const std::vector<char>& buffer);

    static MessageHandler* _instance;

    std::vector<char> _parseBuffer;
    std::vector<char> _combinedBuffer;

    Level _level = Level::Warning;
    bool _showTime = false;
    bool _logToConsole = true;
    bool _logToFile = false;
    
    std::mutex _mutex;

    std::function<void(const char*)> _messageCallback;
    std::string _filename;
    size_t _maxMessageSize = 2048;
    size_t _combinedMessageSize = _maxMessageSize + 32;
};

} // namespace sgct

#endif // __SGCT__MESSAGEHANDLER__H__
