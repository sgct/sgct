/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <sgct/config.h>
#include <sgct/math.h>
#include "schema.h"

using namespace sgct;
using namespace sgct::config;

TEST_CASE("Load: Meta/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {}
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .meta = Meta()
    };


    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}

TEST_CASE("Load: Meta/Author", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "author": "abc"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .author = "abc"
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
  "meta": {
    "author": "def"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .author = "def"
            }
        };


        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Meta/Description", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "description": "abc"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .description = "abc"
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
  "meta": {
    "description": "def"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .description = "def"
            }
        };


        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Meta/License", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "license": "abc"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .license = "abc"
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
  "meta": {
    "license": "def"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .license = "def"
            }
        };


        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Meta/Name", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "name": "abc"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .name = "abc"
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
  "meta": {
    "name": "def"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .name = "def"
            }
        };


        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Meta/Version", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "version": "abc"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .version = "abc"
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
  "meta": {
    "version": "def"
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .meta = Meta {
                .version = "def"
            }
        };


        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}





TEST_CASE("Validate: Meta/Author/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "author": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Meta/Description/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "description": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Meta/License/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "license": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Meta/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "name": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Meta/Version/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "version": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}
