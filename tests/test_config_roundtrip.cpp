/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include "equality.h"
#include <sgct/readconfig.h>
#include <nlohmann/json.hpp>
#include <filesystem>

TEST_CASE("Default constructed", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;
    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Cluster/DebugLog", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;
        input.debugLog = std::nullopt;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.debugLog = false;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.debugLog = true;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Cluster/SetThreadAffinity", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;
        input.setThreadAffinity = std::nullopt;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.setThreadAffinity = false;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.setThreadAffinity = true;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Cluster/FirmSync", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;
        input.firmSync = std::nullopt;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.firmSync = false;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.firmSync = true;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Scene", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;
        input.scene = std::nullopt;
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.scene = sgct::config::Scene();
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.scene = sgct::config::Scene();
        input.scene->offset = sgct::vec3(1.f, 2.f, 3.f);
        
        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.scene = sgct::config::Scene();
        input.scene->orientation = sgct::quat(1.f, 2.f, 3.f, 4.f);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.scene = sgct::config::Scene();
        input.scene->scale = 1.f;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;
        input.scene = sgct::config::Scene();
        input.scene->offset = sgct::vec3(1.f, 2.f, 3.f);
        input.scene->orientation = sgct::quat(1.f, 2.f, 3.f, 4.f);
        input.scene->scale = 1.f;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Node", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Node/DataTransferPort", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;
        node.dataTransferPort = std::nullopt;
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;
        node.dataTransferPort = 0;
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;
        node.dataTransferPort = 1;
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Node/SwapLock", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;
        node.swapLock = std::nullopt;
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;
        node.swapLock = false;
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;
        node.swapLock = true;
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;
        
        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        node.windows.push_back(sgct::config::Window());
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.id = 1;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window1;
        window1.id = 1;
        node.windows.push_back(window1);

        sgct::config::Window window2;
        window2.id = 2;
        node.windows.push_back(window2);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Name", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.name = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.name = "abc";
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.name = "def";
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Tags", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.tags = std::vector<std::string>();
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.tags.push_back("abc");
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.tags.push_back("abc");
        window.tags.push_back("def");
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/BufferBitDepth", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.bufferBitDepth = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth8;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth16;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth16Float;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth32Float;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth16Int;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth32Int;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth16UInt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.bufferBitDepth = sgct::config::Window::ColorBitDepth::Depth32UInt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsFullScreen", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isFullScreen = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isFullScreen = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isFullScreen = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/ShouldAutoIconify", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.shouldAutoiconify = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.shouldAutoiconify = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.shouldAutoiconify = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/HideMouseCursor", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.hideMouseCursor = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.hideMouseCursor = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.hideMouseCursor = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsFloating", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isFloating = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isFloating = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isFloating = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/AlwaysRender", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.alwaysRender = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.alwaysRender = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.alwaysRender = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsHidden", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isHidden = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isHidden = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isHidden = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/DoubleBuffered", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.doubleBuffered = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.doubleBuffered = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.doubleBuffered = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/MSAA", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.msaa = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.msaa = 0;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.msaa = 1;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/UseFXAA", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.useFxaa = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.useFxaa = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.useFxaa = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsDecorated", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isDecorated = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isDecorated = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isDecorated = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsResizable", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isResizable = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isResizable = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isResizable = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Draw2D", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.draw2D = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.draw2D = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.draw2D = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Draw3D", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.draw3D = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.draw3D = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.draw3D = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/IsMirrored", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isMirrored = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isMirrored = false;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.isMirrored = true;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/BlitWindowId", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.blitWindowId = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.blitWindowId = 0;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.blitWindowId = 1;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Monitor", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.monitor = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.monitor = 0;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.monitor = 1;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/MPCDI", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.mpcdi = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.mpcdi = std::filesystem::absolute("abc").string();
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.mpcdi = std::filesystem::absolute("def").string();
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Stereo", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::NoStereo;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::Active;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::AnaglyphRedCyan;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::AnaglyphAmberBlue;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::AnaglyphRedCyanWimmer;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::Checkerboard;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::CheckerboardInverted;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::VerticalInterlaced;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::VerticalInterlacedInverted;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::Dummy;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::SideBySide;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::SideBySideInverted;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::TopBottom;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.stereo = sgct::config::Window::StereoMode::TopBottomInverted;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Pos", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.pos = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.pos = sgct::ivec2(1, 2);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.pos = sgct::ivec2(3, 4);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Size", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.size = sgct::ivec2(1, 2);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.size = sgct::ivec2(3, 4);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Window/Resolution", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.resolution = std::nullopt;
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.resolution = sgct::ivec2(1, 2);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;
        window.resolution = sgct::ivec2(3, 4);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;
    window.viewports.push_back(sgct::config::Viewport());
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("Viewport/User", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.user = std::nullopt;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.user = "abc";
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.user = "def";
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/OverlayTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.overlayTexture = std::nullopt;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.overlayTexture = std::filesystem::absolute("abc").string();
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.overlayTexture = std::filesystem::absolute("def").string();
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/BlendMaskTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.blendMaskTexture = std::nullopt;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.blendMaskTexture = std::filesystem::absolute("abc").string();
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.blendMaskTexture = std::filesystem::absolute("def").string();
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/BlackLevelMaskTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.blackLevelMaskTexture = std::nullopt;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.blackLevelMaskTexture = std::filesystem::absolute("abc").string();
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.blackLevelMaskTexture = std::filesystem::absolute("def").string();
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/CorrectionMeshTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.correctionMeshTexture = std::nullopt;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.correctionMeshTexture = std::filesystem::absolute("abc").string();
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.correctionMeshTexture = std::filesystem::absolute("def").string();
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/IsTracked", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.isTracked = std::nullopt;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.isTracked = false;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.isTracked = true;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/Eye", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.eye = std::nullopt;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.eye = sgct::config::Viewport::Eye::Mono;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.eye = sgct::config::Viewport::Eye::StereoLeft;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.eye = sgct::config::Viewport::Eye::StereoRight;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/Position", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.position = std::nullopt;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.position = sgct::vec2(1.f, 2.f);
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.position = sgct::vec2(3.f, 4.f);
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Viewport/Size", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.size = std::nullopt;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.size = sgct::vec2(1.f, 2.f);
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        viewport.size = sgct::vec2(3.f, 4.f);
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("NoProjection", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::NoProjection projection;
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("CylindricalProjection", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::CylindricalProjection projection;
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("CylindricalProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.quality = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.quality = 256;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.quality = 512;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection/Rotation", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.rotation = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.rotation = 1.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.rotation = 2.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection/HeightOffset", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.heightOffset = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.heightOffset = 1.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.heightOffset = 2.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("CylindricalProjection/Radi", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.radius = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.radius = 1.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::CylindricalProjection projection;
        projection.radius = 2.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("EquirectangularProjection", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::EquirectangularProjection projection;
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("EquirectangularProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::EquirectangularProjection projection;
        projection.quality = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::EquirectangularProjection projection;
        projection.quality = 256;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::EquirectangularProjection projection;
        projection.quality = 512;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::FisheyeProjection projection;
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("FisheyeProjection/FOV", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.fov = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.fov = 1.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.fov = 2.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.quality = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.quality = 256;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.quality = 512;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Interpolation", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.interpolation = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.interpolation = sgct::config::FisheyeProjection::Interpolation::Linear;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.interpolation = sgct::config::FisheyeProjection::Interpolation::Cubic;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Tilt", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.tilt = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.tilt = 1.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.tilt = 2.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Diameter", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.diameter = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.diameter = 1.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.diameter = 2.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Crop", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.crop = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.crop = sgct::config::FisheyeProjection::Crop{ 1.f, 2.f, 3.f, 4.f };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.crop = sgct::config::FisheyeProjection::Crop{ 5.f, 6.f, 7.f, 8.f };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/KeepAspectRatio", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.keepAspectRatio = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.keepAspectRatio = false;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.keepAspectRatio = true;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Offset", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.offset = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.offset = sgct::vec3(1.f, 2.f, 3.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.offset = sgct::vec3(4.f, 5.f, 6.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("FisheyeProjection/Background", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.background = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.background = sgct::vec4(1.f, 2.f, 3.f, 4.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::FisheyeProjection projection;
        projection.background = sgct::vec4(5.f, 6.f, 7.f, 8.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("PlanarProjection", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::PlanarProjection projection;
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("PlanarProjection/FOV", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::PlanarProjection projection;
    projection.fov = sgct::config::PlanarProjection::FOV{ 1.f, 2.f, 3.f, 4.f };
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("PlanarProjection/Orientation", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::PlanarProjection projection;
        projection.orientation = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::PlanarProjection projection;
        projection.orientation = sgct::quat(1.f, 2.f, 3.f, 4.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::PlanarProjection projection;
        projection.orientation = sgct::quat(5.f, 6.f, 7.f, 8.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("PlanarProjection/Offset", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::PlanarProjection projection;
        projection.offset = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::PlanarProjection projection;
        projection.offset = sgct::vec3(1.f, 2.f, 3.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::PlanarProjection projection;
        projection.offset = sgct::vec3(4.f, 5.f, 6.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("ProjectionPlane", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::ProjectionPlane projection;
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("ProjectionPlane/LowerLeft", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::ProjectionPlane projection;
    projection.lowerLeft = sgct::vec3(1.f, 2.f, 3.f);
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("ProjectionPlane/UpperLeft", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::ProjectionPlane projection;
    projection.upperLeft = sgct::vec3(1.f, 2.f, 3.f);
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("ProjectionPlane/UpperRight", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::ProjectionPlane projection;
    projection.upperRight = sgct::vec3(1.f, 2.f, 3.f);
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("SphericalMirrorProjection", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::SphericalMirrorProjection projection;
    projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("SphericalMirrorProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SphericalMirrorProjection projection;
        projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
        projection.quality = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SphericalMirrorProjection projection;
        projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
        projection.quality = 256;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SphericalMirrorProjection projection;
        projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
        projection.quality = 512;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SphericalMirrorProjection/Tilt", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SphericalMirrorProjection projection;
        projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
        projection.tilt = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SphericalMirrorProjection projection;
        projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
        projection.tilt = 1.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SphericalMirrorProjection projection;
        projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
        projection.tilt = 2.f;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SphericalMirrorProjection/Background", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SphericalMirrorProjection projection;
        projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
        projection.background = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SphericalMirrorProjection projection;
        projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
        projection.background = sgct::vec4(1.f, 2.f, 3.f, 4.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SphericalMirrorProjection projection;
        projection.mesh = { .bottom = "abc", .left = "def", .right = "ghi", .top = "jkl" };
        projection.background = sgct::vec4(5.f, 6.f, 7.f, 8.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::SpoutOutputProjection projection;
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("SpoutOutputProjection/Quality", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.quality = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.quality = 256;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.quality = 512;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection/Mapping", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.mapping = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.mapping = sgct::config::SpoutOutputProjection::Mapping::Fisheye;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.mapping =
            sgct::config::SpoutOutputProjection::Mapping::Equirectangular;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.mapping = sgct::config::SpoutOutputProjection::Mapping::Cubemap;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection/MappingSpoutName", "[roundtrip]") {
    sgct::config::Cluster input;
    input.success = true;

    sgct::config::Node node;
    node.address = "abc";
    node.port = 1;

    sgct::config::Window window;

    sgct::config::Viewport viewport;
    sgct::config::SpoutOutputProjection projection;
    projection.mappingSpoutName = "abc";
    viewport.projection = projection;
    window.viewports.push_back(viewport);
    node.windows.push_back(window);
    input.nodes.push_back(node);

    std::string str = sgct::serializeConfig(input);
    sgct::config::Cluster output = sgct::readJsonConfig(str);
    REQUIRE(input == output);
}

TEST_CASE("SpoutOutputProjection/Background", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.background = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.background = sgct::vec4(1.f, 2.f, 3.f, 4.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.background = sgct::vec4(5.f, 6.f, 7.f, 8.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection/Channels", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = false,
            .top = false,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = false,
            .top = false,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = false,
            .top = false,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = false,
            .top = false,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = false,
            .top = true,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = false,
            .top = true,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = false,
            .top = true,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = false,
            .top = true,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = true,
            .top = false,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = true,
            .top = false,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = true,
            .top = false,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = true,
            .top = false,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = true,
            .top = true,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = true,
            .top = true,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = true,
            .top = true,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = true,
            .top = true,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = false,
            .top = false,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = false,
            .top = false,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = false,
            .top = false,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = false,
            .top = false,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = false,
            .top = true,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = false,
            .top = true,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = false,
            .top = true,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = false,
            .top = true,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = true,
            .top = false,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = true,
            .top = false,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = true,
            .top = false,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = true,
            .top = false,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = true,
            .top = true,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = true,
            .top = true,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = true,
            .top = true,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = true,
            .bottom = true,
            .top = true,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = false,
            .top = false,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = false,
            .top = false,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = false,
            .zLeft = false,
            .bottom = false,
            .top = false,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = false,
            .top = false,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = false,
            .top = true,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = false,
            .top = true,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = false,
            .top = true,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = false,
            .top = true,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = true,
            .top = false,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = true,
            .top = false,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = true,
            .top = false,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = true,
            .top = false,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = true,
            .top = true,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = true,
            .top = true,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = true,
            .top = true,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = false,
            .bottom = true,
            .top = true,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = false,
            .top = false,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = false,
            .top = false,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = false,
            .top = false,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = false,
            .top = false,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = false,
            .top = true,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = false,
            .top = true,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = false,
            .top = true,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = false,
            .top = true,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = true,
            .top = false,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = true,
            .top = false,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = true,
            .top = false,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = true,
            .top = false,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = true,
            .top = true,
            .left = false,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = true,
            .top = true,
            .left = false,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = true,
            .top = true,
            .left = true,
            .zRight = false
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.channels = {
            .right = true,
            .zLeft = true,
            .bottom = true,
            .top = true,
            .left = true,
            .zRight = true
        };
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("SpoutOutputProjection/Orientation", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.orientation = std::nullopt;
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.orientation = sgct::vec3(1.f, 2.f, 3.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Node node;
        node.address = "abc";
        node.port = 1;

        sgct::config::Window window;

        sgct::config::Viewport viewport;
        sgct::config::SpoutOutputProjection projection;
        projection.orientation = sgct::vec3(4.f, 5.f, 6.f);
        viewport.projection = projection;
        window.viewports.push_back(viewport);
        node.windows.push_back(window);
        input.nodes.push_back(node);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.users.push_back(sgct::config::User());

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.users.push_back(sgct::config::User());
        input.users.push_back(sgct::config::User());

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.users.push_back(sgct::config::User());
        input.users.push_back(sgct::config::User());
        input.users.push_back(sgct::config::User());

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Name", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.name = std::nullopt;
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.name = "abc";
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.name = "def";
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/EyeSeparation", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.eyeSeparation = std::nullopt;
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.eyeSeparation = 1.f;
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.eyeSeparation = 2.f;
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Position", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.position = std::nullopt;
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.position = sgct::vec3(1.f, 2.f, 3.f);
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.position = sgct::vec3(4.f, 5.f, 6.f);
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Transformation", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.transformation = std::nullopt;
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

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

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

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

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("User/Tracking", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.tracking = std::nullopt;
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.tracking = { .tracker = "abc", .device = "def" };
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::User user;
        user.tracking = { .tracker = "ghi", .device = "jkl" };
        input.users.push_back(user);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture/Path", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->path = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->path = "abc";

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->path = "def";

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture/Format", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->format = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->format = sgct::config::Capture::Format::PNG;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->format = sgct::config::Capture::Format::JPG;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->format = sgct::config::Capture::Format::TGA;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Capture/ScreenShotRange", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->range = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->range = { 1, 2 };

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.capture = sgct::config::Capture();
        input.capture->range = { 3, 4 };

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.trackers.push_back(sgct::config::Tracker());

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.trackers.push_back(sgct::config::Tracker());
        input.trackers.push_back(sgct::config::Tracker());

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.trackers.push_back(sgct::config::Tracker());
        input.trackers.push_back(sgct::config::Tracker());
        input.trackers.push_back(sgct::config::Tracker());

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Name", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        tracker.name = "abc";
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        tracker.name = "def";
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        tracker.devices.push_back(sgct::config::Device());
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        tracker.devices.push_back(sgct::config::Device());
        tracker.devices.push_back(sgct::config::Device());
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        tracker.devices.push_back(sgct::config::Device());
        tracker.devices.push_back(sgct::config::Device());
        tracker.devices.push_back(sgct::config::Device());
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Name", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.name = "abc";
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.name = "def";
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Sensors", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.sensors.push_back({ "abc", 1 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.sensors.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.sensors.push_back({ "abc", 1 });
        device.sensors.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Buttons", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.buttons.push_back({ "abc", 1 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.buttons.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.buttons.push_back({ "abc", 1 });
        device.buttons.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Axes", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.axes.push_back({ "abc", 1 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.axes.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.axes.push_back({ "abc", 1 });
        device.axes.push_back({ "def", 2 });
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Offset", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.offset = std::nullopt;
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.offset = { 1.f, 2.f, 3.f };
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.offset = { 4.f, 5.f, 6.f };
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Device/Transformation", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        sgct::config::Device device;
        device.offset = std::nullopt;
        tracker.devices.push_back(device);
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

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

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

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

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Offset", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        tracker.offset = std::nullopt;
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        tracker.offset = { 1.f, 2.f, 3.f };
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        tracker.offset = { 4.f, 5.f, 6.f };
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Tracker/Transformation", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        sgct::config::Tracker tracker;
        tracker.transformation = std::nullopt;
        input.trackers.push_back(tracker);

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

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

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

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

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/UseDepthTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->useDepthTexture = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->useDepthTexture = false;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->useDepthTexture = true;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/UseNormalTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->useNormalTexture = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->useNormalTexture = false;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->useNormalTexture = true;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/UsePositionTexture", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->usePositionTexture = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->usePositionTexture = false;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->usePositionTexture = true;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/BufferFloatPrecision", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->bufferFloatPrecision = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->bufferFloatPrecision =
            sgct::config::Settings::BufferFloatPrecision::Float16Bit;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->bufferFloatPrecision =
            sgct::config::Settings::BufferFloatPrecision::Float32Bit;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}

TEST_CASE("Settings/Display", "[roundtrip]") {
    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->display = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->display = sgct::config::Settings::Display();

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->display = sgct::config::Settings::Display();
        input.settings->display->swapInterval = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->display = sgct::config::Settings::Display();
        input.settings->display->swapInterval = 1;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->display = sgct::config::Settings::Display();
        input.settings->display->swapInterval = 2;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->display = sgct::config::Settings::Display();
        input.settings->display->refreshRate = std::nullopt;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->display = sgct::config::Settings::Display();
        input.settings->display->refreshRate = 1;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }

    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->display = sgct::config::Settings::Display();
        input.settings->display->refreshRate = 2;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }


    {
        sgct::config::Cluster input;
        input.success = true;

        input.settings = sgct::config::Settings();
        input.settings->display = sgct::config::Settings::Display();
        input.settings->display->swapInterval = 1;
        input.settings->display->refreshRate = 2;

        std::string str = sgct::serializeConfig(input);
        sgct::config::Cluster output = sgct::readJsonConfig(str);
        REQUIRE(input == output);
    }
}
