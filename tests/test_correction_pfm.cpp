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
#include <sgct/correction/pfm.h>
#include <sgct/format.h>
#include <sgct/opengl.h>
#include <algorithm>
#include <string>
#include <vector>

using namespace sgct;
using namespace sgct::correction;
using Catch::Matchers::WithinAbs;

namespace {
    constexpr Error::Component Comp = Error::Component::Pfm;

    // Creates the contents of a PFM file. Each pixel stores the x and y correction
    // values in the first two channels, the third channel is ignored by the loader
    std::string makePfm(unsigned int nCols, unsigned int nRows,
                        const std::vector<float>& values)
    {
        std::string res = std::format("PF\n{} {}\n-1.0\n", nCols, nRows);
        res.append(
            reinterpret_cast<const char*>(values.data()),
            values.size() * sizeof(float)
        );
        return res;
    }

    bool indicesInRange(const Buffer& buf) {
        return std::ranges::all_of(
            buf.indices,
            [&buf](unsigned int i) { return i < buf.vertices.size(); }
        );
    }
} // namespace

TEST_CASE("PFM: Missing file", "[correction][pfm]") {
    test::checkError(
        []() {
            generatePerEyeMeshFromPFMImage("does-not-exist.pfm", { 0, 0 }, { 1, 1 });
        },
        Comp, 2050
    );
}

TEST_CASE("PFM: Empty file", "[correction][pfm]") {
    const test::TempFile file = test::TempFile("empty.pfm", "");
    test::checkError(
        [&]() { generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 }); },
        Comp, 2053
    );
}

TEST_CASE("PFM: Wrong file type", "[correction][pfm]") {
    const test::TempFile file = test::TempFile("wrong.pfm", "P6\n2 2\n255\n");
    test::checkError(
        [&]() { generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 }); },
        Comp, 2053
    );
}

TEST_CASE("PFM: Invalid header", "[correction][pfm]") {
    SECTION("Dimensions") {
        const test::TempFile file = test::TempFile("header.pfm", "PF\na b\n-1.0\n");
        test::checkError(
            [&]() { generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 }); },
            Comp, 2052
        );
    }

    SECTION("Endianness") {
        const test::TempFile file = test::TempFile("header.pfm", "PF\n2 2\nabc\n");
        test::checkError(
            [&]() { generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 }); },
            Comp, 2052
        );
    }
}

TEST_CASE("PFM: Too small dimensions", "[correction][pfm]") {
    SECTION("Empty") {
        const test::TempFile file = test::TempFile("empty.pfm", makePfm(0, 0, {}));
        test::checkError(
            [&]() { generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 }); },
            Comp, 2052
        );
    }

    SECTION("Single row") {
        const std::vector<float> values(4 * 3, 0.f);
        const test::TempFile file = test::TempFile("row.pfm", makePfm(4, 1, values));
        test::checkError(
            [&]() { generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 }); },
            Comp, 2052
        );
    }

    SECTION("Single column per eye") {
        const std::vector<float> values(2 * 2 * 3, 0.f);
        const test::TempFile file = test::TempFile("column.pfm", makePfm(1, 2, values));
        test::checkError(
            [&]() { generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 }); },
            Comp, 2052
        );
        test::checkError(
            [&]() {
                generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 }, true);
            },
            Comp, 2052
        );
    }
}

TEST_CASE("PFM: Truncated data", "[correction][pfm]") {
    const std::vector<float> values(5 * 3, 0.f);
    const test::TempFile file = test::TempFile("truncated.pfm", makePfm(4, 2, values));
    test::checkError(
        [&]() { generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 }); },
        Comp, 2054
    );
}

TEST_CASE("PFM: Texture render mode", "[correction][pfm]") {
    const std::vector<float> values = {
        0.1f, 0.2f, 0.f,   0.3f, 0.4f, 0.f,
        0.5f, 0.6f, 0.f,   0.7f, 0.8f, 0.f
    };
    const test::TempFile file = test::TempFile("texture.pfm", makePfm(2, 2, values));
    const Buffer buf = generatePerEyeMeshFromPFMImage(
        file.path(),
        { 0.5f, 0.25f },
        { 1.f, 1.f },
        true
    );

    REQUIRE(buf.vertices.size() == 4);
    CHECK(buf.geometryType == GL_TRIANGLE_STRIP);
    CHECK(buf.indices == std::vector<unsigned int>{ 0, 2, 1, 3 });

    // The positions cover the entire viewport with y pointing upwards and the texture
    // coordinates are taken from the file
    const float ExpectedX[] = { -1.f, 1.f, -1.f, 1.f };
    const float ExpectedY[] = { 1.f, 1.f, -1.f, -1.f };
    for (size_t i = 0; i < buf.vertices.size(); i++) {
        CAPTURE(i);
        const Buffer::Vertex& v = buf.vertices[i];
        CHECK_THAT(v.x, WithinAbs(ExpectedX[i], 1e-6));
        CHECK_THAT(v.y, WithinAbs(ExpectedY[i], 1e-6));
        CHECK_THAT(v.s, WithinAbs(values[i * 3] + 0.5f, 1e-6));
        CHECK_THAT(v.t, WithinAbs(values[i * 3 + 1] + 0.25f, 1e-6));
        CHECK(v.r == 1.f);
        CHECK(v.a == 1.f);
    }
}

