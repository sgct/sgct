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
#include <sgct/correction/domeprojection.h>
#include <sgct/opengl.h>
#include <algorithm>
#include <vector>

using namespace sgct;
using namespace sgct::correction;
using Catch::Matchers::WithinAbs;

namespace {
    constexpr Error::Component Comp = Error::Component::DomeProjection;

    // A 3x2 grid in which the lines are not sorted by column and row. The format of
    // each line is x;y;u;v;column;row
    constexpr std::string_view Grid3x2 = R"(x;y;u;v;column;row
0.5;0;0.5;0;1;0
0;0;0;0;0;0
1;0;1;0;2;0
0;1;0;1;0;1
1;1;1;1;2;1
0.5;1;0.5;1;1;1
)";
} // namespace

TEST_CASE("DomeProjection: Missing file", "[correction][domeprojection]") {
    test::checkError(
        []() { generateDomeProjectionMesh("does-not-exist.csv", { 0, 0 }, { 1, 1 }); },
        Comp, 2010
    );
}

TEST_CASE("DomeProjection: Empty file", "[correction][domeprojection]") {
    const test::TempFile file = test::TempFile("empty.csv", "x;y;u;v;column;row\n");
    test::checkError(
        [&]() { generateDomeProjectionMesh(file.path(), { 0, 0 }, { 1, 1 }); },
        Comp, 2011
    );
}

TEST_CASE("DomeProjection: Single cell", "[correction][domeprojection]") {
    const test::TempFile file = test::TempFile(
        "cell.csv",
        "0;0;0;0;0;0\n1;0;1;0;1;0\n0;1;0;1;0;1\n1;1;1;1;1;1\n"
    );
    const Buffer buf = generateDomeProjectionMesh(file.path(), { 0, 0 }, { 1, 1 });

    REQUIRE(buf.vertices.size() == 4);
    CHECK(buf.geometryType == GL_TRIANGLES);
    CHECK(buf.indices == std::vector<unsigned int>{ 0, 1, 3, 0, 3, 2 });
}

TEST_CASE("DomeProjection: Unsorted grid", "[correction][domeprojection]") {
    const test::TempFile file = test::TempFile("grid.csv", Grid3x2);
    const Buffer buf = generateDomeProjectionMesh(file.path(), { 0, 0 }, { 1, 1 });

    REQUIRE(buf.vertices.size() == 6);
    // 2 cells with 2 triangles each
    REQUIRE(buf.indices.size() == 12);
    CHECK(std::ranges::all_of(buf.indices, [](unsigned int i) { return i < 6; }));

    // The vertices are stored in row-major order, the y and v coordinates are flipped
    const float ExpectedX[] = { -1.f, 0.f, 1.f, -1.f, 0.f, 1.f };
    const float ExpectedY[] = { 1.f, 1.f, 1.f, -1.f, -1.f, -1.f };
    const float ExpectedS[] = { 0.f, 0.5f, 1.f, 0.f, 0.5f, 1.f };
    const float ExpectedT[] = { 1.f, 1.f, 1.f, 0.f, 0.f, 0.f };
    for (size_t i = 0; i < buf.vertices.size(); i++) {
        CAPTURE(i);
        const Buffer::Vertex& v = buf.vertices[i];
        CHECK_THAT(v.x, WithinAbs(ExpectedX[i], 1e-6));
        CHECK_THAT(v.y, WithinAbs(ExpectedY[i], 1e-6));
        CHECK_THAT(v.s, WithinAbs(ExpectedS[i], 1e-6));
        CHECK_THAT(v.t, WithinAbs(ExpectedT[i], 1e-6));
        CHECK(v.r == 1.f);
        CHECK(v.g == 1.f);
        CHECK(v.b == 1.f);
        CHECK(v.a == 1.f);
    }
}

TEST_CASE("DomeProjection: Viewport position and size", "[correction][domeprojection]") {
    const test::TempFile file = test::TempFile("grid.csv", Grid3x2);
    const Buffer buf =
        generateDomeProjectionMesh(file.path(), { 0.5f, 0.25f }, { 0.5f, 0.5f });

    REQUIRE(buf.vertices.size() == 6);
    // Column 0, row 0
    CHECK_THAT(buf.vertices[0].x, WithinAbs(0.f, 1e-6));
    CHECK_THAT(buf.vertices[0].y, WithinAbs(0.5f, 1e-6));
    CHECK_THAT(buf.vertices[0].s, WithinAbs(0.5f, 1e-6));
    CHECK_THAT(buf.vertices[0].t, WithinAbs(0.75f, 1e-6));
    // Column 2, row 1
    CHECK_THAT(buf.vertices[5].x, WithinAbs(1.f, 1e-6));
    CHECK_THAT(buf.vertices[5].y, WithinAbs(-0.5f, 1e-6));
    CHECK_THAT(buf.vertices[5].s, WithinAbs(1.f, 1e-6));
    CHECK_THAT(buf.vertices[5].t, WithinAbs(0.25f, 1e-6));
}

TEST_CASE("DomeProjection: Positions are clamped", "[correction][domeprojection]") {
    const test::TempFile file = test::TempFile(
        "clamp.csv",
        "-0.5;-0.5;0;0;0;0\n1.5;0;1;0;1;0\n0;1;0;1;0;1\n1;1.5;1;1;1;1\n"
    );
    const Buffer buf = generateDomeProjectionMesh(file.path(), { 0, 0 }, { 1, 1 });

    REQUIRE(buf.vertices.size() == 4);
    CHECK_THAT(buf.vertices[0].x, WithinAbs(-1.f, 1e-6));
    CHECK_THAT(buf.vertices[0].y, WithinAbs(1.f, 1e-6));
    CHECK_THAT(buf.vertices[1].x, WithinAbs(1.f, 1e-6));
    CHECK_THAT(buf.vertices[3].y, WithinAbs(-1.f, 1e-6));
}

TEST_CASE("DomeProjection: Incomplete grid", "[correction][domeprojection]") {
    const test::TempFile file = test::TempFile(
        "incomplete.csv",
        "0;0;0;0;0;0\n1;0;1;0;1;0\n0;1;0;1;0;1\n"
    );
    test::checkError(
        [&]() { generateDomeProjectionMesh(file.path(), { 0, 0 }, { 1, 1 }); },
        Comp, 2012
    );
}

TEST_CASE("DomeProjection: Duplicate grid position", "[correction][domeprojection]") {
    const test::TempFile file = test::TempFile(
        "duplicate.csv",
        "0;0;0;0;0;0\n1;0;1;0;1;0\n0;1;0;1;0;1\n0;1;0;1;0;1\n"
    );
    test::checkError(
        [&]() { generateDomeProjectionMesh(file.path(), { 0, 0 }, { 1, 1 }); },
        Comp, 2012
    );
}
