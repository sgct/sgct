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
#include <sgct/image.h>
#include "testhelpers.h"
#include <array>
#include <cstdint>
#include <cstring>

using namespace sgct;
using namespace sgct::test;

namespace {
    // A 3x2 RGB PNG with the top row (255,0,0), (0,255,0), (0,0,255) and the bottom row
    // (10,20,30), (40,50,60), (70,80,90)
    constexpr std::array<unsigned char, 80> TestPng = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
        0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02,
        0x08, 0x02, 0x00, 0x00, 0x00, 0x12, 0x16, 0xf1, 0x4d, 0x00, 0x00, 0x00,
        0x17, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0xc0,
        0x00, 0xc1, 0x5c, 0x22, 0x72, 0x1a, 0x46, 0x36, 0x6e, 0x01, 0x51, 0x00,
        0x33, 0x59, 0x04, 0xc0, 0xe4, 0x24, 0x98, 0xcb, 0x00, 0x00, 0x00, 0x00,
        0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
    };

    // Returns a deterministic value for the provided pixel coordinate and channel
    unsigned char pattern(int x, int y, int c) {
        return static_cast<unsigned char>(x * 7 + y * 13 + c * 31);
    }
} // namespace

TEST_CASE("Image: Default", "[image]") {
    const Image img;
    CHECK(img.data() == nullptr);
    CHECK(img.channels() == 0);
    CHECK(img.bytesPerChannel() == 1);
    CHECK(img.size() == glm::ivec2(0, 0));
}

TEST_CASE("Image: Load from memory", "[image]") {
    std::array<unsigned char, 80> png = TestPng;
    Image img;
    img.load(png.data(), static_cast<int>(png.size()));
    REQUIRE(img.data() != nullptr);
    CHECK(img.size() == glm::ivec2(3, 2));
    CHECK(img.channels() == 3);
    CHECK(img.bytesPerChannel() == 1);

    // The image is stored bottom-up in BGR order to match the OpenGL conventions
    constexpr std::array<unsigned char, 18> Expected = {
        30, 20, 10,   60, 50, 40,   90, 80, 70,
        0, 0, 255,    0, 255, 0,    255, 0, 0
    };
    CHECK(std::memcmp(img.data(), Expected.data(), Expected.size()) == 0);

    // Loading a second time replaces the data
    img.load(png.data(), static_cast<int>(png.size()));
    CHECK(std::memcmp(img.data(), Expected.data(), Expected.size()) == 0);
}

TEST_CASE("Image: Load from file", "[image]") {
    const TempFile file = TempFile(
        "image.png",
        std::string_view(reinterpret_cast<const char*>(TestPng.data()), TestPng.size())
    );
    Image img;
    img.load(file.path());
    REQUIRE(img.data() != nullptr);
    CHECK(img.size() == glm::ivec2(3, 2));
    CHECK(img.channels() == 3);
    CHECK(img.data()[0] == 30);
    CHECK(img.data()[1] == 20);
    CHECK(img.data()[2] == 10);
    CHECK(img.data()[15] == 255);
    CHECK(img.data()[16] == 0);
    CHECK(img.data()[17] == 0);
}

TEST_CASE("Image: Save and load 8 bit", "[image]") {
    const int channels = GENERATE(1, 2, 3, 4);
    CAPTURE(channels);
    // Use an odd width to catch errors in the row stride
    const glm::ivec2 size = glm::ivec2(5, 3);

    Image img;
    img.setSize(size);
    img.setChannels(channels);
    img.setBytesPerChannel(1);
    img.allocateOrResizeData();
    REQUIRE(img.data() != nullptr);
    for (int y = 0; y < size.y; y++) {
        for (int x = 0; x < size.x; x++) {
            for (int c = 0; c < channels; c++) {
                img.data()[(y * size.x + x) * channels + c] = pattern(x, y, c);
            }
        }
    }

    const TempFile file = TempFile("image.png", "");
    img.save(file.path());

    Image loaded;
    loaded.load(file.path());
    REQUIRE(loaded.data() != nullptr);
    CHECK(loaded.size() == size);
    CHECK(loaded.channels() == channels);
    CHECK(loaded.bytesPerChannel() == 1);
    const size_t nBytes = static_cast<size_t>(size.x * size.y * channels);
    CHECK(std::memcmp(loaded.data(), img.data(), nBytes) == 0);
}

