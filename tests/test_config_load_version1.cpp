/*****************************************************************************************\
 * SGCT                                                                                  *\
 * Simple Graphics Cluster Toolkit                                                       *\
 *                                                                                       *\
 * Copyright (c) 2012-2026                                                               *\
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *\
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <sgct/config.h>
#include <sgct/glm.h>
#include <filesystem>
#include <string>

#include "testhelpers.h"

using namespace sgct;
using namespace sgct::config;

namespace {
    glm::quat fromEuler(float yaw, float pitch, float roll) {
        glm::quat q = glm::quat(1.f, 0.f, 0.f, 0.f);
        q = glm::rotate(q, glm::radians(-yaw), glm::vec3(0.f, 1.f, 0.f));
        q = glm::rotate(q, glm::radians(pitch), glm::vec3(1.f, 0.f, 0.f));
        q = glm::rotate(q, glm::radians(-roll), glm::vec3(0.f, 0.f, 1.f));
        return q;
    }
} // namespace

// The tests in this file load a version 1 configuration together with its corrected
// version 2 equivalent and check that both produce the same Cluster. This validates that
// the version 1 -> 2 upgrade is faithful: 'masteraddress' becomes 'serveraddress', the
// 'swapinterval' of the 'display' settings becomes the 'vsync' of every node, and the
// 'tilt' of the Fisheye and SphericalMirror projections becomes their 'orientation'.

TEST_CASE("Load: Version 1/Server Address", "[parse]") {
    constexpr std::string_view ConfigV1 = R"(
{
  "version": 1,
  "masteraddress": "localhost"
}
)";
    constexpr std::string_view ConfigV2 = R"(
{
  "version": 2,
  "serveraddress": "localhost"
}
)";

    const Cluster v1 = sgct::readJsonConfig(ConfigV1);
    const Cluster v2 = sgct::readJsonConfig(ConfigV2);
    CHECK(v1 == v2);

    const Cluster Object = {
        .success = true,
        .serverAddress = "localhost"
    };
    CHECK(v1 == Object);
}

TEST_CASE("Load: Version 1/VSync", "[parse]") {
    {
        // A positive swapinterval becomes vsync: true on every node
        constexpr std::string_view ConfigV1 = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "depthbuffertexture": true,
    "display": { "swapinterval": 1 }
  },
  "nodes": [
    { "address": "abc", "port": 1 },
    { "address": "def", "port": 2 }
  ]
}
)";
        constexpr std::string_view ConfigV2 = R"(
{
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "depthbuffertexture": true
  },
  "nodes": [
    { "address": "abc", "port": 1, "vsync": true },
    { "address": "def", "port": 2, "vsync": true }
  ]
}
)";

        const Cluster v1 = sgct::readJsonConfig(ConfigV1);
        const Cluster v2 = sgct::readJsonConfig(ConfigV2);
        CHECK(v1 == v2);

        const Cluster Object = {
            .success = true,
            .serverAddress = "localhost",
            .nodes = {
                Node { .address = "abc", .port = 1, .vsync = true },
                Node { .address = "def", .port = 2, .vsync = true }
            },
            .settings = Settings {
                .useDepthTexture = true
            }
        };
        CHECK(v1 == Object);
    }

    {
        // A zero swapinterval becomes vsync: false on every node
        constexpr std::string_view ConfigV1 = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "depthbuffertexture": true,
    "display": { "swapinterval": 0 }
  },
  "nodes": [
    { "address": "abc", "port": 1 }
  ]
}
)";
        constexpr std::string_view ConfigV2 = R"(
{
  "version": 2,
  "serveraddress": "localhost",
  "settings": {
    "depthbuffertexture": true
  },
  "nodes": [
    { "address": "abc", "port": 1, "vsync": false }
  ]
}
)";

        const Cluster v1 = sgct::readJsonConfig(ConfigV1);
        const Cluster v2 = sgct::readJsonConfig(ConfigV2);
        CHECK(v1 == v2);
    }
}

TEST_CASE("Load: Version 1/Fisheye Tilt", "[parse]") {
    constexpr std::string_view ConfigV1 = R"(
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
                "type": "FisheyeProjection",
                "tilt": 1.0
              }
            },
            {
              "projection": {
                "type": "FisheyeProjection",
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
    constexpr std::string_view ConfigV2 = R"(
{
  "version": 2,
  "serveraddress": "localhost",
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
                "type": "FisheyeProjection",
                "orientation": { "pitch": -1.0, "yaw": 0.0, "roll": 0.0 }
              }
            },
            {
              "projection": {
                "type": "FisheyeProjection",
                "orientation": { "pitch": -2.0, "yaw": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

    const Cluster v1 = sgct::readJsonConfig(ConfigV1);
    const Cluster v2 = sgct::readJsonConfig(ConfigV2);
    CHECK(v1 == v2);

    const Cluster Object = {
        .success = true,
        .serverAddress = "localhost",
        .nodes = {
            Node {
                .address = "abc",
                .port = 1,
                .windows = {
                    Window {
                        .size = glm::ivec2(640, 480),
                        .viewports = {
                            Viewport {
                                .projection = FisheyeProjection {
                                    .orientation = fromEuler(0.f, -1.f, 0.f)
                                }
                            },
                            Viewport {
                                .projection = FisheyeProjection {
                                    .orientation = fromEuler(0.f, -2.f, 0.f)
                                }
                            }
                        }
                    }
                }
            }
        }
    };
    CHECK(v1 == Object);
}

TEST_CASE("Load: Version 1/SphericalMirror Tilt", "[parse]") {
    constexpr std::string_view ConfigV1 = R"(
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
    constexpr std::string_view ConfigV2 = R"(
{
  "version": 2,
  "serveraddress": "localhost",
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
                "orientation": { "pitch": -1.0, "yaw": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

    const Cluster v1 = sgct::readJsonConfig(ConfigV1);
    const Cluster v2 = sgct::readJsonConfig(ConfigV2);
    CHECK(v1 == v2);

    const Cluster Object = {
        .success = true,
        .serverAddress = "localhost",
        .nodes = {
            Node {
                .address = "abc",
                .port = 1,
                .windows = {
                    Window {
                        .size = glm::ivec2(640, 480),
                        .viewports = {
                            Viewport {
                                .projection = SphericalMirrorProjection {
                                    .orientation = fromEuler(0.f, -1.f, 0.f),
                                    .mesh = SphericalMirrorProjection::Mesh {
                                        .bottom = std::filesystem::absolute("abc"),
                                        .left = std::filesystem::absolute("def"),
                                        .right = std::filesystem::absolute("ghi"),
                                        .top = std::filesystem::absolute("jkl")
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    };
    CHECK(v1 == Object);
}

TEST_CASE("Load: Version 1/Full", "[parse]") {
    // A realistic version 1 configuration exercising every v1 -> 2 transformation at
    // once: masteraddress -> serveraddress, settings.display.swapinterval -> the vsync
    // of every node, and the Fisheye/SphericalMirror tilt -> orientation.
    constexpr std::string_view ConfigV1 = R"(
{
  "version": 1,
  "masteraddress": "192.168.1.100",
  "settings": {
    "depthbuffertexture": true,
    "display": { "swapinterval": 1 }
  },
  "nodes": [
    {
      "address": "192.168.1.101",
      "port": 20401,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "FisheyeProjection",
                "tilt": 1.0
              }
            }
          ]
        }
      ]
    },
    {
      "address": "192.168.1.102",
      "port": 20402,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "bottom.obj",
                  "left": "left.obj",
                  "right": "right.obj",
                  "top": "top.obj"
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
    constexpr std::string_view ConfigV2 = R"(
{
  "version": 2,
  "serveraddress": "192.168.1.100",
  "settings": {
    "depthbuffertexture": true
  },
  "nodes": [
    {
      "address": "192.168.1.101",
      "port": 20401,
      "vsync": true,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "FisheyeProjection",
                "orientation": { "pitch": -1.0, "yaw": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    },
    {
      "address": "192.168.1.102",
      "port": 20402,
      "vsync": true,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "bottom.obj",
                  "left": "left.obj",
                  "right": "right.obj",
                  "top": "top.obj"
                },
                "orientation": { "pitch": -2.0, "yaw": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

    const Cluster v1 = sgct::readJsonConfig(ConfigV1);
    const Cluster v2 = sgct::readJsonConfig(ConfigV2);
    CHECK(v1 == v2);
}

TEST_CASE("Load: Version 1/Full (File)", "[parse]") {
    // The same comparison as the string-based Full test, but loaded from files to also
    // exercise the file-loading path (readConfig) of version 1 configurations.
    constexpr std::string_view ConfigV1 = R"(
{
  "version": 1,
  "masteraddress": "192.168.1.100",
  "settings": {
    "depthbuffertexture": true,
    "display": { "swapinterval": 1 }
  },
  "nodes": [
    {
      "address": "192.168.1.101",
      "port": 20401,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "FisheyeProjection",
                "tilt": 1.0
              }
            }
          ]
        }
      ]
    },
    {
      "address": "192.168.1.102",
      "port": 20402,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "bottom.obj",
                  "left": "left.obj",
                  "right": "right.obj",
                  "top": "top.obj"
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
    constexpr std::string_view ConfigV2 = R"(
{
  "version": 2,
  "serveraddress": "192.168.1.100",
  "settings": {
    "depthbuffertexture": true
  },
  "nodes": [
    {
      "address": "192.168.1.101",
      "port": 20401,
      "vsync": true,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "FisheyeProjection",
                "orientation": { "pitch": -1.0, "yaw": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    },
    {
      "address": "192.168.1.102",
      "port": 20402,
      "vsync": true,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "SphericalMirrorProjection",
                "geometry": {
                  "bottom": "bottom.obj",
                  "left": "left.obj",
                  "right": "right.obj",
                  "top": "top.obj"
                },
                "orientation": { "pitch": -2.0, "yaw": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

    const sgct::test::TempFile fileV1("version1.json", ConfigV1);
    const sgct::test::TempFile fileV2("version2.json", ConfigV2);

    const Cluster v1 = sgct::readConfig(fileV1.path());
    const Cluster v2 = sgct::readConfig(fileV2.path());
    CHECK(v1 == v2);
}
