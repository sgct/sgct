/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <sgct/config.h>
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace sgct;

// clang-tidy is convinced that it is possible to use emplace_back instead of push_back
// for the projection types, but I haven't been able to convince the Visual Studio
// compiler to agree
// NOLINTBEGIN(modernize-use-emplace)

TEST_CASE("Default constructed", "[roundtrip]") {
    const config::Cluster input = {
        .success = true
    };
    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Cluster/MasterAddress", "[roundtrip]") {
    const config::Cluster input = {
        .success = true,
        .masterAddress = "abc123"
    };
    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Cluster/DebugLog", "[roundtrip]") {
    {
        const config::Cluster input = {
            .success = true,
            .debugLog = std::nullopt
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .debugLog = false
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .debugLog = true
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Cluster/SetThreadAffinity", "[roundtrip]") {
    {
        const config::Cluster input = {
            .success = true,
            .setThreadAffinity = std::nullopt
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .setThreadAffinity = false
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .setThreadAffinity = true
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Cluster/FirmSync", "[roundtrip]") {
    {
        const config::Cluster input = {
            .success = true,
            .firmSync = std::nullopt
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .firmSync = false
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .firmSync = true
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Scene", "[roundtrip]") {
    {
        const config::Cluster input = {
            .success = true,
            .scene = std::nullopt
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .scene = config::Scene()
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Scene scene = {
            .offset = vec3(1.f, 2.f, 3.f)
        };
        const config::Cluster input = {
            .success = true,
            .scene = scene
        };
        
        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Scene scene = {
            .orientation = quat(1.f, 2.f, 3.f, 4.f)
        };
        const config::Cluster input = {
            .success = true,
            .scene = scene
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Scene scene = {
            .scale = 1.f
        };
        const config::Cluster input = {
            .success = true,
            .scene = scene
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Scene scene = {
            .offset = vec3(1.f, 2.f, 3.f),
            .orientation = quat(1.f, 2.f, 3.f, 4.f),
            .scale = 1.f
        };

        const config::Cluster input = {
            .success = true,
            .scene = scene
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Node", "[roundtrip]") {
    const config::Node node = {
        .address = "abc",
        .port = 1
    };

    const config::Cluster input = {
        .success = true,
        .nodes = { node }
    };

    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Node/DataTransferPort", "[roundtrip]") {
    {
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .dataTransferPort = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .dataTransferPort = 0
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .dataTransferPort = 1
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Node/SwapLock", "[roundtrip]") {
    {
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .swapLock = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .swapLock = false
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .swapLock = true
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window", "[roundtrip]") {
    {
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { config::Window() }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .id = 1
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window1 = {
            .id = 1
        };
        const config::Window window2 = {
            .id = 2
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window1, window2 }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Name", "[roundtrip]") {
    {
        const config::Window window = {
            .name = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .name = "abc"
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .name = "def"
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Tags", "[roundtrip]") {
    {
        const config::Window window = {
            .tags = std::vector<std::string>()
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .tags = { "abc" }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .tags = { "abc", "def" }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/BufferBitDepth", "[roundtrip]") {
    {
        const config::Window window = {
            .bufferBitDepth = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .bufferBitDepth = config::Window::ColorBitDepth::Depth8
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .bufferBitDepth = config::Window::ColorBitDepth::Depth16
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .bufferBitDepth = config::Window::ColorBitDepth::Depth16Float
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .bufferBitDepth = config::Window::ColorBitDepth::Depth32Float
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .bufferBitDepth = config::Window::ColorBitDepth::Depth16Int
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .bufferBitDepth = config::Window::ColorBitDepth::Depth32Int
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .bufferBitDepth = config::Window::ColorBitDepth::Depth16UInt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .bufferBitDepth = config::Window::ColorBitDepth::Depth32UInt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsFullScreen", "[roundtrip]") {
    {
        const config::Window window = {
            .isFullScreen = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isFullScreen = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isFullScreen = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/ShouldAutoIconify", "[roundtrip]") {
    {
        const config::Window window = {
            .shouldAutoiconify = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .shouldAutoiconify = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .shouldAutoiconify = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/HideMouseCursor", "[roundtrip]") {
    {
        const config::Window window = {
            .hideMouseCursor = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .hideMouseCursor = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .hideMouseCursor = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsFloating", "[roundtrip]") {
    {
        const config::Window window = {
            .isFloating = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isFloating = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isFloating = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/AlwaysRender", "[roundtrip]") {
    {
        const config::Window window = {
            .alwaysRender = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .alwaysRender = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .alwaysRender = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsHidden", "[roundtrip]") {
    {
        const config::Window window = {
            .isHidden = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isHidden = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isHidden = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/TakeScreenshot", "[roundtrip]") {
    {
        const config::Window window = {
            .takeScreenshot = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .takeScreenshot = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .takeScreenshot = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/MSAA", "[roundtrip]") {
    {
        const config::Window window = {
            .msaa = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .msaa = 0
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .msaa = 1
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/UseFXAA", "[roundtrip]") {
    {
        const config::Window window = {
            .useFxaa = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .useFxaa = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .useFxaa = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsDecorated", "[roundtrip]") {
    {
        const config::Window window = {
            .isDecorated = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isDecorated = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isDecorated = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsResizable", "[roundtrip]") {
    {
        const config::Window window = {
            .isResizable = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isResizable = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isResizable = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Draw2D", "[roundtrip]") {
    {
        const config::Window window = {
            .draw2D = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .draw2D = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .draw2D = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Draw3D", "[roundtrip]") {
    {
        const config::Window window = {
            .draw3D = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .draw2D = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .draw2D = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsMirrored", "[roundtrip]") {
    {
        const config::Window window = {
            .isMirrored = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isMirrored = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .isMirrored = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/NoError", "[roundtrip]") {
    {
        const config::Window window = {
            .noError = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .noError = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .noError = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/BlitWindowId", "[roundtrip]") {
    {
        const config::Window window = {
            .blitWindowId = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .blitWindowId = 0
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .blitWindowId = 1
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/MirrorX", "[roundtrip]") {
    {
        const config::Window window = {
            .mirrorX = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .mirrorX = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .mirrorX = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/MirrorY", "[roundtrip]") {
    {
        const config::Window window = {
            .mirrorY = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .mirrorY = false
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .mirrorY = true
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Monitor", "[roundtrip]") {
    {
        const config::Window window = {
            .monitor = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .monitor = 0
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .monitor = 1
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Stereo", "[roundtrip]") {
    {
        const config::Window window = {
            .stereo = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::NoStereo
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::Active
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::AnaglyphRedCyan
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::AnaglyphAmberBlue
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::AnaglyphRedCyanWimmer
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::Checkerboard
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::CheckerboardInverted
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::VerticalInterlaced
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::VerticalInterlacedInverted
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::Dummy
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::SideBySide
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::SideBySideInverted
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::TopBottom
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .stereo = config::Window::StereoMode::TopBottomInverted
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Spout/Enabled", "[roundtrip]") {
    {
        const config::Window::Spout spout = {
            .enabled = true
        };
        const config::Window window = {
            .spout = spout
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window::Spout spout = {
            .enabled = false
        };
        const config::Window window = {
            .spout = spout
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Spout/Name", "[roundtrip]") {
    {
        const config::Window::Spout spout = {
            .name = std::nullopt
        };
        const config::Window window = {
            .spout = spout
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window::Spout spout = {
            .name = "abc"
        };
        const config::Window window = {
            .spout = spout
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window::Spout spout = {
            .name = "def"
        };
        const config::Window window = {
            .spout = spout
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/NDI/Enabled", "[roundtrip]") {
    {
        const config::Window::NDI ndi = {
            .enabled = true
        };
        const config::Window window = {
            .ndi = ndi
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window::NDI ndi = {
            .enabled = false
        };
        const config::Window window = {
            .ndi = ndi
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/NDI/Name", "[roundtrip]") {
    {
        const config::Window::NDI ndi = {
            .name = std::nullopt
        };
        const config::Window window = {
            .ndi = ndi
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window::NDI ndi = {
            .name = "abc"
        };
        const config::Window window = {
            .ndi = ndi
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window::NDI ndi = {
            .name = "def"
        };
        const config::Window window = {
            .ndi = ndi
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/NDI/Groups", "[roundtrip]") {
    {
        const config::Window::NDI ndi = {
            .groups = std::nullopt
        };
        const config::Window window = {
            .ndi = ndi
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window::NDI ndi = {
            .groups = "abc"
        };
        const config::Window window = {
            .ndi = ndi
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window::NDI ndi = {
            .groups = "def"
        };
        const config::Window window = {
            .ndi = ndi
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Pos", "[roundtrip]") {
    {
        const config::Window window = {
            .pos = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .pos = ivec2{1, 2}
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .pos = ivec2{3, 4}
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Size", "[roundtrip]") {
    {
        const config::Window window = {
            .size = ivec2{1, 2}
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .size = ivec2{3, 4}
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Resolution", "[roundtrip]") {
    {
        const config::Window window = {
            .resolution = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .resolution = ivec2{1, 2}
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .resolution = ivec2{3, 4}
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/ScalableMesh", "[roundtrip]") {
    {
        const config::Window window = {
            .scalableMesh = std::nullopt
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            // We need to add the current_path here as the `readJsonConfig` function will
            // do the same and else the equality check will fail. This is a consequence of
            // us defining the `Cluster` object by hand; the loaded version will have the
            // correct prefix to the path
            .scalableMesh = std::filesystem::current_path() / "abc"
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Window window = {
            .scalableMesh = std::filesystem::current_path() / "def"
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport", "[roundtrip]") {
    const config::Window window = {
        .viewports = { config::Viewport() }
    };
    const config::Node node = {
        .address = "abc",
        .port = 1,
        .windows = { window }
    };
    const config::Cluster input = {
        .success = true,
        .nodes = { node }
    };

    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Viewport/User", "[roundtrip]") {
    {
        const config::Viewport viewport = {
            .user = std::nullopt
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .user = "abc"
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .user = "def"
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/OverlayTexture", "[roundtrip]") {
    {
        const config::Viewport viewport = {
            .overlayTexture = std::nullopt
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .overlayTexture = std::filesystem::absolute("abc")
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .overlayTexture = std::filesystem::absolute("def")
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/BlendMaskTexture", "[roundtrip]") {
    {
        const config::Viewport viewport = {
            .blendMaskTexture = std::nullopt
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .blendMaskTexture = std::filesystem::absolute("abc")
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .blendMaskTexture = std::filesystem::absolute("def")
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/BlackLevelMaskTexture", "[roundtrip]") {
    {
        const config::Viewport viewport = {
            .blackLevelMaskTexture = std::nullopt
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .blackLevelMaskTexture = std::filesystem::absolute("abc")
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .blackLevelMaskTexture = std::filesystem::absolute("def")
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/CorrectionMeshTexture", "[roundtrip]") {
    {
        const config::Viewport viewport = {
            .correctionMeshTexture = std::nullopt
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .correctionMeshTexture = std::filesystem::absolute("abc")
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .correctionMeshTexture = std::filesystem::absolute("def")
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/IsTracked", "[roundtrip]") {
    {
        const config::Viewport viewport = {
            .isTracked = std::nullopt
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .isTracked = false
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .isTracked = true
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/Eye", "[roundtrip]") {
    {
        const config::Viewport viewport = {
            .eye = std::nullopt
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .eye = config::Viewport::Eye::Mono
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .eye = config::Viewport::Eye::StereoLeft
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .eye = config::Viewport::Eye::StereoRight
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/Position", "[roundtrip]") {
    {
        const config::Viewport viewport = {
            .position = std::nullopt
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .position = vec2{1.f, 2.f}
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .position = vec2{3.f, 4.f}
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/Size", "[roundtrip]") {
    {
        const config::Viewport viewport = {
            .size = std::nullopt
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .size = vec2{1.f, 2.f}
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Viewport viewport = {
            .size = vec2{3.f, 4.f}
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CubemapProjection", "[roundtrip]") {
    const config::Viewport viewport = {
        .projection = config::CubemapProjection()
    };
    const config::Window window = {
        .viewports = { viewport }
    };
    const config::Node node = {
        .address = "abc",
        .port = 1,
        .windows = { window }
    };
    const config::Cluster input = {
        .success = true,
        .nodes = { node }
    };

    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("CubemapProjection/Quality", "[roundtrip]") {
    {
        const config::CubemapProjection projection = {
            .quality = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection projection = {
            .quality = 256
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection projection = {
            .quality = 512
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CubemapProjection/Spout/Enabled", "[roundtrip]") {
    {
        const config::CubemapProjection::Spout spout = {
            .enabled = true
        };
        const config::CubemapProjection projection = {
            .spout = spout
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection::Spout spout = {
            .enabled = false
        };
        const config::CubemapProjection projection = {
            .spout = spout
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CubemapProjection/Spout/Name", "[roundtrip]") {
    {
        const config::CubemapProjection::Spout spout = {
            .name = std::nullopt
        };
        const config::CubemapProjection projection = {
            .spout = spout
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection::Spout spout = {
            .name = "abc"
        };
        const config::CubemapProjection projection = {
            .spout = spout
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection::Spout spout = {
            .name = "def"
        };
        const config::CubemapProjection projection = {
            .spout = spout
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CubemapProjection/NDI/Enabled", "[roundtrip]") {
    {
        const config::CubemapProjection::NDI ndi = {
            .enabled = true
        };
        const config::CubemapProjection projection = {
            .ndi = ndi
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection::NDI ndi = {
            .enabled = false
        };
        const config::CubemapProjection projection = {
            .ndi = ndi
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CubemapProjection/NDI/Name", "[roundtrip]") {
    {
        const config::CubemapProjection::NDI ndi = {
            .name = std::nullopt
        };
        const config::CubemapProjection projection = {
            .ndi = ndi
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection::NDI ndi = {
            .name = "abc"
        };
        const config::CubemapProjection projection = {
            .ndi = ndi
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection::NDI ndi = {
            .name = "def"
        };
        const config::CubemapProjection projection = {
            .ndi = ndi
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CubemapProjection/NDI/Groups", "[roundtrip]") {
    {
        const config::CubemapProjection::NDI ndi = {
            .groups = std::nullopt
        };
        const config::CubemapProjection projection = {
            .ndi = ndi
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection::NDI ndi = {
            .groups = "abc"
        };
        const config::CubemapProjection projection = {
            .ndi = ndi
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection::NDI ndi = {
            .groups = "def"
        };
        const config::CubemapProjection projection = {
            .ndi = ndi
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CubemapProjection/Channels", "[roundtrip]") {
    {
        const config::CubemapProjection projection = {
            .channels = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    constexpr auto idxToChannels = [](uint8_t idx) -> config::CubemapProjection::Channels
    {
        config::CubemapProjection::Channels res;
        res.right  = idx & 0b000001;
        res.zLeft  = idx & 0b000010;
        res.bottom = idx & 0b000100;
        res.top    = idx & 0b001000;
        res.left   = idx & 0b010000;
        res.zRight = idx & 0b100000;
        return res;
    };

    for (uint8_t i = 0; i < 0b111111; i++) {
        const config::CubemapProjection projection = {
            .channels = idxToChannels(i)
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CubemapProjection/Orientation", "[roundtrip]") {
    {
        const config::CubemapProjection projection = {
            .orientation = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection projection = {
            .orientation = vec3{ 1.f, 2.f, 3.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CubemapProjection projection = {
            .orientation = vec3{ 4.f, 5.f, 6.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection", "[roundtrip]") {
    const config::Viewport viewport = {
        .projection = config::CylindricalProjection()
    };
    const config::Window window = {
        .viewports = { viewport }
    };
    const config::Node node = {
        .address = "abc",
        .port = 1,
        .windows = { window }
    };
    const config::Cluster input = {
        .success = true,
        .nodes = { node }
    };

    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("CylindricalProjection/Quality", "[roundtrip]") {
    {
        const config::CylindricalProjection projection = {
            .quality = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CylindricalProjection projection = {
            .quality = 256
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CylindricalProjection projection = {
            .quality = 512
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection/Rotation", "[roundtrip]") {
    {
        const config::CylindricalProjection projection = {
            .rotation = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CylindricalProjection projection = {
            .rotation = 1.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CylindricalProjection projection = {
            .rotation = 2.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection/HeightOffset", "[roundtrip]") {
    {
        const config::CylindricalProjection projection = {
            .heightOffset = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CylindricalProjection projection = {
            .heightOffset = 1.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CylindricalProjection projection = {
            .heightOffset = 2.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection/Radi", "[roundtrip]") {
    {
        const config::CylindricalProjection projection = {
            .radius = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CylindricalProjection projection = {
            .radius = 1.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::CylindricalProjection projection = {
            .radius = 2.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("EquirectangularProjection", "[roundtrip]") {
    const config::Viewport viewport = {
        .projection = config::EquirectangularProjection()
    };
    const config::Window window = {
        .viewports = { viewport }
    };
    const config::Node node = {
        .address = "abc",
        .port = 1,
        .windows = { window }
    };
    const config::Cluster input = {
        .success = true,
        .nodes = { node }
    };

    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("EquirectangularProjection/Quality", "[roundtrip]") {
    {
        const config::EquirectangularProjection projection = {
            .quality = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::EquirectangularProjection projection = {
            .quality = 256
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::EquirectangularProjection projection = {
            .quality = 512
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection", "[roundtrip]") {
    const config::Viewport viewport = {
        .projection = config::FisheyeProjection()
    };
    const config::Window window = {
        .viewports = { viewport }
    };
    const config::Node node = {
        .address = "abc",
        .port = 1,
        .windows = { window }
    };
    const config::Cluster input = {
        .success = true,
        .nodes = { node }
    };

    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("FisheyeProjection/FOV", "[roundtrip]") {
    {
        const config::FisheyeProjection projection = {
            .fov = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .fov = 1.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .fov = 2.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Quality", "[roundtrip]") {
    {
        const config::FisheyeProjection projection = {
            .quality = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .quality = 256
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .quality = 512
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Interpolation", "[roundtrip]") {
    {
        const config::FisheyeProjection projection = {
            .interpolation = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .interpolation = config::FisheyeProjection::Interpolation::Linear
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .interpolation = config::FisheyeProjection::Interpolation::Cubic
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Tilt", "[roundtrip]") {
    {
        const config::FisheyeProjection projection = {
            .tilt = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .tilt = 1.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .tilt = 2.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Diameter", "[roundtrip]") {
    {
        const config::FisheyeProjection projection = {
            .diameter = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .diameter = 1.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .diameter = 2.f
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Crop", "[roundtrip]") {
    {
        const config::FisheyeProjection projection = {
            .crop = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .crop = config::FisheyeProjection::Crop{ 1.f, 2.f, 3.f, 4.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .crop = config::FisheyeProjection::Crop{ 5.f, 6.f, 7.f, 8.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/KeepAspectRatio", "[roundtrip]") {
    {
        const config::FisheyeProjection projection = {
            .keepAspectRatio = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .keepAspectRatio = false
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .keepAspectRatio = true
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Offset", "[roundtrip]") {
    {
        const config::FisheyeProjection projection = {
            .offset = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .offset = vec3{ 1.f, 2.f, 3.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .offset = vec3{ 4.f, 5.f, 6.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Background", "[roundtrip]") {
    {
        const config::FisheyeProjection projection = {
            .background = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .background = vec4{ 1.f, 2.f, 3.f, 4.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::FisheyeProjection projection = {
            .background = vec4{ 5.f, 6.f, 7.f, 8.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("PlanarProjection", "[roundtrip]") {
    const config::Viewport viewport = {
        .projection = config::PlanarProjection()
    };
    const config::Window window = {
        .viewports = { viewport }
    };
    const config::Node node = {
        .address = "abc",
        .port = 1,
        .windows = { window }
    };
    const config::Cluster input = {
        .success = true,
        .nodes = { node }
    };

    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("PlanarProjection/FOV", "[roundtrip]") {
    {
        const config::PlanarProjection projection = {
            .fov = config::PlanarProjection::FOV { 1.f, 2.f, 3.f, 4.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::PlanarProjection projection = {
            .fov = config::PlanarProjection::FOV { 5.f, 6.f, 7.f, 8.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("PlanarProjection/Orientation", "[roundtrip]") {
    {
        const config::PlanarProjection projection = {
            .orientation = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::PlanarProjection projection = {
            .orientation = quat{ 1.f, 2.f, 3.f, 4.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::PlanarProjection projection = {
            .orientation = quat{ 5.f, 6.f, 7.f, 8.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("PlanarProjection/Offset", "[roundtrip]") {
    {
        const config::PlanarProjection projection = {
            .offset = std::nullopt
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::PlanarProjection projection = {
            .offset = vec3{ 1.f, 2.f, 3.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::PlanarProjection projection = {
            .offset = vec3{ 4.f, 5.f, 6.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("ProjectionPlane", "[roundtrip]") {
    const config::Viewport viewport = {
        .projection = config::ProjectionPlane()
    };
    const config::Window window = {
        .viewports = { viewport }
    };
    const config::Node node = {
        .address = "abc",
        .port = 1,
        .windows = { window }
    };
    const config::Cluster input = {
        .success = true,
        .nodes = { node }
    };

    const std::string str = serializeConfig(input);
    const config::Cluster output = readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("ProjectionPlane/LowerLeft", "[roundtrip]") {
    {
        const config::ProjectionPlane projection = {
            .lowerLeft = vec3{ 1.f, 2.f, 3.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::ProjectionPlane projection = {
            .lowerLeft = vec3{ 4.f, 5.f, 6.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("ProjectionPlane/UpperLeft", "[roundtrip]") {
    {
        const config::ProjectionPlane projection = {
            .upperLeft = vec3{ 1.f, 2.f, 3.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::ProjectionPlane projection = {
            .upperLeft = vec3{ 4.f, 5.f, 6.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("ProjectionPlane/UpperRight", "[roundtrip]") {
    {
        const config::ProjectionPlane projection = {
            .upperRight = vec3{ 1.f, 2.f, 3.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::ProjectionPlane projection = {
            .upperRight = vec3{ 4.f, 5.f, 6.f }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SphericalMirrorProjection", "[roundtrip]") {
    {
        const config::SphericalMirrorProjection projection = {
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::SphericalMirrorProjection projection = {
            .mesh = {.bottom = "mno", .left = "qpr", .right = "stu", .top = "vwx" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SphericalMirrorProjection/Quality", "[roundtrip]") {
    {
        const config::SphericalMirrorProjection projection = {
            .quality = std::nullopt,
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::SphericalMirrorProjection projection = {
            .quality = 256,
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::SphericalMirrorProjection projection = {
            .quality = 512,
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SphericalMirrorProjection/Tilt", "[roundtrip]") {
    {
        const config::SphericalMirrorProjection projection = {
            .tilt = std::nullopt,
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::SphericalMirrorProjection projection = {
            .tilt = 1.f,
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::SphericalMirrorProjection projection = {
            .tilt = 2.f,
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SphericalMirrorProjection/Background", "[roundtrip]") {
    {
        const config::SphericalMirrorProjection projection = {
            .background = std::nullopt,
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::SphericalMirrorProjection projection = {
            .background = vec4{ 1.f, 2.f, 3.f, 4.f },
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::SphericalMirrorProjection projection = {
            .background = vec4{ 5.f, 6.f, 7.f, 8.f },
            .mesh = {.bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        };
        const config::Viewport viewport = {
            .projection = projection
        };
        const config::Window window = {
            .viewports = { viewport }
        };
        const config::Node node = {
            .address = "abc",
            .port = 1,
            .windows = { window }
        };
        const config::Cluster input = {
            .success = true,
            .nodes = { node }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User", "[roundtrip]") {
    {
        const config::Cluster input = {
            .success = true,
            .users = { config::User() }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .users = { config::User(), config::User() }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .users = { config::User(), config::User(), config::User()}
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Name", "[roundtrip]") {
    {
        const config::User user = {
            .name = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::User user = {
            .name = "abc"
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::User user = {
            .name = "def"
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/EyeSeparation", "[roundtrip]") {
    {
        const config::User user = {
            .eyeSeparation = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::User user = {
            .eyeSeparation = 1.f
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::User user = {
            .eyeSeparation = 2.f
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Position", "[roundtrip]") {
    {
        const config::User user = {
            .position = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::User user = {
            .position = vec3{ 1.f, 2.f, 3.f }
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::User user = {
            .position = vec3{ 4.f, 5.f, 6.f }
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Transformation", "[roundtrip]") {
    {
        const config::User user = {
            .transformation = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        config::User user;
        user.transformation = sgct::mat4();
        user.transformation->values[0] = 1.f;
        user.transformation->values[1] = 2.f;
        user.transformation->values[2] = 3.f;
        user.transformation->values[3] = 4.f;
        user.transformation->values[4] = 5.f;
        user.transformation->values[5] = 6.f;
        user.transformation->values[6] = 7.f;
        user.transformation->values[7] = 8.f;
        user.transformation->values[8] = 8.f;
        user.transformation->values[9] = 10.f;
        user.transformation->values[10] = 11.f;
        user.transformation->values[11] = 12.f;
        user.transformation->values[12] = 13.f;
        user.transformation->values[13] = 14.f;
        user.transformation->values[14] = 15.f;
        user.transformation->values[15] = 16.f;
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::User user;
        user.transformation = sgct::mat4();
        user.transformation->values[0] = 17.f;
        user.transformation->values[1] = 18.f;
        user.transformation->values[2] = 19.f;
        user.transformation->values[3] = 20.f;
        user.transformation->values[4] = 21.f;
        user.transformation->values[5] = 22.f;
        user.transformation->values[6] = 23.f;
        user.transformation->values[7] = 24.f;
        user.transformation->values[8] = 25.f;
        user.transformation->values[9] = 26.f;
        user.transformation->values[10] = 27.f;
        user.transformation->values[11] = 28.f;
        user.transformation->values[12] = 29.f;
        user.transformation->values[13] = 30.f;
        user.transformation->values[14] = 31.f;
        user.transformation->values[15] = 32.f;
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Tracking", "[roundtrip]") {
    {
        const config::User user = {
            .tracking = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::User user = {
            .tracking = config::User::Tracking {.tracker = "abc", .device = "def" }
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::User user = {
            .tracking = config::User::Tracking {.tracker = "ghi", .device = "jkl" }
        };
        const config::Cluster input = {
            .success = true,
            .users = { user }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture", "[roundtrip]") {
    {
        const config::Cluster input = {
            .success = true,
            .capture = std::nullopt
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture()
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture/Path", "[roundtrip]") {
    {
        const config::Capture capture = {
            .path = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .capture = capture
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Capture capture = {
            .path = "abc"
        };
        const config::Cluster input = {
            .success = true,
            .capture = capture
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Capture capture = {
            .path = "def"
        };
        const config::Cluster input = {
            .success = true,
            .capture = capture
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture/ScreenShotRange", "[roundtrip]") {
    {
        const config::Capture capture = {
            .range = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .capture = capture
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Capture capture = {
            .range = config::Capture::ScreenShotRange { 1, 2 }
        };
        const config::Cluster input = {
            .success = true,
            .capture = capture
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Capture capture = {
            .range = config::Capture::ScreenShotRange { 3, 4 }
        };
        const config::Cluster input = {
            .success = true,
            .capture = capture
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker", "[roundtrip]") {
    {
        const config::Cluster input = {
            .success = true,
            .trackers = { config::Tracker() }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .trackers = { config::Tracker(), config::Tracker() }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .trackers = { config::Tracker(), config::Tracker(), config::Tracker() }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Name", "[roundtrip]") {
    {
        const config::Tracker tracker = {
            .name = "abc"
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Tracker tracker = {
            .name = "def"
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device", "[roundtrip]") {
    {
        const config::Tracker tracker = {
            .devices = { config::Device() }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Tracker tracker = {
            .devices = { config::Device(), config::Device() }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Tracker tracker = {
            .devices = { config::Device(), config::Device(), config::Device() }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Name", "[roundtrip]") {
    {
        const config::Device device = {
            .name = "abc"
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Device device = {
            .name = "def"
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Sensors", "[roundtrip]") {
    {
        const config::Device::Sensor sensor = {
            .vrpnAddress = "abc",
            .identifier = 1
        };
        const config::Device device = {
            .sensors = { sensor }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Device::Sensor sensor = {
            .vrpnAddress = "def",
            .identifier = 2
        };
        const config::Device device = {
            .sensors = { sensor }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Device::Sensor sensor1 = {
            .vrpnAddress = "abc",
            .identifier = 1
        };
        const config::Device::Sensor sensor2 = {
            .vrpnAddress = "def",
            .identifier = 2
        };
        const config::Device device = {
            .sensors = { sensor1, sensor2 }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Buttons", "[roundtrip]") {
    {
        const config::Device::Button button = {
            .vrpnAddress = "abc",
            .count = 1
        };
        const config::Device device = {
            .buttons= { button }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Device::Button button = {
            .vrpnAddress = "def",
            .count = 2
        };
        const config::Device device = {
            .buttons = { button }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Device::Button button1 = {
            .vrpnAddress = "abc",
            .count = 1
        };
        const config::Device::Button button2 = {
            .vrpnAddress = "def",
            .count = 2
        };
        const config::Device device = {
            .buttons = { button1, button2 }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Axes", "[roundtrip]") {
    {
        const config::Device::Axis axis = {
            .vrpnAddress = "abc",
            .count = 1
        };
        const config::Device device = {
            .axes = { axis }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Device::Axis axis = {
            .vrpnAddress = "def",
            .count = 2
        };
        const config::Device device = {
            .axes = { axis }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Device::Axis axis1 = {
            .vrpnAddress = "def",
            .count = 1
        };
        const config::Device::Axis axis2 = {
            .vrpnAddress = "abc",
            .count = 2
        };
        const config::Device device = {
            .axes = { axis1, axis2 }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Offset", "[roundtrip]") {
    {
        const config::Device device = {
            .offset = std::nullopt
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Device device = {
            .offset = vec3{ 1.f, 2.f, 3.f }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Device device = {
            .offset = vec3{ 4.f, 5.f, 6.f }
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Transformation", "[roundtrip]") {
    {
        const config::Device device = {
            .transformation = std::nullopt
        };
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        config::Device device;
        device.transformation = sgct::mat4();
        device.transformation->values[0] = 1.f;
        device.transformation->values[1] = 2.f;
        device.transformation->values[2] = 3.f;
        device.transformation->values[3] = 4.f;
        device.transformation->values[4] = 5.f;
        device.transformation->values[5] = 6.f;
        device.transformation->values[6] = 7.f;
        device.transformation->values[7] = 8.f;
        device.transformation->values[8] = 8.f;
        device.transformation->values[9] = 10.f;
        device.transformation->values[10] = 11.f;
        device.transformation->values[11] = 12.f;
        device.transformation->values[12] = 13.f;
        device.transformation->values[13] = 14.f;
        device.transformation->values[14] = 15.f;
        device.transformation->values[15] = 16.f;
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Device device;
        device.transformation = sgct::mat4();
        device.transformation->values[0] = 17.f;
        device.transformation->values[1] = 18.f;
        device.transformation->values[2] = 19.f;
        device.transformation->values[3] = 20.f;
        device.transformation->values[4] = 21.f;
        device.transformation->values[5] = 22.f;
        device.transformation->values[6] = 23.f;
        device.transformation->values[7] = 24.f;
        device.transformation->values[8] = 25.f;
        device.transformation->values[9] = 26.f;
        device.transformation->values[10] = 27.f;
        device.transformation->values[11] = 28.f;
        device.transformation->values[12] = 29.f;
        device.transformation->values[13] = 30.f;
        device.transformation->values[14] = 31.f;
        device.transformation->values[15] = 32.f;
        const config::Tracker tracker = {
            .devices = { device }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Offset", "[roundtrip]") {
    {
        const config::Tracker tracker = {
            .offset = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Tracker tracker = {
            .offset = vec3 { 1.f, 2.f, 3.f }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Tracker tracker = {
            .offset = vec3 { 4.f, 5.f, 6.f }
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Scale", "[roundtrip]") {
    {
        const config::Tracker tracker = {
            .scale = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Tracker tracker = {
            .scale = 1.0
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Tracker tracker = {
            .scale = 2.0
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Transformation", "[roundtrip]") {
    {
        const config::Tracker tracker = {
            .transformation = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Tracker tracker;
        tracker.transformation = sgct::mat4();
        tracker.transformation->values[0] = 1.f;
        tracker.transformation->values[1] = 2.f;
        tracker.transformation->values[2] = 3.f;
        tracker.transformation->values[3] = 4.f;
        tracker.transformation->values[4] = 5.f;
        tracker.transformation->values[5] = 6.f;
        tracker.transformation->values[6] = 7.f;
        tracker.transformation->values[7] = 8.f;
        tracker.transformation->values[8] = 8.f;
        tracker.transformation->values[9] = 10.f;
        tracker.transformation->values[10] = 11.f;
        tracker.transformation->values[11] = 12.f;
        tracker.transformation->values[12] = 13.f;
        tracker.transformation->values[13] = 14.f;
        tracker.transformation->values[14] = 15.f;
        tracker.transformation->values[15] = 16.f;
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Tracker tracker;
        tracker.transformation = sgct::mat4();
        tracker.transformation->values[0] = 17.f;
        tracker.transformation->values[1] = 18.f;
        tracker.transformation->values[2] = 19.f;
        tracker.transformation->values[3] = 20.f;
        tracker.transformation->values[4] = 21.f;
        tracker.transformation->values[5] = 22.f;
        tracker.transformation->values[6] = 23.f;
        tracker.transformation->values[7] = 24.f;
        tracker.transformation->values[8] = 25.f;
        tracker.transformation->values[9] = 26.f;
        tracker.transformation->values[10] = 27.f;
        tracker.transformation->values[11] = 28.f;
        tracker.transformation->values[12] = 29.f;
        tracker.transformation->values[13] = 30.f;
        tracker.transformation->values[14] = 31.f;
        tracker.transformation->values[15] = 32.f;
        const config::Cluster input = {
            .success = true,
            .trackers = { tracker }
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings", "[roundtrip]") {
    {
        const config::Cluster input = {
            .success = true,
            .settings = std::nullopt
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Cluster input = {
            .success = true,
            .settings = config::Settings()
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/UseDepthTexture", "[roundtrip]") {
    {
        const config::Settings settings = {
            .useDepthTexture = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings settings = {
            .useDepthTexture = false
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings settings = {
            .useDepthTexture = true
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/UseNormalTexture", "[roundtrip]") {
    {
        const config::Settings settings = {
            .useNormalTexture = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings settings = {
            .useNormalTexture = false
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings settings = {
            .useNormalTexture = true
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/UsePositionTexture", "[roundtrip]") {
    {
        const config::Settings settings = {
            .usePositionTexture = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings settings = {
            .usePositionTexture = false
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings settings = {
            .usePositionTexture = true
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/BufferFloatPrecision", "[roundtrip]") {
    {
        const config::Settings settings = {
            .bufferFloatPrecision = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings settings = {
            .bufferFloatPrecision = config::Settings::BufferFloatPrecision::Float16Bit
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings settings = {
            .bufferFloatPrecision = config::Settings::BufferFloatPrecision::Float32Bit
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/Display", "[roundtrip]") {
    {
        const config::Settings settings = {
            .display = std::nullopt
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings settings = {
            .display = config::Settings::Display()
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/Display/SwapInterval", "[roundtrip]") {
    {
        const config::Settings::Display display = {
            .swapInterval = std::nullopt
        };
        const config::Settings settings = {
            .display = display
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings::Display display = {
            .swapInterval = 1
        };
        const config::Settings settings = {
            .display = display
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings::Display display = {
            .swapInterval = 2
        };
        const config::Settings settings = {
            .display = display
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/Display/RefreshRate", "[roundtrip]") {
    {
        const config::Settings::Display display = {
            .refreshRate = std::nullopt
        };
        const config::Settings settings = {
            .display = display
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings::Display display = {
            .refreshRate = 1
        };
        const config::Settings settings = {
            .display = display
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        const config::Settings::Display display = {
            .refreshRate = 2
        };
        const config::Settings settings = {
            .display = display
        };
        const config::Cluster input = {
            .success = true,
            .settings = settings
        };

        const std::string str = serializeConfig(input);
        const config::Cluster output = readJsonConfig(str);
        REQUIRE(input == output);
    }
}

// NOLINTEND(modernize-use-emplace)
