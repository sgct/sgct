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
