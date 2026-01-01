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

TEST_CASE("Load: TextureMappedProjection/Minimal", "[parse]") {
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
                "type": "TextureMappedProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                }
              },
              "mesh": "abc"
            }
          ]
        }
      ]
    }
  ]
}
)";

    // Can't use initializer list for this since TextureMappedProjection is actually a
    // PlanarProjection
    TextureMappedProjection proj;
    proj.fov = TextureMappedProjection::FOV {
        .down = -1.f,
        .left = -2.f,
        .right = 3.f,
        .up = 4.f
    };

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
                                .projection = proj,
                                .correctionMeshTexture = std::filesystem::absolute("abc")
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

TEST_CASE("Load: TextureMappedProjection/Distance", "[parse]") {
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
                "type": "TextureMappedProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                },
                "distance": 5.0
              },
              "mesh": "abc"
            }
          ]
        }
      ]
    }
  ]
}
)";
        // Can't use initializer list for this since TextureMappedProjection is actually a
        // PlanarProjection
        TextureMappedProjection proj;
        proj.fov = TextureMappedProjection::FOV {
            .down = -1.f,
            .left = -2.f,
            .right = 3.f,
            .up = 4.f,
            .distance = 5.f
        };

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
                                    .projection = proj,
                                    .correctionMeshTexture =
                                        std::filesystem::absolute("abc")
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
                "type": "TextureMappedProjection",
                "fov": {
                  "down": 6.0,
                  "left": 7.0,
                  "right": 8.0,
                  "up": 9.0
                },
                "distance": 10.0
              },
              "mesh": "abc"
            }
          ]
        }
      ]
    }
  ]
}
)";

        // Can't use initializer list for this since TextureMappedProjection is actually a
        // PlanarProjection
        TextureMappedProjection proj;
        proj.fov = TextureMappedProjection::FOV {
            .down = -6.f,
            .left = -7.f,
            .right = 8.f,
            .up = 9.f,
            .distance = 10.f
        };

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
                                    .projection = proj,
                                    .correctionMeshTexture =
                                        std::filesystem::absolute("abc")
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

TEST_CASE("Load: TextureMappedProjection/Orientation", "[parse]") {
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
                "type": "TextureMappedProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                },
                "orientation": { "w": 4.0, "x": 1.0, "y": 2.0, "z": 3.0 }
              },
              "mesh": "abc"
            }
          ]
        }
      ]
    }
  ]
}
)";

        // Can't use initializer list for this since TextureMappedProjection is actually a
        // PlanarProjection
        TextureMappedProjection proj;
        proj.fov = TextureMappedProjection::FOV {
            .down = -1.f,
            .left = -2.f,
            .right = 3.f,
            .up = 4.f
        };
        proj.orientation = quat { 1.f, 2.f, 3.f, 4.f };

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
                                    .projection = proj,
                                    .correctionMeshTexture =
                                        std::filesystem::absolute("abc")
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
                "type": "TextureMappedProjection",
                "fov": {
                  "down": 6.0,
                  "left": 7.0,
                  "right": 8.0,
                  "up": 9.0
                },
                "orientation": { "w": 8.0, "x": 5.0, "y": 6.0, "z": 7.0 }
              },
              "mesh": "abc"
            }
          ]
        }
      ]
    }
  ]
}
)";

        // Can't use initializer list for this since TextureMappedProjection is actually a
        // PlanarProjection
        TextureMappedProjection proj;
        proj.fov = TextureMappedProjection::FOV {
            .down = -6.f,
            .left = -7.f,
            .right = 8.f,
            .up = 9.f
        };
        proj.orientation = quat { 5.f, 6.f, 7.f, 8.f };

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
                                    .projection = proj,
                                    .correctionMeshTexture =
                                        std::filesystem::absolute("abc")
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

TEST_CASE("Load: TextureMappedProjection/Offset", "[parse]") {
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
                "type": "TextureMappedProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                },
                "offset": { "x": 1.0, "y": 2.0, "z": 3.0 }
              },
              "mesh": "abc"
            }
          ]
        }
      ]
    }
  ]
}
)";

        // Can't use initializer list for this since TextureMappedProjection is actually a
        // PlanarProjection
        TextureMappedProjection proj;
        proj.fov = TextureMappedProjection::FOV {
            .down = -1.f,
            .left = -2.f,
            .right = 3.f,
            .up = 4.f
        };
        proj.offset = vec3 { 1.f, 2.f, 3.f };

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
                                    .projection = proj,
                                    .correctionMeshTexture =
                                        std::filesystem::absolute("abc")
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
                "type": "TextureMappedProjection",
                "fov": {
                  "down": 6.0,
                  "left": 7.0,
                  "right": 8.0,
                  "up": 9.0
                },
                "offset": { "x": 5.0, "y": 6.0, "z": 7.0 }
              },
              "mesh": "abc"
            }
          ]
        }
      ]
    }
  ]
}
)";

        // Can't use initializer list for this since TextureMappedProjection is actually a
        // PlanarProjection
        TextureMappedProjection proj;
        proj.fov = TextureMappedProjection::FOV {
            .down = -6.f,
            .left = -7.f,
            .right = 8.f,
            .up = 9.f
        };
        proj.offset = vec3 { 5.f, 6.f, 7.f };

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
                                    .projection = proj,
                                    .correctionMeshTexture =
                                        std::filesystem::absolute("abc")
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

TEST_CASE("Load: TextureMappedProjection/Full", "[parse]") {
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
                "type": "TextureMappedProjection",
                "fov": {
                  "down": 1.0,
                  "left": 2.0,
                  "right": 3.0,
                  "up": 4.0
                },
                "distance": 5.0,
                "orientation": { "w": 6.0, "x": 7.0, "y": 8.0, "z": 9.0 },
                "offset": { "x": 10.0, "y": 11.0, "z": 12.0 }
              },
              "mesh": "abc"
            }
          ]
        }
      ]
    }
  ]
}
)";

    // Can't use initializer list for this since TextureMappedProjection is actually a
    // PlanarProjection
    TextureMappedProjection proj;
    proj.fov = TextureMappedProjection::FOV {
        .down = -1.f,
        .left = -2.f,
        .right = 3.f,
        .up = 4.f,
        .distance = 5.f
    };
    proj.orientation = quat { 7.f, 8.f, 9.f, 6.f };
    proj.offset = vec3 { 10.f, 11.f, 12.f };

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
                                .projection = proj,
                                .correctionMeshTexture = std::filesystem::absolute("abc")
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





TEST_CASE("Validate: TextureMappedProjection/Mesh/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/All/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Down/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Down/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Left/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Left/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Right/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Right/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Up/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Up/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/All/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Pitch/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Pitch/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Yaw/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Yaw/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Roll/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Roll/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/X/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/X/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Y/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Y/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Z/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Z/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/W/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/W/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Offset/All/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Offset/X/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Offset/X/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Offset/Y/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Offset/Y/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Offset/Z/Missing", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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

TEST_CASE("Validate: TextureMappedProjection/Offset/Z/Wrong Type", "[validate]") {
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
            "mesh": "abc",
            "projection": {
              "type": "TextureMappedProjection",
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
