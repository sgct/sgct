/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <sgct/commandline.h>
#include <array>
#include <string>
#include <string_view>
#include <vector>

using namespace sgct;

TEST_CASE("CommandLine: Empty", "[commandline]") {
    std::vector<std::string> args;
    const Configuration config = parseArguments(args);
    CHECK(!config.configFilename.has_value());
    CHECK(!config.isServer.has_value());
    CHECK(!config.logLevel.has_value());
    CHECK(!config.showHelpText.has_value());
    CHECK(!config.nodeId.has_value());
    CHECK(!config.firmSync.has_value());
    CHECK(!config.ignoreSync.has_value());
    CHECK(!config.nCaptureThreads.has_value());
    CHECK(!config.screenshotPath.has_value());
    CHECK(!config.screenshotPrefix.has_value());
    CHECK(!config.addNodeNameInScreenshot.has_value());
    CHECK(!config.omitWindowNameInScreenshot.has_value());
    CHECK(!config.printWaitMessage.has_value());
    CHECK(!config.waitTimeout.has_value());
    CHECK(args.empty());
}

TEST_CASE("CommandLine: Config", "[commandline]") {
    {
        std::vector<std::string> args = { "--config", "abc.json" };
        const Configuration config = parseArguments(args);
        REQUIRE(config.configFilename.has_value());
        CHECK(*config.configFilename == "abc.json");
        CHECK(args.empty());
    }
    {
        std::vector<std::string> args = { "-c", "abc.json" };
        const Configuration config = parseArguments(args);
        REQUIRE(config.configFilename.has_value());
        CHECK(*config.configFilename == "abc.json");
        CHECK(args.empty());
    }
}

TEST_CASE("CommandLine: Local", "[commandline]") {
    {
        std::vector<std::string> args = { "--local", "2" };
        const Configuration config = parseArguments(args);
        REQUIRE(config.isServer.has_value());
        CHECK(*config.isServer == true);
        REQUIRE(config.nodeId.has_value());
        CHECK(*config.nodeId == 2);
        CHECK(args.empty());
    }
    {
        std::vector<std::string> args = { "-l", "0", "--client" };
        const Configuration config = parseArguments(args);
        REQUIRE(config.isServer.has_value());
        CHECK(*config.isServer == false);
        REQUIRE(config.nodeId.has_value());
        CHECK(*config.nodeId == 0);
        CHECK(args.empty());
    }
}

TEST_CASE("CommandLine: Flags", "[commandline]") {
    std::vector<std::string> args = {
        "--debug", "--help", "--firm-sync", "--ignore-sync",
        "--add-node-name-in-screenshot", "--omit-node-name-in-screenshot",
        "--print-wait-message"
    };
    const Configuration config = parseArguments(args);
    CHECK(config.logLevel == Log::Level::Debug);
    CHECK(config.showHelpText == true);
    CHECK(config.firmSync == true);
    CHECK(config.ignoreSync == true);
    CHECK(config.addNodeNameInScreenshot == true);
    CHECK(config.omitWindowNameInScreenshot == true);
    CHECK(config.printWaitMessage == true);
    CHECK(args.empty());
}

TEST_CASE("CommandLine: Help short", "[commandline]") {
    std::vector<std::string> args = { "-h" };
    const Configuration config = parseArguments(args);
    CHECK(config.showHelpText == true);
    CHECK(args.empty());
}

TEST_CASE("CommandLine: Sync", "[commandline]") {
    {
        std::vector<std::string> args = { "--firm-sync" };
        const Configuration config = parseArguments(args);
        CHECK(config.firmSync == true);
    }
    {
        std::vector<std::string> args = { "--loose-sync" };
        const Configuration config = parseArguments(args);
        CHECK(config.firmSync == false);
    }
    {
        // The last argument wins
        std::vector<std::string> args = { "--firm-sync", "--loose-sync" };
        const Configuration config = parseArguments(args);
        CHECK(config.firmSync == false);
    }
}

TEST_CASE("CommandLine: Notify", "[commandline]") {
    struct Test {
        std::string value;
        Log::Level level;
    };
    const std::vector<Test> tests = {
        { "error", Log::Level::Error },
        { "warning", Log::Level::Warning },
        { "info", Log::Level::Info },
        { "debug", Log::Level::Debug },
        { "unknown", Log::Level::Info }
    };
    for (const Test& test : tests) {
        std::vector<std::string> args = { "--notify", test.value };
        const Configuration config = parseArguments(args);
        REQUIRE(config.logLevel.has_value());
        CHECK(*config.logLevel == test.level);
        CHECK(args.empty());
    }
}

