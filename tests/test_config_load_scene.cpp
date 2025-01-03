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

TEST_CASE("Load: Scene/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {}
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .scene = Scene()
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}

TEST_CASE("Load: Scene/Offset", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": { "x": 0.0, "y": 0.0, "z": 0.0 }
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .scene = Scene {
                .offset = vec3{ 0.f, 0.f, 0.f }
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
  "scene": {
    "offset": { "x": 1.0, "y": 2.0, "z": 3.0 }
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .scene = Scene {
                .offset = vec3{ 1.f, 2.f, 3.f }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Scene/Orientation", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": { "w": 0.0, "x": 0.0, "y": 0.0, "z": 0.0 }
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .scene = Scene {
                .orientation = quat{ 0.f, 0.f, 0.f, 0.f }
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
  "scene": {
    "orientation": { "w": 1.0, "x": 2.0, "y": 3.0, "z": 4.0 }
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .scene = Scene {
                .orientation = quat{ 2.f, 3.f, 4.f, 1.f }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Scene/Scale", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "scale": 0.0
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .scene = Scene {
                .scale = 0.f
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
  "scene": {
    "scale": 1.0
  }
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .scene = Scene {
                .scale = 1.f
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Scene/Full", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": { "x": 1.0, "y": 2.0, "z": 3.0 },
    "orientation": { "w": 4.0, "x": 5.0, "y": 6.0, "z": 7.0 },
    "scale": 8.0
  }
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .scene = Scene {
            .offset = vec3{ 1.f, 2.f, 3.f },
            .orientation = quat{ 5.f, 6.f, 7.f, 4.f },
            .scale = 8.f
        }
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}





TEST_CASE("Validate: Scene/Offset/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": 1
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {}
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": "abc",
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": 1,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": 1,
      "y": "abc",
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": 1,
      "y": 2
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": 1,
      "y": 2,
      "z": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {}
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": 1
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Pitch/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "yaw": 2,
      "roll": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Pitch/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": "abc",
      "yaw": 2,
      "roll": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Yaw/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": 1,
      "roll": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Yaw/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": 1,
      "yaw": "abc",
      "roll": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Roll/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": 1,
      "yaw": 2
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Roll/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": 1,
      "yaw": 2,
      "roll": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "y": 2,
      "z": 3,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": "abc",
      "y": 2,
      "z": 3,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "z": 3,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": "abc",
      "z": 3,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": 2,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": 2,
      "z": "abc",
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/W/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/W/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": 2,
      "z": 3,
      "w": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Scale/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "scale": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}
