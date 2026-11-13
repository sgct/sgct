/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testhelpers.h"
#include <sgct/correction/buffer.h>
#include <sgct/correction/simcad.h>
#include <sgct/format.h>
#include <sgct/opengl.h>
#include <string>
#include <string_view>
#include <vector>

using namespace sgct;
using namespace sgct::correction;
using Catch::Matchers::WithinAbs;

namespace {
    constexpr Error::Component Comp = Error::Component::SimCAD;

    std::string makeSimCAD(std::string_view x, std::string_view y) {
        return std::format(
            R"(<?xml version="1.0" encoding="utf-8"?>
<GeometryFile>
  <GeometryDefinition>
    <X-FlatParameters range="2.0">{}</X-FlatParameters>
    <Y-FlatParameters range="2.0">{}</Y-FlatParameters>
  </GeometryDefinition>
</GeometryFile>
)",
            x, y
        );
    }

    void checkSimCADError(std::string_view content, int code) {
        const test::TempFile file = test::TempFile("mesh.simcad", content);
        test::checkError(
            [&]() { generateSimCADMesh(file.path(), { 0, 0 }, { 1, 1 }); },
            Comp, code
        );
    }
} // namespace

TEST_CASE("SimCAD: Missing file", "[correction][simcad]") {
    test::checkError(
        []() { generateSimCADMesh("does-not-exist.simcad", { 0, 0 }, { 1, 1 }); },
        Comp, 2080
    );
}

TEST_CASE("SimCAD: Invalid XML", "[correction][simcad]") {
    checkSimCADError("<GeometryFile><GeometryDefinition>", 2080);
}

TEST_CASE("SimCAD: Missing elements", "[correction][simcad]") {
    checkSimCADError("<Geometry></Geometry>", 2081);
    checkSimCADError("<GeometryFile><Definition/></GeometryFile>", 2082);
}

TEST_CASE("SimCAD: Different number of values", "[correction][simcad]") {
    checkSimCADError(makeSimCAD("0 0 0 0", "0 0 0"), 2083);
}

TEST_CASE("SimCAD: Invalid matrix size", "[correction][simcad]") {
    SECTION("Not square") {
        checkSimCADError(makeSimCAD("0 0 0", "0 0 0"), 2084);
    }

    SECTION("Single value") {
        checkSimCADError(makeSimCAD("0", "0"), 2084);
    }

    SECTION("Empty") {
        checkSimCADError(makeSimCAD("", ""), 2084);
    }

    SECTION("Missing parameters") {
        checkSimCADError(
            "<GeometryFile><GeometryDefinition/></GeometryFile>",
            2084
        );
    }
}

TEST_CASE("SimCAD: Invalid value", "[correction][simcad]") {
    checkSimCADError(makeSimCAD("0 abc 0 0", "0 0 0 0"), 2085);
}

TEST_CASE("SimCAD: Without correction", "[correction][simcad]") {
    // Values separated by multiple spaces and newlines are accepted
    const test::TempFile file = test::TempFile(
        "mesh.simcad",
        makeSimCAD("0 0\n0  0", "0 0 0 0")
    );
    const Buffer buf = generateSimCADMesh(file.path(), { 0, 0 }, { 1, 1 });

    REQUIRE(buf.vertices.size() == 4);
    CHECK(buf.geometryType == GL_TRIANGLE_STRIP);
    CHECK(buf.indices == std::vector<unsigned int>{ 0, 2, 1, 3 });

    // The first row is at the top of the viewport
    const float ExpectedX[] = { -1.f, 1.f, -1.f, 1.f };
    const float ExpectedY[] = { 1.f, 1.f, -1.f, -1.f };
    const float ExpectedS[] = { 0.f, 1.f, 0.f, 1.f };
    const float ExpectedT[] = { 1.f, 1.f, 0.f, 0.f };
    for (size_t i = 0; i < buf.vertices.size(); i++) {
        CAPTURE(i);
        const Buffer::Vertex& v = buf.vertices[i];
        CHECK_THAT(v.x, WithinAbs(ExpectedX[i], 1e-6));
        CHECK_THAT(v.y, WithinAbs(ExpectedY[i], 1e-6));
        CHECK_THAT(v.s, WithinAbs(ExpectedS[i], 1e-6));
        CHECK_THAT(v.t, WithinAbs(ExpectedT[i], 1e-6));
        CHECK(v.r == 1.f);
        CHECK(v.a == 1.f);
    }
}

TEST_CASE("SimCAD: Triangle strip", "[correction][simcad]") {
    SECTION("3x3") {
        // The strip goes from left to right in even rows and from right to left in odd
        // rows, so that it covers all cells
        const test::TempFile file = test::TempFile(
            "mesh.simcad",
            makeSimCAD("0 0 0 0 0 0 0 0 0", "0 0 0 0 0 0 0 0 0")
        );
        const Buffer buf = generateSimCADMesh(file.path(), { 0, 0 }, { 1, 1 });
        REQUIRE(buf.vertices.size() == 9);
        CHECK(buf.geometryType == GL_TRIANGLE_STRIP);
        CHECK(buf.indices == std::vector<unsigned int>{
            0, 3, 1, 4, 2, 5,
            8, 4, 7, 3, 6
        });
    }

    SECTION("4x4") {
        const test::TempFile file = test::TempFile(
            "mesh.simcad",
            makeSimCAD(
                "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
                "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"
            )
        );
        const Buffer buf = generateSimCADMesh(file.path(), { 0, 0 }, { 1, 1 });
        REQUIRE(buf.vertices.size() == 16);
        CHECK(buf.indices == std::vector<unsigned int>{
            0, 4, 1, 5, 2, 6, 3, 7,
            11, 6, 10, 5, 9, 4, 8,
            8, 12, 9, 13, 10, 14, 11, 15
        });
    }
}

TEST_CASE("SimCAD: With correction", "[correction][simcad]") {
    // The values are divided by the range of 2, x is added and y is subtracted
    const test::TempFile file = test::TempFile(
        "mesh.simcad",
        makeSimCAD("0.5 0 0 0", "0 0 0 0.5")
    );
    const Buffer buf = generateSimCADMesh(file.path(), { 0.5f, 0.f }, { 0.5f, 0.5f });

    REQUIRE(buf.vertices.size() == 4);
    // x = 2 * ((0 + 0.25) * 0.5 + 0.5) - 1
    CHECK_THAT(buf.vertices[0].x, WithinAbs(0.25f, 1e-6));
    CHECK_THAT(buf.vertices[0].y, WithinAbs(0.f, 1e-6));
    CHECK_THAT(buf.vertices[0].s, WithinAbs(0.5f, 1e-6));
    CHECK_THAT(buf.vertices[0].t, WithinAbs(0.5f, 1e-6));
    // y = 2 * ((0 - 0.25) * 0.5 + 0) - 1
    CHECK_THAT(buf.vertices[3].x, WithinAbs(1.f, 1e-6));
    CHECK_THAT(buf.vertices[3].y, WithinAbs(-1.25f, 1e-6));
    CHECK_THAT(buf.vertices[3].s, WithinAbs(1.f, 1e-6));
    CHECK_THAT(buf.vertices[3].t, WithinAbs(0.f, 1e-6));
}
