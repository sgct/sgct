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

TEST_CASE("Load: PlanarProjection/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "abc",
      "port": 1,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .nodes = {
            Node {
                .address = "abc",
                .port = 1,
                .windows = {
                    Window {
                        .size = ivec2{ 640, 480 },
                        .viewports = {
                            Viewport {
                                .projection = PlanarProjection {
                                    .fov = PlanarProjection::FOV {
                                        .down = -1.f,
                                        .left = -2.f,
                                        .right = 3.f,
                                        .up = 4.f
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}

TEST_CASE("Load: PlanarProjection/Distance", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "abc",
      "port": 1,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                },
                "distance": 5.0
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .nodes = {
                Node {
                    .address = "abc",
                    .port = 1,
                    .windows = {
                        Window {
                            .size = ivec2{ 640, 480 },
                            .viewports = {
                                Viewport {
                                    .projection = PlanarProjection {
                                        .fov = PlanarProjection::FOV {
                                            .down = -1.f,
                                            .left = -2.f,
                                            .right = 3.f,
                                            .up = 4.f,
                                            .distance = 5.f
                                        }
                                    }
                                }
                            }
                        }
                    }
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
  "nodes": [
    {
      "address": "abc",
      "port": 1,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 6.0,
                  "left": 7.0,
                  "right": 8.0,
                  "up": 9.0
                },
                "distance": 10.0
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .nodes = {
                Node {
                    .address = "abc",
                    .port = 1,
                    .windows = {
                        Window {
                            .size = ivec2{ 640, 480 },
                            .viewports = {
                                Viewport {
                                    .projection = PlanarProjection {
                                        .fov = PlanarProjection::FOV {
                                            .down = -6.f,
                                            .left = -7.f,
                                            .right = 8.f,
                                            .up = 9.f,
                                            .distance = 10.f
                                        }
                                    }
                                }
                            }
                        }
                    }
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

TEST_CASE("Load: PlanarProjection/Orientation", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "abc",
      "port": 1,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                },
                "orientation": { "w": 4.0, "x": 1.0, "y": 2.0, "z": 3.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .nodes = {
                Node {
                    .address = "abc",
                    .port = 1,
                    .windows = {
                        Window {
                            .size = ivec2{ 640, 480 },
                            .viewports = {
                                Viewport {
                                    .projection = PlanarProjection {
                                        .fov = PlanarProjection::FOV {
                                            .down = -1.f,
                                            .left = -2.f,
                                            .right = 3.f,
                                            .up = 4.f
                                        },
                                        .orientation = quat{ 1.f, 2.f, 3.f, 4.f }
                                    }
                                }
                            }
                        }
                    }
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
  "nodes": [
    {
      "address": "abc",
      "port": 1,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 6.0,
                  "left": 7.0,
                  "right": 8.0,
                  "up": 9.0
                },
                "orientation": { "w": 8.0, "x": 5.0, "y": 6.0, "z": 7.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .nodes = {
                Node {
                    .address = "abc",
                    .port = 1,
                    .windows = {
                        Window {
                            .size = ivec2{ 640, 480 },
                            .viewports = {
                                Viewport {
                                    .projection = PlanarProjection {
                                        .fov = PlanarProjection::FOV {
                                            .down = -6.f,
                                            .left = -7.f,
                                            .right = 8.f,
                                            .up = 9.f
                                        },
                                        .orientation = quat{5.f, 6.f, 7.f, 8.f }
                                    }
                                }
                            }
                        }
                    }
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

TEST_CASE("Load: PlanarProjection/Offset", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "abc",
      "port": 1,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                },
                "offset": { "x": 1.0, "y": 2.0, "z": 3.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .nodes = {
                Node {
                    .address = "abc",
                    .port = 1,
                    .windows = {
                        Window {
                            .size = ivec2{ 640, 480 },
                            .viewports = {
                                Viewport {
                                    .projection = PlanarProjection {
                                        .fov = PlanarProjection::FOV {
                                            .down = -1.f,
                                            .left = -2.f,
                                            .right = 3.f,
                                            .up = 4.f
                                        },
                                        .offset = vec3{ 1.f, 2.f, 3.f }
                                    }
                                }
                            }
                        }
                    }
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
  "nodes": [
    {
      "address": "abc",
      "port": 1,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 6.0,
                  "left": 7.0,
                  "right": 8.0,
                  "up": 9.0
                },
                "offset": { "x": 5.0, "y": 6.0, "z": 7.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .nodes = {
                Node {
                    .address = "abc",
                    .port = 1,
                    .windows = {
                        Window {
                            .size = ivec2{ 640, 480 },
                            .viewports = {
                                Viewport {
                                    .projection = PlanarProjection {
                                        .fov = PlanarProjection::FOV {
                                            .down = -6.f,
                                            .left = -7.f,
                                            .right = 8.f,
                                            .up = 9.f
                                        },
                                        .offset = vec3{5.f, 6.f, 7.f }
                                    }
                                }
                            }
                        }
                    }
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

TEST_CASE("Load: PlanarProjection/Full", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "abc",
      "port": 1,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                },
                "distance": 5.0,
                "orientation": { "w": 6.0, "x": 7.0, "y": 8.0, "z": 9.0 },
                "offset": { "x": 10.0, "y": 11.0, "z": 12.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .nodes = {
            Node {
                .address = "abc",
                .port = 1,
                .windows = {
                    Window {
                        .size = ivec2{ 640, 480 },
                        .viewports = {
                            Viewport {
                                .projection = PlanarProjection {
                                    .fov = PlanarProjection::FOV {
                                        .down = -1.f,
                                        .left = -2.f,
                                        .right = 3.f,
                                        .up = 4.f,
                                        .distance = 5.f
                                    },
                                    .orientation = quat{ 7.f, 8.f, 9.f, 6.f },
                                    .offset = vec3{ 10.f, 11.f, 12.f }
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}





TEST_CASE("Validate: PlanarProjection/FOV/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": "abc"
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/FOV/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": {}
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/FOV/Down/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": {
                "left": 2,
                "right": 3,
                "up": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/FOV/Down/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": {
                "down": "abc",
                "left": 2,
                "right": 3,
                "up": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/FOV/Left/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "right": 3,
                "up": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/FOV/Left/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": "abc",
                "right": 3,
                "up": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/FOV/Right/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "up": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/FOV/Right/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "right": "abc",
                "up": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/FOV/Up/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "right": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/FOV/Up/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "right": 3,
                "up": "abc"
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": "abc"
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {}
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Pitch/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "yaw": 2,
                "roll": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Pitch/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "pitch": "abc",
                "yaw": 2,
                "roll": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Yaw/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "pitch": 1,
                "roll": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Yaw/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "pitch": 1,
                "yaw": "abc",
                "roll": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Roll/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "pitch": 1,
                "yaw": 2
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Roll/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "pitch": 1,
                "yaw": 2,
                "roll": "abc"
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "y": 2,
                "z": 3,
                "w": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "x": "abc",
                "y": 2,
                "z": 3,
                "w": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "z": 3,
                "w": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": "abc",
                "z": 3,
                "w": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "w": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": "abc",
                "w": 4
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/W/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Orientation/W/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": 3,
                "w": "abc"
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Offset/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "offset": {}
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Offset/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "offset": {
                "y": 2,
                "z": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Offset/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "offset": {
                "x": "abc",
                "y": 2,
                "z": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Offset/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "offset": {
                "x": 1,
                "z": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Offset/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "offset": {
                "x": 1,
                "y": "abc",
                "z": 3
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Offset/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "offset": {
                "x": 1,
                "y": 2
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: PlanarProjection/Offset/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "viewport": {
            "projection": {
              "type": "PlanarProjection",
              "offset": {
                "x": 1,
                "y": 2,
                "z": "abc"
              }
            }
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}
