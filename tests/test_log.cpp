/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <sgct/log.h>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace sgct;

namespace {
    // Captures everything that is written to std::cout and counts the number of times the
    // stream is flushed
    class CoutCapture : public std::stringbuf {
    public:
        CoutCapture() : _previous(std::cout.rdbuf(this)) {}
        ~CoutCapture() override { std::cout.rdbuf(_previous); }

        int nFlushes = 0;

    protected:
        int sync() override {
            nFlushes++;
            return std::stringbuf::sync();
        }

    private:
        std::streambuf* _previous;
    };

    // Resets the log to its default state when the test starts and ends, as it is a
    // global object that is shared between all tests
    class LogReset {
    public:
        LogReset() { Log::destroy(); }
        ~LogReset() { Log::destroy(); }
    };

    using Messages = std::vector<std::pair<Log::Level, std::string>>;

    void logAllLevels() {
        Log::Debug("debug");
        Log::Info("info");
        Log::Warning("warning");
        Log::Error("error");
    }
} // namespace

TEST_CASE("Log: Default level", "[log]") {
    const LogReset reset;
    const CoutCapture capture;
    Messages messages;
    Log::instance().setLogCallback(
        [&messages](Log::Level level, std::string_view msg) {
            messages.emplace_back(level, std::string(msg));
        }
    );

    logAllLevels();
    const Messages expected = {
        { Log::Level::Info, "(Info) info" },
        { Log::Level::Warning, "(Warning) warning" },
        { Log::Level::Error, "(Error) error" }
    };
    CHECK(messages == expected);
    CHECK(capture.str() == "(Info) info\n(Warning) warning\n(Error) error\n");
}

TEST_CASE("Log: Notify level", "[log]") {
    const LogReset reset;
    const CoutCapture capture;
    Log::instance().setShowLogLevel(false);
    std::vector<Log::Level> levels;
    Log::instance().setLogCallback(
        [&levels](Log::Level level, std::string_view) { levels.push_back(level); }
    );

    SECTION("Debug") {
        Log::instance().setNotifyLevel(Log::Level::Debug);
        logAllLevels();
        CHECK(capture.str() == "debug\ninfo\nwarning\nerror\n");
        const std::vector<Log::Level> expected = {
            Log::Level::Debug, Log::Level::Info, Log::Level::Warning, Log::Level::Error
        };
        CHECK(levels == expected);
    }

    SECTION("Warning") {
        Log::instance().setNotifyLevel(Log::Level::Warning);
        logAllLevels();
        CHECK(capture.str() == "warning\nerror\n");
        const std::vector<Log::Level> expected = {
            Log::Level::Warning, Log::Level::Error
        };
        CHECK(levels == expected);
    }

    SECTION("Error") {
        Log::instance().setNotifyLevel(Log::Level::Error);
        logAllLevels();
        CHECK(capture.str() == "error\n");
        CHECK(levels == std::vector<Log::Level>{ Log::Level::Error });
    }
}

TEST_CASE("Log: Console output is flushed", "[log]") {
    const LogReset reset;
    const CoutCapture capture;

    // Applications that read the output of SGCT rely on every message being flushed
    Log::Info("first");
    CHECK(capture.nFlushes == 1);
    Log::Info("second");
    CHECK(capture.nFlushes == 2);
}

TEST_CASE("Log: Disable console", "[log]") {
    const LogReset reset;
    const CoutCapture capture;
    int nMessages = 0;
    Log::instance().setLogCallback(
        [&nMessages](Log::Level, std::string_view) { nMessages++; }
    );
    Log::instance().setLogToConsole(false);

    logAllLevels();
    CHECK(capture.str().empty());
    CHECK(nMessages == 3);

    Log::instance().setLogToConsole(true);
    Log::Info("info");
    CHECK(capture.str() == "(Info) info\n");
    CHECK(nMessages == 4);
}

TEST_CASE("Log: Remove callback", "[log]") {
    const LogReset reset;
    const CoutCapture capture;
    int nMessages = 0;
    Log::instance().setLogCallback(
        [&nMessages](Log::Level, std::string_view) { nMessages++; }
    );
    Log::Info("info");
    CHECK(nMessages == 1);

    Log::instance().setLogCallback(nullptr);
    Log::Info("info");
    CHECK(nMessages == 1);
    CHECK(capture.str() == "(Info) info\n(Info) info\n");
}

TEST_CASE("Log: Show time", "[log]") {
    using Catch::Matchers::Matches;

    const LogReset reset;
    const CoutCapture capture;
    std::string message;
    Log::instance().setLogCallback(
        [&message](Log::Level, std::string_view msg) { message = msg; }
    );
    Log::instance().setShowTime(true);

    Log::Info("info");
    CHECK_THAT(message, Matches(R"(\(Info\) \S+ \| info)"));

    Log::instance().setShowLogLevel(false);
    Log::Info("info");
    CHECK_THAT(message, Matches(R"(\S+ \| info)"));
}
