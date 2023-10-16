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
#include <sgct/readconfig.h>
#include <fstream>
#include <sstream>

namespace {
    std::string stringify(const std::string filename) {
        std::ifstream myfile;
        myfile.open(filename);
        std::stringstream buffer;
        buffer << myfile.rdbuf();
        return buffer.str();
    }

    void attemptValidation(const std::string cfgString) {
        std::string schemaString =
            stringify(std::string(BASE_PATH) + "/sgct.schema.json");
        std::filesystem::path schemaDir =
            std::filesystem::u8path(std::string(BASE_PATH));
        sgct::validateConfigAgainstSchema(cfgString, schemaString, schemaDir);
    }
} // namespace

TEST_CASE("Parse Required Schema: Version", "[parse schema]") {
    const std::string Sources = R"(
{}
)";
    CHECK_THROWS_MATCHES(
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At  of {} - required property 'version' not found in object\n"
        )
    );
}

TEST_CASE("Parse Required Schema: Cluster/Master Address", "[parse schema]") {
    const std::string Sources = R"(
{
  "version": 1
}
)";
    CHECK_THROWS_MATCHES(
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At  of {\"version\":1} - required property 'masteraddress' not found in "
            "object\n"
        )
    );
}

TEST_CASE("Parse Required Schema: Node/Address", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0 of {\"port\":1} - required property 'address' not found "
            "in object\n"
        )
    );
}

TEST_CASE("Parse Required Schema: Node/Port", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0 of {\"address\":\"localhost\"} - required property 'port' "
            "not found in object\n"
        )
    );
}

TEST_CASE("Parse Required Schema: Window/Size", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
          "At /nodes/0/windows/0 of {} - required property 'size' not found in object\n"
        )
    );
}

TEST_CASE("Parse Required Schema: FisheyeProjection/Crop/Left", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"crop\":{\"bottom\":1.0,"
            "\"right\":1.0,\"top\":1.0},\"type\":\"FisheyeProjection\"} - no subschema "
            "has succeeded, but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: FisheyeProjection/Crop/Right", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"crop\":{\"bottom\":1.0,"
            "\"left\":1.0,\"top\":1.0},\"type\":\"FisheyeProjection\"} - no subschema "
            "has succeeded, but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: FisheyeProjection/Crop/Bottom", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"crop\":{\"left\":1.0,"
            "\"right\":1.0,\"top\":1.0},\"type\":\"FisheyeProjection\"} - no subschema "
            "has succeeded, but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: FisheyeProjection/Crop/Top", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"crop\":{\"bottom\":1.0,"
            "\"left\":1.0,\"right\":1.0},\"type\":\"FisheyeProjection\"} - no subschema "
            "has succeeded, but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: PlanarProjection/FOV/Down", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of "
            "{\"fov\":{\"left\":1.0,\"right\":1.0,\"up\":1.0},\"type\":\"PlanarProjection\"} "
            "- no subschema has succeeded, but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: PlanarProjection/FOV/Left", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"fov\":{\"down\":1.0,"
            "\"right\":1.0,\"up\":1.0},\"type\":\"PlanarProjection\"} - no subschema "
            "has succeeded, but one of them is required to validate\n"
        )
    );
}


TEST_CASE("Parse Required Schema: PlanarProjection/FOV/Right", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"fov\":{\"down\":1.0,"
            "\"left\":1.0,\"up\":1.0},\"type\":\"PlanarProjection\"} - no subschema "
            "has succeeded, but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: PlanarProjection/FOV/Up", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"fov\":{\"down\":1.0,"
            "\"left\":1.0,\"right\":1.0},\"type\":\"PlanarProjection\"} - no subschema "
            "has succeeded, but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: ProjectionPlane/LowerLeft", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"type\":"
            "\"ProjectionPlane\",\"upperleft\":{\"x\":1.0,\"y\":1.0,\"z\":1.0},"
            "\"upperright\":{\"x\":1.0,\"y\":1.0,\"z\":1.0}} "
            "- no subschema has succeeded, but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: ProjectionPlane/UpperLeft", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"lowerleft\":"
            "{\"x\":1.0,\"y\":1.0,\"z\":1.0},\"type\":\"ProjectionPlane\",\"upperright\":"
            "{\"x\":1.0,\"y\":1.0,\"z\":1.0}} - no subschema has succeeded, "
            "but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: ProjectionPlane/UpperRight", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"lowerleft\":"
            "{\"x\":1.0,\"y\":1.0,\"z\":1.0},\"type\":\"ProjectionPlane\",\"upperleft\":"
            "{\"x\":1.0,\"y\":1.0,\"z\":1.0}} - no subschema has succeeded, "
            "but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: SphericalMirror/Mesh", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"type\":"
            "\"SphericalMirrorProjection\"} - no subschema has succeeded, but one of "
            "them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: SphericalMirror/Mesh/Bottom", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"mesh\":{\"left\":\"abc\","
            "\"right\":\"abc\",\"top\":\"abc\"},\"type\":\"SphericalMirrorProjection\"} "
            "- no subschema has succeeded, but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: SphericalMirror/Mesh/Left", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"mesh\":{\"bottom\":"
            "\"abc\",\"right\":\"abc\",\"top\":\"abc\"},\"type\":"
            "\"SphericalMirrorProjection\"} - no subschema has succeeded, but one of "
            "them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: SphericalMirror/Mesh/Right", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"mesh\":{\"bottom\":"
            "\"abc\",\"left\":\"abc\",\"top\":\"abc\"},\"type\":"
            "\"SphericalMirrorProjection\"} - no subschema has succeeded, but one of "
            "them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: SphericalMirror/Mesh/Top", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"mesh\":{\"bottom\":"
            "\"abc\",\"left\":\"abc\",\"right\":\"abc\"},\"type\":"
            "\"SphericalMirrorProjection\"} - no subschema has succeeded, "
            "but one of them is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: SpoutOutputProjection/MappingSpoutName", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /nodes/0/windows/0/viewports/0/projection of {\"type\":"
            "\"SpoutOutputProjection\"} - no subschema has succeeded, but one of them "
            "is required to validate\n"
        )
    );
}

TEST_CASE("Parse Required Schema: User/Tracking/Tracker", "[parse schema]") {
    const std::string Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": [
        {
          "device": "abc"
        }
      ]
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /users/0/tracking/0 of {\"device\":\"abc\"} - required property "
            "'tracker' not found in object\n"
        )
    );
}

TEST_CASE("Parse Required Schema: User/Tracking/Device", "[parse schema]") {
    const std::string Sources = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": [
        {
          "tracker": "abc"
        }
      ]
    }
  ]
}
)";
    CHECK_THROWS_MATCHES(
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /users/0/tracking/0 of {\"tracker\":\"abc\"} - required property "
            "'device' not found in object\n"
        )
    );
}

TEST_CASE("Parse Required Schema: Tracker/Name", "[parse schema]") {
    const std::string Sources = R"(
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
        attemptValidation(Sources),
        std::exception,
        Catch::Matchers::Message(
            "At /trackers/0 of {} - required property 'name' not found in object\n"
        )
    );
}
