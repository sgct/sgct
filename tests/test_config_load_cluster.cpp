/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <sgct/config.h>
#include <sgct/math.h>
#include "schema.h"

using namespace sgct;
using namespace sgct::config;

TEST_CASE("Load: Cluster/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost"
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost"
    };


    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}

TEST_CASE("Load: Cluster/DebugLog", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "debuglog": false
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .debugLog = false
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "debuglog": true
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .debugLog = true
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Cluster/ThreadAffinity", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "threadaffinity": 0
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .threadAffinity = 0
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "threadaffinity": 1
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .threadAffinity = 1
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Cluster/FirmSync", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "firmsync": false
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .firmSync = false
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "firmsync": true
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .firmSync = true
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}





TEST_CASE("Validate: Cluster/Empty", "[validate]") {
    constexpr std::string_view Config = R"(
{}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing 'version' information")
    );
}

TEST_CASE("Validate: Cluster/Version/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "masteraddress": "localhost"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing 'version' information")
    );
}

TEST_CASE("Validate: Cluster/Version/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": "abc",
  "masteraddress": "localhost"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    //CHECK_THROWS_MATCHES(
    //    readJsonConfig(Config),
    //    std::runtime_error,
    //    Catch::Matchers::Message("")
    //);
}

TEST_CASE("Validate: Cluster/Master Address/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6084): Cannot find master address")
    );
}

TEST_CASE("Validate: Cluster/Master Address/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": 1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/DebugLog/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "debuglog": 1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/ThreadAffinity/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "threadaffinity": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/ThreadAffinity/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "threadaffinity": -1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6088): Thread Affinity must be 0 or positive"
        )
    );
}

TEST_CASE("Validate: Cluster/FirmSync/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "debuglog": 1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Scene/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Nodes/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Users/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Capture/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Trackers/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Settings/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Generator/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Meta/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}
