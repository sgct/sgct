/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include "equality.h"
#include <sgct/readconfig.h>
#include <nlohmann/json.hpp>
#include <filesystem>

TEST_CASE("Default constructed", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Cluster/DebugLog", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .debugLog = std::nullopt
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .debugLog = false
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .debugLog = true
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Cluster/SetThreadAffinity", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .setThreadAffinity = std::nullopt
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .setThreadAffinity = false
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .setThreadAffinity = true
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Cluster/FirmSync", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .firmSync = std::nullopt
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .firmSync = false
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .firmSync = true
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Scene", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .scene = std::nullopt
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .scene = sgct::config::Scene()
        };
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .scene = sgct::config::Scene(),
        };
        input.scene->offset = sgct::vec3(1.f, 2.f, 3.f);
        
        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .scene = sgct::config::Scene()
        };
        input.scene->orientation = sgct::quat(1.f, 2.f, 3.f, 4.f);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .scene = sgct::config::Scene()
        };
        input.scene->scale = 1.f;

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .scene = sgct::config::Scene()
        };
        input.scene = {
            .offset = sgct::vec3(1.f, 2.f, 3.f),
            .orientation = sgct::quat(1.f, 2.f, 3.f, 4.f),
            .scale = 1.f
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Node", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true,
    };
    input.nodes.push_back({
        .address = "abc",
        .port = 1
    });

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Node/DataTransferPort", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.nodes.push_back({
            .address = "abc",
            .port = 1,
            .dataTransferPort = std::nullopt
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.nodes.push_back({
            .address = "abc",
            .port = 1,
            .dataTransferPort = 0
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.nodes.push_back({
            .address = "abc",
            .port = 1,
            .dataTransferPort = 1
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Node/SwapLock", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.nodes.push_back({
            .address = "abc",
            .port = 1,
            .swapLock = std::nullopt
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.nodes.push_back({
            .address = "abc",
            .port = 1,
            .swapLock = false
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.nodes.push_back({
            .address = "abc",
            .port = 1,
            .swapLock = true
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back(sgct::config::Window());
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .id = 1
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .id = 1
        });
        node.windows.push_back({
            .id = 2
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Name", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .name = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .name = "abc"
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .name = "def"
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Tags", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .tags = std::vector<std::string>()
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .tags = { "abc" }
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .tags = { "abc", "def" }
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/BufferBitDepth", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .bufferBitDepth = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth8
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth16
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth16Float
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth32Float
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth16Int
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth32Int
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth16UInt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth32UInt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsFullScreen", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isFullScreen = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isFullScreen = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isFullScreen = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/ShouldAutoIconify", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .shouldAutoiconify = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .shouldAutoiconify = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .shouldAutoiconify = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/HideMouseCursor", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .hideMouseCursor = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .hideMouseCursor = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .hideMouseCursor = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsFloating", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isFloating = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isFloating = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isFloating = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/AlwaysRender", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .alwaysRender = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .alwaysRender = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .alwaysRender = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsHidden", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isHidden = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isHidden = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isHidden = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/DoubleBuffered", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .doubleBuffered = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .doubleBuffered = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .doubleBuffered = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/MSAA", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .msaa = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .msaa = 0
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .msaa = 1
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/UseFXAA", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .useFxaa = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .useFxaa = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .useFxaa = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsDecorated", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isDecorated = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isDecorated = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isDecorated = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsResizable", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isResizable = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isResizable = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isResizable = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Draw2D", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .draw2D = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .draw2D = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .draw2D = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Draw3D", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .draw3D = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .draw3D = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .draw3D = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsMirrored", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isMirrored = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isMirrored = false
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .isMirrored = true
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/BlitWindowId", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .blitWindowId = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .blitWindowId = 0
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .blitWindowId = 1
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Monitor", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .monitor = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .monitor = 0
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .monitor = 1
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Stereo", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::NoStereo
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::Active
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::AnaglyphRedCyan
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::AnaglyphAmberBlue
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::AnaglyphRedCyanWimmer
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::Checkerboard
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::CheckerboardInverted
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::VerticalInterlaced
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::VerticalInterlacedInverted
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::Dummy
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::SideBySide
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::SideBySideInverted
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::TopBottom
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .stereo = sgct::config::Window::StereoMode::TopBottomInverted
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Pos", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .pos = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .pos = sgct::ivec2(1, 2)
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .pos = sgct::ivec2(3, 4)
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Size", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .size = sgct::ivec2(1, 2)
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .size = sgct::ivec2(3, 4)
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Resolution", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .resolution = std::nullopt
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .resolution = sgct::ivec2(1, 2)
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        node.windows.push_back({
            .resolution = sgct::ivec2(3, 4)
        });
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back(sgct::config::Viewport());
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Viewport/User", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .user = std::nullopt
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .user = "abc"
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .user = "def"
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/OverlayTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .overlayTexture = std::nullopt
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .overlayTexture = std::filesystem::absolute("abc").string()
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .overlayTexture = std::filesystem::absolute("def").string()
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/BlendMaskTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .blendMaskTexture = std::nullopt
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .blendMaskTexture = std::filesystem::absolute("abc").string()
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .blendMaskTexture = std::filesystem::absolute("def").string()
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/BlackLevelMaskTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .blackLevelMaskTexture = std::nullopt
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .blackLevelMaskTexture = std::filesystem::absolute("abc").string()
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .blackLevelMaskTexture = std::filesystem::absolute("def").string()
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/CorrectionMeshTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .correctionMeshTexture = std::nullopt
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .correctionMeshTexture = std::filesystem::absolute("abc").string()
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .correctionMeshTexture = std::filesystem::absolute("def").string()
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/IsTracked", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .isTracked = std::nullopt
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .isTracked = false
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .isTracked = true
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/Eye", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .eye = std::nullopt
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .eye = sgct::config::Viewport::Eye::Mono
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .eye = sgct::config::Viewport::Eye::StereoLeft
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .eye = sgct::config::Viewport::Eye::StereoRight
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/Position", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .position = std::nullopt
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .position = sgct::vec2(1.f, 2.f)
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .position = sgct::vec2(3.f, 4.f)
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/Size", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .size = std::nullopt
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .size = sgct::vec2(1.f, 2.f)
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .size = sgct::vec2(3.f, 4.f)
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("NoProjection", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::NoProjection()
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("CylindricalProjection", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::CylindricalProjection()
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("CylindricalProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection {
                .quality = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection{
                .quality = 256
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection{
                .quality = 512
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection/Rotation", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection{
                .rotation = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection {
                .rotation = 1.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection {
                .rotation = 2.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection/HeightOffset", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection {
                .heightOffset = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection {
                .heightOffset = 1.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection {
                .heightOffset = 2.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection/Radi", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection {
                .radius = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection {
                .radius = 1.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::CylindricalProjection {
                .radius = 2.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("EquirectangularProjection", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::EquirectangularProjection()
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("EquirectangularProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::EquirectangularProjection {
                .quality = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::EquirectangularProjection {
                .quality = 256
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::EquirectangularProjection {
                .quality = 512
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::FisheyeProjection()
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("FisheyeProjection/FOV", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .fov = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .fov = 1.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .fov = 2.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .quality = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .quality = 256
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .quality = 512
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Interpolation", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .interpolation = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .interpolation = sgct::config::FisheyeProjection::Interpolation::Linear
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .interpolation = sgct::config::FisheyeProjection::Interpolation::Cubic
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Tilt", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .tilt = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .tilt = 1.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .tilt = 2.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Diameter", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .diameter = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .diameter = 1.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .diameter = 2.f
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Crop", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .crop = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .crop = sgct::config::FisheyeProjection::Crop{ 1.f, 2.f, 3.f, 4.f }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .crop = sgct::config::FisheyeProjection::Crop{ 5.f, 6.f, 7.f, 8.f }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/KeepAspectRatio", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .keepAspectRatio = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .keepAspectRatio = false
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .keepAspectRatio = true
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Offset", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .offset = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .offset = sgct::vec3(1.f, 2.f, 3.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .offset = sgct::vec3(4.f, 5.f, 6.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Background", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .background = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .background = sgct::vec4(1.f, 2.f, 3.f, 4.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::FisheyeProjection {
                .background = sgct::vec4(5.f, 6.f, 7.f, 8.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("PlanarProjection", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };

    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };

    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::PlanarProjection()
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("PlanarProjection/FOV", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::PlanarProjection {
            .fov = sgct::config::PlanarProjection::FOV { 1.f, 2.f, 3.f, 4.f }
        }
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("PlanarProjection/Orientation", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::PlanarProjection {
                .orientation = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::PlanarProjection {
                .orientation = sgct::quat(1.f, 2.f, 3.f, 4.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::PlanarProjection {
                .orientation = sgct::quat(5.f, 6.f, 7.f, 8.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("PlanarProjection/Offset", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::PlanarProjection {
                .offset = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::PlanarProjection {
                .offset = sgct::vec3(1.f, 2.f, 3.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::PlanarProjection {
                .offset = sgct::vec3(4.f, 5.f, 6.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("ProjectionPlane", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::ProjectionPlane()
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("ProjectionPlane/LowerLeft", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::ProjectionPlane {
            .lowerLeft = sgct::vec3(1.f, 2.f, 3.f)
        }
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("ProjectionPlane/UpperLeft", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::ProjectionPlane {
            .upperLeft = sgct::vec3(1.f, 2.f, 3.f)
        }
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("ProjectionPlane/UpperRight", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::ProjectionPlane {
            .upperRight = sgct::vec3(1.f, 2.f, 3.f)
        }
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("SphericalMirrorProjection", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::SphericalMirrorProjection {
            .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
        }
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("SphericalMirrorProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SphericalMirrorProjection {
                .quality = std::nullopt,
                .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SphericalMirrorProjection {
                .quality = 256,
                .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SphericalMirrorProjection {
                .quality = 512,
                .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SphericalMirrorProjection/Tilt", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SphericalMirrorProjection {
                .tilt = std::nullopt,
                .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SphericalMirrorProjection {
                .tilt = 1.f,
                .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SphericalMirrorProjection {
                .tilt = 2.f,
                .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SphericalMirrorProjection/Background", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SphericalMirrorProjection {
                .background = std::nullopt,
                .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SphericalMirrorProjection {
                .background = sgct::vec4(1.f, 2.f, 3.f, 4.f),
                .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SphericalMirrorProjection {
                .background = sgct::vec4(5.f, 6.f, 7.f, 8.f),
                .mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::SpoutOutputProjection()
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("SpoutOutputProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .quality = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .quality = 256
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .quality = 512
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection/Mapping", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .mapping = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .mapping = sgct::config::SpoutOutputProjection::Mapping::Fisheye
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .mapping = sgct::config::SpoutOutputProjection::Mapping::Equirectangular
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .mapping = sgct::config::SpoutOutputProjection::Mapping::Cubemap
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection/MappingSpoutName", "[roundtrip]") {
    sgct::config::Cluster input = {
        .success = true
    };
    sgct::config::Node node = {
        .address = "abc",
        .port = 1
    };
    sgct::config::Window window;
    window.viewports.push_back({
        .projection = sgct::config::SpoutOutputProjection {
            .mappingSpoutName = "abc"
        }
    });
    node.windows.push_back(window);
    input.nodes.push_back(node);

    const std::string str = sgct::serializeConfig(input);
    const sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("SpoutOutputProjection/Background", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .background = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .background = sgct::vec4(1.f, 2.f, 3.f, 4.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .background = sgct::vec4(5.f, 6.f, 7.f, 8.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection/Channels", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = false,
                    .top = false,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = false,
                    .top = false,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = false,
                    .top = false,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = false,
                    .top = false,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = false,
                    .top = true,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = false,
                    .top = true,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = false,
                    .top = true,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = false,
                    .top = true,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = true,
                    .top = false,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = true,
                    .top = false,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = true,
                    .top = false,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = true,
                    .top = false,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = true,
                    .top = true,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = true,
                    .top = true,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = true,
                    .top = true,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = true,
                    .top = true,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = false,
                    .top = false,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = false,
                    .top = false,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = false,
                    .top = false,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = false,
                    .top = false,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = false,
                    .top = true,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = false,
                    .top = true,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = false,
                    .top = true,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = false,
                    .top = true,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = true,
                    .top = false,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = true,
                    .top = false,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = true,
                    .top = false,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = true,
                    .top = false,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = true,
                    .top = true,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = true,
                    .top = true,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = true,
                    .top = true,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = true,
                    .bottom = true,
                    .top = true,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = false,
                    .top = false,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = false,
                    .top = false,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = false,
                    .zLeft = false,
                    .bottom = false,
                    .top = false,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = false,
                    .top = false,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = false,
                    .top = true,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = false,
                    .top = true,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = false,
                    .top = true,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = false,
                    .top = true,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = true,
                    .top = false,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = true,
                    .top = false,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = true,
                    .top = false,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = true,
                    .top = false,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = true,
                    .top = true,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = true,
                    .top = true,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = true,
                    .top = true,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = false,
                    .bottom = true,
                    .top = true,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = false,
                    .top = false,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = false,
                    .top = false,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = false,
                    .top = false,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = false,
                    .top = false,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = false,
                    .top = true,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = false,
                    .top = true,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = false,
                    .top = true,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = false,
                    .top = true,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = true,
                    .top = false,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = true,
                    .top = false,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = true,
                    .top = false,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = true,
                    .top = false,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = true,
                    .top = true,
                    .left = false,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = true,
                    .top = true,
                    .left = false,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = true,
                    .top = true,
                    .left = true,
                    .zRight = false
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .channels = sgct::config::SpoutOutputProjection::Channels {
                    .right = true,
                    .zLeft = true,
                    .bottom = true,
                    .top = true,
                    .left = true,
                    .zRight = true
                }
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection/Orientation", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .orientation = std::nullopt
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .orientation = sgct::vec3(1.f, 2.f, 3.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Node node = {
            .address = "abc",
            .port = 1
        };
        sgct::config::Window window;
        window.viewports.push_back({
            .projection = sgct::config::SpoutOutputProjection {
                .orientation = sgct::vec3(4.f, 5.f, 6.f)
            }
        });
        node.windows.push_back(window);
        input.nodes.push_back(node);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.emplace_back(sgct::config::User());

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.emplace_back(sgct::config::User());
        input.users.emplace_back(sgct::config::User());

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.emplace_back(sgct::config::User());
        input.users.emplace_back(sgct::config::User());
        input.users.emplace_back(sgct::config::User());

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Name", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .name = std::nullopt
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .name = "abc"
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .name = "def"
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/EyeSeparation", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .eyeSeparation = std::nullopt
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .eyeSeparation = 1.f
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .eyeSeparation = 2.f
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Position", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .position = std::nullopt
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .position = sgct::vec3(1.f, 2.f, 3.f)
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .position = sgct::vec3(4.f, 5.f, 6.f)
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Transformation", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .transformation = std::nullopt
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::User user;
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
        input.users.push_back(user);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
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
        input.users.push_back(user);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Tracking", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .tracking = std::nullopt
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .tracking = sgct::config::User::Tracking { .tracker = "abc", .device = "def" }
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.users.push_back({
            .tracking = sgct::config::User::Tracking {.tracker = "ghi", .device = "jkl" }
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = std::nullopt
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture()
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture/Path", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .path = std::nullopt
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .path = "abc"
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .path = "def"
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture/Format", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .format = std::nullopt
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .format = sgct::config::Capture::Format::PNG
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .format = sgct::config::Capture::Format::JPG
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .format = sgct::config::Capture::Format::TGA
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture/ScreenShotRange", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .range = std::nullopt
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .range = sgct::config::Capture::ScreenShotRange { 1, 2 }
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .capture = sgct::config::Capture {
                .range = sgct::config::Capture::ScreenShotRange { 3, 4 }
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.trackers.emplace_back(sgct::config::Tracker());

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.trackers.emplace_back(sgct::config::Tracker());
        input.trackers.emplace_back(sgct::config::Tracker());

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.trackers.emplace_back(sgct::config::Tracker());
        input.trackers.emplace_back(sgct::config::Tracker());
        input.trackers.emplace_back(sgct::config::Tracker());

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Name", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.trackers.push_back({
            .name = "abc"
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.trackers.push_back({
            .name = "def"
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        tracker.devices.emplace_back(sgct::config::Device());
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        tracker.devices.emplace_back(sgct::config::Device());
        tracker.devices.emplace_back(sgct::config::Device());
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        tracker.devices.emplace_back(sgct::config::Device());
        tracker.devices.emplace_back(sgct::config::Device());
        tracker.devices.emplace_back(sgct::config::Device());
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Name", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        tracker.devices.push_back({
            .name = "abc"
        });
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        tracker.devices.push_back({
            .name = "def"
        });
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Sensors", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.sensors.push_back({ "abc", 1 });
        tracker.devices.push_back({});
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.sensors.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.sensors.push_back({ "abc", 1 });
        device.sensors.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Buttons", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.buttons.push_back({ "abc", 1 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.buttons.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.buttons.push_back({ "abc", 1 });
        device.buttons.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Axes", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.axes.push_back({ "abc", 1 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.axes.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.axes.push_back({ "abc", 1 });
        device.axes.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Offset", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        tracker.devices.push_back({
            .offset = std::nullopt
        });
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        tracker.devices.push_back({
            .offset = sgct::vec3 { 1.f, 2.f, 3.f }
        });
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };

        sgct::config::Tracker tracker;
        tracker.devices.push_back({
            .offset = sgct::vec3 { 4.f, 5.f, 6.f }
        });
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Transformation", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        tracker.devices.push_back({
            .offset = std::nullopt
        });
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
        sgct::config::Device device;
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
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        sgct::config::Tracker tracker;
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
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Offset", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.trackers.push_back({
            .offset = std::nullopt
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.trackers.push_back({
            .offset = sgct::vec3 { 1.f, 2.f, 3.f }
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.trackers.push_back({
            .offset = sgct::vec3 { 4.f, 5.f, 6.f }
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Transformation", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true
        };
        input.trackers.push_back({
            .transformation = std::nullopt
        });

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
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
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true
        };
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
        input.trackers.push_back(tracker);

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = std::nullopt
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings()
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/UseDepthTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .useDepthTexture = std::nullopt
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .useDepthTexture = false
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .useDepthTexture = true
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/UseNormalTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .useNormalTexture = std::nullopt
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .useNormalTexture = false
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .useNormalTexture = true
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/UsePositionTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .usePositionTexture = std::nullopt
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .usePositionTexture = false
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .usePositionTexture = true
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/BufferFloatPrecision", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .bufferFloatPrecision = std::nullopt
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .bufferFloatPrecision =
                    sgct::config::Settings::BufferFloatPrecision::Float16Bit
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .bufferFloatPrecision =
                    sgct::config::Settings::BufferFloatPrecision::Float32Bit
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/Display", "[roundtrip]") {
    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .display = std::nullopt
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .display = sgct::config::Settings::Display()
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .display = sgct::config::Settings::Display {
                    .swapInterval = std::nullopt
                }
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .display = sgct::config::Settings::Display {
                    .swapInterval = 1
                }
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .display = sgct::config::Settings::Display {
                    .swapInterval = 2
                }
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .display = sgct::config::Settings::Display {
                    .refreshRate = std::nullopt
                }
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .display = sgct::config::Settings::Display {
                    .refreshRate = 1
                }
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .display = sgct::config::Settings::Display {
                    .refreshRate = 2
                }
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }


    {
        sgct::config::Cluster input = {
            .success = true,
            .settings = sgct::config::Settings {
                .display = sgct::config::Settings::Display {
                    .swapInterval = 1,
                    .refreshRate = 2
                }
            }
        };

        const std::string str = sgct::serializeConfig(input);
        const sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}
