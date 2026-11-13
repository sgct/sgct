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
#include <sgct/correction/paulbourke.h>
#include <sgct/opengl.h>
#include <algorithm>
#include <string_view>
#include <vector>

using namespace sgct;
using namespace sgct::correction;
using Catch::Matchers::WithinAbs;

namespace {
    constexpr Error::Component Comp = Error::Component::PaulBourke;

    // A 2x2 grid covering the entire [-1, 1] range. The format of each line after the
    // header is x y s t intensity
    constexpr std::string_view Grid2x2 =
        "2\n"
        "2 2\n"
        "-1 -1 0 0 0.5\n"
        "1 -1 1 0 1\n"
        "-1 1 0 1 1\n"
        "1 1 1 1 0.25\n";

    void checkPaulBourkeError(std::string_view content, int code) {
        const test::TempFile file = test::TempFile("mesh.data", content);
        test::checkError(
            [&]() { generatePaulBourkeMesh(file.path(), { 0, 0 }, { 1, 1 }, 1.f); },
            Comp, code
        );
    }
} // namespace

TEST_CASE("PaulBourke: Missing file", "[correction][paulbourke]") {
    test::checkError(
        []() { generatePaulBourkeMesh("does-not-exist.data", { 0, 0 }, { 1, 1 }, 1.f); },
        Comp, 2040
    );
}

TEST_CASE("PaulBourke: Invalid mapping type", "[correction][paulbourke]") {
    checkPaulBourkeError("", 2041);
    checkPaulBourkeError("abc\n2 2\n", 2041);
}

TEST_CASE("PaulBourke: Invalid dimensions", "[correction][paulbourke]") {
    checkPaulBourkeError("2\na b\n", 2042);
    checkPaulBourkeError("2\n1 5\n", 2042);
    checkPaulBourkeError("2\n5 1\n", 2042);
    checkPaulBourkeError("2\n-3 2\n", 2042);
}

TEST_CASE("PaulBourke: Wrong number of vertices", "[correction][paulbourke]") {
    checkPaulBourkeError("2\n2 2\n-1 -1 0 0 1\n1 -1 1 0 1\n-1 1 0 1 1\n", 2043);
    checkPaulBourkeError("2\n2 2\n", 2043);
}

TEST_CASE("PaulBourke: Grid", "[correction][paulbourke]") {
    const test::TempFile file = test::TempFile("grid.data", Grid2x2);
    const Buffer buf = generatePaulBourkeMesh(file.path(), { 0, 0 }, { 1, 1 }, 1.f);

    CHECK(buf.geometryType == GL_TRIANGLES);
    CHECK(buf.indices == std::vector<unsigned int>{ 0, 1, 3, 0, 3, 2 });

    REQUIRE(buf.vertices.size() == 4);
    const float ExpectedX[] = { -1.f, 1.f, -1.f, 1.f };
    const float ExpectedY[] = { -1.f, -1.f, 1.f, 1.f };
    const float ExpectedS[] = { 0.f, 1.f, 0.f, 1.f };
    const float ExpectedT[] = { 0.f, 0.f, 1.f, 1.f };
    const float ExpectedIntensity[] = { 0.5f, 1.f, 1.f, 0.25f };
    for (size_t i = 0; i < buf.vertices.size(); i++) {
        CAPTURE(i);
        const Buffer::Vertex& v = buf.vertices[i];
        CHECK_THAT(v.x, WithinAbs(ExpectedX[i], 1e-6));
        CHECK_THAT(v.y, WithinAbs(ExpectedY[i], 1e-6));
        CHECK_THAT(v.s, WithinAbs(ExpectedS[i], 1e-6));
        CHECK_THAT(v.t, WithinAbs(ExpectedT[i], 1e-6));
        CHECK(v.r == ExpectedIntensity[i]);
        CHECK(v.g == ExpectedIntensity[i]);
        CHECK(v.b == ExpectedIntensity[i]);
        CHECK(v.a == 1.f);
    }
}

TEST_CASE("PaulBourke: Aspect ratio", "[correction][paulbourke]") {
    // The x coordinates are divided by the aspect ratio of the window and viewport
    const test::TempFile file = test::TempFile("grid.data", Grid2x2);
    const Buffer buf = generatePaulBourkeMesh(file.path(), { 0, 0 }, { 1, 1 }, 2.f);

    REQUIRE(buf.vertices.size() == 4);
    CHECK_THAT(buf.vertices[0].x, WithinAbs(-0.5f, 1e-6));
    CHECK_THAT(buf.vertices[1].x, WithinAbs(0.5f, 1e-6));
    CHECK_THAT(buf.vertices[0].y, WithinAbs(-1.f, 1e-6));
    CHECK_THAT(buf.vertices[2].y, WithinAbs(1.f, 1e-6));
}

TEST_CASE("PaulBourke: Viewport position and size", "[correction][paulbourke]") {
    const test::TempFile file = test::TempFile("grid.data", Grid2x2);
    const Buffer buf = generatePaulBourkeMesh(
        file.path(),
        { 0.5f, 0.f },
        { 0.5f, 0.5f },
        1.f
    );

    REQUIRE(buf.vertices.size() == 4);
    CHECK_THAT(buf.vertices[0].x, WithinAbs(0.f, 1e-6));
    CHECK_THAT(buf.vertices[0].y, WithinAbs(-1.f, 1e-6));
    CHECK_THAT(buf.vertices[0].s, WithinAbs(0.5f, 1e-6));
    CHECK_THAT(buf.vertices[0].t, WithinAbs(0.f, 1e-6));
    CHECK_THAT(buf.vertices[3].x, WithinAbs(1.f, 1e-6));
    CHECK_THAT(buf.vertices[3].y, WithinAbs(0.f, 1e-6));
    CHECK_THAT(buf.vertices[3].s, WithinAbs(1.f, 1e-6));
    CHECK_THAT(buf.vertices[3].t, WithinAbs(0.5f, 1e-6));
}

TEST_CASE("PaulBourke: Example mesh", "[correction][paulbourke]") {
    // The example mesh uses tabs as separators and is made for a 16:9 window
    const Buffer buf = generatePaulBourkeMesh(
        std::filesystem::path(BASE_PATH) / "config/mesh/standard_16x9.data",
        { 0, 0 },
        { 1, 1 },
        16.f / 9.f
    );

    REQUIRE(buf.vertices.size() == 100 * 60);
    CHECK(buf.indices.size() == 99 * 59 * 6);
    CHECK(std::ranges::all_of(buf.indices, [](unsigned int i) { return i < 6000; }));
    CHECK_THAT(buf.vertices[0].x, WithinAbs(-1.f, 1e-4));
    CHECK_THAT(buf.vertices[0].y, WithinAbs(-1.f, 1e-4));
}
