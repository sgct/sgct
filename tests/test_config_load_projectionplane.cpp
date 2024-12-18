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

TEST_CASE("Load: ProjectionPlane", "[parse]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1.0, "y": 2.0, "z": 3.0 },
                "upperleft": { "x": 4.0, "y": 5.0, "z": 6.0 },
                "upperright": { "x": 7.0, "y": 8.0, "z": 9.0 }
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
                                    .projection = ProjectionPlane {
                                        .lowerLeft = vec3 { 1.f, 2.f, 3.f },
                                        .upperLeft = vec3 { 4.f, 5.f, 6.f },
                                        .upperRight = vec3 { 7.f, 8.f, 9.f }
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 10.0, "y": 11.0, "z": 12.0 },
                "upperleft": { "x": 13.0, "y": 14.0, "z": 15.0 },
                "upperright": { "x": 16.0, "y": 17.0, "z": 18.0 }
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
                                    .projection = ProjectionPlane {
                                        .lowerLeft = vec3 { 10.f, 11.f, 12.f },
                                        .upperLeft = vec3 { 13.f, 14.f, 15.f },
                                        .upperRight = vec3 { 16.f, 17.f, 18.f }
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