TEST_CASE("CommandLine: Capture threads", "[commandline]") {
    {
        std::vector<std::string> args = { "--number-capture-threads", "4" };
        const Configuration config = parseArguments(args);
        CHECK(config.nCaptureThreads == 4);
        CHECK(args.empty());
    }
    {
        // Values are clamped to at least one thread
        std::vector<std::string> args = { "--number-capture-threads", "-3" };
        const Configuration config = parseArguments(args);
        CHECK(config.nCaptureThreads == 1);
        CHECK(args.empty());
    }
}

TEST_CASE("CommandLine: Screenshot", "[commandline]") {
    std::vector<std::string> args = {
        "--screenshot-path", "some/path", "--screenshot-prefix", "prefix"
    };
    const Configuration config = parseArguments(args);
    REQUIRE(config.screenshotPath.has_value());
    CHECK(*config.screenshotPath == std::filesystem::path("some/path"));
    REQUIRE(config.screenshotPrefix.has_value());
    CHECK(*config.screenshotPrefix == "prefix");
    CHECK(args.empty());
}

TEST_CASE("CommandLine: Wait timeout", "[commandline]") {
    std::vector<std::string> args = { "--wait-timeout", "2.5" };
    const Configuration config = parseArguments(args);
    REQUIRE(config.waitTimeout.has_value());
    CHECK(*config.waitTimeout == 2.5f);
    CHECK(args.empty());
}

TEST_CASE("CommandLine: Wait timeout keeps preceding arguments", "[commandline]") {
    std::vector<std::string> args = { "app", "first", "second", "--wait-timeout", "1" };
    const Configuration config = parseArguments(args);
    CHECK(config.waitTimeout == 1.f);
    const std::vector<std::string> expected = { "app", "first", "second" };
    CHECK(args == expected);
}

TEST_CASE("CommandLine: Unknown arguments are kept", "[commandline]") {
    std::vector<std::string> args = {
        "app", "--unknown", "-c", "a.json", "value", "--debug", "--other"
    };
    const Configuration config = parseArguments(args);
    CHECK(config.configFilename == "a.json");
    CHECK(config.logLevel == Log::Level::Debug);
    const std::vector<std::string> expected = { "app", "--unknown", "value", "--other" };
    CHECK(args == expected);
}

TEST_CASE("CommandLine: Missing values", "[commandline]") {
    const std::vector<std::string> flags = {
        "--config", "-c", "--local", "-l", "--notify", "--number-capture-threads",
        "--screenshot-path", "--screenshot-prefix", "--wait-timeout"
    };
    for (const std::string& flag : flags) {
        // A flag that requires a value but is the last argument is ignored and kept
        std::vector<std::string> args = { "app", flag };
        const Configuration config = parseArguments(args);
        CHECK(!config.configFilename.has_value());
        CHECK(!config.nodeId.has_value());
        CHECK(!config.logLevel.has_value());
        CHECK(!config.nCaptureThreads.has_value());
        CHECK(!config.screenshotPath.has_value());
        CHECK(!config.screenshotPrefix.has_value());
        CHECK(!config.waitTimeout.has_value());
        const std::vector<std::string> expected = { "app", flag };
        CHECK(args == expected);
    }
}

TEST_CASE("CommandLine: Invalid numbers", "[commandline]") {
    {
        std::vector<std::string> args = { "--local", "abc" };
        const Configuration config = parseArguments(args);
        CHECK(!config.nodeId.has_value());
        CHECK(args.empty());
    }
    {
        std::vector<std::string> args = { "--number-capture-threads", "abc" };
        const Configuration config = parseArguments(args);
        CHECK(!config.nCaptureThreads.has_value());
        CHECK(args.empty());
    }
    {
        std::vector<std::string> args = { "--wait-timeout", "abc" };
        const Configuration config = parseArguments(args);
        CHECK(!config.waitTimeout.has_value());
        CHECK(args.empty());
    }
    {
        std::vector<std::string> args = { "--local", "99999999999999999999" };
        const Configuration config = parseArguments(args);
        CHECK(!config.nodeId.has_value());
        CHECK(args.empty());
    }
}

TEST_CASE("CommandLine: Help message", "[commandline]") {
    const std::string msg = helpMessage();
    constexpr std::array<std::string_view, 16> Flags = {
        "--config", "--help", "--local", "--client", "--debug", "--firm-sync",
        "--loose-sync", "--ignore-sync", "--notify", "--screenshot-path",
        "--screenshot-prefix", "--add-node-name-in-screenshot",
        "--omit-node-name-in-screenshot", "--number-capture-threads",
        "--print-wait-message", "--wait-timeout"
    };
    for (std::string_view flag : Flags) {
        CHECK(msg.find(flag) != std::string::npos);
    }
}
