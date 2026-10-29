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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = ProjectionPlane {
                                        .lowerLeft = glm::vec3(1.f, 2.f, 3.f),
                                        .upperLeft = glm::vec3(4.f, 5.f, 6.f),
                                        .upperRight = glm::vec3(7.f, 8.f, 9.f)
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
                            .size = glm::ivec2(640, 480),
                            .viewports = {
                                Viewport {
                                    .projection = ProjectionPlane {
                                        .lowerLeft = glm::vec3(10.f, 11.f, 12.f),
                                        .upperLeft = glm::vec3(13.f, 14.f, 15.f),
                                        .upperRight = glm::vec3(16.f, 17.f, 18.f)
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





TEST_CASE("Validate: ProjectionPlane/LowerLeft/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": "abc",
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/LowerLeft/All/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": {},
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/LowerLeft/X/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": {
                  "y": 2,
                  "z": 3
                },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/LowerLeft/X/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": {
                  "x": "abc",
                  "y": 2,
                  "z": 3
                },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/LowerLeft/Y/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": {
                  "x": 1,
                  "z": 3
                },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/LowerLeft/Y/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": {
                  "x": 1,
                  "y": "abc",
                  "z": 3
                },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/LowerLeft/Z/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": {
                  "x": 1,
                  "y": 2
                },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/LowerLeft/Z/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": {
                  "x": 1,
                  "y": 2,
                  "z": "abc"
                },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/UpperLeft/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": "abc",
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/UpperLeft/All/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": {},
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/UpperLeft/X/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": {
                  "y": 2,
                  "z": 3
                },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/UpperLeft/X/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": {
                  "x": "abc",
                  "y": 2,
                  "z": 3
                },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/UpperLeft/Y/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": {
                  "x": 1,
                  "z": 3
                },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/UpperLeft/Y/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": {
                  "x": 1,
                  "y": "abc",
                  "z": 3
                },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/UpperLeft/Z/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": {
                  "x": 1,
                  "y": 2
                },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/UpperLeft/Z/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": {
                  "x": 1,
                  "y": 2,
                  "z": "abc"
                },
                "upperright": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: ProjectionPlane/UpperRight/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": "abc"
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

TEST_CASE("Validate: ProjectionPlane/UpperRight/All/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": {}
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

TEST_CASE("Validate: ProjectionPlane/UpperRight/X/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": {
                  "y": 2,
                  "z": 3
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

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: ProjectionPlane/UpperRight/X/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": {
                  "x": "abc",
                  "y": 2,
                  "z": 3
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

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: ProjectionPlane/UpperRight/Y/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": {
                  "x": 1,
                  "z": 3
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

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: ProjectionPlane/UpperRight/Y/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": {
                  "x": 1,
                  "y": "abc",
                  "z": 3
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

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: ProjectionPlane/UpperRight/Z/Missing", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": {
                  "x": 1,
                  "y": 2
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

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: ProjectionPlane/UpperRight/Z/Wrong Type", "[validate]") {
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
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1, "y": 2, "z": 3 },
                "upperleft": { "x": 1, "y": 2, "z": 3 },
                "upperright": {
                  "x": 1,
                  "y": 2,
                  "z": "abc"
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

    CHECK_THROWS_AS(validate(Config), ParsingError);
}
