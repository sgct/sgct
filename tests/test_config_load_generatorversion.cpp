/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <sgct/config.h>
#include <sgct/math.h>
#include "schema.h"

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





TEST_CASE("Validate: GeneratorVersion/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": 123,
    "major": 1,
    "minor": 2
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: GeneratorVersion/Major/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": "abc",
    "major": "abc",
    "minor": 2
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: GeneratorVersion/Major/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": "abc",
    "major": -1,
    "minor": 2
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: GeneratorVersion/Minor/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": "abc",
    "major": 2,
    "minor": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: GeneratorVersion/Minor/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": "abc",
    "major": 2,
    "minor": -1
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}
