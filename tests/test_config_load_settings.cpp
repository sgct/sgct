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
#include "schema.h"

using namespace sgct;
using namespace sgct::config;

TEST_CASE("Load: Settings/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 2,
  "serveraddress": "localhost",
  "settings": {}
}
)";

    const Cluster Object = {
        .success = true,
        .serverAddress = "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "depthbuffertexture": false
  }
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "depthbuffertexture": true
  }
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "normaltexture": false
  }
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "normaltexture": true
  }
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "positiontexture": false
  }
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "positiontexture": true
  }
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "precision": 16.0
  }
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "precision": 32.0
  }
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
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

TEST_CASE("Load: Settings/Display/SwapInterval (Version 1)", "[parse]") {
    // A positive swap interval enables the V-Sync of all nodes
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 1
    }
  },
  "nodes": [
    {
      "address": "abc",
      "port": 1
    },
    {
      "address": "def",
      "port": 2
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
            .nodes = {
                Node {
                    .address = "abc",
                    .port = 1,
                    .vsync = true
                },
                Node {
                    .address = "def",
                    .port = 2,
                    .vsync = true
                }
            },
            .settings = Settings()
        };

        CHECK_NOTHROW(validate(String));
        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);
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
  },
  "nodes": [
    {
      "address": "abc",
      "port": 1
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
            .nodes = {
                Node {
                    .address = "abc",
                    .port = 1,
                    .vsync = true
                }
            },
            .settings = Settings()
        };

        CHECK_NOTHROW(validate(String));
        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 0
    }
  },
  "nodes": [
    {
      "address": "abc",
      "port": 1
    },
    {
      "address": "def",
      "port": 2
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
            .nodes = {
                Node {
                    .address = "abc",
                    .port = 1,
                    .vsync = false
                },
                Node {
                    .address = "def",
                    .port = 2,
                    .vsync = false
                }
            },
            .settings = Settings()
        };

        CHECK_NOTHROW(validate(String));
        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);
    }
}

TEST_CASE("Load: Settings/Full", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "depthbuffertexture": true,
    "normaltexture": true,
    "positiontexture": true,
    "precision": 16.0
  }
}
)";

    const Cluster Object = {
        .success = true,
        .serverAddress = "localhost",
        .settings = Settings {
            .useDepthTexture = true,
            .useNormalTexture = true,
            .usePositionTexture = true,
            .bufferFloatPrecision = Settings::BufferFloatPrecision::Float16Bit
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
  "version": 2,
  "serveraddress": "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
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
  "version": 2,
  "serveraddress": "localhost",
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

TEST_CASE("Validate: Settings/Display/SwapInterval (Version 2)", "[validate]") {
    // Replaced by the 'vsync' value of the nodes
    constexpr std::string_view Config = R"(
{
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 1
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);

    const Cluster Object = {
        .success = true,
        .serverAddress = "localhost",
        .settings = Settings()
    };
    CHECK(readJsonConfig(Config) == Object);
}

TEST_CASE("Validate: Settings/Display/RefreshRate/Removed", "[validate]") {
    // Removed as the value was not applied to the windows anymore
    constexpr std::string_view Config = R"(
{
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "display": {
      "refreshrate": 60
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);

    const Cluster Object = {
        .success = true,
        .serverAddress = "localhost",
        .settings = Settings()
    };
    CHECK(readJsonConfig(Config) == Object);
}
