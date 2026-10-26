/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <sgct/config.h>
#include <glm/gtc/type_ptr.hpp>
#include "schema.h"

using namespace sgct;
using namespace sgct::config;

TEST_CASE("Load: User/Minimal", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": []
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {}
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [ {} ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = { User() }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: User/Name", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "name": "abc"
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .name = "abc"
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "name": "def"
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .name = "def"
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: User/EyeSeparation", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "eyeseparation": 1.0
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .eyeSeparation = 1.f
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "eyeseparation": 2.0
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .eyeSeparation = 2.f
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: User/Position", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": { "x": 1.0, "y": 2.0, "z": 3.0 }
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .position = glm::vec3(1.f, 2.f, 3.f)
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": { "x": 4.0, "y": 5.0, "z": 6.0 }

    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .position = glm::vec3(4.f, 5.f, 6.f)
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: User/Transformation", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "matrix": [
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
        9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0
      ]
    }
  ]
}
)";

        glm::mat4 m;
        glm::value_ptr(m)[0] = 1.f;
        glm::value_ptr(m)[1] = 2.f;
        glm::value_ptr(m)[2] = 3.f;
        glm::value_ptr(m)[3] = 4.f;
        glm::value_ptr(m)[4] = 5.f;
        glm::value_ptr(m)[5] = 6.f;
        glm::value_ptr(m)[6] = 7.f;
        glm::value_ptr(m)[7] = 8.f;
        glm::value_ptr(m)[8] = 9.f;
        glm::value_ptr(m)[9] = 10.f;
        glm::value_ptr(m)[10] = 11.f;
        glm::value_ptr(m)[11] = 12.f;
        glm::value_ptr(m)[12] = 13.f;
        glm::value_ptr(m)[13] = 14.f;
        glm::value_ptr(m)[14] = 15.f;
        glm::value_ptr(m)[15] = 16.f;
        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .transformation = m
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "matrix": [
        17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
        25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0
      ]
    }
  ]
}
)";

        glm::mat4 m;
        glm::value_ptr(m)[0] = 17.f;
        glm::value_ptr(m)[1] = 18.f;
        glm::value_ptr(m)[2] = 19.f;
        glm::value_ptr(m)[3] = 20.f;
        glm::value_ptr(m)[4] = 21.f;
        glm::value_ptr(m)[5] = 22.f;
        glm::value_ptr(m)[6] = 23.f;
        glm::value_ptr(m)[7] = 24.f;
        glm::value_ptr(m)[8] = 25.f;
        glm::value_ptr(m)[9] = 26.f;
        glm::value_ptr(m)[10] = 27.f;
        glm::value_ptr(m)[11] = 28.f;
        glm::value_ptr(m)[12] = 29.f;
        glm::value_ptr(m)[13] = 30.f;
        glm::value_ptr(m)[14] = 31.f;
        glm::value_ptr(m)[15] = 32.f;
        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .transformation = m
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: User/Tracking", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "tracker": "abc",
        "device": "def"
      }
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .tracking = User::Tracking {
                        .tracker = "abc",
                        .device = "def"
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "tracker": "ghi",
        "device": "jkl"
      }
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .users = {
                User {
                    .tracking = User::Tracking {
                        .tracker = "ghi",
                        .device = "jkl"
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: User/Full", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "name": "abc",
      "eyeseparation": 1.0,
      "pos": { "x": 2.0, "y": 3.0, "z": 4.0 },
      "matrix": [
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
        9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0
      ],
      "tracking": {
        "tracker": "def",
        "device": "ghi"
      }
    }
  ]
}
)";

    glm::mat4 m;
    glm::value_ptr(m)[0] = 1.f;
    glm::value_ptr(m)[1] = 2.f;
    glm::value_ptr(m)[2] = 3.f;
    glm::value_ptr(m)[3] = 4.f;
    glm::value_ptr(m)[4] = 5.f;
    glm::value_ptr(m)[5] = 6.f;
    glm::value_ptr(m)[6] = 7.f;
    glm::value_ptr(m)[7] = 8.f;
    glm::value_ptr(m)[8] = 9.f;
    glm::value_ptr(m)[9] = 10.f;
    glm::value_ptr(m)[10] = 11.f;
    glm::value_ptr(m)[11] = 12.f;
    glm::value_ptr(m)[12] = 13.f;
    glm::value_ptr(m)[13] = 14.f;
    glm::value_ptr(m)[14] = 15.f;
    glm::value_ptr(m)[15] = 16.f;
    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .users = {
            {
                .name = "abc",
                .eyeSeparation = 1.f,
                .position = glm::vec3(2.f, 3.f, 4.f),
                .transformation = m,
                .tracking = User::Tracking {
                    .tracker = "def",
                    .device = "ghi"
                }
            }
        }
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}





TEST_CASE("Validate: User/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "name": 123
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Name/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "name": ""
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/EyeSeparation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "eyeseparation": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {}
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "y": 2,
        "z": 3
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": "abc",
        "y": 2,
        "z": 3
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": 1,
        "z": 3
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": 1,
        "y": "abc",
        "z": 3
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": 1,
        "y": 2
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Position/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "pos": {
        "x": 1,
        "y": 2,
        "z": "abc"
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Transformation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "matrix": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Transformation/Missing", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "matrix": []
    }
  ]
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }

    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "matrix": [ 1.0, 2.0, 3.0, 4.0 ]
    }
  ]
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: User/Tracking/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": "abc"
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Tracking/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {}
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Tracking/Tracker/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "device": "abc"
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Tracking/Tracker/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "tracker": 123,
        "device": "abc"
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Tracking/Device/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "tracker": "abc"
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: User/Tracking/Device/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "users": [
    {
      "tracking": {
        "tracker": "abc",
        "device": 123
      }
    }
  ]
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}
