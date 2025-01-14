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

TEST_CASE("Load: Capture/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {}
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .capture = Capture()
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}

TEST_CASE("Load: Capture/Path", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "path": "abc"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .capture = Capture {
                .path = "abc"
            }
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
  "capture": {
    "path": "def"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .capture = Capture {
                .path = "def"
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Capture/ScreenShotRange", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangebegin": 1,
    "rangeend": 2
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .capture = Capture {
                .range = Capture::ScreenShotRange {
                    .first = 1,
                    .last= 2
                }
            }
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
  "capture": {
    "rangebegin": 3,
    "rangeend": 4
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .capture = Capture {
                .range = Capture::ScreenShotRange {
                    .first = 3,
                    .last = 4
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}





TEST_CASE("Validate: Capture/Path/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "path": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Path/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "path": ""
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range-Begin/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangebegin": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range-Begin/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangebegin": -1000
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range-End/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangeend": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range-End/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangeend": -1000
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangebegin": 10,
    "rangeend": 5
  }
}
)";

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6051): End of range must be greater than beginning of range"
        )
    );
}
