/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <sgct/network.h>
#include <sgct/shareddata.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using namespace sgct;

namespace {
    struct Pod {
        int32_t a;
        float b;
        double c;
        bool d;

        bool operator==(const Pod&) const noexcept = default;
    };

    template <typename T>
    T roundTrip(const T& value) {
        std::vector<std::byte> buffer;
        serializeObject(buffer, value);
        unsigned int pos = 0;
        T result = {};
        deserializeObject(buffer, pos, result);
        REQUIRE(pos == buffer.size());
        return result;
    }
} // namespace

TEST_CASE("SharedData: Serialize fundamental types", "[shareddata]") {
    CHECK(roundTrip<int8_t>(-12) == -12);
    CHECK(roundTrip<uint8_t>(250) == 250);
    CHECK(roundTrip<int16_t>(-1234) == -1234);
    CHECK(roundTrip<int32_t>(-123456) == -123456);
    CHECK(roundTrip<uint32_t>(4000000000u) == 4000000000u);
    CHECK(roundTrip<int64_t>(-1234567890123) == -1234567890123);
    CHECK(roundTrip<float>(1.5f) == 1.5f);
    CHECK(roundTrip<double>(-2.25) == -2.25);
    CHECK(roundTrip<bool>(true) == true);
    CHECK(roundTrip<bool>(false) == false);
    CHECK(roundTrip<char>('x') == 'x');
}

TEST_CASE("SharedData: Serialize sizes", "[shareddata]") {
    std::vector<std::byte> buffer;
    serializeObject(buffer, int32_t(1));
    CHECK(buffer.size() == sizeof(int32_t));
    serializeObject(buffer, double(1.0));
    CHECK(buffer.size() == sizeof(int32_t) + sizeof(double));
    serializeObject(buffer, std::vector<int16_t>{ 1, 2, 3 });
    CHECK(buffer.size() == sizeof(int32_t) + sizeof(double) + sizeof(uint32_t) + 6);
    serializeObject(buffer, std::string("abcd"));
    CHECK(buffer.size() == sizeof(int32_t) + sizeof(double) + 2 * sizeof(uint32_t) + 10);
}

TEST_CASE("SharedData: Serialize struct", "[shareddata]") {
    const Pod value = { .a = 42, .b = 3.f, .c = -7.0, .d = true };
    CHECK(roundTrip(value) == value);
}

TEST_CASE("SharedData: Serialize vector", "[shareddata]") {
    CHECK(roundTrip(std::vector<int>{}).empty());
    CHECK(roundTrip(std::vector<int>{ 1, -2, 3 }) == std::vector<int>{ 1, -2, 3 });
    CHECK(
        roundTrip(std::vector<double>{ 0.5, 1e100 }) == std::vector<double>{ 0.5, 1e100 }
    );

    const std::vector<Pod> pods = {
        { .a = 1, .b = 2.f, .c = 3.0, .d = false },
        { .a = 4, .b = 5.f, .c = 6.0, .d = true }
    };
    CHECK(roundTrip(pods) == pods);
}

TEST_CASE("SharedData: Deserialize vector replaces content", "[shareddata]") {
    std::vector<std::byte> buffer;
    serializeObject(buffer, std::vector<int>{});
    unsigned int pos = 0;
    std::vector<int> value = { 1, 2, 3 };
    deserializeObject(buffer, pos, value);
    CHECK(value.empty());
    CHECK(pos == sizeof(uint32_t));
}

TEST_CASE("SharedData: Serialize string", "[shareddata]") {
    CHECK(roundTrip(std::string()).empty());
    CHECK(roundTrip(std::string("Hello world")) == "Hello world");
    CHECK(roundTrip(std::string("with\0null", 9)) == std::string("with\0null", 9));

    std::vector<std::byte> buffer;
    serializeObject(buffer, std::string_view("view"));
    unsigned int pos = 0;
    std::string result;
    deserializeObject(buffer, pos, result);
    CHECK(result == "view");
    CHECK(pos == buffer.size());
}

