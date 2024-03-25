/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <sgct/readconfig.h>


TEST_CASE("Parse Required: Version", "[parse]") {
    constexpr std::string_view Sources = R"(
{}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("Missing 'version' information")
    );
}

TEST_CASE("Parse Required: Cluster/Master Address", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6084): Cannot find master address")
    );
}

TEST_CASE("Parse Required: Node/Address", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "port": 1
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6040): Missing field address in node")
    );
}

TEST_CASE("Parse Required: Node/Port", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost"
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6041): Missing field port in node")
    );
}

TEST_CASE("Parse Required: Window/Size", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
        }
      ]
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("Could not find required key 'size'")
    );
}

TEST_CASE("Parse Required: FisheyeProjection/Crop/Left", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "FisheyeProjection",
                "crop": {
                  "right": 1.0,
                  "bottom": 1.0,
                  "top": 1.0
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'left' in FisheyeProjection/Crop")
    );
}

TEST_CASE("Parse Required: FisheyeProjection/Crop/Right", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "FisheyeProjection",
                "crop": {
                  "left": 1.0,
                  "bottom": 1.0,
                  "top": 1.0
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'right' in FisheyeProjection/Crop")
    );
}

TEST_CASE("Parse Required: FisheyeProjection/Crop/Bottom", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "FisheyeProjection",
                "crop": {
                  "left": 1.0,
                  "right": 1.0,
                  "top": 1.0
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'bottom' in FisheyeProjection/Crop")
    );
}

TEST_CASE("Parse Required: FisheyeProjection/Crop/Top", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "FisheyeProjection",
                "crop": {
                  "left": 1.0,
                  "right": 1.0,
                  "bottom": 1.0
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'top' in FisheyeProjection/Crop")
    );
}

TEST_CASE("Parse Required: PlanarProjection/FOV", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection"
              }
            }
          ]
        }
      ]
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6000): Missing specification of field-of-view values")
    );
}

TEST_CASE("Parse Required: PlanarProjection/FOV/Down", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "left": 1.0,
                  "right": 1.0,
                  "up": 1.0
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6000): Missing specification of field-of-view values")
    );
}

TEST_CASE("Parse Required: PlanarProjection/FOV/Left", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 1.0,
                  "right": 1.0,
                  "up": 1.0
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6000): Missing specification of field-of-view values")
    );
}


TEST_CASE("Parse Required: PlanarProjection/FOV/Right", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 1.0,
                  "left": 1.0,
                  "up": 1.0
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6000): Missing specification of field-of-view values")
    );
}

TEST_CASE("Parse Required: PlanarProjection/FOV/Up", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "down": 1.0,
                  "left": 1.0,
                  "right": 1.0
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6000): Missing specification of field-of-view values"
        )
    );
}

TEST_CASE("Parse Required: ProjectionPlane/LowerLeft", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "ProjectionPlane",
                "upperleft": { "x": 1.0, "y": 1.0, "z": 1.0 },
                "upperright": { "x": 1.0, "y": 1.0, "z": 1.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6010): Failed parsing coordinates. Missing elements"
        )
    );
}

TEST_CASE("Parse Required: ProjectionPlane/UpperLeft", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1.0, "y": 1.0, "z": 1.0 },
                "upperright": { "x": 1.0, "y": 1.0, "z": 1.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6010): Failed parsing coordinates. Missing elements"
        )
    );
}

TEST_CASE("Parse Required: ProjectionPlane/UpperRight", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "ProjectionPlane",
                "lowerleft": { "x": 1.0, "y": 1.0, "z": 1.0 },
                "upperleft": { "x": 1.0, "y": 1.0, "z": 1.0 }
              }
            }
          ]
        }
      ]
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6010): Failed parsing coordinates. Missing elements"
        )
    );
}

TEST_CASE("Parse Required: SphericalMirror/Mesh", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "SphericalMirrorProjection"
              }
            }
          ]
        }
      ]
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Parse Required: SphericalMirror/Mesh/Bottom", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "SphericalMirrorProjection",
                "mesh": {
                    "left": "abc",
                    "right": "abc",
                    "top": "abc"
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Parse Required: SphericalMirror/Mesh/Left", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "SphericalMirrorProjection",
                "mesh": {
                    "bottom": "abc",
                    "right": "abc",
                    "top": "abc"
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Parse Required: SphericalMirror/Mesh/Right", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "SphericalMirrorProjection",
                "mesh": {
                    "bottom": "abc",
                    "left": "abc",
                    "top": "abc"
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Parse Required: SphericalMirror/Mesh/Top", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "SphericalMirrorProjection",
                "mesh": {
                    "bottom": "abc",
                    "left": "abc",
                    "right": "abc"
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Parse Required: SpoutOutputProjection/MappingSpoutName", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "SpoutOutputProjection"
              }
            }
          ]
        }
      ]
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("Could not find required key 'mappingspoutname'")
    );
}

TEST_CASE("Parse Required: User/Tracking/Tracker", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "device": "abc"
      }
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'tracker' in User")
    );
}

TEST_CASE("Parse Required: User/Tracking/Device", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "tracker": "abc"
      }
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'device' in User")
    );
}

TEST_CASE("Parse Required: Tracker/Name", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6070): Tracker is missing 'name'")
    );
}

TEST_CASE("Parse Required: TextureMappedProjection mesh", "[parse]") {
    constexpr std::string_view Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {
          "size": { "x": 1, "y": 2 },
          "viewports": [
            {
              "projection": {
                "type": "TextureMappedProjection",
                "fov": {
                  "up": 1.0,
                  "down": 1.0,
                  "left": 1.0,
                  "right": 1.0
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
    CHECK_THROWS_MATCHES(
        sgct::readJsonConfig(Sources),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6110): Missing correction mesh for TextureMappedProjection"
        )
    );
}
