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

TEST_CASE("Validate: Cluster/DebugLog/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "debuglog": 1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/ThreadAffinity/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "threadaffinity": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/ThreadAffinity/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "threadaffinity": -1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6088): Thread Affinity must be 0 or positive"
        )
    );
}

TEST_CASE("Validate: Cluster/FirmSync/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "debuglog": 1
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Scene/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Nodes/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Users/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Capture/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Trackers/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Settings/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Generator/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Cluster/Meta/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": "abc"
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": 1
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {}
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": "abc",
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": 1,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": 1,
      "y": "abc",
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": 1,
      "y": 2
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Offset/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": {
      "x": 1,
      "y": 2,
      "z": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {}
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": 1
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Pitch/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "yaw": 2,
      "roll": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Pitch/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": "abc",
      "yaw": 2,
      "roll": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Yaw/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": 1,
      "roll": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Yaw/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": 1,
      "yaw": "abc",
      "roll": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Roll/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": 1,
      "yaw": 2
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Roll/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "pitch": 1,
      "yaw": 2,
      "roll": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "y": 2,
      "z": 3,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": "abc",
      "y": 2,
      "z": 3,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "z": 3,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": "abc",
      "z": 3,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": 2,
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": 2,
      "z": "abc",
      "w": 4
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/W/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Orientation/W/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "orientation": {
      "x": 1,
      "y": 2,
      "z": 3,
      "w": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Scale/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "scale": "abc"
  }
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

TEST_CASE("Validate: Node/Address/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "port": 123
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Node/Address/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": 123,
      "port": 123
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Node/Port/Missing", "[validate]") {
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
}

TEST_CASE("Validate: Node/Port/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Node/DataTransferPort/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "datatransferport": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Node/SwapLock/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "swaplock": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Node/Windows/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 123,
      "windows": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Empty", "[validate]") {
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
}

TEST_CASE("Validate: Window/ID/Wrong Type", "[validate]") {
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
          "id": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Name/Wrong Type", "[validate]") {
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
          "name": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Tags/Wrong Type", "[validate]") {
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
          "tags": [ 123 ]
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
          "tags": [ 123, "abc" ]
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
          "tags": [ "abc", 123 ]
        }
      ]
    }
  ]
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Window/Tags/Duplicate", "[validate]") {
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
          "tags": [ "abc", "def", "abc" ]
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/BufferBitDepth/Wrong Type", "[validate]") {
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
          "bufferbitdepth": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/BufferBitDepth/Illegal Value", "[validate]") {
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
          "bufferbitdepth": "1"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/IsFullScreen/Wrong Type", "[validate]") {
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
          "fullscreen": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/ShouldAutoIconify/Wrong Type", "[validate]") {
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
          "autoiconify": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/HideMouseCursor/Wrong Type", "[validate]") {
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
          "hidemousecursor": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/IsFloating/Wrong Type", "[validate]") {
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
          "floating": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/AlwaysRender/Wrong Type", "[validate]") {
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
          "alwaysrender": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/IsHidden/Wrong Type", "[validate]") {
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
          "hidden": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/TakeScreenshot/Wrong Type", "[validate]") {
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
          "takescreenshot": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/MSAA/Wrong Type", "[validate]") {
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
          "msaa": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/MSAA/Illegal Value", "[validate]") {
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
          "msaa": -10
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/UseFxaa/Wrong Type", "[validate]") {
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
          "fxaa": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/IsDecorated/Wrong Type", "[validate]") {
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
          "border": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/IsResizable/Wrong Type", "[validate]") {
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
          "resizable": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Draw2D/Wrong Type", "[validate]") {
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
          "draw2d": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Draw3D/Wrong Type", "[validate]") {
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
          "draw3d": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/IsMirrored/Wrong Type", "[validate]") {
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
          "mirror": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/NoError/Wrong Type", "[validate]") {
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
          "noerror": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/BlitWindowId/Wrong Type", "[validate]") {
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
          "blitwindowid": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/BlitWindowId/Illegal Value", "[validate]") {
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
          "blitwindowid": -5000
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
          "blitwindowid": 5000
        }
      ]
    }
  ]
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Window/MirrorX/Wrong Type", "[validate]") {
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
          "mirrorx": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/MirrorY/Wrong Type", "[validate]") {
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
          "mirrory": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Monitor/Wrong Type", "[validate]") {
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
          "monitor": "abc"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Monitor/Illegal Value", "[validate]") {
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
          "monitor": -1
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
          "monitor": 500
        }
      ]
    }
  ]
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Window/Stereo/Wrong Type", "[validate]") {
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
          "stereo": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Stereo/Illegal Value", "[validate]") {
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
          "stereo": "does-not-exist"
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Spout/Wrong Type", "[validate]") {
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
          "spout": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Spout/Enabled/Wrong Type", "[validate]") {
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
          "spout": {
            "enabled": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Spout/Name/Wrong Type", "[validate]") {
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
          "spout": {
            "enabled": true,
            "name": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Spout/Name/Illegal Value", "[validate]") {
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
          "spout": {
            "enabled": true,
            "name": ""
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/NDI/Wrong Type", "[validate]") {
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
          "ndi": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/NDI/Enabled/Wrong Type", "[validate]") {
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
          "ndi": {
            "enabled": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/NDI/Name/Wrong Type", "[validate]") {
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
          "ndi": {
            "enabled": true,
            "name": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/NDI/Name/Illegal Value", "[validate]") {
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
          "ndi": {
            "enabled": true,
            "name": ""
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/NDI/Groups/Wrong Type", "[validate]") {
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
          "ndi": {
            "enabled": true,
            "groups": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/NDI/Groups/Illegal Value", "[validate]") {
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
          "ndi": {
            "enabled": true,
            "groups": ""
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Pos/Wrong Type", "[validate]") {
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
          "pos": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Pos/All/Missing", "[validate]") {
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
          "pos": {}
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Pos/X/Missing", "[validate]") {
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
          "pos": {
            "y": 2
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Pos/X/Wrong Type", "[validate]") {
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
          "pos": {
            "x": "abc",
            "y": 2
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Pos/Y/Missing", "[validate]") {
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
          "pos": {
            "x": 1
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Pos/Y/Wrong Type", "[validate]") {
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
          "pos": {
            "x": 1,
            "y": "abc"
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Size/Wrong Type", "[validate]") {
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
          "size": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Size/All/Missing", "[validate]") {
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
          "size": {}
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Size/X/Missing", "[validate]") {
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
          "size": {
            "y": 2
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Size/X/Wrong Type", "[validate]") {
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
          "size": {
            "x": "abc",
            "y": 2
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Size/Y/Missing", "[validate]") {
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
          "size": {
            "x": 1
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Size/Y/Wrong Type", "[validate]") {
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
          "size": {
            "x": 1,
            "y": "abc"
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Resolution/Wrong Type", "[validate]") {
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
          "res": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Resolution/All/Missing", "[validate]") {
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
          "res": {}
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Resolution/X/Missing", "[validate]") {
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
          "res": {
            "y": 2
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Resolution/X/Wrong Type", "[validate]") {
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
          "res": {
            "x": "abc",
            "y": 2
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Resolution/Y/Missing", "[validate]") {
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
          "res": {
            "x": 1
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Scene/Resolution/Y/Wrong Type", "[validate]") {
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
          "res": {
            "x": 1,
            "y": "abc"
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/ScalableMesh/Wrong Type", "[validate]") {
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
          "scalableMesh": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Window/Viewports/Wrong Type", "[validate]") {
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
          "viewports": 123
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/User/Wrong Type", "[validate]") {
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
            "user": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/OverlayTexture/Wrong Type", "[validate]") {
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
            "overlayTexture": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/BlendMaskTexture/Wrong Type", "[validate]") {
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
            "blendMaskTexture": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/BlackLevelMaskTexture/Wrong Type", "[validate]") {
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
            "blacklevelmask": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/CorrectionMeshTexture/Wrong Type", "[validate]") {
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
            "mesh": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/IsTracked/Wrong Type", "[validate]") {
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
            "tracked": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/Eye/Wrong Type", "[validate]") {
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
            "eye": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/Eye/Illegal Value", "[validate]") {
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
            "eye": "abc"
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/Position/Wrong Type", "[validate]") {
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
            "pos": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/Position/All/Missing", "[validate]") {
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
            "pos": {}
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/Position/X/Missing", "[validate]") {
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
            "pos": {
              "y": 2
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

TEST_CASE("Validate: Viewport/Position/X/Wrong Type", "[validate]") {
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
            "pos": {
              "x": "abc",
              "y": 2
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

TEST_CASE("Validate: Viewport/Position/Y/Missing", "[validate]") {
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
            "pos": {
              "x": 1
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

TEST_CASE("Validate: Viewport/Position/Y/Wrong Type", "[validate]") {
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
            "pos": {
              "x": 1,
              "y": "abc"
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

TEST_CASE("Validate: Viewport/Size/Wrong Type", "[validate]") {
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
            "size": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/Size/All/Missing", "[validate]") {
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
            "size": {}
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Viewport/Size/X/Missing", "[validate]") {
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
            "size": {
              "y": 2
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

TEST_CASE("Validate: Viewport/Size/X/Wrong Type", "[validate]") {
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
            "size": {
              "x": "abc",
              "y": 2
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

TEST_CASE("Validate: Viewport/Size/Y/Missing", "[validate]") {
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
            "size": {
              "x": 1
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

TEST_CASE("Validate: Viewport/Size/Y/Wrong Type", "[validate]") {
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
            "size": {
              "x": 1,
              "y": "abc"
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

TEST_CASE("Validate: Viewport/Projection/Wrong Type", "[validate]") {
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
            "projection": 123
          }
        }
      ]
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: CubemapProjection/Quality/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
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

TEST_CASE("Validate: CubemapProjection/Quality/Illegal Value", "[validate]") {
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
              "type": "CubemapProjection",
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
              "type": "CubemapProjection",
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

TEST_CASE("Validate: CubemapProjection/Spout/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "spout": "abc"
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

TEST_CASE("Validate: CubemapProjection/Spout/Enabled/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "spout": {
                "enabled": "abc"
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

TEST_CASE("Validate: CubemapProjection/Spout/Name/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "spout": {
                "enabled": true,
                "name": 123
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

TEST_CASE("Validate: CubemapProjection/Spout/Name/Illegal Value", "[validate]") {
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
              "type": "CubemapProjection",
              "spout": {
                "enabled": true,
                "name": ""
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

TEST_CASE("Validate: CubemapProjection/NDI/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "ndi": "abc"
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

TEST_CASE("Validate: CubemapProjection/NDI/Enabled/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "ndi": {
                "enabled": "abc"
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

TEST_CASE("Validate: CubemapProjection/NDI/Name/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "ndi": {
                "enabled": true,
                "name": 123
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

TEST_CASE("Validate: CubemapProjection/NDI/Name/Illegal Value", "[validate]") {
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
              "type": "CubemapProjection",
              "ndi": {
                "enabled": true,
                "name": ""
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

TEST_CASE("Validate: CubemapProjection/NDI/Groups/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "ndi": {
                "enabled": true,
                "groups": 123
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

TEST_CASE("Validate: CubemapProjection/NDI/Groups/Illegal Value", "[validate]") {
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
              "type": "CubemapProjection",
              "ndi": {
                "enabled": true,
                "groups": ""
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

TEST_CASE("Validate: CubemapProjection/Channels/All/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {}
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

TEST_CASE("Validate: CubemapProjection/Channels/Right/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "zLeft": true,
                "bottom": true,
                "top": true,
                "left": true,
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/Right/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": "abc",
                "zLeft": true,
                "bottom": true,
                "top": true,
                "left": true,
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/zLeft/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "bottom": true,
                "top": true,
                "left": true,
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/zLeft/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "zLeft": "abc",
                "bottom": true,
                "top": true,
                "left": true,
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/Bottom/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "zLeft": true,
                "top": true,
                "left": true,
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/Bottom/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "zLeft": true,
                "bottom": "abc",
                "top": true,
                "left": true,
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/Top/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "zLeft": true,
                "bottom": true,
                "left": true,
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/Top/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "zLeft": true,
                "bottom": true,
                "top": "abc",
                "left": true,
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/Left/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "zLeft": true,
                "bottom": true,
                "top": true,
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/Left/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "zLeft": true,
                "bottom": true,
                "top": true,
                "left": "abc",
                "zRight": true
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

TEST_CASE("Validate: CubemapProjection/Channels/zRight/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "zLeft": true,
                "bottom": true,
                "top": true,
                "left": true
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

TEST_CASE("Validate: CubemapProjection/Channels/zRight/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "channels": {
                "right": true,
                "zLeft": true,
                "bottom": true,
                "top": true,
                "left": true,
                "zRight": "abc"
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

TEST_CASE("Validate: CubemapProjection/Orientation/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "orientation": "abc"
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

TEST_CASE("Validate: CubemapProjection/Orientation/All/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "orientation": {}
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

TEST_CASE("Validate: CubemapProjection/Orientation/X/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "orientation": {
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: CubemapProjection/Orientation/X/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "orientation": {
                "x": "abc",
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: CubemapProjection/Orientation/Y/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "orientation": {
                "x": 1,
                "z": 3
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

TEST_CASE("Validate: CubemapProjection/Orientation/Y/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "orientation": {
                "x": 1,
                "y": "abc",
                "z": 3
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

TEST_CASE("Validate: CubemapProjection/Orientation/Z/Missing", "[validate]") {
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
              "type": "CubemapProjection",
              "orientation": {
                "x": 1,
                "y": 2
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

TEST_CASE("Validate: CubemapProjection/Orientation/Z/Wrong Type", "[validate]") {
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
              "type": "CubemapProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": "abc"
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

TEST_CASE("Validate: CylindricalProjection/Quality/Wrong Type", "[validate]") {
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
              "type": "CylindricalProjection",
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

TEST_CASE("Validate: CylindricalProjection/Quality/Illegal Value", "[validate]") {
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
              "type": "CylindricalProjection",
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
              "type": "CylindricalProjection",
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

TEST_CASE("Validate: CylindricalProjection/Rotation/Wrong Type", "[validate]") {
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
              "type": "CylindricalProjection",
              "rotation": "abc"
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

TEST_CASE("Validate: CylindricalProjection/HeightOffset/Wrong Type", "[validate]") {
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
              "type": "CylindricalProjection",
              "heightoffset": "abc"
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

TEST_CASE("Validate: CylindricalProjection/Radius/Wrong Type", "[validate]") {
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
              "type": "CylindricalProjection",
              "radius": "abc"
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
          "viewport": {
            "projection": {
              "type": "EquirectangularProjection",
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
          "viewport": {
            "projection": {
              "type": "EquirectangularProjection",
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
              "type": "EquirectangularProjection",
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

TEST_CASE("Validate: FisheyeProjection/FOV/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "fov": "abc"
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

TEST_CASE("Validate: FisheyeProjection/FOV/Illegal value", "[validate]") {
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
              "type": "FisheyeProjection",
              "fov": -50.0
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

TEST_CASE("Validate: FisheyeProjection/Quality/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
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

TEST_CASE("Validate: FisheyeProjection/Quality/Illegal Value", "[validate]") {
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
              "type": "FisheyeProjection",
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
              "type": "FisheyeProjection",
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

TEST_CASE("Validate: FisheyeProjection/Interpolation/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "interpolation": 123
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

TEST_CASE("Validate: FisheyeProjection/Interpolation/Illegal Value", "[validate]") {
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
              "type": "FisheyeProjection",
              "interpolation": "abc"
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

TEST_CASE("Validate: FisheyeProjection/Tilt/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
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

TEST_CASE("Validate: FisheyeProjection/Diameter/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "diameter": "abc"
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

TEST_CASE("Validate: FisheyeProjection/Diameter/Illegal Value", "[validate]") {
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
              "type": "FisheyeProjection",
              "diameter": -50.0
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

TEST_CASE("Validate: FisheyeProjection/Crop/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": "abc"
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

TEST_CASE("Validate: FisheyeProjection/Crop/All/Missing Value", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": {}
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

TEST_CASE("Validate: FisheyeProjection/Crop/Left/Missing Value", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": {
                "right": 2,
                "bottom": 3,
                "top": 4
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

TEST_CASE("Validate: FisheyeProjection/Crop/Left/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": {
                "left": "abc",
                "right": 2,
                "bottom": 3,
                "top": 4
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

TEST_CASE("Validate: FisheyeProjection/Crop/Right/Missing Value", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": {
                "left": 1,
                "bottom": 3,
                "top": 4
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

TEST_CASE("Validate: FisheyeProjection/Crop/Right/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": {
                "left": 1,
                "right": "abc",
                "bottom": 3,
                "top": 4
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

TEST_CASE("Validate: FisheyeProjection/Crop/Bottom/Missing Value", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": {
                "left": 1,
                "right": 2,
                "top": 4
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

TEST_CASE("Validate: FisheyeProjection/Crop/Bottom/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": {
                "left": 1,
                "right": 2,
                "bottom": "abc",
                "top": 4
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

TEST_CASE("Validate: FisheyeProjection/Crop/Top/Missing Value", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": {
                "left": 1,
                "right": 2,
                "bottom": 3
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

TEST_CASE("Validate: FisheyeProjection/Crop/Top/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "crop": {
                "left": 1,
                "right": 2,
                "bottom": 3,
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

TEST_CASE("Validate: FisheyeProjection/KeepAspectRatio/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "keepaspectratio": "abc"
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

TEST_CASE("Validate: FisheyeProjection/Offset/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "offset": "abc"
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

TEST_CASE("Validate: FisheyeProjection/Offset/All/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "offset": {}
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

TEST_CASE("Validate: FisheyeProjection/Offset/X/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "offset": {
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: FisheyeProjection/Offset/X/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "offset": {
                "x": "abc",
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: FisheyeProjection/Offset/Y/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "offset": {
                "x": 1,
                "z": 3
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

TEST_CASE("Validate: FisheyeProjection/Offset/Y/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "offset": {
                "x": 1,
                "y": "abc",
                "z": 3
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

TEST_CASE("Validate: FisheyeProjection/Offset/Z/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "offset": {
                "x": 1,
                "y": 2
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

TEST_CASE("Validate: FisheyeProjection/Offset/Z/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "offset": {
                "x": 1,
                "y": 2,
                "z": "abc"
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

TEST_CASE("Validate: FisheyeProjection/Background/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
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

TEST_CASE("Validate: FisheyeProjection/Background/All/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
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

TEST_CASE("Validate: FisheyeProjection/Background/X/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "y": 2,
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/X/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "x": "abc",
                "y": 2,
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/Y/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "x": 1,
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/Y/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "x": 1,
                "y": "abc",
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/Z/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "x": 1,
                "y": 2,
                "w": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/Z/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "x": 1,
                "y": 2,
                "z": "abc",
                "w": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/W/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "x": 1,
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: FisheyeProjection/Background/W/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "x": 1,
                "y": 2,
                "z": 3,
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

TEST_CASE("Validate: FisheyeProjection/Background/R/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "g": 2,
                "b": 3,
                "a": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/R/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "r": "abc",
                "g": 2,
                "b": 3,
                "a": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/G/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "r": 1,
                "b": 3,
                "a": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/G/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "r": 1,
                "g": "abc",
                "b": 3,
                "a": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/B/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "r": 1,
                "g": 2,
                "a": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/B/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "r": 1,
                "g": 2,
                "b": "abc",
                "a": 4
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

TEST_CASE("Validate: FisheyeProjection/Background/A/Missing", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "r": 1,
                "g": 2,
                "b": 3
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

TEST_CASE("Validate: FisheyeProjection/Background/A/Wrong Type", "[validate]") {
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
              "type": "FisheyeProjection",
              "background": {
                "r": 1,
                "g": 2,
                "b": 3,
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

TEST_CASE("Validate: PlanarProjection/FOV/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": "abc"
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

TEST_CASE("Validate: PlanarProjection/FOV/All/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": {}
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

TEST_CASE("Validate: PlanarProjection/FOV/Down/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": {
                "left": 2,
                "right": 3,
                "up": 4
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

TEST_CASE("Validate: PlanarProjection/FOV/Down/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": {
                "down": "abc",
                "left": 2,
                "right": 3,
                "up": 4
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

TEST_CASE("Validate: PlanarProjection/FOV/Left/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "right": 3,
                "up": 4
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

TEST_CASE("Validate: PlanarProjection/FOV/Left/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": "abc",
                "right": 3,
                "up": 4
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

TEST_CASE("Validate: PlanarProjection/FOV/Right/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "up": 4
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

TEST_CASE("Validate: PlanarProjection/FOV/Right/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "right": "abc",
                "up": 4
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

TEST_CASE("Validate: PlanarProjection/FOV/Up/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "right": 3
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

TEST_CASE("Validate: PlanarProjection/FOV/Up/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "right": 3,
                "up": "abc"
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

TEST_CASE("Validate: PlanarProjection/Orientation/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": "abc"
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

TEST_CASE("Validate: PlanarProjection/Orientation/All/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {}
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

TEST_CASE("Validate: PlanarProjection/Orientation/Pitch/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "yaw": 2,
                "roll": 3
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

TEST_CASE("Validate: PlanarProjection/Orientation/Pitch/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "pitch": "abc",
                "yaw": 2,
                "roll": 3
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

TEST_CASE("Validate: PlanarProjection/Orientation/Yaw/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "pitch": 1,
                "roll": 3
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

TEST_CASE("Validate: PlanarProjection/Orientation/Yaw/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "pitch": 1,
                "yaw": "abc",
                "roll": 3
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

TEST_CASE("Validate: PlanarProjection/Orientation/Roll/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "pitch": 1,
                "yaw": 2
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

TEST_CASE("Validate: PlanarProjection/Orientation/Roll/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "pitch": 1,
                "yaw": 2,
                "roll": "abc"
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

TEST_CASE("Validate: PlanarProjection/Orientation/X/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "y": 2,
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: PlanarProjection/Orientation/X/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "x": "abc",
                "y": 2,
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: PlanarProjection/Orientation/Y/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: PlanarProjection/Orientation/Y/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": "abc",
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: PlanarProjection/Orientation/Z/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "w": 4
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

TEST_CASE("Validate: PlanarProjection/Orientation/Z/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": "abc",
                "w": 4
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

TEST_CASE("Validate: PlanarProjection/Orientation/W/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: PlanarProjection/Orientation/W/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": 3,
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

TEST_CASE("Validate: PlanarProjection/Offset/All/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "offset": {}
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

TEST_CASE("Validate: PlanarProjection/Offset/X/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "offset": {
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: PlanarProjection/Offset/X/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "offset": {
                "x": "abc",
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: PlanarProjection/Offset/Y/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "offset": {
                "x": 1,
                "z": 3
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

TEST_CASE("Validate: PlanarProjection/Offset/Y/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "offset": {
                "x": 1,
                "y": "abc",
                "z": 3
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

TEST_CASE("Validate: PlanarProjection/Offset/Z/Missing", "[validate]") {
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
              "type": "PlanarProjection",
              "offset": {
                "x": 1,
                "y": 2
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

TEST_CASE("Validate: PlanarProjection/Offset/Z/Wrong Type", "[validate]") {
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
              "type": "PlanarProjection",
              "offset": {
                "x": 1,
                "y": 2,
                "z": "abc"
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
          "viewport": {
            "projection": {
              "type": "ProjectionPlane",
              "lowerleft": "abc",
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/LowerLeft/All/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": {},
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/LowerLeft/X/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": {
                "y": 2,
                "z": 3
              },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/LowerLeft/X/Wrong Type", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": {
                "x": "abc",
                "y": 2,
                "z": 3
              },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/LowerLeft/Y/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": {
                "x": 1,
                "z": 3
              },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/LowerLeft/Y/Wrong Type", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": {
                "x": 1,
                "y": "abc",
                "z": 3
              },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/LowerLeft/Z/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": {
                "x": 1,
                "y": 2
              },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/LowerLeft/Z/Wrong Type", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": {
                "x": 1,
                "y": 2,
                "z": "abc"
              },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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
          "viewport": {
            "projection": {
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": "abc",
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/UpperLeft/All/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": {},
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/UpperLeft/X/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": {
                "y": 2,
                "z": 3
              },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/UpperLeft/X/Wrong Type", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": {
                "x": "abc",
                "y": 2,
                "z": 3
              },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/UpperLeft/Y/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": {
                "x": 1,
                "z": 3
              },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/UpperLeft/Y/Wrong Type", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": {
                "x": 1,
                "y": "abc",
                "z": 3
              },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/UpperLeft/Z/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": {
                "x": 1,
                "y": 2
              },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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

TEST_CASE("Validate: PlanarProjection/UpperLeft/Z/Wrong Type", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": {
                "x": 1,
                "y": 2,
                "z": "abc"
              },
              "upperRight": { "x": 1, "y": 2, "z": 3 }
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
          "viewport": {
            "projection": {
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": "abc"
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

TEST_CASE("Validate: PlanarProjection/UpperRight/All/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": {}
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

TEST_CASE("Validate: PlanarProjection/UpperRight/X/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": {
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: PlanarProjection/UpperRight/X/Wrong Type", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": {
                "x": "abc",
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: PlanarProjection/UpperRight/Y/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": {
                "x": 1,
                "z": 3
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

TEST_CASE("Validate: PlanarProjection/UpperRight/Y/Wrong Type", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": {
                "x": 1,
                "y": "abc",
                "z": 3
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

TEST_CASE("Validate: PlanarProjection/UpperRight/Z/Missing", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": {
                "x": 1,
                "y": 2
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

TEST_CASE("Validate: PlanarProjection/UpperRight/Z/Wrong Type", "[validate]") {
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
              "type": "ProjectionPlane",
              "lowerleft": { "x": 1, "y": 2, "z": 3 },
              "upperLeft": { "x": 1, "y": 2, "z": 3 },
              "upperRight": {
                "x": 1,
                "y": 2,
                "z": "abc"
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": "abc"
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

TEST_CASE("Validate: TextureMappedProjection/FOV/All/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": {}
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Down/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": {
                "left": 2,
                "right": 3,
                "up": 4
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Down/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": {
                "down": "abc",
                "left": 2,
                "right": 3,
                "up": 4
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Left/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": {
                "down": 1,
                "right": 3,
                "up": 4
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Left/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": {
                "down": 1,
                "left": "abc",
                "right": 3,
                "up": 4
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Right/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "up": 4
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Right/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "right": "abc",
                "up": 4
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Up/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "right": 3
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

TEST_CASE("Validate: TextureMappedProjection/FOV/Up/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "fov": {
                "down": 1,
                "left": 2,
                "right": 3,
                "up": "abc"
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": "abc"
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/All/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {}
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Pitch/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "yaw": 2,
                "roll": 3
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Pitch/Wrong Type", "[validate]")
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
              "type": "TextureMappedProjection",
              "orientation": {
                "pitch": "abc",
                "yaw": 2,
                "roll": 3
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Yaw/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "pitch": 1,
                "roll": 3
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Yaw/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "pitch": 1,
                "yaw": "abc",
                "roll": 3
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Roll/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "pitch": 1,
                "yaw": 2
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Roll/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "pitch": 1,
                "yaw": 2,
                "roll": "abc"
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/X/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "y": 2,
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/X/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "x": "abc",
                "y": 2,
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Y/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "x": 1,
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Y/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "x": 1,
                "y": "abc",
                "z": 3,
                "w": 4
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Z/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "w": 4
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/Z/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": "abc",
                "w": 4
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/W/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: TextureMappedProjection/Orientation/W/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "orientation": {
                "x": 1,
                "y": 2,
                "z": 3,
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

TEST_CASE("Validate: TextureMappedProjection/Offset/All/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "offset": {}
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

TEST_CASE("Validate: TextureMappedProjection/Offset/X/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "offset": {
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: TextureMappedProjection/Offset/X/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "offset": {
                "x": "abc",
                "y": 2,
                "z": 3
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

TEST_CASE("Validate: TextureMappedProjection/Offset/Y/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "offset": {
                "x": 1,
                "z": 3
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

TEST_CASE("Validate: TextureMappedProjection/Offset/Y/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "offset": {
                "x": 1,
                "y": "abc",
                "z": 3
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

TEST_CASE("Validate: TextureMappedProjection/Offset/Z/Missing", "[validate]") {
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
              "type": "TextureMappedProjection",
              "offset": {
                "x": 1,
                "y": 2
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

TEST_CASE("Validate: TextureMappedProjection/Offset/Z/Wrong Type", "[validate]") {
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
              "type": "TextureMappedProjection",
              "offset": {
                "x": 1,
                "y": 2,
                "z": "abc"
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

TEST_CASE("Validate: User/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "name": 123
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Name/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "name": ""
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/EyeSeparation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "eyeseparation": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {}
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "y": 2,
        "z": 3
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": "abc",
        "y": 2,
        "z": 3
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": 1,
        "z": 3
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": 1,
        "y": "abc",
        "z": 3
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": 1,
        "y": 2
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": 1,
        "y": 2,
        "z": "abc"
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Transformation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "matrix": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Transformation/Missing", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "matrix": []
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
  "users": [
    {
      "matrix": [ 1.0, 2.0, 3.0, 4.0 ]
    }
  ]
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: User/Tracking/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Tracking/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {}
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Tracking/Tracker/Missing", "[validate]") {
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
}

TEST_CASE("Validate: User/Tracking/Tracker/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "tracker": 123,
        "device": "abc"
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Tracking/Device/Missing", "[validate]") {
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
}

TEST_CASE("Validate: User/Tracking/Device/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "tracker": "abc",
        "device": 123
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Path/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "path": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Path/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "path": ""
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range-Begin/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangebegin": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range-Begin/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangebegin": -1000
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range-End/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangeend": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range-End/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangeend": -1000
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Capture/Range/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "capture": {
    "rangebegin": 10,
    "rangeend": 5
  }
}
)";

    CHECK_THROWS_MATCHES(
        readJsonConfig(Config),
        std::runtime_error,
        Catch::Matchers::Message(
            "[ReadConfig] (6051): End of range must be greater than beginning of range"
        )
    );
}

TEST_CASE("Validate: Tracker/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "name": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Name/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "name": ""
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "name": 123
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Name/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "name": ""
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Sensors/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "sensors": 123
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Sensors/Address/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "sensors": {
          "vrpnaddress": 123,
          "id": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Sensors/Address/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "sensors": {
          "vrpnaddress": "",
          "id": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Sensors/Identifier/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "sensors": {
          "vrpnaddress": "abc",
          "id": "abc"
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": 123
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Address/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": {
          "vrpnaddress": 123,
          "count": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Address/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": {
          "vrpnaddress": "",
          "count": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Count/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": {
          "vrpnaddress": "abc",
          "count": "abc"
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Count/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": {
          "vrpnaddress": "abc",
          "count": -1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": 123
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Address/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": {
          "vrpnaddress": 123,
          "count": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Address/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": {
          "vrpnaddress": "",
          "count": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Count/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": {
          "vrpnaddress": "abc",
          "count": "abc"
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Count/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": {
          "vrpnaddress": "abc",
          "count": -1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": "abc"
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {}
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "y": 2,
          "z": 3
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": "abc",
          "y": 2,
          "z": 3
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": 1,
          "z": 3
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": 1,
          "y": "abc",
          "z": 3
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": 1,
          "y": 2
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": 1,
          "y": 2,
          "z": "abc"
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Transformation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "matrix": "abc"
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Transformation/Illegal Value", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "matrix": []
      }
    ]
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }

    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "matrix": [ 1.0, 2.0, 3.0, 4.0 ]
      }
    ]
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Tracker/Offset/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {}
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": "abc",
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": 1,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": 1,
      "y": "abc",
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": 1,
      "y": 2
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": 1,
      "y": 2,
      "z": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Scale/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "scale": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Transformation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "matrix": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Transformation/Illegal Value", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "matrix": []
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }

    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "matrix": [ 1.0, 2.0, 3.0, 4.0 ]
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Settings/UseDepthTexture/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "depthbuffertexture": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/UseNormalTexture/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "normaltexture": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/UsePositionTexture/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "positiontexture": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/BufferFloatPrecision/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "precision": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/BufferFloatPrecision/Illegal Value", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "precision": -20
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }

    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "precision": 256
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Settings/Display/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/Display/SwapInterval/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/Display/SwapInterval/Illegal Value", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": -1
    }
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }

    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 1000
    }
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Settings/Display/RefreshRate/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "refreshrate": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Settings/Display/RefreshRate/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "refreshrate": -1
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: GeneratorVersion/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": 123,
    "major": 1,
    "minor": 2
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: GeneratorVersion/Major/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": "abc",
    "major": "abc",
    "minor": 2
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: GeneratorVersion/Major/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": "abc",
    "major": -1,
    "minor": 2
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: GeneratorVersion/Minor/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": "abc",
    "major": 2,
    "minor": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: GeneratorVersion/Minor/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "generator": {
    "name": "abc",
    "major": 2,
    "minor": -1
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Meta/Author/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "author": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Meta/Description/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "description": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Meta/License/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "license": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Meta/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "name": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Meta/Version/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "meta": {
    "version": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}