TEST_CASE("SharedData: Serialize wstring", "[shareddata]") {
    CHECK(roundTrip(std::wstring()).empty());
    CHECK(roundTrip(std::wstring(L"Hello world")) == L"Hello world");
    CHECK(roundTrip(std::wstring(L"\u00e5\u00e4\u00f6")) == L"\u00e5\u00e4\u00f6");
}

TEST_CASE("SharedData: Serialize sequence", "[shareddata]") {
    // Mixing types of different sizes causes values to be stored at unaligned offsets
    std::vector<std::byte> buffer;
    serializeObject(buffer, uint8_t(7));
    serializeObject(buffer, double(1.25));
    serializeObject(buffer, std::string("abc"));
    serializeObject(buffer, int32_t(-9));
    serializeObject(buffer, std::vector<float>{ 1.f, 2.f });
    serializeObject(buffer, std::wstring(L"xyz"));
    serializeObject(buffer, bool(true));

    unsigned int pos = 0;
    uint8_t u8 = 0;
    deserializeObject(buffer, pos, u8);
    double d = 0.0;
    deserializeObject(buffer, pos, d);
    std::string s;
    deserializeObject(buffer, pos, s);
    int32_t i32 = 0;
    deserializeObject(buffer, pos, i32);
    std::vector<float> v;
    deserializeObject(buffer, pos, v);
    std::wstring ws;
    deserializeObject(buffer, pos, ws);
    bool b = false;
    deserializeObject(buffer, pos, b);

    CHECK(u8 == 7);
    CHECK(d == 1.25);
    CHECK(s == "abc");
    CHECK(i32 == -9);
    CHECK(v == std::vector<float>{ 1.f, 2.f });
    CHECK(ws == L"xyz");
    CHECK(b == true);
    CHECK(pos == buffer.size());
}

TEST_CASE("SharedData: Encode", "[shareddata]") {
    SharedData& data = SharedData::instance();
    data.setEncodeFunction([]() {
        std::vector<std::byte> buffer;
        serializeObject(buffer, int32_t(1234));
        return buffer;
    });
    data.encode();

    REQUIRE(data.dataSize() == static_cast<int>(Network::HeaderSize + sizeof(int32_t)));
    CHECK(data.bufferSize() >= data.dataSize());
    const unsigned char* block = data.dataBlock();
    CHECK(block[0] == Network::DataId);
    for (size_t i = 1; i < Network::HeaderSize; i++) {
        CHECK(block[i] == Network::DefaultId);
    }
    int32_t payload = 0;
    std::memcpy(&payload, block + Network::HeaderSize, sizeof(int32_t));
    CHECK(payload == 1234);

    // Encoding a second time must not accumulate data
    data.encode();
    CHECK(data.dataSize() == static_cast<int>(Network::HeaderSize + sizeof(int32_t)));

    SharedData::destroy();
}

TEST_CASE("SharedData: Encode without function", "[shareddata]") {
    SharedData& data = SharedData::instance();
    data.encode();
    CHECK(data.dataSize() == static_cast<int>(Network::HeaderSize));
    SharedData::destroy();
}

TEST_CASE("SharedData: Decode", "[shareddata]") {
    static std::vector<std::byte> received;
    received.clear();

    SharedData& data = SharedData::instance();
    data.setDecodeFunction([](const std::vector<std::byte>& buffer) {
        received = buffer;
    });

    std::vector<std::byte> buffer;
    serializeObject(buffer, std::string("payload"));
    serializeObject(buffer, float(2.5f));
    data.decode(
        reinterpret_cast<const char*>(buffer.data()),
        static_cast<int>(buffer.size())
    );

    CHECK(received == buffer);
    CHECK(data.dataSize() == static_cast<int>(buffer.size()));

    unsigned int pos = 0;
    std::string s;
    deserializeObject(received, pos, s);
    float f = 0.f;
    deserializeObject(received, pos, f);
    CHECK(s == "payload");
    CHECK(f == 2.5f);

    SharedData::destroy();
}
