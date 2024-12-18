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

TEST_CASE("Load: Generator/Minimal", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": "abc",
    "major": 1,
    "minor": 2
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .generator = GeneratorVersion {
                .name = "abc",
                .major = 1,
                .minor = 2
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
  "generator": {
    "name": "def",
    "major": 3,
    "minor": 4
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .generator = GeneratorVersion {
                .name = "def",
                .major = 3,
                .minor = 4
            }
        };


        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}
