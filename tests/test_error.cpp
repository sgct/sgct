/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <sgct/error.h>
#include <sgct/format.h>
#include <string>
#include <utility>

using namespace sgct;

TEST_CASE("Error: Message format", "[error]") {
    using C = Error::Component;
    const auto [component, name] = GENERATE(
        std::pair(C::Config, "Config"),
        std::pair(C::CorrectionMesh, "CorrectionMesh"),
        std::pair(C::DomeProjection, "DomeProjection"),
        std::pair(C::Engine, "Engine"),
        std::pair(C::Image, "Image"),
        std::pair(C::Network, "Network"),
        std::pair(C::OBJ, "OBJ"),
        std::pair(C::PaulBourke, "PaulBourke"),
        std::pair(C::Pfm, "Pfm"),
        std::pair(C::ReadConfig, "ReadConfig"),
        std::pair(C::Scalable, "Scalable"),
        std::pair(C::SCISS, "SCISS"),
        std::pair(C::Shader, "Shader"),
        std::pair(C::SimCAD, "SimCAD"),
        std::pair(C::SkySkan, "SkySkan"),
        std::pair(C::Window, "Window")
    );
    CAPTURE(name);

    const Error e = Error(component, 1234, "message");
    CHECK(e.component == component);
    CHECK(e.code == 1234);
    CHECK(e.message == "message");
    CHECK(std::string(e.what()) == std::format("[{}] (1234): message", name));
}
