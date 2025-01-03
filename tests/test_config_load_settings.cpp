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

TEST_CASE("Load: Settings/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {}
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .settings = Settings()
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}

TEST_CASE("Load: Settings/UseDepthTexture", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "depthbuffertexture": false
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .useDepthTexture = false
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
  "settings": {
    "depthbuffertexture": true
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .useDepthTexture = true
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Settings/UseNormalTexture", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "normaltexture": false
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .useNormalTexture = false
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
  "settings": {
    "normaltexture": true
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .useNormalTexture = true
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Settings/UsePositionTexture", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "positiontexture": false
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .usePositionTexture = false
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
  "settings": {
    "positiontexture": true
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .usePositionTexture = true
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Settings/BufferFloatPrecision", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "precision": 16.0
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .bufferFloatPrecision = Settings::BufferFloatPrecision::Float16Bit
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
  "settings": {
    "precision": 32.0
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .bufferFloatPrecision = Settings::BufferFloatPrecision::Float32Bit
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Settings/Display/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {}
  }
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .settings = Settings {
            .display = Settings::Display()
        }
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}

TEST_CASE("Load: Settings/Display/SwapInterval", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 1
    }
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .display = Settings::Display {
                    .swapInterval = 1
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
  "settings": {
    "display": {
      "swapinterval": 2
    }
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .display = Settings::Display {
                    .swapInterval = 2
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

TEST_CASE("Load: Settings/Display/RefreshRate", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "refreshrate": 1
    }
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .display = Settings::Display {
                    .refreshRate = 1
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
  "settings": {
    "display": {
      "refreshrate": 2
    }
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .settings = Settings {
                .display = Settings::Display {
                    .refreshRate = 2
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

TEST_CASE("Load: Settings/Full", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "depthbuffertexture": true,
    "normaltexture": true,
    "positiontexture": true,
    "precision": 16.0,
    "display": {
      "swapinterval": 2,
      "refreshrate": 3
    }
  }
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .settings = Settings {
            .useDepthTexture = true,
            .useNormalTexture = true,
            .usePositionTexture = true,
            .bufferFloatPrecision = Settings::BufferFloatPrecision::Float16Bit,
            .display = Settings::Display {
                .swapInterval = 2,
                .refreshRate = 3
            }
        }
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}





TEST_CASE("Validate: Settings/UseDepthTexture/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "depthbuffertexture": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/UseNormalTexture/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "normaltexture": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/UsePositionTexture/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "positiontexture": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/BufferFloatPrecision/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "precision": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/BufferFloatPrecision/Illegal Value", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "precision": -20
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }

    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "precision": 256
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Settings/Display/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/Display/SwapInterval/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/Display/SwapInterval/Illegal Value", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": -1
    }
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }

    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 1000
    }
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Settings/Display/RefreshRate/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "refreshrate": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/Display/RefreshRate/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "refreshrate": -1
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}
