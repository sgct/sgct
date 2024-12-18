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

TEST_CASE("Load: SphericalMirrorProjection/Mesh", "[parse]") {
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
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "123",
                  "left": "456",
                  "right": "789",
                  "top": "101112"
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
                                    .projection = SphericalMirrorProjection {
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "123",
                                            .left = "456",
                                            .right = "789",
                                            .top = "101112"
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

TEST_CASE("Load: SphericalMirrorProjection/Quality", "[parse]") {
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
                "type": "SphericalMirrorProjection",
                "quality": "low",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 256,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "256",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 256,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "medium",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 512,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "512",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 512,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "high",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 1024,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "1k",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 1024,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "1024",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 1024,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "1.5k",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 1536,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "1536",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 1536,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "2k",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 2048,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "2048",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 2048,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "4k",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 4096,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "4096",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 4096,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "8k",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 8192,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "8192",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 8192,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "16k",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 16384,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "16384",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 16384,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "32k",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 32768,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "32768",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 32768,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "64k",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 65536,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "quality": "65536",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
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
                                    .projection = SphericalMirrorProjection {
                                        .quality = 65536,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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

TEST_CASE("Load: SphericalMirrorProjection/Tilt", "[parse]") {
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
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
                },
                "tilt": 1.0
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
                                    .projection = SphericalMirrorProjection {
                                        .tilt = 1.f,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "123",
                  "left": "456",
                  "right": "789",
                  "top": "101112"
                },
                "tilt": 2.0
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
                                    .projection = SphericalMirrorProjection {
                                        .tilt = 2.f,
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "123",
                                            .left = "456",
                                            .right = "789",
                                            .top = "101112"
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

TEST_CASE("Load: SphericalMirrorProjection/Background/XYZW", "[parse]") {
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
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
                },
                "background": { "x": 1.0, "y": 2.0, "z": 3.0, "w": 4.0 }
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
                                    .projection = SphericalMirrorProjection {
                                        .background = vec4{ 1.f, 2.f, 3.f, 4.f },
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "123",
                  "left": "456",
                  "right": "789",
                  "top": "101112"
                },
                "background": { "x": 5.0, "y": 6.0, "z": 7.0, "w": 8.0 }
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
                                    .projection = SphericalMirrorProjection {
                                        .background = vec4{ 5.f, 6.f, 7.f, 8.f },
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "123",
                                            .left = "456",
                                            .right = "789",
                                            .top = "101112"
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

TEST_CASE("Load: SphericalMirrorProjection/Background/RGBA", "[parse]") {
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
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
                },
                "background": { "r": 1.0, "g": 2.0, "b": 3.0, "a": 4.0 }
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
                                    .projection = SphericalMirrorProjection {
                                        .background = vec4{ 1.f, 2.f, 3.f, 4.f },
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "abc",
                                            .left = "def",
                                            .right = "ghi",
                                            .top = "jkl"
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
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "123",
                  "left": "456",
                  "right": "789",
                  "top": "101112"
                },
                "background": { "r": 5.0, "g": 6.0, "b": 7.0, "a": 8.0 }
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
                                    .projection = SphericalMirrorProjection {
                                        .background = vec4{ 5.f, 6.f, 7.f, 8.f },
                                        .mesh = SphericalMirrorProjection::Mesh {
                                            .bottom = "123",
                                            .left = "456",
                                            .right = "789",
                                            .top = "101112"
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

TEST_CASE("Load: SphericalMirrorProjection/Full", "[parse]") {
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
                "type": "SphericalMirrorProjection",
                "quality": "low",
                "geometry": {
                  "bottom": "abc",
                  "left": "def",
                  "right": "ghi",
                  "top": "jkl"
                },
                "tilt": 1.0,
                "background": { "x": 1.0, "y": 2.0, "z": 3.0, "w": 4.0 }
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
                                .projection = SphericalMirrorProjection {
                                    .quality = 256,
                                    .tilt = 1.f,
                                    .background = vec4{ 1.f, 2.f, 3.f, 4.f },
                                    .mesh = SphericalMirrorProjection::Mesh {
                                        .bottom = "abc",
                                        .left = "def",
                                        .right = "ghi",
                                        .top = "jkl"
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
