/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <sgct/config.h>
#include <sgct/math.h>

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
