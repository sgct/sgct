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





TEST_CASE("Validate: SphericalMirrorProjection/Quality/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "quality": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Quality/Illegal Value", "[validate]") {
    {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "quality": -1
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

    {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "quality": 1111
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
}

TEST_CASE("Validate: SphericalMirrorProjection/Tilt/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "tilt": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/All/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {}
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/X/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "y": 1,
                "z": 1,
                "w": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/X/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "x": "abc",
                "y": 1,
                "z": 1,
                "w": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/Y/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "x": 1,
                "z": 1,
                "w": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/Y/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "x": 1,
                "y": "abc",
                "z": 1,
                "w": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/Z/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "x": 1,
                "y": 1,
                "w": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/Z/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "x": 1,
                "y": 1,
                "z": "abc",
                "w": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/W/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "x": 1,
                "y": 1,
                "z": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/W/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "x": 1,
                "y": 1,
                "z": 1,
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/R/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "g": 1,
                "b": 1,
                "a": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/R/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "r": "abc",
                "g": 1,
                "b": 1,
                "a": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/G/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "r": 1,
                "b": 1,
                "a": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/G/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "r": 1,
                "g": "abc",
                "b": 1,
                "a": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/B/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "r": 1,
                "g": 1,
                "a": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/B/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "r": 1,
                "g": 1,
                "b": "abc",
                "a": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/A/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "r": 1,
                "g": 1,
                "b": 1
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

TEST_CASE("Validate: SphericalMirrorProjection/Background/A/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": "abc"
              },
              "background": {
                "r": 1,
                "g": 1,
                "b": 1,
                "a": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/All/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {}
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Bottom/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "left": "abc",
                "right": "abc",
                "top": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Bottom/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": 123,
                "left": "abc",
                "right": "abc",
                "top": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Bottom/Illegal Value", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "",
                "left": "abc",
                "right": "abc",
                "top": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Left/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "right": "abc",
                "top": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Left/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": 123,
                "right": "abc",
                "top": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Left/Illegal Value", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "",
                "right": "abc",
                "top": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Right/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "top": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Right/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": 123,
                "top": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Right/Illegal Value", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "",
                "top": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Top/Missing", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc"
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Top/Wrong Type", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": 123
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

TEST_CASE("Validate: SphericalMirrorProjection/Mesh/Top/Illegal Value", "[validate]") {
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
              "type": "SphericalMirrorProjection",
              "mesh": {
                "bottom": "abc",
                "left": "abc",
                "right": "abc",
                "top": ""
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
