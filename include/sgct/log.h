/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__LOGGER__H__
#define __SGCT__LOGGER__H__

#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace sgct {

class Log {
public:
    /// Different notify levels for messages
    enum class Level { Error, Warning, Info, Debug };
    
    static Log& instance();
    static void destroy();

    static void Debug(const char* fmt, ...);
    static void Warning(const char* fmt, ...);
    static void Info(const char* fmt, ...);
    static void Error(const char* fmt, ...);

    /// Set the notify level for displaying messages
    void setNotifyLevel(Level nl);

    /// Set if time of day should be displayed with each print message
    void setShowTime(bool state);

    /// Set if log to console should be enabled. It is enabled on default
    void setLogToConsole(bool state);

    /// Set the callback that gets invoked for each log. If you want to disable logging to
    /// the callback, pass a null function as a parameter
    void setLogCallback(std::function<void(const char *)> fn);

private:
    Log();

    void printv(const char* fmt, va_list ap);

    static Log* _instance;

    std::vector<char> _parseBuffer;

    Level _level = Level::Warning;
    bool _showTime = false;
    bool _logToConsole = true;
    
    std::mutex _mutex;

    std::function<void(const char*)> _messageCallback;
};

} // namespace sgct

#endif // __SGCT__LOGGER__H__
