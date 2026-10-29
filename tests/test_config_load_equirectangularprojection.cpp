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
#include <sgct/glm.h>
#include "schema.h"

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

TEST_CASE("Load: EquirectangularProjection/Minimal", "[parse]") {
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
                "type": "EquirectangularProjection"
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
                        .size = glm::ivec2(640, 480),
                        .viewports = {
                            Viewport {
                                .projection = EquirectangularProjection()
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

TEST_CASE("Load: EquirectangularProjection/Quality", "[parse]") {
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
                "type": "EquirectangularProjection",
                "quality": "low"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 256
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
                "type": "EquirectangularProjection",
                "quality": "256"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 256
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
                "type": "EquirectangularProjection",
                "quality": "medium"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 512
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
                "type": "EquirectangularProjection",
                "quality": "512"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 512
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
                "type": "EquirectangularProjection",
                "quality": "high"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 1024
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
                "type": "EquirectangularProjection",
                "quality": "1k"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 1024
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
                "type": "EquirectangularProjection",
                "quality": "1024"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 1024
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
                "type": "EquirectangularProjection",
                "quality": "1.5k"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 1536
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
                "type": "EquirectangularProjection",
                "quality": "1536"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 1536
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
                "type": "EquirectangularProjection",
                "quality": "2k"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 2048
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
                "type": "EquirectangularProjection",
                "quality": "2048"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 2048
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
                "type": "EquirectangularProjection",
                "quality": "4k"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 4096
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
                "type": "EquirectangularProjection",
                "quality": "4096"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 4096
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
                "type": "EquirectangularProjection",
                "quality": "8k"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 8192
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
                "type": "EquirectangularProjection",
                "quality": "8192"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 8192
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
                "type": "EquirectangularProjection",
                "quality": "16k"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 16384
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
                "type": "EquirectangularProjection",
                "quality": "16384"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 16384
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
                "type": "EquirectangularProjection",
                "quality": "32k"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 32768
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
                "type": "EquirectangularProjection",
                "quality": "32768"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 32768
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
                "type": "EquirectangularProjection",
                "quality": "64k"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 65536
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
                "type": "EquirectangularProjection",
                "quality": "65536"
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .quality = 65536
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

TEST_CASE("Load: EquirectangularProjection/Orientation", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 2,
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
                "type": "EquirectangularProjection",
                "orientation": { "x": 1.0, "y": 2.0, "z": 3.0, "w": 4.0 }
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .orientation = glm::quat(4.f, 1.f, 2.f, 3.f)
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
  "version": 2,
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
                "type": "EquirectangularProjection",
                "orientation": { "pitch": 1.0, "yaw": 2.0, "roll": 3.0 }
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = EquirectangularProjection {
                                        .orientation = fromEuler(2.f, 1.f, 3.f)
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





TEST_CASE("Validate: EquirectangularProjection/Orientation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 2,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "EquirectangularProjection",
                "orientation": "abc"
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: EquirectangularProjection/Quality/Wrong Type", "[validate]") {
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
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "EquirectangularProjection",
                "quality": "abc"
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: EquirectangularProjection/Quality/Illegal Value", "[validate]") {
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
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "EquirectangularProjection",
                "quality": -1
              }
            }
          ]
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
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "projection": {
                "type": "EquirectangularProjection",
                "quality": 1111
              }
            }
          ]
        }
      ]
    }
  ]
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}
