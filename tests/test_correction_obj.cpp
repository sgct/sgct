/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "testhelpers.h"
#include <sgct/correction/buffer.h>
#include <sgct/correction/obj.h>
#include <sgct/opengl.h>
#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace sgct;
using namespace sgct::correction;
using Catch::Matchers::WithinAbs;

namespace {
    constexpr Error::Component Comp = Error::Component::OBJ;

    void checkObjError(std::string_view content, int code) {
        const test::TempFile file = test::TempFile("mesh.obj", content);
        test::checkError([&]() { generateOBJMesh(file.path()); }, Comp, code);
    }
} // namespace

TEST_CASE("OBJ: Missing file", "[correction][obj]") {
    test::checkError([]() { generateOBJMesh("does-not-exist.obj"); }, Comp, 2030);
}

TEST_CASE("OBJ: Quad", "[correction][obj]") {
    // Uses all three supported face formats, Windows line endings, and elements that
    // are ignored by the loader
    constexpr std::string_view Content =
        "# A quad\r\n"
        "mtllib quad.mtl\r\n"
        "o Quad\r\n"
        "\r\n"
        "v -1 -1 0\r\n"
        "v 1 -1 0\r\n"
        "v 1 1 0\r\n"
        "v -1 1 0\r\n"
        "vt 0 0\r\n"
        "vt 1 0\r\n"
        "vt 1 1\r\n"
        "vt 0 1\r\n"
        "vn 0 0 1\r\n"
        "s off\r\n"
        "f 1 2 3\r\n"
        "f 1/1/1 3/3/1 4/4/1\r\n"
        "f 2/2 3/3 1/1\r\n";
    const test::TempFile file = test::TempFile("quad.obj", Content);
    const Buffer buf = generateOBJMesh(file.path());

    CHECK(buf.geometryType == GL_TRIANGLES);
    CHECK(buf.indices == std::vector<unsigned int>{ 0, 1, 2, 0, 2, 3, 1, 2, 0 });

    REQUIRE(buf.vertices.size() == 4);
    const float ExpectedX[] = { -1.f, 1.f, 1.f, -1.f };
    const float ExpectedY[] = { -1.f, -1.f, 1.f, 1.f };
    const float ExpectedS[] = { 0.f, 1.f, 1.f, 0.f };
    const float ExpectedT[] = { 0.f, 0.f, 1.f, 1.f };
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

TEST_CASE("OBJ: z coordinate is ignored", "[correction][obj]") {
    const test::TempFile file = test::TempFile(
        "z.obj",
        "v 0.5 0.25 3\nv 0 0 0\nv 1 0 0\nvt 0 0\nvt 1 0\nvt 1 1\nf 1 2 3\n"
    );
    const Buffer buf = generateOBJMesh(file.path());
    REQUIRE(buf.vertices.size() == 3);
    CHECK_THAT(buf.vertices[0].x, WithinAbs(0.5f, 1e-6));
    CHECK_THAT(buf.vertices[0].y, WithinAbs(0.25f, 1e-6));
}

TEST_CASE("OBJ: Mismatched texture coordinates", "[correction][obj]") {
    checkObjError("v 0 0 0\nv 1 0 0\nv 1 1 0\nvt 0 0\nvt 1 0\nf 1 2 3\n", 2031);
}

TEST_CASE("OBJ: Undefined vertex", "[correction][obj]") {
    const std::string face = GENERATE("f 1 2 4", "f 0 1 2", "f 1 0 2", "f 1 2 0");
    CAPTURE(face);
    checkObjError(
        "v 0 0 0\nv 1 0 0\nv 1 1 0\nvt 0 0\nvt 1 0\nvt 1 1\n" + face + "\n",
        2032
    );
}

TEST_CASE("OBJ: Relative indices", "[correction][obj]") {
    checkObjError(
        "v 0 0 0\nv 1 0 0\nv 1 1 0\nvt 0 0\nvt 1 0\nvt 1 1\nf -3 -2 -1\n",
        2033
    );
}

TEST_CASE("OBJ: Illegal vertex", "[correction][obj]") {
    checkObjError("v 1\n", 2034);
    checkObjError("v 1 2\n", 2034);
}

TEST_CASE("OBJ: Illegal face", "[correction][obj]") {
    checkObjError("f 1\n", 2035);
    checkObjError("f 1 2\n", 2035);
}

TEST_CASE("OBJ: Invalid number", "[correction][obj]") {
    checkObjError("v a 0 0\n", 2036);
    checkObjError("vt 0 b\n", 2036);
    checkObjError("v 0 0 0\nvt 0 0\nf 1 x 1\n", 2036);
    checkObjError("v 0 0 0\nvt 0 0\nf 1 1 99999999999\n", 2036);
}

TEST_CASE("OBJ: Example meshes", "[correction][obj]") {
    const auto [name, nFaces] = GENERATE(
        std::pair<std::string_view, size_t>("bottom_warp2.obj", 4802),
        std::pair<std::string_view, size_t>("left_warp2.obj", 4802),
        std::pair<std::string_view, size_t>("right_warp2.obj", 4802),
        std::pair<std::string_view, size_t>("top_warp2.obj", 4785)
    );
    CAPTURE(name);

    const Buffer buf = generateOBJMesh(
        std::filesystem::path(BASE_PATH) / "config/mesh" / name
    );
    CHECK(buf.vertices.size() == 2500);
    CHECK(buf.indices.size() == nFaces * 3);
    CHECK(std::ranges::all_of(buf.indices, [](unsigned int i) { return i < 2500; }));
}
