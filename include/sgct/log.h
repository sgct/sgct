/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__LOGGER__H__
#define __SGCT__LOGGER__H__

#include <sgct/sgctexports.h>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace sgct {

class SGCT_EXPORT Log {
public:
    /**
     * Different notify levels for messages.
     */
    enum class Level { Error = 3, Warning = 2, Info = 1, Debug = 0 };

    static Log& instance();
    static void destroy();

    static void Debug(std::string_view message);
    static void Warning(std::string_view message);
    static void Info(std::string_view message);
    static void Error(std::string_view message);

    /**
     * Set the notify level for displaying messages.
     */
    void setNotifyLevel(Level nl);

    /**
     * Set whether time of day should be displayed with each print message.
     */
    void setShowTime(bool state);

    /**
     * Sets whether the log level should be displayed with each print message.
     */
    void setShowLogLevel(bool state);

    /**
     * Set if log to console should be enabled. It is enabled on default.
     */
    void setLogToConsole(bool state);

    /**
     * Set the callback that gets invoked for each log. If you want to disable logging to
     * the callback, pass a null function as a parameter.
     */
    void setLogCallback(std::function<void(Level, std::string_view)> fn);

private:
    Log();
    Log(const Log&) = delete;
    Log(Log&&) = delete;
    Log& operator=(const Log&) = delete;
    Log& operator=(Log&&) = delete;

    void printv(Level level, std::string message);

    static Log* _instance;

    std::vector<char> _parseBuffer;

    Level _level = Level::Info;
    bool _showTime = false;
    bool _showLevel = true;
    bool _logToConsole = true;

    std::mutex _mutex;

    std::function<void(Level, std::string_view)> _messageCallback;
};

} // namespace sgct

#endif // __SGCT__LOGGER__H__
