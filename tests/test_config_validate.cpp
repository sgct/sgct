/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <sgct/config.h>
#include <fstream>
#include <sstream>

using namespace sgct;

namespace {
    struct ParsingError : std::runtime_error {
        explicit ParsingError(std::string msg) : std::runtime_error(std::move(msg)) {}
    };

    void validate(std::string_view cfgString) {
        const std::string schema = std::string(BASE_PATH) + "/sgct.schema.json";
        std::string err = validateConfigAgainstSchema(cfgString, schema);
        if (!err.empty()) {
            throw ParsingError(err);
        }
    }
} // namespace

TEST_CASE("Validate: Cluster/Empty", "[validate]") {
    constexpr std::string_view Config = R"(
{}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    CHECK_THROWS_AS(readJsonConfig(Config), ParsingError);
    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing 'version' information")
    );
}

TEST_CASE("Validate: Cluster/Version/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "masteraddress": "localhost"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing 'version' information")
    );
}

TEST_CASE("Validate: Cluster/Version/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": "abc",
  "masteraddress": "localhost"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    //CHECK_THROWS_MATCHES(
    //    readJsonConfig(Config),
    //    std::runtime_error,
    //    Catch::Matchers::Message("")
    //);
}

TEST_CASE("Validate: Cluster/Master Address/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6084): Cannot find master address")
    );
}

TEST_CASE("Validate: Cluster/Master Address/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": 1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Node/Empty", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {}
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Node/Address", "[validate]") {
    constexpr std::string_view Config = R"(
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6040): Missing field address in node")
    );
}

TEST_CASE("Validate: Node/Port", "[validate]") {
    constexpr std::string_view Config = R"(
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6041): Missing field port in node")
    );
}

TEST_CASE("Validate: Window/Size", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": [
        {}
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Could not find required key 'size'")
    );
}

TEST_CASE("Validate: FisheyeProjection/Crop/Left", "[validate]") {
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
          "size": { "x": 1, "y": 1 },
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'left' in FisheyeProjection/Crop")
    );
}

TEST_CASE("Validate: FisheyeProjection/Crop/Right", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'right' in FisheyeProjection/Crop")
    );
}

TEST_CASE("Validate: FisheyeProjection/Crop/Bottom", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'bottom' in FisheyeProjection/Crop")
    );
}

TEST_CASE("Validate: FisheyeProjection/Crop/Top", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'top' in FisheyeProjection/Crop")
    );
}

TEST_CASE("Validate: PlanarProjection/FOV", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6000): Missing specification of field-of-view values")
    );
}

TEST_CASE("Validate: PlanarProjection/FOV/Down", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6000): Missing specification of field-of-view values")
    );
}

TEST_CASE("Validate: PlanarProjection/FOV/Left", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6000): Missing specification of field-of-view values")
    );
}


TEST_CASE("Validate: PlanarProjection/FOV/Right", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6000): Missing specification of field-of-view values")
    );
}

TEST_CASE("Validate: PlanarProjection/FOV/Up", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6000): Missing specification of field-of-view values"
        )
    );
}

TEST_CASE("Validate: ProjectionPlane/LowerLeft", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6010): Failed parsing coordinates. Missing elements"
        )
    );
}

TEST_CASE("Validate: ProjectionPlane/UpperLeft", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6010): Failed parsing coordinates. Missing elements"
        )
    );
}

TEST_CASE("Validate: ProjectionPlane/UpperRight", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6010): Failed parsing coordinates. Missing elements"
        )
    );
}

TEST_CASE("Validate: SphericalMirror/Mesh", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Validate: SphericalMirror/Mesh/Bottom", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Validate: SphericalMirror/Mesh/Left", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Validate: SphericalMirror/Mesh/Right", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Validate: SphericalMirror/Mesh/Top", "[validate]") {
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6100): Missing geometry paths")
    );
}

TEST_CASE("Validate: TextureMappedProjection mesh", "[validate]") {
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

    // There is no check for the schema since we can't express that the *viewport* needs
    // a correction mesh iff the projection type is 'TextureMappedProjection'

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6110): Missing correction mesh for TextureMappedProjection"
        )
    );
}

TEST_CASE("Validate: Tracker/Name", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {}
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("[ReadConfig] (6070): Tracker is missing 'name'")
    );
}

TEST_CASE("Validate: User/Tracking/Tracker", "[validate]") {
    constexpr std::string_view Config = R"(
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'tracker' in User")
    );
}

TEST_CASE("Validate: User/Tracking/Device", "[validate]") {
    constexpr std::string_view Config = R"(
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

    CHECK_THROWS_AS(validate(Config), ParsingError);

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message("Missing key 'device' in User")
    );
}