TEST_CASE("Image: Save 16 bit", "[image]") {
    const int channels = GENERATE(1, 3, 4);
    CAPTURE(channels);
    const glm::ivec2 size = glm::ivec2(3, 2);

    Image img;
    img.setSize(size);
    img.setChannels(channels);
    img.setBytesPerChannel(2);
    img.allocateOrResizeData();
    REQUIRE(img.data() != nullptr);
    for (int y = 0; y < size.y; y++) {
        for (int x = 0; x < size.x; x++) {
            for (int c = 0; c < channels; c++) {
                // The high and low bytes differ so that a wrong byte order is detected
                const uint16_t v = static_cast<uint16_t>(pattern(x, y, c) << 8 | 0x5a);
                const size_t idx = static_cast<size_t>((y * size.x + x) * channels + c);
                std::memcpy(img.data() + idx * sizeof(uint16_t), &v, sizeof(uint16_t));
            }
        }
    }

    const TempFile file = TempFile("image.png", "");
    img.save(file.path());

    // Images are always loaded with 8 bits per channel, which keeps the high byte
    Image loaded;
    loaded.load(file.path());
    REQUIRE(loaded.data() != nullptr);
    CHECK(loaded.size() == size);
    CHECK(loaded.channels() == channels);
    for (int y = 0; y < size.y; y++) {
        for (int x = 0; x < size.x; x++) {
            for (int c = 0; c < channels; c++) {
                CAPTURE(x, y, c);
                const int idx = (y * size.x + x) * channels + c;
                CHECK(loaded.data()[idx] == pattern(x, y, c));
            }
        }
    }
}

TEST_CASE("Image: Reallocate", "[image]") {
    Image img;
    img.setSize(glm::ivec2(4, 4));
    img.setChannels(4);
    img.allocateOrResizeData();
    unsigned char* data = img.data();
    REQUIRE(data != nullptr);

    // Same size keeps the buffer
    img.allocateOrResizeData();
    CHECK(img.data() == data);

    // Different size reallocates the buffer
    img.setSize(glm::ivec2(64, 64));
    img.allocateOrResizeData();
    REQUIRE(img.data() != nullptr);
    img.data()[64 * 64 * 4 - 1] = 1;

    // Reallocating memory that was loaded by the image loader
    std::array<unsigned char, 80> png = TestPng;
    img.load(png.data(), static_cast<int>(png.size()));
    img.setSize(glm::ivec2(8, 8));
    img.allocateOrResizeData();
    REQUIRE(img.data() != nullptr);
    img.data()[8 * 8 * 3 - 1] = 1;
}

TEST_CASE("Image: Errors", "[image]") {
    SECTION("9000: Load empty path") {
        Image img;
        checkError([&]() { img.load(""); }, Error::Component::Image, 9000);
    }

    SECTION("9001: Load missing file") {
        Image img;
        checkError(
            [&]() { img.load("this-file-does-not-exist.png"); },
            Error::Component::Image,
            9001
        );
    }

    SECTION("9001: Load invalid file") {
        const TempFile file = TempFile("image.png", "not a png");
        Image img;
        checkError([&]() { img.load(file.path()); }, Error::Component::Image, 9001);
    }

    SECTION("9002: Save empty path") {
        Image img;
        checkError([&]() { img.save(""); }, Error::Component::Image, 9002);
    }

    SECTION("9003: Load invalid memory") {
        std::string data = "not a png";
        Image img;
        checkError(
            [&]() {
                img.load(
                    reinterpret_cast<unsigned char*>(data.data()),
                    static_cast<int>(data.size())
                );
            },
            Error::Component::Image,
            9003
        );
        CHECK(img.data() == nullptr);
    }

    SECTION("9006: Save without data") {
        Image img;
        checkError([&]() { img.save("image.png"); }, Error::Component::Image, 9006);
    }

    SECTION("9007: Save 32 bit") {
        Image img;
        img.setSize(glm::ivec2(2, 2));
        img.setChannels(3);
        img.setBytesPerChannel(4);
        img.allocateOrResizeData();
        checkError([&]() { img.save("image.png"); }, Error::Component::Image, 9007);
    }

    SECTION("9008: Save to missing folder") {
        Image img;
        img.setSize(glm::ivec2(2, 2));
        img.setChannels(3);
        img.allocateOrResizeData();
        const std::filesystem::path path =
            std::filesystem::temp_directory_path() / "sgct-missing-folder" / "image.png";
        checkError([&]() { img.save(path); }, Error::Component::Image, 9008);
    }

    SECTION("9012: Allocate empty image") {
        const glm::ivec2 size = GENERATE(glm::ivec2(0, 2), glm::ivec2(2, 0));
        Image img;
        img.setSize(size);
        img.setChannels(3);
        checkError([&]() { img.allocateOrResizeData(); }, Error::Component::Image, 9012);

        Image img2;
        img2.setSize(glm::ivec2(2, 2));
        checkError(
            [&]() { img2.allocateOrResizeData(); },
            Error::Component::Image,
            9012
        );
    }
}

TEST_CASE("Image: Move", "[image]") {
    std::array<unsigned char, 80> png = TestPng;
    Image img;
    img.load(png.data(), static_cast<int>(png.size()));
    const unsigned char* data = img.data();

    Image moved = std::move(img);
    CHECK(moved.data() == data);
    CHECK(moved.size() == glm::ivec2(3, 2));
    CHECK(moved.channels() == 3);
    CHECK(img.data() == nullptr);
    CHECK(img.size() == glm::ivec2(0, 0));

    Image other;
    other.setSize(glm::ivec2(2, 2));
    other.setChannels(1);
    other.allocateOrResizeData();
    other = std::move(moved);
    CHECK(other.data() == data);
    CHECK(other.size() == glm::ivec2(3, 2));
    CHECK(moved.data() == nullptr);

    // Assigning an empty image releases the data
    other = Image();
    CHECK(other.data() == nullptr);
}