TEST_CASE("PFM: Side-by-side stereo", "[correction][pfm]") {
    // The left half of the image is used for the left eye, the right half for the right
    // eye. The x correction of pixel k is k / 10 and the y correction is 1 - k / 10
    std::vector<float> values;
    for (int k = 0; k < 8; k++) {
        values.push_back(k / 10.f);
        values.push_back(1.f - k / 10.f);
        values.push_back(0.f);
    }
    const test::TempFile file = test::TempFile("stereo.pfm", makePfm(4, 2, values));

    SECTION("Full viewport") {
        const Buffer buf =
            generatePerEyeMeshFromPFMImage(file.path(), { 0, 0 }, { 1, 1 });

        REQUIRE(buf.vertices.size() == 8);
        CHECK(buf.geometryType == GL_TRIANGLE_STRIP);
        CHECK(buf.indices.size() == 8);
        CHECK(indicesInRange(buf));

        // Pixel index in the file for each of the vertices
        const int Pixel[] = { 0, 1, 4, 5, 2, 3, 6, 7 };
        // The texture coordinates are in the center of each pixel, with v flipped
        const float ExpectedS[] = { 0.25f, 0.75f, 0.25f, 0.75f };
        const float ExpectedT[] = { 0.75f, 0.75f, 0.25f, 0.25f };
        for (size_t i = 0; i < buf.vertices.size(); i++) {
            CAPTURE(i);
            const Buffer::Vertex& v = buf.vertices[i];
            CHECK_THAT(v.x, WithinAbs(2.f * (Pixel[i] / 10.f) - 1.f, 1e-6));
            CHECK_THAT(v.y, WithinAbs(2.f * (1.f - Pixel[i] / 10.f) - 1.f, 1e-6));
            CHECK_THAT(v.s, WithinAbs(ExpectedS[i % 4], 1e-6));
            CHECK_THAT(v.t, WithinAbs(ExpectedT[i % 4], 1e-6));
        }
    }

    SECTION("Partial viewport") {
        const Buffer buf = generatePerEyeMeshFromPFMImage(
            file.path(),
            { 0.5f, 0.f },
            { 0.5f, 0.5f }
        );

        REQUIRE(buf.vertices.size() == 8);
        CHECK_THAT(buf.vertices[0].x, WithinAbs(0.f, 1e-6));
        CHECK_THAT(buf.vertices[0].y, WithinAbs(0.f, 1e-6));
        CHECK_THAT(buf.vertices[0].s, WithinAbs(0.625f, 1e-6));
        CHECK_THAT(buf.vertices[0].t, WithinAbs(0.375f, 1e-6));
    }
}

TEST_CASE("PFM: Example mesh", "[correction][pfm]") {
    const std::filesystem::path path =
        std::filesystem::path(BASE_PATH) / "config/mesh/surface1.pfm";

    SECTION("Stereo") {
        const Buffer buf = generatePerEyeMeshFromPFMImage(path, { 0, 0 }, { 1, 1 });
        // 100x100 pixels split into two 50x100 meshes
        CHECK(buf.vertices.size() == 10000);
        // Per eye: 50 even rows with 2 * 50 indices and 49 odd rows with 2 * 49 indices
        CHECK(buf.indices.size() == 2 * (50 * 100 + 49 * 98));
        CHECK(indicesInRange(buf));
    }

    SECTION("Texture render mode") {
        const Buffer buf =
            generatePerEyeMeshFromPFMImage(path, { 0, 0 }, { 1, 1 }, true);
        CHECK(buf.vertices.size() == 10000);
        CHECK(buf.indices.size() == 50 * 200 + 49 * 198);
        CHECK(indicesInRange(buf));
    }
}
